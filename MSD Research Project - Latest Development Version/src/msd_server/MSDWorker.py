import os
import sys
import traceback
import json

# import MSD library from correct path: ~/lib/python
MSD_LIB_PATH = os.path.join(os.path.realpath(os.path.dirname(__file__)), "../../lib/python")
sys.path.append(MSD_LIB_PATH)
from MSD import *


def molType(t: str) -> c_void_p:
	''' Convert str to MSD MolType '''
	return {
		"LINEAR": MSD.LINEAR_MOL,
		"CIRCULAR": MSD.CIRCULAR_MOL
	}[t.upper()]

def vec(v: list) -> Vector:
	''' Convert list to MSD Vector '''
	return Vector(*[float(x) for x in v])

# JSON keys mapped to their respective parse functions
MSD_INIT_ARGS = {
	"width": int,
	"height": int,
	"depth": int,
	
	"molType": molType,
	# TODO: molProto
	"molPosL": int,
	"molPosR": int,
	
	"topL": int,
	"bottomL": int,
	"frontR": int ,
	"backR": int
}
CONFIG_PARAMS = {
	"seed": int,
	"randomize": bool
}
MSD_PARAMS = {
	"kT": float,
	"B": vec,
	"SL": float, "SR": float,
	"FL": float, "FR": float,
	"JL": float, "JR": float, "JmL": float, "JmR": float, "JLR": float,
	"Je0L": float, "Je0R": float,
	"Je1L": float, "Je1R": float, "Je1mL": float, "Je1mR": float, "Je1LR": float,
	"JeeL": float, "JeeR": float, "JeemL": float, "JeemR": float, "JeeLR": float,
	"bL": float, "bR": float, "bmL": float, "bmR": float, "bLR": float,
	"AL": vec, "AR": vec,
	"DL": vec, "DR": vec, "DmL": vec, "DmR": vec, "DLR": vec
}
MOL_NODE_PARAMS = {
	"Sm": float,
	"Fm": float,
	"Je0m": float,
	"Am": vec
}
MOL_EDGE_PARAMS = {
	"Jm": float,
	"Je1m": float,
	"Jeem": float,
	"bm": float,
	"Dm": vec
}
SIM_ARGS = {
	"simCount": int,
	"freq": int,
	"dkT": float,
	"dB": vec
}
ALL_TYPES = {
	**MSD_INIT_ARGS,
	**CONFIG_PARAMS,
	**MSD_PARAMS,
	**MOL_NODE_PARAMS,
	**MOL_EDGE_PARAMS,
	**SIM_ARGS
}

def readJSON() -> dict:
	''' Load and parse JSON from given input stream as a dictionary,
		converting data types where appropriate, and return.
	'''
	kw = json.loads(input())
	for k,v in kw.items():
		if k in ALL_TYPES:
			kw[k] = ALL_TYPES[k](v)
	return kw

def setParameters(msd: MSD, kw: dict):
	''' Updates the msd parameters (including molProto parameters)
	    as efficiently as possible. Ignores any keys in given dict that aren't
	    MSD.Parameters, Molecule.NodeParameters, or Molecule.EdgeParameters.
	'''
	if {"kT", "B"}.issuperset(kw.keys()):
		# Efficient update?:
		if "B" in kw:
			msd.B = kw["B"]
		if "kT" in kw:
			msd.kT = kw["kT"]
	else:
		# Filter dict
		kw_p = { k: v for k,v in kw.items() if k in MSD_PARAMS }
		kw_node_p = { k: v for k,v in kw.items() if k in MOL_NODE_PARAMS }
		kw_edge_p = { k: v for k,v in kw.items() if k in MOL_EDGE_PARAMS }
		
		# Update parameters
		if (kw_p):
			msd.parameters = MSD.Parameters(**{
				**msd.getParameters().__dict__,
				**kw_p })
		if kw_node_p or kw_edge_p:
			molProto = msd.getMolProto()
			if kw_node_p:
				for node in molProto.nodes:
					molProto.setNodeParamters(node.index, Molecule.NodeParameters(**{
						**node.getParameters().__dict__,
						**kw_node_p }))
			if kw_edge_p:
				for edge in molProto.edgesUnique:
					molProto.setEdgeParamters(edge.index, Molecule.EdgeParameters(**{
						**edge.getParameters().__dict__,
						**kw_edge_p }))
			msd.molProto = molProto

def state(msd: MSD) -> str:
	''' Gets the state of the current MSD and converts it into JSON. '''

	def encodeMSD(obj):
		TYPES = {
			Vector: list,
			c_ulonglong: int,
			c_double: float
		}
		for pyType, encType in TYPES.items():
			if isinstance(obj, pyType):
				return encType(obj)
		raise TypeError("Can't encode Unrecognized type: " + str(type(obj)))
	
	molProto = msd.getMolProto()
	return json.dumps({
		"results": msd.getResults().__dict__,
		"parameters": msd.getParameters().__dict__,
		
		"msd": [{
			"index": a.index,
			"pos": a.pos,
			"spin": a.spin,
			"flux": a.flux,
			"localM": a.localM
		} for a in msd],

		"mol": {
			"nodes": [{
				"index": node.index,
				"parameters": node.getParameters().__dict__
			} for node in molProto.nodes],

			"edges": [{
				"index": edge.index,
				"parameters": edge.getParameters().__dict__,
				"src": edge.src,
				"dest": edge.dest,
				"direction": edge.direction
			} for edge in molProto.edgesUnique]
		}
	}, default=encodeMSD, indent=None)

def sim(msd: MSD, n: int, dkT = 0., dB = 0.):
	if dkT == 0. and dB == 0.:
		msd.metropolis(n)
	else:
		p = msd.getParameters()
		for _ in range(n):
			msd.metropolis(1)
			# increment kT and B
			p.kT += dkT
			p.B += dB
			# update MSD object
			msd.kT = p.kT
			msd.B = p.B

def main():
	try:
		# 1. Read init parameters from stdin as single line JSON
		kw = readJSON()

		# Construct MSD object and set it up based on input
		msd = MSD(**{k: v for k,v in kw.items() if k in MSD_INIT_ARGS})
		if "seed" in kw:
			msd.seed(kw["seed"])
		if kw.get("randomize", False):
			msd.randomize()
		setParameters(msd, kw)
		print("READY")  # Confirm worker process is crrectly setup and ready for job requests

		# 2. Read a command, then execute
		while True:
			match(input().upper()):
				case "EXIT":
					# Gracefuly destroy this simulation and subprocess.
					break  # stop loop

				case "SET":
					# Change the parameters to the given values (efficiently).
					setParameters(msd, readJSON())
					print("DONE")  # parameters are set
				
				case "RUN":
					# Run metropolis() with the given "simCount" and "freq".
					# MSD state will be periodically sent to stdout.
					# After each state is printed, an input is expected before the simulation can continue.
					# This allows the user to prematurely end the program by sending "CANCEL".
					# All other messages will be treated as "CONTINUE", and the simulation will continue.
					kw = readJSON()
					simCount = kw["simCount"]
					freq = kw.get("freq", 0)
					dkT = kw.get("dkT", 0.)
					dB = kw.get("dB", 0.)
					if freq <= 0:
						freq = simCount
					cont = True
					while cont and simCount >= freq:
						sim(msd, freq, dkT, dB)
						simCount -= freq
						print(state(msd))
						cont = input().upper() != "CANCEL"  # should we continue?
					if cont and simCount > 0:
						sim(msd, simCount, dkT, dB)
						print(state(msd))
						input()  # capture and ignore the last CANCEL? input
					print("DONE")  # signals end of new data

				case "GET":
					# Get the current state of the MSD without running.
					print(state(msd))
				
				case cmd:
					print("Unrecognized command: " + cmd, file=sys.stderr)
	
		# after loop
		print("GOODBYE")

	except EOFError:
		pass  # do nothing, proceed with shutdown

	except:
		print(traceback.format_exc(), file=sys.stderr)
		exit(1)
	
	finally:
		pass  # shutdown

if __name__ == "__main__":
	main()

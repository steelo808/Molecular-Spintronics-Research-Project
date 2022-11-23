# The Python binding of MSD.
# Uses ctypes to call underlying C code.
# See: MSD-export.cpp
#
# Author: Christopher D'Angelo
# Last Updated: November 23, 2022
# Version: 1.0

from ctypes import *
from typing import *
import os


# Load DLL
# TODO: how should the py lib. be configured, e.g. path?
MSD_EXPORT_DLL_PATH = os.path.join(os.path.realpath(os.path.dirname(__file__)), "MSD-export.dll")
msd_clib = CDLL(MSD_EXPORT_DLL_PATH)


# Helpers (classes, functions, etc.)
class _StructWithDict(Structure):
	''' Version of ctypes.Structure that defines __dict__ and overrides __repr__ '''
	def __init__(self, *args, **kw): super().__init__(*args, **kw)
	__dict__ = property(fget = lambda self: {k: getattr(self, k) for k, _ in self._fields_})
	__repr__ = lambda self: str(vars(self))

class _MMTStruct(_StructWithDict):
	''' Adds the MSD-Molecule-Text, mmt() method for subclasses '''
	def __init__(self, *args, **kw): super().__init__(*args, **kw)
	def mmt(self) -> str:
		return "; ".join([f"{k}={v}" for k, v in vars(self).items()])

def _tupler(f, obj, c_types):
	values = [c_type() for c_type in c_types]  # instantiate each c_type in c_types list
	f(obj, *[pointer(v) for v in values])  # pass-by-pointer
	return tuple(values)

def _sig(restype, f, argtypes):
	'''Specify the signature of the given C fucntion'''
	f.restype = restype
	f.argtypes = argtypes


# public Types exposed by MSD.py library
class Vector(_StructWithDict):
	'''Python version of udc::Vector class in Vector.h'''

	_fields_ = [("x", c_double), ("y", c_double), ("z", c_double)]

	def __init__(self, *args, **kw): super().__init__(*args, **kw)

	def __iter__(self): return (self.x, self.y, self.z)
	def __len__(self): return 3
	def __getitem__(self, idx): return (self.x, self.y, self.z)[idx]
	def __repr__(self): return f"({self.x}, {self.y}, {self.z})"
	
	# TODO: add Vector operation ? Low-level??

Vector.I = Vector(1.0, 0.0, 0.0)
Vector.J = Vector(0.0, 1.0, 0.0)
Vector.K = Vector(0.0, 0.0, 1.0)
Vector.ZERO = Vector(0.0, 0.0, 0.0)


class Molecule:

	# inner classes
	class EdgeParameters(_MMTStruct):
		_fields_ = [
			("Jm", c_double),
			("Je1m", c_double),
			("Jeem", c_double),
			("bm", c_double),
			("Dm", Vector)
			]

		def __init__(self, *args, **kw):
			self.Jm = 0.0
			self.Je1m = 0.0
			self.Jeem = 0.0
			self.bm = 0.0
			self.Dm = Vector.ZERO
			super().__init__(*args, **kw)
	

	class NodeParameters(_MMTStruct):
		_fields_ = [
			("Sm", c_double),
			("Fm", c_double),
			("Je0m", c_double),
			("Am", Vector)
			]
		
		def __init__(self, *args, **kw):
			self.Sm = 1.0
			self.Fm = 0.0
			self.Je0m = 0.0
			self.Am = Vector.ZERO
			super().__init__(*args, **kw)
	

	# (export "C") Globals as static constants
	HEADER = c_char_p.in_dll(msd_clib, "HEADER")
	HEADER_SIZE = c_size_t.in_dll(msd_clib, "HEADER_SIZE")
	NOT_FOUND = c_uint.in_dll(msd_clib, "NOT_FOUND")

	# Molecule methods and properties
	def __init__(self, nodeCount = None, nodeParams = None):
		'''
		Construct a new Molecule prototype.
		An Optional nodeCount, and initial NodeParameters can be given.
		If nodeCount is None, an empty prototype is created with
		no nodes, and no edges.

		Both the left and right leads are initialized to 0.
		'''
		self._proto: c_void_p = None
		self._edges: dict[c_uint, tuple[c_uint, c_uint]] = {}
		# TODO: It's redundant to store edges in both Python and C++,
		# 	but no way to access all edge info from Python without new C++ methods.

		if nodeCount is None:
			self._proto = msd_clib.createMolProto_e()
		elif nodeParams is None:
			self._proto = msd_clib.createMolProto_n(nodeCount)
		else:
			self._proto = msd_clib.createMolProto_p(nodeCount, byref(nodeParams))

	def __del__(self): msd_clib.destroyMolProto(self._proto)

	def __repr__(self): return self.mmt()

	def serialize(self, buffer: bytearray): msd_clib.serialize(self._proto, buffer)
	def deserialize(self, buffer: bytearray): msd_clib.deserialize(self._proto, buffer)  # TODO, known bug: edges aren't preserved
	serializationSize = property(fget = lambda self: msd_clib.serializationSize(self._proto))
	
	# TODO: read, write, and load are not yet defined
	def read(self, istream): raise NotImplementedError()  # TODO stub
	def write(self, ostream): raise NotImplementedError()  # TODO stub
	def load(istream): raise NotImplementedError()  # static, TODO stub
	
	def createNode(self, nodeParams = None):
		'''
		Creates a new node in the molecule prototype, and returns its index.
		Optional NodeParameters can be given. Returns the new node's index.
		
		If nodeParam is None, this method uses C++ defaults for nodeParams. This should
		have the same effect as using a default-constructed Python NodeParameter object.
		'''
		if nodeParams is None:
			return msd_clib.createNode_d(self._proto)
		return msd_clib.createNode_p(self._proto, byref(nodeParams))

	nodeCount = property(fget = lambda self: msd_clib.nodeCount(self._proto))
	
	def connectNodes(self, nodeA, nodeB, edgeParams = None):
		'''
		Connects the two given nodes (by index), and
		returns an edge index of the newly constructed edge.
		Optional EdgeParameters can be given.

		Note: this method does not check if the two nodes are already connected.
	 	It is possible to creates more then one edge connecting the same two nodes.

		If edgeParam is None, this method the uses C++ defaults for nodeParams. This should
		have the same effect as using a default-constructed Python NodeParameter object.
		'''
		idx = None
		if edgeParams is None:
			idx = msd_clib.connectNodes_d(self._proto, nodeA, nodeB)
		else:
			idx = msd_clib.connectNodes_p(self._proto, nodeA, nodeB, byref(edgeParams))
		self._edges[idx] = (nodeA, nodeB)
		return idx

	def edgeIndex(self, nodeA, nodeB):
		'''
		Search for and return the index of the edge connecting the given nodes if one exists,
		or Molecule.NOT_FOUND if one doesn't.
	 	'''
		return msd_clib.edgeIndex(self._proto, nodeA, nodeB)
	
	def getEdgeParameters(self, index): return msd_clib.getEdgeParameters(self._proto, index)
	def setEdgeParamters(self, index, edgeParams): msd_clib.setEdgeParameters(self._proto, index, byref(edgeParams))
	def getNodeParameters(self, index): return msd_clib.getNodeParameters(self._proto, index)
	def setNodeParamters(self, index, nodeParams): msd_clib.setNodeParameters(self._proto, index, byref(nodeParams))
	def setAllParameters(self, nodeParams, edgeParams): msd_clib.setAllParameters(self._proto, byref(nodeParams), byref(edgeParams))

	leftLead = property(
		fget = lambda self: msd_clib.getLeftLead(self._proto),
		fset = lambda self, node: msd_clib.setLeftLead(self._proto, node)
		)
	
	rightLead = property(
		fget = lambda self: msd_clib.getRightLead(self._proto),
		fset = lambda self, node: msd_clib.setRightLead(self._proto, node)
		)

	leads = property(
		fget = lambda self: _tupler(msd_clib.getLeads, self._proto, 2 * [c_uint]),
		fset = lambda self, leads: msd_clib.setLeads(self._proto, *leads)
		)
	
	def mmt(self):
		''' MMT formated string '''
		# Nodes
		n = self.nodeCount
		mmt = str(n) + "\n"
		for i in range(n):
			mmt += self.getNodeParameters(i).mmt() + "\n"
		mmt += "\n"

		# Edges
		n = len(self._edges)
		mmt += str(n) + "\n"
		for edge, verts in self._edges.items():
			mmt += self.getEdgeParameters(edge).mmt()
			mmt += f"; srcNode={verts[0]}; destNode={verts[1]}\n"
		mmt += "\n"

		# Leads
		mmt += f"{self.leftLead}\n{self.rightLead}\n"

		return mmt

class MSD:

	# static typedef
	MolProto = Molecule

	# (export "C") Globals as static constants
	LINEAR_MOL = c_void_p.in_dll(msd_clib, "LINEAR_MOL")
	CIRCULAR_MOL = c_void_p.in_dll(msd_clib, "CIRCULAR_MOL")

	UP_DOWN_MODEL = c_void_p.in_dll(msd_clib, "UP_DOWN_MODEL")
	CONTINUOUS_SPIN_MODEL = c_void_p.in_dll(msd_clib, "CONTINUOUS_SPIN_MODEL")


	# inner classes
	class Parameters(_StructWithDict):
		_fields_ = [
			("kT", c_double),
			("B", Vector),
			("SL", c_double), ("SR", c_double),
			("FL", c_double), ("FR", c_double),
			("JL", c_double), ("JR", c_double), ("JmL", c_double), ("JmR", c_double), ("JLR", c_double),
			("Je0L", c_double), ("Je0R", c_double),
			("Je1L", c_double), ("Je1R", c_double), ("Je1mL", c_double), ("Je1mR", c_double), ("Je1LR", c_double),
			("JeeL", c_double), ("JeeR", c_double), ("JeemL", c_double), ("JeemR", c_double), ("JeeLR", c_double),
			("bL", c_double), ("bR", c_double), ("bmL", c_double), ("bmR", c_double), ("bLR", c_double),
			("AL", Vector), ("AR", Vector),
			("DL", Vector), ("DR", Vector), ("DmL", Vector), ("DmR", Vector), ("DLR", Vector)				
			]
		
		def __init__(self, *args, **kw):
			self.kT = 0.25
			self.B = Vector.ZERO
			self.SL, self.SR = 1.0, 1.0
			self.FL, self.FR = 0.0, 0.0
			self.JL, self.JR, self.JmL, self.JmR, self.JLR = 1.0, 1.0, 1.0, -1.0, 0.0
			self.Je0L, self.Je0R = 0.0, 0.0
			self.Je1L, self.Je1R, self.Je1mL, self.Je1mR, self.Je1LR = 0.0, 0.0, 0.0, 0.0, 0.0
			self.JeeL, self.JeeR, self.JeemL, self.JeemR, self.JeeLR = 0.0, 0.0, 0.0, 0.0, 0.0
			self.bL, self.bR, self.bmL, self.bmR, self.bLR = 0.0, 0.0, 0.0, 0.0, 0.0
			self.AL, self.AR = Vector.ZERO, Vector.ZERO
			self.DL, self.DR, self.DmL, self.DmR, self.DLR = Vector.ZERO, Vector.ZERO, Vector.ZERO, Vector.ZERO, Vector.ZERO
			super().__init__(*args, **kw)


	class Results(_StructWithDict):
		_fields_ = [
			("t", c_ulonglong),
			("M", Vector), ("ML", Vector), ("MR", Vector), ("Mm", Vector),
			("MS", Vector), ("MSL", Vector), ("MSR", Vector), ("MSm", Vector),
			("MF", Vector), ("MFL", Vector), ("MFR", Vector), ("MFm", Vector),
			("U", c_double), ("UL", c_double), ("UR", c_double), ("Um", c_double), ("UmL", c_double), ("UmR", c_double), ("ULR", c_double)
			]

		def __init__(self, *args, **kw):
			self.t = 0
			self.M, self.ML, self.MR, self.Mm = Vector.ZERO, Vector.ZERO, Vector.ZERO, Vector.ZERO
			self.MS, self.MSL, self.MSR, self.MSm = Vector.ZERO, Vector.ZERO, Vector.ZERO, Vector.ZERO
			self.MF, self.MFL, self.MFR, self.MFm = Vector.ZERO, Vector.ZERO, Vector.ZERO, Vector.ZERO
			self.U, self.UL, self.UR, self.Um, self.UmL, self.UmR, self.ULR = 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
			super().__init__(*args, **kw)


	# MSD Methods and Properties
	def __init__(self, width, height, depth, \
			molProto: Optional[MolProto] = None, molType: Optional[c_void_p] = None, \
			molPosL = None, molPosR = None, \
			topL = None, bottomL = None, frontR = None, backR = None, \
			molPos = None, molLen = None, heightL = None, depthR = None
	):
		self._msd: c_void_p = None
		
		if molProto is not None:
			nodeCount = molProto.nodeCount
			if molLen is None:
				molLen = nodeCount
			elif nodeCount != molLen:
				raise ArgumentError(f"molProto.nodeCount ({nodeCount}) != molLen ({molLen})")				

		if molPos is not None:
			if molPosL is None:
				molPosL = molPos
			elif molPosL != molPos:
				raise ArgumentError(f"molPos ({molPos}) != molPosL ({molPosL})")
			
			if molPosR is None:
				molPosR = molPos
			elif molPos != molPos:
				raise ArgumentError(f"molPos ({molPos}) != molPosR ({molPosR})")

		if molLen is not None:
			L = molPosL is not None
			R = molPosR is not None
			if L and R:
				if molLen != molPosR - molPosL + 1:
					raise ArgumentError(f"molLen ({molLen}) != molPosR ({molPosR}) - molPosL ({molPosL}) + 1")
			elif L:
				molPosR = molPosL + (molLen - 1)
			elif R:
				molPosL = molPosR - (molLen - 1)
			else:
				molPosL = int((width - molLen) / 2)
				molPosR = molPosL + (molLen - 1)

		if heightL is not None:
			T = topL is not None
			B = bottomL is not None
			if T and B:
				if heightL != bottomL - topL + 1:
					raise ArgumentError(f"heightL ({heightL}) != bottomL ({bottomL}) - topL ({topL}) + 1")
			elif T:
				bottomL = topL + (heightL - 1)
			elif B:
				topL = bottomL - (heightL - 1)
			else:
				topL = int((height - heightL) / 2)
				bottomL = topL + (heightL - 1)
		
		if depthR is not None:
			F = frontR is not None
			B = backR is not None
			if F and B:
				if depthR != backR - frontR + 1:
					raise ArgumentError(f"depthR ({depthR}) != backR ({backR}) - frontR ({frontR}) + 1")
			elif F:
				backR = frontR + (depthR - 1)
			elif B:
				frontR = backR - (depthR - 1)
			else:
				frontR = int((depth - depthR) / 2)
				backR = frontR + (depthR - 1)
		
		if molPosL is None != molPosR is None:
			raise ArgumentError("molPosL and molPosR must both be specified, implied, or omitted together")
		
		if molPosL is None:  # and molPosR is None
			molPosL = int((width - 1) / 2)
			molPosR = molPosL
		
		if topL is None:
			topL = 0
		if bottomL is None:
			bottomL = height - 1
		if frontR is None:
			frontR = 0
		if backR is None:
			backR = depth - 1

		if molProto is not None and molType is not None:
			raise ArgumentError("Can not specify both molProto and molType")

		if molProto is not None:
			self._msd = msd_clib.createMSD_p(width, height, depth, molProto._proto, molPosL, topL, bottomL, frontR, backR)
		elif molType is not None:
			self._msd = msd_clib.createMSD_f(width, height, depth, molType, molPosL, molPosR, topL, bottomL, frontR, backR)
		else:
			self._msd = msd_clib.createMSD_i(width, height, depth, molPosL, molPosR, topL, bottomL, frontR, backR)

	def __del__(self): msd_clib.destroyMSD(self._msd)

	@property
	def record(self):
		n = msd_clib.getRecordSize(self._msd)
		cArr = msd_clib.getRecord(self._msd)
		return [cArr[i] for i in range(n)]
	
	@record.setter
	def record(self, record):
		n = len(record)
		msd_clib.setRecord(self._msd, (MSD.Results * n)(*record), n)
	
	flippingAlgorithm = property(fset = lambda self, algo: msd_clib.setFlippingAlgorithm(self._msd, algo))

	def getParameters(self): return msd_clib.getParameters(self._msd)
	def setParameters(self, parameters): msd_clib.setParameters(self._msd, parameters)
	# No parameters property fget because I don't want to accidentally expose the anti-pattern:
	# msd.parameters.kT = 0.2 wouldn't doesn't change the underlying parameters!!
	parameters = property(fset = setParameters)
	def getResults(self): return msd_clib.getResults(self._msd)
	results = property(fget = getResults)

	def set_kT(self, kT): msd_clib.set_kT(self._msd, kT)
	kT = property(fset = set_kT)	
	def setB(self, B): msd_clib.setB(self._msd, byref(B))
	B = property(fset = setB)

	def getMolProto(self): pass  #TODO
	def setMolProto(self, molProto): pass  #TODO
	def setMolParameters(self, nodeParams = Molecule.NodeParameters(), edgeParams = Molecule.EdgeParameters()):
		pass  #TODO

	def getSpin(self, x, y = None, z = None):
		if y is None:
			return msd_clib.getSpin_i(self._msd, x)
		return msd_clib.getSpin_v(self._msd, x, y, z)
	
	def getFlux(self, x, y = None, z = None):
		if y is None:
			return msd_clib.getFlux_i(self._msd, x)
		return msd_clib.getFlux_v(self._msd, x, y, z)
	
	def getLocalM(self, x, y = None, z = None):
		if y is None:
			return msd_clib.getLocalM_i(self._msd, a)
		return msd_clib.getLocalM_v(self._msd, x, y, z)
	
	def setSpin(self, x, y = None, z = None, spin = None):
		'''
		Version 1: setSpin(a, spin)
		Version 2: setSpin(x, y, z, spin)
		
		Note: spin can be passed as a positional argument or a named argument
		'''
		if spin is None:
			if y is None:
				raise ArgumentError("spin must be passed as second positional argument, or as named parameter")
			spin = y
		
		spin = byref(spin)
		if z is None:
			msd_clib.setSpin_i(self._msd, x, spin)
		else:
			msd_clib.setSpin_v(self._msd, x, y, z, spin)
	
	def setFlux(self, x, y = None, z = None, flux = None):
		'''
		Version 1: setFlux(a, flux)
		Version 2: setFlux(x, y, z, flux)

		Note: flux can be passed as a positional argument or a named argument
		'''
		if flux is None:
			if y is None:
				raise ArgumentError("flux must be passed as second positional argument, or as named parameter")
			flux = y
		
		flux = byref(flux)
		if z is None:
			msd_clib.setFlux_i(self._msd, x, flux)
		else:
			msd_clib.setFlux_v(self._msd, x, y, z, flux)
	
	def setLocalM(self, x, y = None, z = None, spin = None, flux = None):
		'''
		Version 1: setLocalM(a, spin, flux)
		Version 2: setLocalM(x, y, z, spin, flux)

		Note: spin and flux can be passed as positional arguments or named arguments
		'''
		mode_i = y is None or spin is None

		if spin is None:
			if y is None:
				raise ArgumentError("spin must be passed as second positional argument, or as named parameter")
			spin = y
		
		if flux is None:
			if z is None:
				raise ArgumentError("flux must be passed as third positional argument, or as named parameter")
			flux = z

		spin = byref(spin)
		flux = byref(flux)
		if mode_i:
			msd_clib.setLocalM_i(self._msd, x, spin, flux)
		else:
			msd_clib.setLocalM_v(self._msd, x, y, z, spin, flux)
	
	def __getitem__(self, idx):
		if isinstance(idx, Iterable):
			return (self.getSpin(*idx), self.getFlux(*idx))
		return (self.getSpin(idx), self.getFlux(idx))
	
	def __setitem__(self, idx, localM):
		if isinstance(idx, Iterable):
			self.setLocalM(*idx, *localM)
		else:
			self.setLocalM(idx, spin = localM[0], flux = localM[1])
	
	n = property(fget = lambda self: msd_clib.getN(self._msd))
	nL = property(fget = lambda self: msd_clib.getNL(self._msd))
	nR = property(fget = lambda self: msd_clib.getNR(self._msd))
	n_m = property(fget = lambda self: msd_clib.getNm(self._msd))
	n_mL = property(fget = lambda self: msd_clib.getNmL(self._msd))
	n_mR = property(fget = lambda self: msd_clib.getNmR(self._msd))
	nLR = property(fget = lambda self: msd_clib.getNLR(self._msd))
	width = property(fget = lambda self: msd_clib.getWidth(self._msd))
	height = property(fget = lambda self: msd_clib.getHeight(self._msd))
	depth = property(fget = lambda self: msd_clib.getDepth(self._msd))
	dimensions = property(fget = lambda self : _tupler(msd_clib.getDimensions, self._msd, 3 * [c_uint]))
	molPosL = property(fget = lambda self: msd_clib.getMolPosL(self._msd))
	molPosR = property(fget = lambda self: msd_clib.getMolPosR(self._msd))
	molPos = property(fget = lambda self: _tupler(msd_clib.getMolPos, self._msd, 2 * [c_uint]))
	topL = property(fget = lambda self: msd_clib.getTopL(self._msd))
	bottomL = property(fget = lambda self: msd_clib.getBottomL(self._msd))
	frontR = property(fget = lambda self: msd_clib.getFrontR(self._msd))
	backR = property(fget = lambda self: msd_clib.getBackR(self._msd))
	innerBounds = property(fget = lambda self: _tupler(msd_clib.getInnerBounds, self._msd, 4 * [c_uint]))
	FM_L_exists = property(fget = lambda self: msd_clib.getFM_L_exists(self._msd))
	FM_R_exists = property(fget = lambda self: msd_clib.getFM_R_exists(self._msd))
	mol_exists = property(fget = lambda self: msd_clib.getMol_exists(self._msd))
	regions = property(fget = lambda self: _tupler(msd_clib.getRegions, self._msd, 3 * [c_bool]))

	seed = property(
		fget = lambda self : msd_clib.getSeed(self._msd),
		fset = lambda self, seed: msd_clib.setSeed(self._msd, seed)
		)

	def reinitialize(self, reseed = True): msd_clib.reinitialize(self._msd, reseed)
	def randomize(self, reseed = True): msd_clib.randomize(self._msd, reseed)

	def metropolis(self, N, freq = None):
		if freq is None:
			msd_clib.metropolis_o(self._msd, N)
		else:
			msd_clib.metropolis_r(self._msd, N, freq)
	
	specificHeat = property(fget = lambda self : msd_clib.specificHeat(self._msd))
	specificHeat_L = property(fget = lambda self : msd_clib.specificHeat_L(self._msd))
	specificHeat_R = property(fget = lambda self : msd_clib.specificHeat_R(self._msd))
	specificHeat_m = property(fget = lambda self : msd_clib.specificHeat_m(self._msd))
	specificHeat_mL = property(fget = lambda self : msd_clib.specificHeat_mL(self._msd))
	specificHeat_mR = property(fget = lambda self : msd_clib.specificHeat_mR(self._msd))
	specificHeat_LR = property(fget = lambda self : msd_clib.specificHeat_LR(self._msd))
	magneticSusceptibility = property(fget = lambda self: msd_clib.magneticSusceptibility(self._msd))
	magneticSusceptibility_L = property(fget = lambda self: msd_clib.magneticSusceptibility_L(self._msd))
	magneticSusceptibility_R = property(fget = lambda self: msd_clib.magneticSusceptibility_R(self._msd))
	magneticSusceptibility_m = property(fget = lambda self: msd_clib.magneticSusceptibility_m(self._msd))

	meanM = property(fget = lambda self : msd_clib.meanM(self._msd))
	meanML = property(fget = lambda self : msd_clib.meanML(self._msd))
	meanMR = property(fget = lambda self : msd_clib.meanMR(self._msd))
	meanMm = property(fget = lambda self : msd_clib.meanMm(self._msd))
	meanMS = property(fget = lambda self : msd_clib.meanMS(self._msd))
	meanMSL = property(fget = lambda self : msd_clib.meanMSL(self._msd))
	meanMSR = property(fget = lambda self : msd_clib.meanMSR(self._msd))
	meanMSm = property(fget = lambda self : msd_clib.meanMSm(self._msd))
	meanMF = property(fget = lambda self : msd_clib.meanMF(self._msd))
	meanMFL = property(fget = lambda self : msd_clib.meanMFL(self._msd))
	meanMFR = property(fget = lambda self : msd_clib.meanMFR(self._msd))
	meanMFm = property(fget = lambda self : msd_clib.meanMFm(self._msd))
	meanU = property(fget = lambda self : msd_clib.meanU(self._msd))
	meanUL = property(fget = lambda self : msd_clib.meanUL(self._msd))
	meanUR = property(fget = lambda self : msd_clib.meanUR(self._msd))
	meanUm = property(fget = lambda self : msd_clib.meanUm(self._msd))
	meanUmL = property(fget = lambda self : msd_clib.meanUmL(self._msd))
	meanUmR = property(fget = lambda self : msd_clib.meanUmR(self._msd))
	meanULR = property(fget = lambda self : msd_clib.meanULR(self._msd))


# (export "C") Function Signatures/Declarations
_sig(c_void_p, msd_clib.createMSD_p, 3 * [c_uint] + [c_void_p] + 5 * [c_uint])
_sig(c_void_p, msd_clib.createMSD_f, 3 * [c_uint] + [c_void_p] + 6 * [c_uint])
_sig(c_void_p, msd_clib.createMSD_i, 9 * [c_uint])
_sig(c_void_p, msd_clib.createMSD_c, 5 * [c_uint])
_sig(c_void_p, msd_clib.createMSD_d, 3 * [c_uint])
_sig(None, msd_clib.destroyMSD, [c_void_p])

_sig(POINTER(MSD.Results), msd_clib.getRecord, [c_void_p])  # returns c-array
_sig(c_size_t, msd_clib.getRecordSize, [c_void_p])
_sig(None, msd_clib.setRecord, [c_void_p, POINTER(MSD.Results), c_size_t])  # takes c-array
_sig(None, msd_clib.setFlippingAlgorithm, 2 * [c_void_p])

_sig(MSD.Parameters, msd_clib.getParameters, [c_void_p])
_sig(None, msd_clib.setParameters, [c_void_p, MSD.Parameters])
_sig(MSD.Results, msd_clib.getResults, [c_void_p])

_sig(None, msd_clib.set_kT, [c_void_p, c_double])
_sig(None, msd_clib.setB, [c_void_p, POINTER(Vector)])

_sig(None, msd_clib.setMolProto, 2 * [c_void_p])
_sig(None, msd_clib.setMolParameters, 3 * [c_void_p])

_sig(Vector, msd_clib.getSpin_i, [c_void_p, c_uint])
_sig(Vector, msd_clib.getSpin_v, [c_void_p] + 3 * [c_uint])
_sig(Vector, msd_clib.getFlux_i, [c_void_p, c_uint])
_sig(Vector, msd_clib.getFlux_v, [c_void_p] + 3 * [c_uint])
_sig(Vector, msd_clib.getLocalM_i, [c_void_p, c_uint])
_sig(Vector, msd_clib.getLocalM_v, [c_void_p] + 3 * [c_uint])
_sig(None, msd_clib.setSpin_i, [c_void_p, c_uint, POINTER(Vector)])
_sig(None, msd_clib.setSpin_v, [c_void_p] + 3 * [c_uint] + [POINTER(Vector)])
_sig(None, msd_clib.setFlux_i, [c_void_p, c_uint, POINTER(Vector)])
_sig(None, msd_clib.setFlux_v, [c_void_p] + 3 * [c_uint] + [POINTER(Vector)])
_sig(None, msd_clib.setLocalM_i, [c_void_p, c_uint] + 2 * [POINTER(Vector)])
_sig(None, msd_clib.setLocalM_v, [c_void_p] + 3 * [c_uint] + 2 * [POINTER(Vector)])

_sig(c_uint, msd_clib.getN, [c_void_p])
_sig(c_uint, msd_clib.getNL, [c_void_p])
_sig(c_uint, msd_clib.getNR, [c_void_p])
_sig(c_uint, msd_clib.getNm, [c_void_p])
_sig(c_uint, msd_clib.getNmL, [c_void_p])
_sig(c_uint, msd_clib.getNmR, [c_void_p])
_sig(c_uint, msd_clib.getNLR, [c_void_p])
_sig(c_uint, msd_clib.getWidth, [c_void_p])
_sig(c_uint, msd_clib.getHeight, [c_void_p])
_sig(c_uint, msd_clib.getDepth, [c_void_p])
_sig(None, msd_clib.getDimensions, [c_void_p]  + 3 * [POINTER(c_uint)])
_sig(c_uint, msd_clib.getMolPosL, [c_void_p])
_sig(c_uint, msd_clib.getMolPosR, [c_void_p])
_sig(None, msd_clib.getMolPos, [c_void_p] + 2 * [POINTER(c_uint)])
_sig(c_uint, msd_clib.getTopL, [c_void_p])
_sig(c_uint, msd_clib.getBottomL, [c_void_p])
_sig(c_uint, msd_clib.getFrontR, [c_void_p])
_sig(c_uint, msd_clib.getBackR, [c_void_p])
_sig(None, msd_clib.getInnerBounds, [c_void_p] + 4 * [POINTER(c_uint)])
_sig(c_bool, msd_clib.getFM_L_exists, [c_void_p])
_sig(c_bool, msd_clib.getFM_R_exists, [c_void_p])
_sig(c_bool, msd_clib.getMol_exists, [c_void_p])
_sig(None, msd_clib.getRegions, [c_void_p] + 3 * [POINTER(c_bool)])

_sig(None, msd_clib.setSeed, [c_void_p, c_ulong])
_sig(c_ulong, msd_clib.getSeed, [c_void_p])

_sig(None, msd_clib.reinitialize, [c_void_p, c_bool])
_sig(None, msd_clib.randomize, [c_void_p, c_bool])
_sig(None, msd_clib.metropolis_o, [c_void_p, c_ulonglong])
_sig(None, msd_clib.metropolis_r, [c_void_p] + 2 * [c_ulonglong])

_sig(c_double, msd_clib.specificHeat, [c_void_p])
_sig(c_double, msd_clib.specificHeat_L, [c_void_p])
_sig(c_double, msd_clib.specificHeat_R, [c_void_p])
_sig(c_double, msd_clib.specificHeat_m, [c_void_p])
_sig(c_double, msd_clib.specificHeat_mL, [c_void_p])
_sig(c_double, msd_clib.specificHeat_mR, [c_void_p])
_sig(c_double, msd_clib.specificHeat_LR, [c_void_p])
_sig(c_double, msd_clib.magneticSusceptibility, [c_void_p])
_sig(c_double, msd_clib.magneticSusceptibility_L, [c_void_p])
_sig(c_double, msd_clib.magneticSusceptibility_R, [c_void_p])
_sig(c_double, msd_clib.magneticSusceptibility_m, [c_void_p])

_sig(Vector, msd_clib.meanM, [c_void_p])
_sig(Vector, msd_clib.meanML, [c_void_p])
_sig(Vector, msd_clib.meanMR, [c_void_p])
_sig(Vector, msd_clib.meanMm, [c_void_p])
_sig(Vector, msd_clib.meanMS, [c_void_p])
_sig(Vector, msd_clib.meanMSL, [c_void_p])
_sig(Vector, msd_clib.meanMSR, [c_void_p])
_sig(Vector, msd_clib.meanMSm, [c_void_p])
_sig(Vector, msd_clib.meanMF, [c_void_p])
_sig(Vector, msd_clib.meanMFL, [c_void_p])
_sig(Vector, msd_clib.meanMFR, [c_void_p])
_sig(Vector, msd_clib.meanMFm, [c_void_p])
_sig(c_double, msd_clib.meanU, [c_void_p])
_sig(c_double, msd_clib.meanUL, [c_void_p])
_sig(c_double, msd_clib.meanUR, [c_void_p])
_sig(c_double, msd_clib.meanUm, [c_void_p])
_sig(c_double, msd_clib.meanUmL, [c_void_p])
_sig(c_double, msd_clib.meanUmR, [c_void_p])
_sig(c_double, msd_clib.meanULR, [c_void_p])

_sig(c_void_p, msd_clib.createMolProto_e, [])
_sig(c_void_p, msd_clib.createMolProto_n, [c_size_t])
_sig(c_void_p, msd_clib.createMolProto_p, [c_size_t, POINTER(Molecule.NodeParameters)])
_sig(c_void_p, msd_clib.destroyMolProto, [c_void_p])

_sig(None, msd_clib.serialize, [c_void_p, c_char_p])
_sig(None, msd_clib.deserialize, [c_void_p, c_char_p])
_sig(c_size_t, msd_clib.serializationSize, [c_void_p])

_sig(c_uint, msd_clib.createNode_d, [c_void_p])
_sig(c_uint, msd_clib.createNode_p, [c_void_p, POINTER(Molecule.NodeParameters)])
_sig(c_uint, msd_clib.nodeCount, [c_void_p])
_sig(c_uint, msd_clib.connectNodes_d, [c_void_p] + 2 * [c_uint])
_sig(c_uint, msd_clib.connectNodes_p, [c_void_p] + 2 * [c_uint] + [POINTER(Molecule.EdgeParameters)])
_sig(c_uint, msd_clib.edgeIndex, [c_void_p] + 2 * [c_uint])

_sig(Molecule.EdgeParameters, msd_clib.getEdgeParameters, [c_void_p, c_uint])
_sig(None, msd_clib.setEdgeParameters, [c_void_p, c_uint, POINTER(Molecule.EdgeParameters)])
_sig(Molecule.NodeParameters, msd_clib.getNodeParameters, [c_void_p, c_uint])
_sig(None, msd_clib.setNodeParameters, [c_void_p, c_uint, POINTER(Molecule.NodeParameters)])
_sig(None, msd_clib.setAllParameters, [c_void_p, POINTER(Molecule.NodeParameters), POINTER(Molecule.EdgeParameters)])

_sig(None, msd_clib.setLeftLead, [c_void_p, c_uint])
_sig(None, msd_clib.setRightLead, [c_void_p, c_uint])
_sig(None, msd_clib.setLeads, [c_void_p] + 2 * [c_uint])
_sig(c_uint, msd_clib.getLeftLead, [c_void_p])
_sig(c_uint, msd_clib.getRightLead, [c_void_p])
_sig(None, msd_clib.getLeads, [c_void_p] + 2 * [POINTER(c_uint)])



# DEBUG: TESTS
if __name__ == "__main__":
	print("Under construction. (11-23-2022)")
	print()

	print("---- Molecule, i.e. MSD.MolProto test ----")
	cube_mol = MSD.MolProto(8)
	vertPairs = [
		(0, 1), (1, 2), (2, 3), (3, 0),
		(4, 5), (5, 6), (6, 7), (7, 0),
		(0, 4), (1, 5), (2, 6), (3, 7)
		]
	edgeIdx = []
	for a, b in vertPairs:
		edgeIdx.append(cube_mol.connectNodes(a, b))
	print("edgeIdx:", edgeIdx)
	for i in range(0, 4):
		cube_mol.setNodeParamters(i, Molecule.NodeParameters(Sm = 2, Fm = 0.1))
	for i in range(4, 8):
		cube_mol.setNodeParamters(i, Molecule.NodeParameters(Sm = 5, Fm = 0.25))
	cube_mol.leads = (0, 6)
	print("cube_mol:", cube_mol, sep = "\n")
	print()

	print("---- Serialization/Deserialization test ----")
	buf_size = cube_mol.serializationSize
	buffer = create_string_buffer(buf_size)
	print("buffer (empty):", buffer)
	cube_mol.serialize(buffer)
	print("buffer (full):", buffer)
	copy_mol = MSD.MolProto()
	print("copy_mol (empty):", copy_mol, sep = "\n")
	copy_mol.deserialize(buffer)
	print("copy_mol (full):", copy_mol, sep = "\n")
	print()

	print("---- MSD constructor test ----")
	msd = MSD(13, 10, 10, molPosL = 5, molPosR = 7, molType = MSD.CIRCULAR_MOL, topL = 3, bottomL = 7, frontR = 3, backR = 7)
	msd = MSD(18, 10, 10, molPosL = 5, molProto = copy_mol, topL = 3, bottomL = 7, frontR = 3, backR = 7)
	msd.flippingAlgorithm = MSD.UP_DOWN_MODEL
	print()

	print("---- seed test ----")
	print("seed (pre):", msd.seed)
	msd.seed = 0
	print("seed (post):", msd.seed)
	print()

	print("--- parameters/results test ----")
	p = MSD.Parameters(FL = 0.1, FR = 0.1)
	msd.setParameters(p)
	msd.kT = 0.5
	msd.B = Vector(0.2, 0, 0)
	print("Parameters:", msd.getParameters())
	msd.metropolis(100000)
	print("Results:", msd.results)
	print()

	print("---- spin/flux/localM test ----")
	x, y, z = 0, 3, 0
	a = x + msd.width * (y + msd.height * z)
	msd.setSpin(x, y, z, Vector(10, 0, 0))
	msd.setFlux(a, Vector(0, 0, -1))
	msd.setLocalM(a, Vector.I, Vector.J)
	print("spin:", msd.getSpin(x, y, z), msd.getSpin(a))
	print("flux:", msd.getFlux(x, y, z), msd.getFlux(a))
	print("localM:", msd.getLocalM(x, y, z), msd.getLocalM(a))
	print()

	print("---- dimensions and other properties test ----")
	print("n:", msd.n, msd.nL, msd.nR, msd.n_m, msd.n_mL, msd.n_mR, msd.nLR)
	print("dimensions:", msd.width, msd.height, msd.depth, msd.dimensions)
	print("molPos:", msd.molPosL, msd.molPosR, msd.molPos)
	print("innerBounds:", msd.topL, msd.bottomL, msd.frontR, msd.backR, msd.innerBounds)
	print("regions:", msd.FM_L_exists, msd.FM_R_exists, msd.mol_exists, msd.regions)
	print()
	
	print("---- reinitialize/randomize test ----")
	msd.reinitialize()
	print("Parameters (post-reinitialize):", msd.getParameters())
	print("Results (post-reinitialize):", msd.getResults())
	print("seed (post-reinitialize):", msd.seed)
	msd.randomize(reseed = False)
	print("Parameters (post-randomize):", msd.getParameters())
	print("Results (post-randomize):", msd.getResults())
	print("seed (post-randomize):", msd.seed)
	print()

	print("---- record test ----")
	msd.metropolis(int(2e6), freq = int(500e3))
	print(msd.record)
	msd.record = []
	print(msd.record)
	msd.record = [MSD.Results(t = 200)]
	print(msd.record)
	print()

	print("---- __getitem__/__setitem__ test")
	msd[x,y,z] = (Vector.ZERO, Vector.K)
	msd[a] = msd[x,y,z]
	print("__getitem__:", msd[x,y,z], msd[a])
	print()

	print("---- __del__ test ----")
	del msd
	print()

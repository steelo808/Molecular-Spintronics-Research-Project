/*
 * MSD.h
 *
 *  Last Edited: August 20, 2021
 *       Author: Christopher D'Angelo
 * 
 * TODO: update MSD::init, MSD fields, MSD::setLocalM, MSD::setParameters, and etc. to utilize new graph-based Mol.
 */

#ifndef UDC_MSD
#define UDC_MSD

#define UDC_MSD_VERSION "6.0"

#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cstring>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include "Vector.h"
#include "udc.h"
#include "SparseArray.h"


namespace udc {

using std::bind;
using std::function;
using std::invalid_argument;
using std::mt19937_64;
using std::ostream;
using std::out_of_range;
using std::ref;
using std::string;
using std::uniform_int_distribution;
using std::uniform_real_distribution;

using udc::E;
using udc::PI;
using udc::sq;
using udc::Vector;
using udc::SparseArray;
using udc::bread;
using udc::bwrite;


/**
 * An abstract molecule. Used in MSD.
 * A "Molecule" object only represents a prototype for a molecule design.
 * Use the instantiate() method to create a Molecule::Instance which has a unique spin state.
 */
class Molecule {
	friend class MSD;

 public:
	/**
	 * Information about an edge between two Nodes. 
	 */
	struct EdgeParameters {
		// nearest-neighbor parameters
		double Jm;    // spin * spin
		double Je1m;  // spin * flux
		double Jeem;  // flux * flux
		double bm;    // Biquadratic Coupling
		Vector Dm;    // Dzyaloshinskii-Moriya interaction (i.e. Skyrmions)

		EdgeParameters();
	};

	/** local parameters */
	struct NodeParameters {
		double Sm, Fm;  // spin and spin fluctuation magnetude scalars
		double Je0m;     // spin * flux (local)
		Vector Am;      // Anisotropy

		NodeParameters();
	};

 private:
	struct Node;
	struct Edge;

	struct Edge {
		size_t edgeIndex, nodeIndex, selfIndex;  // the indices in the Molecule's EdgeParameter or Node array
		EdgeParameters *parameters;
		Node *node, *self;  // "node" is the neighbor. "self" is the node this Edge is attached to.

		Edge(size_t edgeIndex, size_t nodeIndex, size_t selfIndex, EdgeParameters *parameters, Node *node, Node *self);
		Edge(Molecule &mol, size_t edgeIndex, size_t nodeIndex, size_t selfIndex);
	};

	struct Node {
		NodeParameters parameters;
		std::vector<Edge> neighbors;
	};

 // ----- Molecule (prototype) fields and methods -----
 private:
	std::vector<EdgeParameters> edgeParameters;   // array of edges
	std::vector<Node> nodes;  // array of nodes (e.g. atoms) making up this Molecule
	size_t leftLead, rightLead;  // the position of the nodes connected to the left FM and right FM, respectively.

 public:
	static const unsigned int NOT_FOUND = -1;  // -1 will be the maximum unsigned int

 public:
	/**
	 * Create a default molecule with no nodes or edges.
	 * Both the left and right leads are initalize to 0.
	 */
	Molecule();

	/**
	 * Create a molecule with the given number of nodes, but no edges.
	 * Both the left and right leads are initalized to 0.
	 * 
	 * May also supply a set of parameters to use for each node.
	 * By default, the default node parameters are used.
	 */
	Molecule(size_t n_m, const NodeParameters &nodeParams = NodeParameters());

 public:
	void serialize(unsigned char *buffer) const;  // store this Molecule object into the given buffer 
	void deserialize(const unsigned char *buffer);  // reconstruct this Molecule object using the given buffer
	size_t serializationSize() const;  // the size needed (in bytes) to serialize this Molecule object

	/**
	 * Returns the new node's index.
	 */
	unsigned int createNode(const NodeParameters &parameters = NodeParameters());

	/**
	 * (i.e. "n_m") 
	 * Returns the number of nodes/atoms in this molecule.
	 */
	unsigned int nodeCount() const;

	/**
	 * Creates a connection (Edge) between the two given nodes, and
	 * returns the new Edge's index.
	 * 
	 * Note: this method does not check if the two nodes are already connected.
	 * It is possible to creates more then one edge connecting the same two nodes.
	 */
	unsigned int connectNodes(unsigned int nodeA, unsigned int nodeB, const EdgeParameters &parameters = EdgeParameters());

	/**
	 * Search for and return the index of the edge connecting the given nodes if one exists,
	 * or MOLECULE::NOT_FOUND if one doesn't.
	 */
	unsigned int edgeIndex(unsigned int nodeA, unsigned int nodeB) const;

	EdgeParameters getEdgeParameters(unsigned int index) const;
	void setEdgeParameters(unsigned int index, const EdgeParameters &p);
	NodeParameters getNodeParameters(unsigned int index) const;
	void setNodeParameters(unsigned int index, const NodeParameters &p);

	// TODO? Add methods for getting adjacency list for each node?

	void setLeftLead(unsigned int node);
	void setRightLead(unsigned int node);
	void setLeads(unsigned int left, unsigned int right);
	unsigned int getLeftLead() const;
	unsigned int getRightLead() const;
	void getLeads(unsigned int &left, unsigned int &right) const;

 // ----- Molecule::Instance stuff -----
 private:
	/**
	 * Represents an actuallized instance of the prototype Molecule,
	 * and contains state information for each part/atom of the molecule.
	 */
	class Instance {
		friend class Molecule;
	
	 private:
		const Molecule &prototype;  // contains a reference to the the Molecular structure and parameters
		MSD &msd;  // a reference to the MSD this Molecule::Instace is attached to
		unsigned int y, z;  // the (x,y,z) position of this Molecule::Instance. Note: the mol's x position is determined by msd.molPosL, msd.molPosR.  
		std::vector<Vector> spins;  // states of each "Node"
		std::vector<Vector> fluxes;

		Instance(const Molecule &prototype, MSD &msd, unsigned int y, unsigned int z);
		Instance(const Instance &other);

		Instance& operator=(const Instance &other);  // copies the spin/flux states
	
	 public:
		void setLocalM(unsigned int a, const Vector &spin, const Vector &flux);
		void setSpin(unsigned int a, const Vector &spin);
		void setFlux(unsigned int a, const Vector &flux);
		Vector getSpin(unsigned int a) const;
		Vector getFlux(unsigned int a) const;
		void getLocalM(unsigned int a, Vector &spin, Vector &flux) const;
	};

	/**
	 * All spins start up (Sm * Vector::I), and all fluxes start at 0 (Vector::ZERO).
	 * 
	 * Instances should be created after the Molecule (prototype) has been configured.
	 * If the parent Molecule (prototype) object is modified after this method is called,
	 * the previously existing Instance objects returned by those calls may be invalid;
	 * they should be discarded and new Instance objects should be created.
	 * 
	 * TL;DR modifying the Molecule object after invoking instantiate leads to undefined behavior.
	 * 
	 * @param msd: The MSD which this Mol is attached.
	 * @param y: The y-cordinate for this Mol within the MSD.
	 * @param z: The z-cordinate for this Mol within the MSD.
	 */
	Instance instantiate(MSD &msd, unsigned int y, unsigned int z) const;
};


/**
 * An abstract Molecular Spintronic Device.
 */
class MSD {
 public:
	typedef function<Vector(const Vector &, function<double()>)> FlippingAlgorithm;
	typedef Molecule MolProto;
	typedef Molecule::Instance Mol;

	// needed so these methods can update energy and magnetization of the MSD
	friend void Mol::setLocalM(unsigned int a, const Vector &spin, const Vector &flux);
	friend void Mol::setSpin(unsigned int a, const Vector &spin);
	friend void Mol::setFlux(unsigned int a, const Vector &flux);

	struct Parameters {
		double kT;  // Temperature
		Vector B;  // Magnetic field
		double sL, sm, sR;  // spin magnitudes
		double FL, FR, Fm;  // Spin fluctuation constants, flux.norm() = uniform random between [0, F)
		double JL, JR, Jm, JmL, JmR, JLR;  // Heisenberg exchange coupling between s_i and s_j: local spin and neighboring spins
		double Je0L, Je0R, Je0m;  // Exchange coupling between s_i and f_i: local spin and local flux vector
		double Je1L, Je1R, Je1m, Je1mL, Je1mR, Je1LR;  // Exchange coupling between s_i and f_j: local spin and neighboring flux vectors
		double JeeL, JeeR, Jeem, JeemL, JeemR, JeeLR;  // Exchange coupling between f_i and f_j: local flux and neighboring flux vectors
		double bL, bR, bm, bmL, bmR, bLR;  // Biquadratic Coupling
		Vector AL, AR, Am;  // Anisotropy
		Vector DL, DR, Dm, DmL, DmR, DLR;  // Dzyaloshinskii-Moriya interaction (i.e. Skyrmions)
		
		Parameters();
		
		bool operator==(const Parameters &) const;
		bool operator!=(const Parameters &) const;
	};
	
	struct Results {
		unsigned long long t;  // time, i.e. current iteration
		Vector M, ML, MR, Mm;  // Magnetization
		Vector MS, MSL, MSR, MSm;  // Magnetization due to spin only (no flux)
		Vector MF, MFL, MFR, MFm;  // Magnetization due to flux only (no spin) 
		double U, UL, UR, Um, UmL, UmR, ULR;  // Internal Energy
		
		Results();
		
		bool operator==(const Results &) const;
		bool operator!=(const Results &) const;
	};
	
	class Iterator {
		friend class MSD;
		
	 private:
		const MSD &msd;
		unsigned int i;  // index in msd::indicies
		
		Iterator(const MSD &, unsigned int i);
		
	 public:
		unsigned int getIndex() const;
		unsigned int getX() const;
		unsigned int getY() const;
		unsigned int getZ() const;
		
		Vector getSpin() const;
		Vector getFlux() const;
		Vector getLocalM() const;
		
		Vector operator*() const;
		Vector operator[](int) const;
		operator unsigned int() const;
		
		Iterator& operator++();
		Iterator operator++(int);
		Iterator& operator--();
		Iterator operator--(int);
		Iterator& operator+=(int);
		Iterator& operator-=(int);
		
		Iterator operator+(int) const;
		Iterator operator-(int) const;
		
		bool operator==(const Iterator &) const;
		bool operator!=(const Iterator &) const;
		bool operator>(const Iterator &) const;
		bool operator<(const Iterator &) const;
		bool operator>=(const Iterator &) const;
		bool operator<=(const Iterator &) const;
	};
	
	static const FlippingAlgorithm UP_DOWN_MODEL;
	static const FlippingAlgorithm CONTINUOUS_SPIN_MODEL;

 private:
	static Vector initSpin; //initial spin of all atoms
	static Vector initFlux; //initial spin fluctuation (direction only) for each atom
	
	SparseArray<Vector> spins;
	SparseArray<Vector> fluxes;
	Parameters parameters;
	Results results;
	unsigned int width, height, depth;
	unsigned int molPosL, molPosR;
	unsigned int topL, bottomL, frontR, backR;  // "inner sizes/boundaries"

	// number of atoms in both the entire device (n) and in each region (nL, nR, etc.)
	// n_mL, n_mR, mLR are defined as twice the number of bonds since these regions exist only as an interaction between two regions.
	unsigned int n, nL, nR, n_m, n_mL, n_mR, nLR;

	// flags for each region to remember if they exist based on given dimensions and inner bounds
	bool FM_L_exists, FM_R_exists, mol_exists;
	
	std::vector<unsigned int> indices; //valid indices
	
	mt19937_64 prng; //pseudo random number generator
	uniform_real_distribution<double> rand; //uniform probability density function on the interval [0, 1)
	unsigned long seed; //store seed so that every run can follow the same sequence
	unsigned char seed_count; //to help keep seeds from repeating because of temporal proximity
	
	unsigned int index(unsigned int x, unsigned int y, unsigned int z) const;
	unsigned int x(unsigned int a) const;
	unsigned int y(unsigned int a) const;
	unsigned int z(unsigned int a) const;
	
	unsigned long genSeed(); //generates a new seed
	
	MSD& operator=(const MSD&); //undefined, do not use!
	MSD(const MSD &m); //undefined, do not use!

	void init();
	
 public:
	std::vector<Results> record;
	FlippingAlgorithm flippingAlgorithm; //algorithm used to "flip" an atom in metropolis
	
	MSD(unsigned int width, unsigned int height, unsigned int depth,
			unsigned int molPosL, unsigned int molPosR,
			unsigned int topL, unsigned int bottomL, unsigned int frontR, unsigned int backR);
	MSD(unsigned int width, unsigned int height, unsigned int depth,
			unsigned int heightL, unsigned depthR);
	MSD(unsigned int width, unsigned int height, unsigned int depth);
	
	Parameters getParameters() const;
	void setParameters(const Parameters &);
	Results getResults() const;
	
	void setB(const Vector &B);
	
	Vector getSpin(unsigned int a) const;
	Vector getSpin(unsigned int x, unsigned int y, unsigned int z) const;
	Vector getFlux(unsigned int a) const;
	Vector getFlux(unsigned int x, unsigned int y, unsigned int z) const;
	Vector getLocalM(unsigned int a) const;
	Vector getLocalM(unsigned int x, unsigned int y, unsigned int z) const;
	void setSpin(unsigned int a, const Vector &);
	void setSpin(unsigned int x, unsigned int y, unsigned int z, const Vector &);
	void setFlux(unsigned int a, const Vector &);
	void setFlux(unsigned int x, unsigned int y, unsigned int z, const Vector &);
	void setLocalM(unsigned int a, const Vector &, const Vector &);
	void setLocalM(unsigned int x, unsigned int y, unsigned int z, const Vector &, const Vector &);
	
	unsigned int getN() const;
	unsigned int getNL() const;
	unsigned int getNR() const;
	unsigned int getNm() const;
	unsigned int getNmL() const;
	unsigned int getNmR() const;
	unsigned int getNLR() const;
	unsigned int getWidth() const;
	unsigned int getHeight() const;
	unsigned int getDepth() const;
	void getDimensions(unsigned int &width, unsigned int &height, unsigned int &depth) const;
	unsigned int getMolPosL() const;
	unsigned int getMolPosR() const;
	void getMolPos(unsigned int &molPosL, unsigned int &molPosR) const;
	unsigned int getTopL() const;
	unsigned int getBottomL() const;
	unsigned int getFrontR() const;
	unsigned int getBackR() const;
	void getInnerBounds(unsigned int &topL, unsigned int &bottomL, unsigned int &frontR, unsigned int &backR) const;
	bool getFM_L_exists() const;
	bool getFM_R_exists() const;
	bool getMol_exists() const;
	void getRegions(bool &FM_L_exists, bool &FM_R_exists, bool &mol_exists) const;
	
	void setSeed(unsigned long seed);  // change the seed of the prng, and restart the pseudo-random sequence
	unsigned long getSeed() const;  // get the seed currently being used

	void reinitialize(bool reseed = true); //reseed iff you want a new seed, true by default
	void randomize(bool reseed = true); //similar to reinitialize, but initial state is random
	void metropolis(unsigned long long N);
	void metropolis(unsigned long long N, unsigned long long freq);
	
	double specificHeat() const;
	double specificHeat_L() const;
	double specificHeat_R() const;
	double specificHeat_m() const;
	double specificHeat_mL() const;
	double specificHeat_mR() const;
	double specificHeat_LR() const;
	double magneticSusceptibility() const;
	double magneticSusceptibility_L() const;
	double magneticSusceptibility_R() const;
	double magneticSusceptibility_m() const;
	
	Vector meanM() const;
	Vector meanML() const;
	Vector meanMR() const;
	Vector meanMm() const;
	Vector meanMS() const;
	Vector meanMSL() const;
	Vector meanMSR() const;
	Vector meanMSm() const;
	Vector meanMF() const;
	Vector meanMFL() const;
	Vector meanMFR() const;
	Vector meanMFm() const;
	double meanU() const;
	double meanUL() const;
	double meanUR() const;
	double meanUm() const;
	double meanUmL() const;
	double meanUmR() const;
	double meanULR() const;
	
	Iterator begin() const;
	Iterator end() const;
};

ostream& operator <<(ostream &out, const MSD::Parameters &p);
ostream& operator <<(ostream &out, const MSD::Results &r);


//--------------------------------------------------------------------------------
ostream& operator <<(ostream &out, const MSD::Parameters &p) {
	return out
		<< "kT = " << p.kT << '\n' << "B = " << p.B << "\n\n"

		<< "sL = " << p.sL << '\n' << "sR = " << p.sR << '\n' << "sm = " << p.sm  << '\n'
		<< "FL = " << p.FL << '\n' << "FR = " << p.FR << '\n' << "Fm = " << p.Fm  << "\n\n"
		
		<< "JL  = " << p.JL  << '\n' << "JR  = " << p.JR  << '\n' << "Jm  = " << p.Jm  << '\n'
		<< "JmL = " << p.JmL << '\n' << "JmR = " << p.JmR << '\n' << "JLR = " << p.JLR << "\n\n"
		
		<< "Je0L  = " << p.Je0L  << '\n' << "Je0R  = " << p.Je0R  << '\n' << "Je0m  = " << p.Je0m  << '\n'
		<< "Je1L  = " << p.Je1L  << '\n' << "Je1R  = " << p.Je1R  << '\n' << "Je1m  = " << p.Je1m  << '\n'
		<< "Je1mL = " << p.Je1mL << '\n' << "Je1mR = " << p.Je1mR << '\n' << "Je1LR = " << p.Je1LR << "\n"
		<< "JeeL  = " << p.JeeL  << '\n' << "JeeR  = " << p.JeeR  << '\n' << "Jeem  = " << p.Jeem  << '\n'
		<< "JeemL = " << p.JeemL << '\n' << "JeemR = " << p.JeemR << '\n' << "JeeLR = " << p.JeeLR << "\n\n"
		
		<< "bL  = " << p.bL  << '\n' << "bR  = " << p.bR  << '\n' << "bm  = " << p.bm  << '\n'
		<< "bmL = " << p.bmL << '\n' << "bmR = " << p.bmR << '\n' << "bLR = " << p.bLR << "\n\n"
		
		<< "AL = " << p.AL << '\n' << "AR = " << p.AR << '\n' << "Am = " << p.Am  << "\n\n"
		
		<< "DL  = " << p.DL  << '\n' << "DR  = " << p.DR  << '\n' << "Dm  = " << p.Dm  << '\n'
		<< "DmL = " << p.DmL << '\n' << "DmR = " << p.DmR << '\n' << "DLR = " << p.DLR << '\n';
}

ostream& operator <<(ostream &out, const MSD::Results &r) {
	return out
		<< "t   = " << r.t   << "\n\n"
		<< "M   = " << r.M   << '\n' << "ML  = " << r.ML  << '\n' << "MR  = " << r.MR  << '\n' << "Mm  = " << r.Mm  << "\n\n"
		<< "MS  = " << r.MS  << '\n' << "MSL = " << r.MSL << '\n' << "MSR = " << r.MSR << '\n' << "MSm = " << r.MSm << "\n\n"
		<< "MF  = " << r.MF  << '\n' << "MFL = " << r.MFL << '\n' << "MFR = " << r.MFR << '\n' << "MFm = " << r.MFm << "\n\n"
		<< "U   = " << r.U   << '\n' << "UL  = " << r.UL  << '\n' << "UR  = " << r.UR  << '\n' << "Um  = " << r.Um  << '\n'
		<< "UmL = " << r.UmL << '\n' << "UmR = " << r.UmR << '\n' << "ULR = " << r.ULR << '\n';
}


Molecule::EdgeParameters::EdgeParameters()
: Jm(0), Je1m(0), Jeem(0), bm(0), Dm(Vector::ZERO)
{}

Molecule::NodeParameters::NodeParameters()
: Sm(1), Fm(0), Je0m(0), Am(Vector::ZERO)
{}

Molecule::Edge::Edge(size_t eIdx, size_t nIdx, size_t sIdx, EdgeParameters *eParam, Node *node, Node *self)
: edgeIndex(eIdx), nodeIndex(nIdx), selfIndex(sIdx), parameters(eParam), node(node), self(self)
{}

Molecule::Edge::Edge(Molecule &mol, size_t eIdx, size_t nIdx, size_t sIdx)
: edgeIndex(eIdx), nodeIndex(nIdx), selfIndex(sIdx)
{
	parameters = &(mol.edgeParameters.at(eIdx));
	node = & mol.nodes.at(nIdx);
	self = & mol.nodes.at(sIdx);
}

Molecule::Molecule() : leftLead(0), rightLead(0) {
}

Molecule::Molecule(size_t n_m, const NodeParameters &nodeParams)
: nodes(n_m), leftLead(0), rightLead(0)
{
	for (Node &node : nodes)
		node.parameters = nodeParams;
}

void Molecule::serialize(unsigned char *buffer) const {
	// First write the edge-parameters section
	bwrite(edgeParameters.size(), buffer);
	for (const EdgeParameters &eParam : edgeParameters) {
		bwrite(eParam.Jm, buffer);  // order must be correct (must match Molecule::deserialize)
		bwrite(eParam.Je1m, buffer);
		bwrite(eParam.Jeem, buffer);
		bwrite(eParam.bm, buffer);
		bwrite(eParam.Dm.x, buffer);
		bwrite(eParam.Dm.y, buffer);
		bwrite(eParam.Dm.z, buffer);
	}

	// Second, write the node-parameters section
	bwrite(nodes.size(), buffer);
	for (const Node &node : nodes) {
		bwrite(node.parameters.Sm, buffer);  // Order must be correct (must match Molecule::deserialize)
		bwrite(node.parameters.Fm, buffer);
		bwrite(node.parameters.Je0m, buffer);
		bwrite(node.parameters.Am.x, buffer);
		bwrite(node.parameters.Am.y, buffer);
		bwrite(node.parameters.Am.z, buffer);
	}

	// Thridly, write the adjacency section
	for (const Node &node : nodes) {
		bwrite(node.neighbors.size(), buffer);
		for (const Edge &edge : node.neighbors) {
			bwrite(edge.edgeIndex, buffer);
			bwrite(edge.nodeIndex, buffer);
		}
	}

	// Lastly write "extra stuff"
	bwrite(leftLead, buffer);
	bwrite(rightLead, buffer);
}

void Molecule::deserialize(const unsigned char *buffer) {
	// First read edges
	size_t edgeCount;
	bread(edgeCount, buffer);  // First the number of edges is read.
	edgeParameters.reserve(edgeCount);
	for (size_t i = 0; i < edgeCount; i++) {
		EdgeParameters eParam;
		bread(eParam.Jm, buffer);    // Then each edge's parameters are read.
		bread(eParam.Je1m, buffer);  // (Take careful note of the order in which the fields are stored.)
		bread(eParam.Jeem, buffer);
		bread(eParam.bm, buffer);
		bread(eParam.Dm.x, buffer);
		bread(eParam.Dm.y, buffer);
		bread(eParam.Dm.z, buffer);
		edgeParameters.push_back(eParam);
	}

	// Then read nodes (in two "sweeps")
	size_t nodeCount;
	udc::bread(nodeCount, buffer);  // Next the number of nodes is read.
	for (size_t i = 0; i < nodeCount; i++) {
		Node node;
		bread(node.parameters.Sm, buffer);  // For each node, we read its (local) parameter fields.
		bread(node.parameters.Fm, buffer);  // (Take careful note of the order in which the fields are stored.)
		bread(node.parameters.Je0m, buffer);
		bread(node.parameters.Am.x, buffer);
		bread(node.parameters.Am.y, buffer);
		bread(node.parameters.Am.z, buffer);
		nodes.push_back(node);
	}
	for (size_t i = 0; i < nodeCount; i++) {
		Node &node = nodes[i];
		size_t neighborCount;
		bread(neighborCount, buffer);  // ("Spweep" 2) Again for each node, get the number of neighbors this time.
		node.neighbors.reserve(neighborCount);
		for (size_t j = 0; j < neighborCount; j++) {
			int eIndex, nIndex;
			bread(eIndex, buffer);  // Finally we read each (edge index, node index) pair for this node.
			bread(nIndex, buffer);
			Edge edge(eIndex, nIndex, i, & edgeParameters[eIndex], & nodes[nIndex], & node);
			node.neighbors.push_back(edge);
		}
	}

	// Lastly, we read the leads' positions
	bread(leftLead, buffer);  // (Left lead first, then right lead.)
	bread(rightLead, buffer);
}

size_t Molecule::serializationSize() const {
	// compile-time constants
	const size_t EDGE_PARAMETERS_SERIALIZATION_SIZE = 7 * sizeof(double);
	const size_t NODE_PARAMETERS_SERIALIZATION_SIZE = 6 * sizeof(double);
	const size_t EDGE_SERIALIZATION_SIZE = 2 * sizeof(size_t);
	const size_t LEADS_SERIALIZATION_SIZE = 2 * sizeof(size_t);
	
	// calculate at runtime
	size_t eBlock = sizeof(size_t) + edgeParameters.size() * EDGE_PARAMETERS_SERIALIZATION_SIZE;
	size_t nBlock = sizeof(size_t) + nodes.size() * NODE_PARAMETERS_SERIALIZATION_SIZE;

	size_t adjBlock = sizeof(size_t) * nodes.size();  // headers for each sub-block adjacency list
	for (const Node & node : nodes)
		adjBlock += node.neighbors.size() * EDGE_SERIALIZATION_SIZE;
	
	// total
	return eBlock + nBlock + adjBlock + LEADS_SERIALIZATION_SIZE;
}

unsigned int Molecule::createNode(const NodeParameters &parameters) {
	if (nodes.size() >= NOT_FOUND)
		throw out_of_range("Molecule::createNode: Can not create node because the maximum number of nodes has been reached");

	Node node;
	node.parameters = parameters;
	nodes.push_back(node);
	return nodes.size() - 1;
}

unsigned int Molecule::nodeCount() const {
	return nodes.size();
}

unsigned int Molecule::connectNodes(unsigned int a, unsigned int b, const EdgeParameters &parameters) {
	if (edgeParameters.size() == NOT_FOUND)
		throw out_of_range("Molecule::connectNodes: Can not create edge because the maximum number of edges has been reached");
	if (a >= nodes.size() || b >= nodes.size())
		throw out_of_range("Molecule::connectNodes: Invalid node index");
	
	edgeParameters.push_back(parameters);
	size_t index = edgeParameters.size() - 1;

	Edge edgeA(*this, index, b, a);
	nodes[a].neighbors.push_back(edgeA);

	Edge edgeB(*this, index, a, b);
	nodes[b].neighbors.push_back(edgeB);

	return index;
}

unsigned int Molecule::edgeIndex(unsigned int a, unsigned int b) const {
	if (a >= nodes.size() || b >= nodes.size())
		throw out_of_range("Molecule::edgeIndex: Invalid node index");

	for (const Edge &edge : nodes[a].neighbors)
		if (edge.nodeIndex == b)
			return edge.edgeIndex;
	return NOT_FOUND;
}

Molecule::EdgeParameters Molecule::getEdgeParameters(unsigned int index) const {
	return edgeParameters.at(index);
}

void Molecule::setEdgeParameters(unsigned int index, const Molecule::EdgeParameters &p) {
	edgeParameters.at(index) = p;
}

Molecule::NodeParameters Molecule::getNodeParameters(unsigned int index) const {
	return nodes.at(index).parameters;
}

void Molecule::setNodeParameters(unsigned int index, const Molecule::NodeParameters &p) {
	nodes.at(index).parameters = p;
}

void Molecule::setLeftLead(unsigned int node) {
	leftLead = node;
}

void Molecule::setRightLead(unsigned int node) {
	rightLead = node;
}

void Molecule::setLeads(unsigned int left, unsigned int right) {
	leftLead = left;
	rightLead = right;
}

unsigned int Molecule::getLeftLead() const {
	return leftLead;
}

unsigned int Molecule::getRightLead() const {
	return rightLead;
}

void Molecule::getLeads(unsigned int &left, unsigned int &right) const {
	left = leftLead;
	right = rightLead;
}

Molecule::Instance::Instance(const Molecule &prototype, MSD &msd, unsigned int y, unsigned int z)
: prototype(prototype), msd(msd), y(y), z(z)
{
	const size_t n_m = prototype.nodes.size();
	spins.reserve(n_m);
	for (size_t i = 0; i < n_m; i++)
		spins.push_back(Vector::I * prototype.nodes[i].parameters.Sm);
	fluxes.assign(prototype.nodes.size(), Vector::ZERO);
}

Molecule::Instance::Instance(const Instance &other)
: prototype(other.prototype), msd(other.msd), y(y), z(z), spins(other.spins), fluxes(other.fluxes)
{}

Molecule::Instance& Molecule::Instance::operator=(const Molecule::Instance &other) {
	this->spins = other.spins;
	this->fluxes = other.fluxes;
}

void Molecule::Instance::setLocalM(unsigned int a, const Vector &spin, const Vector &flux) {
	Vector &s = spins.at(a);   // previous spin
	Vector &f = fluxes.at(a);  // previous flux

	Vector m = s + f;          // previous local mag.
	Vector mag = spin + flux;  // new local mag.

	Vector deltaS = spin - s;
	Vector deltaF = flux - f;
	Vector deltaM = mag - m;

	const Node &node = prototype.nodes[a];
	const MSD::Parameters &msdParams = msd.parameters;
	const NodeParameters &nodeParams = node.parameters;

	MSD::Results &results = msd.results;

	// ---- update magnetization, M ----
	results.M += deltaM;
	results.MS += deltaS;
	results.MF += deltaF;

	results.Mm += deltaM;
	results.MSm += deltaS;
	results.MFm += deltaF;

	// ---- update energy, U ----
	// local energy
	{	double deltaU = msdParams.B * deltaM
		              + nodeParams.Am * ( Vector(sq(mag.x), sq(mag.y), sq(mag.z)) - Vector(sq(m.x), sq(m.y), sq(m.z)) )
		              + nodeParams.Je0m * ( spin * flux - s * f );
		results.U -= deltaU;
		results.Um -= deltaU;
	}

	// energy from edges (i.e. bonds)
	for (const Edge &edge : node.neighbors) {
		unsigned int a1 = edge.nodeIndex;  // index of neighbor
		Vector neighbor_s = spins[a1];
		Vector neighbor_f = fluxes[a1];
		Vector neighbor_m = neighbor_s + neighbor_f;
		EdgeParameters edgeParams = *edge.parameters;
		double deltaU = edgeParams.Jm * ( neighbor_s * deltaS )
		              + edgeParams.Je1m * ( neighbor_f * deltaS + neighbor_s * deltaF )
		              + edgeParams.Jeem * ( neighbor_f * deltaF )
		              + edgeParams.bm * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
		              + edgeParams.Dm * neighbor_m.crossProduct(deltaM);
		results.U -= deltaU;
		results.Um -= deltaU;
	}

	// energy from leads
	unsigned int molPosL = msd.molPosL;
	unsigned int molPosR = msd.molPosR;
	if (molPosL <= molPosR) {
		const auto &msdSpins = msd.spins;
		const auto &msdFluxes = msd.fluxes;

		// left lead
		if (a == prototype.leftLead && molPosL != 0) {
			unsigned int a1 = msd.index(molPosL - 1, y, z);
			Vector neighbor_s = msdSpins[a1];
			Vector neighbor_f = msdFluxes[a1];
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = msdParams.JmL * ( neighbor_s * deltaS )
			              + msdParams.Je1mL * ( neighbor_f * deltaS + neighbor_s * deltaF )
			              + msdParams.JeemL * ( neighbor_f * deltaF )
			              + msdParams.bmL * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
			              + msdParams.DmL * neighbor_m.crossProduct(deltaM);
			results.U -= deltaU;
			results.UmL -= deltaU;
		}

		// right lead
		unsigned int x2 = molPosR + 1;
		if (a == prototype.rightLead && x2 != msd.width) {
			unsigned int a1 = msd.index(x2, y, z);
			Vector neighbor_s = msdSpins[a1];
			Vector neighbor_f = msdFluxes[a1];
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = msdParams.JmR * ( neighbor_s * deltaS )
			              + msdParams.Je1mR * ( neighbor_f * deltaS + neighbor_s * deltaF )
			              + msdParams.JeemR * ( neighbor_f * deltaF )
			              + msdParams.bmR * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
			              + msdParams.DmR * neighbor_m.crossProduct(deltaM);
			results.U -= deltaU;
			results.UmR -= deltaU;
		}
	}

	// ---- Done: update spin and flux values ----
	s = spin;
	f = flux;
}

void Molecule::Instance::setSpin(unsigned int a, const Vector &spin) {
	setLocalM(a, spin, getFlux(a));
}

void Molecule::Instance::setFlux(unsigned int a, const Vector &flux) {
	setLocalM(a, getSpin(a), flux);
}

Vector Molecule::Instance::getSpin(unsigned int a) const {
	return spins.at(a);
}

Vector Molecule::Instance::getFlux(unsigned int a) const {
	return fluxes.at(a);
}

void Molecule::Instance::getLocalM(unsigned int a, Vector &spin, Vector &flux) const {
	spin = spins.at(a);
	flux = fluxes.at(a);
}

Molecule::Instance Molecule::instantiate(MSD &msd, unsigned int y, unsigned int z) const {
	return Instance(*this, msd, y, z);
}


MSD::Parameters::Parameters()
: kT(0.25), B(Vector::ZERO),
  sL(1), sR(1), sm(1), FL(0), FR(0), Fm(0),
  JL(1), JR(1), Jm(0), JmL(1), JmR(-1), JLR(0),
  Je0L(0), Je0R(0), Je0m(0),
  Je1L(0), Je1R(0), Je1m(0), Je1mL(0), Je1mR(0), Je1LR(0),
  JeeL(0), JeeR(0), Jeem(0), JeemL(0), JeemR(0), JeeLR(0),
  bL(0), bR(0), bm(0), bmL(0), bmR(0), bLR(0),
  AL(Vector::ZERO), AR(Vector::ZERO), Am(Vector::ZERO),
  DL(Vector::ZERO), DR(Vector::ZERO), Dm(Vector::ZERO), DmL(Vector::ZERO), DmR(Vector::ZERO), DLR(Vector::ZERO) {
}

bool MSD::Parameters::operator==(const Parameters &p) const {
	return kT  == p.kT  && B   == p.B
	    && sL  == p.sL  && sR  == p.sR  && sm  == p.sm
	    && FL  == p.FL  && FR  == p.FR  && Fm  == p.Fm
	    && JL  == p.JL  && JR  == p.JR  && Jm  == p.Jm
	    && JmL == p.JmL && JmR == p.JmR && JLR == p.JLR
		&& Je0L  == p.Je0L && Je0R  == p.Je0R && Je0m == p.Je0m
		&& Je1L  == p.Je1L && Je1R  == p.Je1R && Je1m == p.Je1m
		&& Je1mL == p.Je1L && Je1mR == p.Je1R && Je1m == p.Je1LR
		&& JeeL  == p.JeeL && JeeR  == p.JeeR && Jeem == p.Jeem
		&& JeemL == p.JeeL && JeemR == p.JeeR && Jeem == p.JeeLR
	    && bL  == p.bL  && bR  == p.bR  && bm  == p.bm
	    && bmL == p.bmL && bmR == p.bmR && bLR == p.bLR
	    && AL  == p.AL  && AR  == p.AR  && Am  == p.Am
	    && DL  == p.DL  && DR  == p.DR  && Dm  == p.Dm
	    && DmL == p.DmL && DmR == p.DmR && DLR == p.DLR;
}

bool MSD::Parameters::operator!=(const Parameters &p) const {
	return !(*this == p);
}


MSD::Results::Results()
: t(0), M(Vector::ZERO), ML(Vector::ZERO), MR(Vector::ZERO), Mm(Vector::ZERO),
  MS(Vector::ZERO), MSL(Vector::ZERO), MSR(Vector::ZERO), MSm(Vector::ZERO),
  MF(Vector::ZERO), MFL(Vector::ZERO), MFR(Vector::ZERO), MFm(Vector::ZERO),
  U(0), UL(0), UR(0), Um(0), UmL(0), UmR(0), ULR(0) {
}

bool MSD::Results::operator==(const Results &r) const {
	return t   == r.t
	    && M   == r.M   && ML  == r.ML  && MR  == r.MR  && Mm  == r.Mm
		&& MS  == r.MS  && MSL == r.MSL && MSR == r.MSR && MSm == r.MSm
		&& MF  == r.MF  && MFL == r.MFL && MFR == r.MFR && MFm == r.MFm
	    && U   == r.U   && UL  == r.UL  && UR  == r.UR  && Um == r.Um
		&& UmL == r.UmL && UmR == r.UmR && ULR == r.ULR;
}

bool MSD::Results::operator!=(const Results &r) const {
	return !(*this == r);
}


MSD::Iterator::Iterator(const MSD &msd, unsigned int i) : msd(msd), i(i) {
}

unsigned int MSD::Iterator::getIndex() const {
	return msd.indices.at(i);
}

unsigned int MSD::Iterator::getX() const {
	return msd.x( getIndex() );
}

unsigned int MSD::Iterator::getY() const {
	return msd.y( getIndex() );
}

unsigned int MSD::Iterator::getZ() const {
	return msd.z( getIndex() );
}

Vector MSD::Iterator::getSpin() const {
	return msd.getSpin( this->getIndex() );
}

Vector MSD::Iterator::getFlux() const {
	return msd.getFlux( this->getIndex() );
}

Vector MSD::Iterator::getLocalM() const {
	return msd.getLocalM( this->getIndex() );
}

Vector MSD::Iterator::operator*() const {
	return getLocalM();
}

Vector MSD::Iterator::operator[](int n) const {
	return *(*this + n);
}

MSD::Iterator::operator unsigned int() const {
	return getIndex();
}

MSD::Iterator& MSD::Iterator::operator++() {
	if (i >= msd.indices.size())
		throw out_of_range("can't increment MSD::Iterator == msd.end()");
	++i;
	return *this;
}

MSD::Iterator MSD::Iterator::operator++(int) {
	Iterator prev = *this;
	++(*this);
	return prev;
}

MSD::Iterator& MSD::Iterator::operator--() {
	if (i <= 0)
		throw out_of_range("can't decrement MSD::Iterator == msd.begin()");
	--i;
	return *this;
}

MSD::Iterator MSD::Iterator::operator--(int) {
	Iterator prev = *this;
	--(*this);
	return prev;
}

MSD::Iterator& MSD::Iterator::operator+=(int n) {
	if( n < 0 )
		return *this -= -n;
	
	unsigned int sum = i + n; 
	if (sum > msd.indices.size())
		throw out_of_range("can't increment past msd.end()");
	i = sum;
	return *this;
}

MSD::Iterator& MSD::Iterator::operator-=(int n) {
	if( n < 0 )
		return *this += -n;
	
	if (n > i)
		throw out_of_range("can't decrement past msd.begin()");
	i -= n;
	return *this;
}

MSD::Iterator MSD::Iterator::operator+(int n) const {
	Iterator iter = *this;
	iter += n;
	return iter;
}

MSD::Iterator MSD::Iterator::operator-(int n) const {
	Iterator iter = *this;
	iter -= n;
	return iter;
}

bool MSD::Iterator::operator==(const Iterator &iter) const {
	if( &msd != &iter.msd )
		throw invalid_argument("must compare two iterators from the same MSD");
	return i == iter.i;
}

bool MSD::Iterator::operator!=(const Iterator &iter) const {
	return !(*this == iter);
}

bool MSD::Iterator::operator>(const Iterator &iter) const {
	if( &msd != &iter.msd )
		throw invalid_argument("must compare two iterators from the same MSD");
	return i > iter.i;
}

bool MSD::Iterator::operator<(const Iterator &iter) const {
	if( &msd != &iter.msd )
		throw invalid_argument("must compare two iterators from the same MSD");
	return i < iter.i;
}

bool MSD::Iterator::operator>=(const Iterator &iter) const {
	return !(*this < iter);
}

bool MSD::Iterator::operator<=(const Iterator &iter) const {
	return !(*this > iter);
}


const MSD::FlippingAlgorithm MSD::UP_DOWN_MODEL = [](const Vector &spin, function<double()> rand) {
	return -spin;
};

const MSD::FlippingAlgorithm MSD::CONTINUOUS_SPIN_MODEL = [](const Vector &spin, function<double()> rand) {
	return Vector::sphericalForm( spin.norm(), 2 * PI * rand(), asin(2 * rand() - 1) );
};


Vector MSD::initSpin = Vector::J;
Vector MSD::initFlux = Vector::ZERO;


unsigned int MSD::index(unsigned int x, unsigned int y, unsigned int z) const {
	return (z * height + y) * width + x;
}

unsigned int MSD::x(unsigned int a) const {
	return a % width;
}

unsigned int MSD::y(unsigned int a) const {
	return a % (width * height) / width;
}

unsigned int MSD::z(unsigned int a) const {
	return a / (width * height);
}


unsigned long MSD::genSeed() {
	return (  static_cast<unsigned long>(time(NULL))      << 16 )
	     | ( (static_cast<unsigned long>(clock()) & 0xFF) << 8  )
	     | ( (static_cast<unsigned long>(seed_count++) & 0xFF) );
}


void MSD::init() {
	// preconditions:
	if (width == 0)         width = 1;
	if (height == 0)        height = 1;
	if (depth == 0)         depth = 1;
	if (molPosR >= width)   molPosR = width - 1;
	if (molPosL > width)    molPosL = width;
	if (molPosR < molPosL)  molPosR = molPosL - 1;
	if (bottomL >= height)  bottomL = height - 1;
	if (topL > height)      topL = height;
	if (bottomL < topL)     topL = bottomL - 1;
	if (backR >= depth)     backR = depth - 1;
	if (frontR > depth)     frontR = depth;
	if (backR < frontR)     backR = frontR - 1;

	spins.resize(width * height * depth);
	fluxes.resize(width * height * depth);

	FM_L_exists = (molPosL != 0);
	FM_R_exists = (molPosR + 1 < width);
	mol_exists = (molPosL <= molPosR);

	seed = genSeed();
	prng.seed(seed);
	
	n = nL = nR = n_m = n_mL = n_mR = nLR = 0;
	unsigned int a;
	for( unsigned int z = 0; z < depth; z++ )
		for( unsigned int y = 0; y < height; y++ ) {
			// left
			for( unsigned int x = 0; x < molPosL; x++ )
				if (topL <= y && y <= bottomL) {
					a = index(x, y, z);
					indices.push_back(a);
					spins[a] = initSpin;
					fluxes[a] = initFlux;
					n++;
					nL++;
					if (x + 1 == molPosL) {
						if (mol_exists)
							n_mL++;
						if (FM_R_exists)
							nLR++;
					}
				}
			// mol
			if( ((y == topL || y == bottomL) && frontR <= z && z <= backR) || ((z == frontR || z == backR) && topL <= y && y <= bottomL) )
				for( unsigned int x = molPosL; x <= molPosR; x++ ) {
					a = index(x, y, z);
					indices.push_back(a);
					spins[a] = initSpin;
					fluxes[a] = initFlux;
					n++;
					n_m++;
					if (x == molPosL && FM_L_exists)
						n_mL++;
					if (x == molPosR && FM_R_exists)
						n_mR++;
				}
			// right
			for( unsigned int x = molPosR + 1; x < width; x++ )
				if (frontR <= z && z <= backR) {
					a = index(x, y, z);
					indices.push_back(a);
					spins[a] = initSpin;
					fluxes[a] = initFlux;
					n++;
					nR++;
					if (x == molPosR + 1) {
						if (mol_exists)
							n_mR++;
						if (FM_L_exists)
							nLR++;
					}
				}
		}
	
	setParameters(parameters); //calculate initial state (results)
	flippingAlgorithm = CONTINUOUS_SPIN_MODEL; //set default "flipping" algorithm
}


MSD::MSD(unsigned int width, unsigned int height, unsigned int depth,
		unsigned int molPosL, unsigned int molPosR,
		unsigned int topL, unsigned int bottomL, unsigned int frontR, unsigned int backR)
: width(width), height(height), depth(depth), molPosL(molPosL), molPosR(molPosR),
		topL(topL), bottomL(bottomL), frontR(frontR), backR(backR)
{
	init();
}


MSD::MSD(unsigned int width, unsigned int height, unsigned int depth,
		unsigned int heightL, unsigned depthR)
: width(width), height(height), depth(depth), molPosL((width - 1) / 2), molPosR((width - 1) / 2),
		topL( (unsigned int) ceil((height - 1 - heightL) / 2.0) ),
		bottomL( (unsigned int) floor((height - 1 + heightL) / 2.0) ),
		frontR( (unsigned int) ceil((depth - 1 - depthR) / 2.0) ),
		backR( (unsigned int) floor((depth - 1 + depthR) / 2.0) )
{
	init();
}


MSD::MSD(unsigned int width, unsigned int height, unsigned int depth)
: width(width), height(height), depth(depth), molPosL((width - 1) / 2), molPosR(width / 2),
		topL(0), bottomL(height - 1), frontR(0), backR(depth - 1)
{
	init();
}


MSD::Parameters MSD::getParameters() const {
	return parameters;
}

void MSD::setParameters(const MSD::Parameters &p) {
	MSD::Parameters p0 = parameters;  // old parameters
	parameters = p;  // update to new parameters
	
	//Spin and Spin Flux Magnitudes
	for( auto iter = begin(); iter != end(); ++iter ) {
		unsigned int i = iter.getIndex();
		unsigned int x = iter.getX();
		if( x < molPosL ) {
			spins[i].normalize() *= parameters.sL;
			fluxes[i] *= p0.FL != 0 ? parameters.FL / p0.FL : 0;
		} else if( x > molPosR ) {
			spins[i].normalize() *= parameters.sR;
			fluxes[i] *= p0.FR != 0 ? parameters.FR / p0.FR : 0;
		} else {
			spins[i].normalize() *= parameters.sm;
			fluxes[i] *= p0.Fm != 0 ? parameters.Fm / p0.Fm : 0;
		}
	}
	
	// Magnetization, Anisotropy, and other local phenomenon
	results.MSL = results.MSR = results.MSm =
	results.MFL = results.MFR = results.MFm = Vector::ZERO;
	Vector anisotropy_L = Vector::ZERO, anisotropy_R = Vector::ZERO, anisotropy_m = Vector::ZERO;
	double couple_e0_L = 0, couple_e0_R = 0, couple_e0_m = 0;  // sum(s_i * f_i)
	for( auto iter = begin(); iter != end(); ++iter ) {
		unsigned int x = iter.getX();
		Vector s = iter.getSpin();
		Vector f = iter.getFlux();
		Vector localM = s + f;
		if( x < molPosL ) {
			results.MSL += s;
			results.MFL += f;
			anisotropy_L.x += sq( localM.x );
			anisotropy_L.y += sq( localM.y );
			anisotropy_L.z += sq( localM.z );
			couple_e0_L += s * f;
		} else if( x > molPosR ) {
			results.MSR += s;
			results.MFR += f;
			anisotropy_R.x += sq( localM.x );
			anisotropy_R.y += sq( localM.y );
			anisotropy_R.z += sq( localM.z );
			couple_e0_R += s * f;
		} else {
			results.MSm += s;
			results.MFm += f;
			anisotropy_m.x += sq( localM.x );
			anisotropy_m.y += sq( localM.y );
			anisotropy_m.z += sq( localM.z );
			couple_e0_m += s * f;
		}
	}
	results.MS = results.MSL + results.MSR + results.MSm;  // aggregate spins
	results.MF = results.MFL + results.MFR + results.MFm;  // aggregate fluxes
	results.ML = results.MSL + results.MFL;  // aggregate Left FM
	results.MR = results.MSR + results.MFR;  // aggregate Right FM
	results.Mm = results.MSm + results.MFm;  // aggregate mol.
	results.M = results.ML + results.MR + results.Mm;  // aggregate total
	

	//Internal Energy
	double couple_ss_L = 0;  // sum(s_i * s_j)
	double couple_e1_L = 0;  // sum(s_i * f_j)
	double couple_ee_L = 0;  // sum(f_i * f_j)
	double biquad_L = 0;
	Vector dmi_L = Vector::ZERO;  // sum(m_i x m_j);  where x is cross product, and i < j
	for( unsigned int z = 0; z < depth; z++ )
		for( unsigned int y = topL; y <= bottomL; y++ )
			for( unsigned int x = 0; x < molPosL; x++ ) {
				unsigned int a = index(x, y, z);
				Vector s = getSpin(a);
				Vector f = getFlux(a);
				Vector m = s + f;
				if( x + 1 < molPosL ) {
					unsigned int neighbor = index(x + 1, y, z);
					Vector spin = getSpin(neighbor);
					Vector flux = getFlux(neighbor);
					Vector mag = spin + flux;
					couple_ss_L += s * spin;
					couple_e1_L += s * flux;
					couple_e1_L += f * spin;
					couple_ee_L += f * flux;
					biquad_L += sq(m * mag);
					dmi_L += m.crossProduct(mag);
				}
				if( y + 1 <= bottomL ) {
					unsigned int neighbor = index(x, y + 1, z);
					Vector spin = getSpin(neighbor);
					Vector flux = getFlux(neighbor);
					Vector mag = spin + flux;
					couple_ss_L += s * spin;
					couple_e1_L += s * flux;
					couple_e1_L += f * spin;
					couple_ee_L += f * flux;
					biquad_L += sq(m * mag);
					dmi_L += m.crossProduct(mag);
				}
				if( z + 1 < depth ) {
					unsigned int neighbor = index(x, y, z + 1);
					Vector spin = getSpin(neighbor);
					Vector flux = getFlux(neighbor);
					Vector mag = spin + flux;
					couple_ss_L += s * spin;
					couple_e1_L += s * flux;
					couple_e1_L += f * spin;
					couple_ee_L += f * flux;
					biquad_L += sq(m * mag);
					dmi_L += m.crossProduct(mag);
				}
			}
	results.UL = 0;
	results.UL -= parameters.JL * couple_ss_L;
	results.UL -= parameters.Je0L * couple_e0_L;
	results.UL -= parameters.Je1L * couple_e1_L;
	results.UL -= parameters.JeeL * couple_ee_L;
	results.UL -= parameters.B * results.ML;
	results.UL -= parameters.AL * anisotropy_L;
	results.UL -= parameters.bL * biquad_L;
	results.UL -= parameters.DL * dmi_L;
	
	double couple_ss_R = 0;  // sum(s_i * s_j)
	double couple_e1_R = 0;  // sum(s_i * f_j)
	double couple_ee_R = 0;  // sum(f_i * f_j)
	double biquad_R = 0;
	Vector dmi_R = Vector::ZERO;  // sum(m_i x m_j);  where x is cross product, and i < j
	for( unsigned int z = frontR; z <= backR; z++ )
		for( unsigned int y = 0; y < height; y++ )
			for( unsigned int x = molPosR + 1; x < width; x++ ) {
				unsigned int a = index(x, y, z);
				Vector s = getSpin(a);
				Vector f = getFlux(a);
				Vector m = s + f;
				if( x + 1 < width ) {
					unsigned int neighbor = index(x + 1, y, z);
					Vector spin = getSpin(neighbor);
					Vector flux = getFlux(neighbor);
					Vector mag = spin + flux;
					couple_ss_R += s * spin;
					couple_e1_R += s * flux;
					couple_e1_R += f * spin;
					couple_ee_R += f * flux;
					biquad_R += sq(m * mag);
					dmi_R += m.crossProduct(mag);
				}
				if( y + 1 < height ) {
					unsigned int neighbor = index(x, y + 1, z);
					Vector spin = getSpin(neighbor);
					Vector flux = getFlux(neighbor);
					Vector mag = spin + flux;
					couple_ss_R += s * spin;
					couple_e1_R += s * flux;
					couple_e1_R += f * spin;
					couple_ee_R += f * flux;
					biquad_R += sq(m * mag);
					dmi_R += m.crossProduct(mag);
				}
				if( z + 1 <= backR ) {
					unsigned int neighbor = index(x, y, z + 1);
					Vector spin = getSpin(neighbor);
					Vector flux = getFlux(neighbor);
					Vector mag = spin + flux;
					couple_ss_R += s * spin;
					couple_e1_R += s * flux;
					couple_e1_R += f * spin;
					couple_ee_R += f * flux;
					biquad_R += sq(m * mag);
					dmi_R += m.crossProduct(mag);
				}
			}
	results.UR = 0;
	results.UR -= parameters.JR * couple_ss_R;
	results.UR -= parameters.Je0R * couple_e0_R;
	results.UR -= parameters.Je1R * couple_e1_R;
	results.UR -= parameters.JeeR * couple_ee_R;
	results.UR -= parameters.B * results.MR;
	results.UR -= parameters.AR * anisotropy_R;
	results.UR -= parameters.bR * biquad_R;
	results.UR -= parameters.DR * dmi_R;
	
	double couple_ss_m = 0;  // sum(s_i * s_j)
	double couple_e1_m = 0;  // sum(s_i * f_j)
	double couple_ee_m = 0;  // sum(f_i * f_j)
	double biquad_m = 0;
	Vector dmi_m = Vector::ZERO;  // sum(m_i x m_j);  where x is cross product, and i < j
	for( unsigned int x = molPosL; x < molPosR; x++ )
		for( unsigned int y = topL; y <= bottomL; y++ )
			for( unsigned int z = frontR; z <= backR; z++ )
				if( y == topL || z == frontR || y == bottomL || z == backR ) {
					unsigned int a = index(x, y, z);
					Vector s = getSpin(a);
					Vector f = getFlux(a);
					Vector m = s + f;
					unsigned int neighbor = index(x + 1, y, z);
					Vector spin = getSpin(neighbor);
					Vector flux = getFlux(neighbor);
					Vector mag = spin + flux;
					couple_ss_m += s * spin;
					couple_e1_m += s * flux;
					couple_e1_m += f * spin;
					couple_ee_m += f * flux;
					biquad_m += sq(m * mag);
					dmi_m += m.crossProduct(mag);
				}
	results.Um = 0;
	results.Um -= parameters.Jm * couple_ss_m;
	results.Um -= parameters.Je0m * couple_e0_m;
	results.Um -= parameters.Je1m * couple_e1_m;
	results.Um -= parameters.Jeem * couple_ee_m;
	results.Um -= parameters.B * results.Mm;
	results.Um -= parameters.Am * anisotropy_m;
	results.Um -= parameters.bm * biquad_m;
	results.Um -= parameters.Dm * dmi_m;
	
	double couple_ss_mL = 0;  // sum(s_i * s_j)
	double couple_e1_mL = 0;  // sum(s_i * f_j)
	double couple_ee_mL = 0;  // sum(f_i * f_j)
	double biquad_mL = 0;
	Vector dmi_mL = Vector::ZERO;  // sum(m_i x m_j);  where x is cross product, and i < j
	const unsigned int x1 = molPosL - 1; //only useful if( molPosL != 0 )
	if( molPosL != 0 && molPosL <= molPosR )
		for( unsigned int y = topL; y <= bottomL; y++ )
			for( unsigned int z = frontR; z <= backR; z++ )
				if( y == topL || z == frontR || y == bottomL || z == backR ) {
					unsigned int a = index(x1, y, z);
					Vector s = getSpin(a);
					Vector f = getFlux(a);
					Vector m = s + f;
					unsigned int neighbor = index(molPosL, y, z);
					Vector spin = getSpin(neighbor);
					Vector flux = getFlux(neighbor);
					Vector mag = spin + flux;
					couple_ss_mL += s * spin;
					couple_e1_mL += s * flux;
					couple_e1_mL += f * spin;
					couple_ee_mL += f * flux;
					biquad_mL += sq(m * mag);
					dmi_mL += m.crossProduct(mag);  // location(m)=x1 in "L" < location(mag)=molPosL in "m"
				}
	results.UmL = 0;
	results.UmL -= parameters.JmL * couple_ss_mL;
	results.UmL -= parameters.Je1mL * couple_e1_mL;
	results.UmL -= parameters.JeemL * couple_ee_mL;
	results.UmL -= parameters.bmL * biquad_mL;
	results.UmL -= parameters.DmL * dmi_mL;
	
	double couple_ss_mR = 0;  // sum(s_i * s_j)
	double couple_e1_mR = 0;  // sum(s_i * f_j)
	double couple_ee_mR = 0;  // sum(f_i * f_j)
	double biquad_mR = 0;
	Vector dmi_mR = Vector::ZERO;  // sum(m_i x m_j);  where x is cross product, and i < j
	const unsigned int x2 = molPosR + 1; //only useful if( molPosR != width - 1 )
	if( x2 != width && molPosR >= molPosL )
		for( unsigned int y = topL; y <= bottomL; y++ )
			for( unsigned int z = frontR; z <= backR; z++ )
				if( y == topL || z == frontR || y == bottomL || z == backR ) {
					unsigned int a = index(molPosR, y, z);
					Vector s = getSpin(a);
					Vector f = getFlux(a);
					Vector m = s + f;
					unsigned int neighbor = index(x2, y, z);
					Vector spin = getSpin(neighbor);
					Vector flux = getFlux(neighbor);
					Vector mag = spin + flux;
					couple_ss_mR += s * spin;
					couple_e1_mR += s * flux;
					couple_e1_mR += f * spin;
					couple_ee_mR += f * flux;
					biquad_mR += sq(m * mag);
					dmi_mR += m.crossProduct(mag); // location(m)=molPosR in "m" < location(mag)=x2 in "R"
				}
	results.UmR = 0;
	results.UmR -= parameters.JmR * couple_ss_mR;
	results.UmR -= parameters.Je1mR * couple_e1_mR;
	results.UmR -= parameters.JeemR * couple_ee_mR;
	results.UmR -= parameters.bmR * biquad_mR;
	results.UmR -= parameters.DmR * dmi_mR;
	
	double couple_ss_LR = 0;  // sum(s_i * s_j)
	double couple_e1_LR = 0;  // sum(s_i * f_j)
	double couple_ee_LR = 0;  // sum(f_i * f_j)
	double biquad_LR = 0;
	Vector dmi_LR = Vector::ZERO;  // sum(m_i x m_j);  where x is cross product, and i < j
	if( molPosL != 0 && x2 != width )
		for( unsigned int z = frontR; z <= backR; z++ )
			for( unsigned int y = topL; y <= bottomL; y++ ) {
				unsigned int a = index(x1, y, z);
				Vector s = getSpin(a);
				Vector f = getFlux(a);
				Vector m = s + f;
				unsigned int neighbor = index(x2, y, z);
				Vector spin = getSpin(neighbor);
				Vector flux = getFlux(neighbor);
				Vector mag = spin + flux;
				couple_ss_LR += s * spin;
				couple_e1_LR += s * flux;
				couple_e1_LR += f * spin;
				couple_ee_LR += f * flux;
				biquad_LR += sq(m * mag);
				dmi_LR += m.crossProduct(mag); // location(m) = x1 in "L" < location(mag) = x2 in "R"
			}
	results.ULR = 0;
	results.ULR -= parameters.JLR * couple_ss_LR;
	results.ULR -= parameters.Je1LR * couple_e1_LR;
	results.ULR -= parameters.JeeLR * couple_ee_LR;
	results.ULR -= parameters.bLR * biquad_LR;
	results.ULR -= parameters.DLR * dmi_LR;
	 
	results.U = results.UL + results.UR + results.Um + results.UmL + results.UmR + results.ULR;
}

MSD::Results MSD::getResults() const {
	return results;
}


void MSD::setB(const Vector &B) {
	// optimized energy recalculation when only changing magnetic field
	Vector deltaB = B - parameters.B;
	
	results.UL -= deltaB * results.ML;
	results.UR -= deltaB * results.MR;
	results.Um -= deltaB * results.Mm;
	results.U = results.UL + results.UR + results.Um + results.UmL + results.UmR + results.ULR;
	
	parameters.B = B;
}


Vector MSD::getSpin(unsigned int a) const {
	return spins.at(a);
}

Vector MSD::getSpin(unsigned int x, unsigned int y, unsigned int z) const {
	if( x >= width || y >= height || z >= depth )
		throw out_of_range("(x,y,z) coordinate not in range");
	return getSpin( index(x, y, z) );
}

Vector MSD::getFlux(unsigned int a) const {
	return fluxes.at(a);
}

Vector MSD::getFlux(unsigned int x, unsigned int y, unsigned int z) const {
	if( x >= width || y >= height || z >= depth )
		throw out_of_range("(x,y,z) coordinate not in range");
	return getFlux( index(x, y, z) );
}

Vector MSD::getLocalM(unsigned int a) const {
	return getSpin(a) + getFlux(a);
}

Vector MSD::getLocalM(unsigned int x, unsigned int y, unsigned int z) const {
	if( x >= width || y >= height || z >= depth )
		throw out_of_range("(x,y,z) coordinate not in range");
	return getLocalM( index(x, y, z) );
}

void MSD::setSpin(unsigned int a, const Vector &spin) {
	setLocalM( a, spin, getFlux(a) );
}

void MSD::setSpin(unsigned int x, unsigned int y, unsigned int z, const Vector &spin) {
	if( x >= width || y >= height || z >= depth )
		throw out_of_range("(x,y,z) coordinate not in range");
	setSpin( index(x, y, z), spin );
}

void MSD::setFlux(unsigned int a, const Vector &flux) {
	setLocalM( a, getSpin(a), flux );
}

void MSD::setFlux(unsigned int x, unsigned int y, unsigned int z, const Vector &flux) {
	if( x >= width || y >= height || z >= depth )
		throw out_of_range("(x,y,z) coordinate not in range");
	setFlux( index(x, y, z), flux );
}

void MSD::setLocalM(unsigned int a, const Vector &spin, const Vector &flux) {
	try {

	Vector &s = spins.at(a); //previous spin
	Vector &f = fluxes.at(a); //previous spin fluctuation
	
	// TODO: remove this "optimization" ???
	if( s == spin && f == flux )
		return;
	
	Vector m = s + f; // previous local magnetization
	Vector mag = spin + flux; // new local magnetization
	
	unsigned int x = MSD::x(a);
	unsigned int y = MSD::y(a);
	unsigned int z = MSD::z(a);
	
	Vector deltaS = spin - s;
	Vector deltaF = flux - f;
	Vector deltaM = mag - m;
	results.M += deltaM;
	results.MS += deltaS;
	results.MF += deltaF;
	
	// delta U's are actually negative, simply grouping the negatives in front of each energy coefficient into deltaU -= ... (instead of +=)
	double deltaU_B = parameters.B * deltaM;
	results.U -= deltaU_B;
	
	if( x < molPosL ) {
	
		results.ML += deltaM;
		results.MSL += deltaS;
		results.MFL += deltaF;
		results.UL -= deltaU_B;
		
		{	double deltaU = parameters.AL * ( Vector(sq(mag.x), sq(mag.y), sq(mag.z)) - Vector(sq(m.x), sq(m.y), sq(m.z)) )
		                  + parameters.Je0L * ( spin * flux - s * f );
			results.U -= deltaU;
			results.UL -= deltaU;
		}
		
		if( x != 0 ) {
			unsigned int a1 = index(x - 1, y, z);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = parameters.JL * ( neighbor_s * deltaS )
						  + parameters.Je1L * ( neighbor_f * deltaS + neighbor_s * deltaF )
						  + parameters.JeeL * ( neighbor_f * deltaF )
			              + parameters.bL * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
						  + parameters.DL * neighbor_m.crossProduct(deltaM);  // (a1 < a): right vector changed
			results.U -= deltaU;
			results.UL -= deltaU;
		} // else, x - 1 neighbor doesn't exist
		if( y != topL ) {
			unsigned int a1 = index(x, y - 1, z);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = parameters.JL * ( neighbor_s * deltaS )
						  + parameters.Je1L * ( neighbor_f * deltaS + neighbor_s * deltaF )
						  + parameters.JeeL * ( neighbor_f * deltaF )
			              + parameters.bL * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
						  + parameters.DL * neighbor_m.crossProduct(deltaM);  // (a1 < a): right vector changed
			results.U -= deltaU;
			results.UL -= deltaU;
		} // else, y - 1 neighbor doesn't exist
		if( y != bottomL ) {
			unsigned int a1 = index(x, y + 1, z);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = parameters.JL * ( neighbor_s * deltaS )
						  + parameters.Je1L * ( neighbor_f * deltaS + neighbor_s * deltaF )
						  + parameters.JeeL * ( neighbor_f * deltaF )
			              + parameters.bL * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
						  + parameters.DL * deltaM.crossProduct(neighbor_m);  // (a < a1): left vector changed
			results.U -= deltaU;
			results.UL -= deltaU;
		} // else, y + 1 neighbor doesn't exist
		if( z != 0 ) {
			unsigned int a1 = index(x, y, z - 1);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = parameters.JL * ( neighbor_s * deltaS )
						  + parameters.Je1L * ( neighbor_f * deltaS + neighbor_s * deltaF )
						  + parameters.JeeL * ( neighbor_f * deltaF )
			              + parameters.bL * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
						  + parameters.DL * neighbor_m.crossProduct(deltaM);  // (a1 < a): right vector changed
			results.U -= deltaU;
			results.UL -= deltaU;
		} // else, z - 1 neighbor doesn't exist
		if( z + 1 != depth ) {
			unsigned int a1 = index(x, y, z + 1);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = parameters.JL * ( neighbor_s * deltaS )
						  + parameters.Je1L * ( neighbor_f * deltaS + neighbor_s * deltaF )
						  + parameters.JeeL * ( neighbor_f * deltaF )
			              + parameters.bL * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
						  + parameters.DL * deltaM.crossProduct(neighbor_m);  // (a < a1): left vector changed
			results.U -= deltaU;
			results.UL -= deltaU;
		} // else, z + 1 neighbor doesn't exist
		
		if( x + 1 != width )
			if( x + 1 == molPosL ) {
				if( molPosR >= molPosL )
					try {
						unsigned int a1 = index(x + 1, y, z);
						Vector neighbor_s = getSpin(a1);
						Vector neighbor_f = getFlux(a1);
						Vector neighbor_m = neighbor_s + neighbor_f;
						double deltaU = parameters.JmL * ( neighbor_s * deltaS )
						              + parameters.Je1mL * ( neighbor_f * deltaS + neighbor_s * deltaF )
						              + parameters.JeemL * ( neighbor_f * deltaF )
						              + parameters.bmL * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
									  + parameters.DmL * deltaM.crossProduct(neighbor_m);  // (a < a1): left vector changed
						results.U -= deltaU;
						results.UmL -= deltaU;
					} catch(const out_of_range &e) {} // x + 1 neighbor doesn't exist because it's in the buffer zone
				// else, the molecule doesn't exist
				
				if( molPosR + 1 != width ) {
					try {
						unsigned int a1 = index(molPosR + 1, y, z);
						Vector neighbor_s = getSpin(a1);
						Vector neighbor_f = getFlux(a1);
						Vector neighbor_m = neighbor_s + neighbor_f;
						double deltaU = parameters.JLR * ( neighbor_s * deltaS )
						              + parameters.Je1LR * ( neighbor_f * deltaS + neighbor_s * deltaF )
						              + parameters.JeeLR * ( neighbor_f * deltaF )
						              + parameters.bLR * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
									  + parameters.DLR * deltaM.crossProduct(neighbor_m);  // (a < a1): left vector changed
						results.U -= deltaU;
						results.ULR -= deltaU;
					} catch(const out_of_range &e) {} // molPosR + 1 atom doesn't exist because we're not in the center
				} // else, the right ferromagnet doesn't exist
			} else {
				unsigned int a1 = index(x + 1, y, z);
				Vector neighbor_s = getSpin(a1);
				Vector neighbor_f = getFlux(a1);
				Vector neighbor_m = neighbor_s + neighbor_f;
				double deltaU = parameters.JL * ( neighbor_s * deltaS )
				              + parameters.Je1L * ( neighbor_f * deltaS + neighbor_s * deltaF )
				              + parameters.JeeL * ( neighbor_f * deltaF )
				              + parameters.bL * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
							  + parameters.DL * deltaM.crossProduct(neighbor_m);  // (a < a1): left vector changed
				results.U -= deltaU;
				results.UL -= deltaU;
			}
		// else, x + 1 neighbor doesn't exist (because molPosL == width)
		
	} else if( x > molPosR ) {
	
		results.MR += deltaM;
		results.MSR += deltaS;
		results.MFR += deltaF;
		results.UR -= deltaU_B;
		
		{	double deltaU = parameters.AR * ( Vector(sq(mag.x), sq(mag.y), sq(mag.z)) - Vector(sq(m.x), sq(m.y), sq(m.z)) )
			              + parameters.Je0R * ( spin * flux - s * f );
			results.U -= deltaU;
			results.UR -= deltaU;
		}
		
		if( x + 1 != width ) {
			unsigned int a1 = index(x + 1, y, z);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = parameters.JR * ( neighbor_s * deltaS )
			              + parameters.Je1R * ( neighbor_f * deltaS + neighbor_s * deltaF )
			              + parameters.JeeR * ( neighbor_f * deltaF )
			              + parameters.bR * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
						  + parameters.DR * deltaM.crossProduct(neighbor_m);  // (a < a1): left vector changed
			results.U -= deltaU;
			results.UR -= deltaU;
		} // else, x + 1 neighbor doesn't exist
		if( y != 0 ) {
			unsigned int a1 = index(x, y - 1, z);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = parameters.JR * ( neighbor_s * deltaS )
			              + parameters.Je1R * ( neighbor_f * deltaS + neighbor_s * deltaF )
			              + parameters.JeeR * ( neighbor_f * deltaF )
			              + parameters.bR * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
						  + parameters.DR * neighbor_m.crossProduct(deltaM);  // (a1 < a): right vector changed
			results.U -= deltaU;
			results.UR -= deltaU;
		} // else, y - 1 neighbor doesn't exist
		if( y + 1 != height ) {
			unsigned int a1 = index(x, y + 1, z);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = parameters.JR * ( neighbor_s * deltaS )
			              + parameters.Je1R * ( neighbor_f * deltaS + neighbor_s * deltaF )
			              + parameters.JeeR * ( neighbor_f * deltaF )
			              + parameters.bR * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
						  + parameters.DR * deltaM.crossProduct(neighbor_m);  // (a < a1): left vector changed
			results.U -= deltaU;
			results.UR -= deltaU;
		} // else, y + 1 neighbor doesn't exist
		if( z != frontR ) {
			unsigned int a1 = index(x, y, z - 1);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = parameters.JR * ( neighbor_s * deltaS )
			              + parameters.Je1R * ( neighbor_f * deltaS + neighbor_s * deltaF )
			              + parameters.JeeR * ( neighbor_f * deltaF )
			              + parameters.bR * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
						  + parameters.DR * neighbor_m.crossProduct(deltaM);  // (a1 < a): right vector changed
			results.U -= deltaU;
			results.UR -= deltaU;
		} // else, z - 1 neighbor doesn't exist
		if( z != backR ) {
			unsigned int a1 = index(x, y, z + 1);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = parameters.JR * ( neighbor_s * deltaS )
			              + parameters.Je1R * ( neighbor_f * deltaS + neighbor_s * deltaF )
			              + parameters.JeeR * ( neighbor_f * deltaF )
			              + parameters.bR * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
						  + parameters.DR * deltaM.crossProduct(neighbor_m);  // (a < a1): left vector changed
			results.U -= deltaU;
			results.UR -= deltaU;
		} // else, z + 1 neighbor doesn't exist
		
		// x != 0, because x > (unsigned molPosR) >= 0
		if( x - 1 == molPosR ) {
			if( molPosR >= molPosL )
				try {
					unsigned int a1 = index(x - 1, y, z);
					Vector neighbor_s = getSpin(a1);
					Vector neighbor_f = getFlux(a1);
					Vector neighbor_m = neighbor_s + neighbor_f;
					double deltaU = parameters.JmR * ( neighbor_s * deltaS )
					              + parameters.Je1mR * ( neighbor_f * deltaS + neighbor_s * deltaF )
					              + parameters.JeemR * ( neighbor_f * deltaF )
					              + parameters.bmR * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
								  + parameters.DmR * neighbor_m.crossProduct(deltaM);  // (a1 < a): right vector changed
					results.U -= deltaU;
					results.UmR -= deltaU;
				} catch(const out_of_range &e) {} // x - 1 neighbor doesn't exist because it's in the buffer zone
			// else, the molecule doesn't exist
			
			if( molPosL != 0 ) {
				try {
					unsigned int a1 = index(molPosL - 1, y, z);
					Vector neighbor_s = getSpin(a1);
					Vector neighbor_f = getFlux(a1);
					Vector neighbor_m = neighbor_s + neighbor_f;
					double deltaU = parameters.JLR * ( neighbor_s * deltaS )
					              + parameters.Je1LR * ( neighbor_f * deltaS + neighbor_s * deltaF )
					              + parameters.JeeLR * ( neighbor_f * deltaF )
					              + parameters.bLR * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
								  + parameters.DLR * neighbor_m.crossProduct(deltaM);  // (a1 < a): right vector changed
					results.U -= deltaU;
					results.ULR -= deltaU;
				} catch(const out_of_range &e) {} // molPos - 1 atom doesn't exist because we're not in the center
			} // else, the left ferromagnet doesn't exist
		} else {
			unsigned int a1 = index(x - 1, y, z);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			double deltaU = parameters.JR * ( neighbor_s * deltaS )
			              + parameters.Je1R * ( neighbor_f * deltaS + neighbor_s * deltaF )
			              + parameters.JeeR * ( neighbor_f * deltaF )
			              + parameters.bR * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
						  + parameters.DR * neighbor_m.crossProduct(deltaM);  // (a1 < a): right vector changed
			results.U -= deltaU;
			results.UR -= deltaU;
		}
		
	} else { // molPosL <= x <= molPosR
	
		results.Mm += deltaM;
		results.MSm += deltaS;
		results.MFm += deltaF;
		results.Um -= deltaU_B;
		
		{	double deltaU = parameters.Am * ( Vector(sq(mag.x), sq(mag.y), sq(mag.z)) - Vector(sq(m.x), sq(m.y), sq(m.z)) )
			              + parameters.Je0m * ( spin * flux - s * f );
			results.U -= deltaU;
			results.Um -= deltaU;
		}
		
		if( x != 0 ) {
			unsigned int a1 = index(x - 1, y, z);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			if( x == molPosL ) {
				double deltaU = parameters.JmL * ( neighbor_s * deltaS )
				              + parameters.Je1mL * ( neighbor_f * deltaS + neighbor_s * deltaF )
				              + parameters.JeemL * ( neighbor_f * deltaF )
				              + parameters.bmL * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
							  + parameters.DmL * neighbor_m.crossProduct(deltaM);  // (a1 < a): right vector changed
				results.U -= deltaU;
				results.UmL -= deltaU;
			} else {
				double deltaU = parameters.Jm * ( neighbor_s * deltaS )
				              + parameters.Je1m * ( neighbor_f * deltaS + neighbor_s * deltaF )
				              + parameters.Jeem * ( neighbor_f * deltaF )
				              + parameters.bm * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
							  + parameters.Dm * neighbor_m.crossProduct(deltaM);  // (a1 < a): right vector changed
				results.U -= deltaU;
				results.Um -= deltaU;
			}
		}
		
		if( x + 1 != width ) {
			unsigned int a1 = index(x + 1, y, z);
			Vector neighbor_s = getSpin(a1);
			Vector neighbor_f = getFlux(a1);
			Vector neighbor_m = neighbor_s + neighbor_f;
			if( x == molPosR ) {
				double deltaU = parameters.JmR * ( neighbor_s * deltaS )
				              + parameters.Je1mR * ( neighbor_f * deltaS + neighbor_s * deltaF )
				              + parameters.JeemR * ( neighbor_f * deltaF )
				              + parameters.bmR * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
							  + parameters.DmR * deltaM.crossProduct(neighbor_m);  // (a < a1): left vector changed
				results.U -= deltaU;
				results.UmR -= deltaU;
			} else {
				double deltaU = parameters.Jm * ( neighbor_s * deltaS )
				              + parameters.Je1m * ( neighbor_f * deltaS + neighbor_s * deltaF )
				              + parameters.Jeem * ( neighbor_f * deltaF )
				              + parameters.bm * ( sq(neighbor_m * mag) - sq(neighbor_m * m) )
							  + parameters.Dm * deltaM.crossProduct(neighbor_m);  // (a < a1): left vector changed
				results.U -= deltaU;
				results.Um -= deltaU;
			}
		}
		
	}
	
	s = spin;
	f = flux;
	// setParameters(parameters); //recalculate results (the long way!!!)

	} catch(const out_of_range &ex) {
		// For debugging. This exception should not happen in production!
		std::cerr << "ERROR in MSD::setLocalM(unsigned int, udc::Vector, udc::Vector)\n";
		std::cerr << a << " == (" << x(a) << ", " << y(a) << ", " << z(a) << ")\n";
		std::cerr << ex.what() << "\n\n";
		std::cerr << "topL=" << topL << ", bottomL=" << bottomL << ", frontR=" << frontR << ", backR=" << backR << '\n';
		std::cerr << "Valid Indicies:\n";
		for (auto iter = indices.begin(); iter != indices.end(); ++iter)
			std::cerr << *iter << " == (" << x(*iter) << ", " << y(*iter) << ", " << z(*iter) << ")\n";
		exit(200);
	}
}

void MSD::setLocalM(unsigned int x, unsigned int y, unsigned int z, const Vector &spin, const Vector &flux) {
	if( x >= width || y >= height || z >= depth )
		throw out_of_range("(x,y,z) coordinate not in range");
	setLocalM( index(x, y, z), spin, flux );
}


unsigned int MSD::getN() const {
	return n;
}

unsigned int MSD::getNL() const {
	return nL;
}

unsigned int MSD::getNR() const {
	return nR;
}

unsigned int MSD::getNm() const {
	return n_m;
}

unsigned int MSD::getNmL() const {
	return n_mL;
}

unsigned int MSD::getNmR() const {
	return n_mR;
}

unsigned int MSD::getNLR() const {
	return nLR;
}

unsigned int MSD::getWidth() const {
	return width;
}

unsigned int MSD::getHeight() const {
	return height;
}

unsigned int MSD::getDepth() const {
	return depth;
}

void MSD::getDimensions(unsigned int &width, unsigned int &height, unsigned int &depth) const {
	width = this->width;
	height = this->height;
	depth = this->depth;
}

unsigned int MSD::getMolPosL() const {
	return molPosL;
}

unsigned int MSD::getMolPosR() const {
	return molPosR;
}

void MSD::getMolPos(unsigned int &molPosL, unsigned int &molPosR) const {
	molPosL = this->molPosL;
	molPosR = this->molPosR;
}

unsigned int MSD::getTopL() const {
	return topL;
}

unsigned int MSD::getBottomL() const {
	return bottomL;
}

unsigned int MSD::getFrontR() const {
	return frontR;
}

unsigned int MSD::getBackR() const {
	return backR;
}

void MSD::getInnerBounds(unsigned int &topL, unsigned int &bottomL, unsigned int &frontR, unsigned int &backR) const{
	topL = this->topL;
	bottomL = this->bottomL;
	frontR = this->frontR;
	backR = this->backR;
}

bool MSD::getFM_L_exists() const {
	return FM_L_exists;
}

bool MSD::getFM_R_exists() const {
	return FM_R_exists;
}

bool MSD::getMol_exists() const {
	return mol_exists;
}

void MSD::getRegions(bool &FM_L_exists, bool &FM_R_exists, bool &mol_exists) const {
	FM_L_exists = this->FM_L_exists;
	FM_R_exists = this->FM_R_exists;
	mol_exists = this->mol_exists;
}


void MSD::setSeed(unsigned long seed) {
	this->seed = seed;
	prng.seed(seed);
}

unsigned long MSD::getSeed() const {
	return seed;
}


void MSD::reinitialize(bool reseed) {
	if( reseed )
		seed = genSeed();
	prng.seed(seed);
	for( auto i = begin(); i != end(); i++ )
		setLocalM( i, initSpin, initFlux );
	record.clear();
	setParameters(parameters);
	results.t = 0;
}

void MSD::randomize(bool reseed) {
	if( reseed )
		seed = genSeed();
	prng.seed(seed);

	// so that setParameters scales the flux magnitudes correctly
	Parameters p0 = getParameters();
	Parameters pTemp = p0;
	pTemp.FL = pTemp.FR = pTemp.Fm = 1;
	setParameters(pTemp);

	for( auto i = begin(); i != end(); i++ )
		setLocalM( i,
				Vector::sphericalForm(1, 2 * PI * rand(prng), asin(2 * rand(prng) - 1)),
				Vector::sphericalForm(rand(prng), 2 * PI * rand(prng), asin(2 * rand(prng) - 1)) );
	record.clear();
	setParameters(p0);
	results.t = 0;
}

void MSD::metropolis(unsigned long long N) {
	function<double()> random = bind( rand, ref(prng) );
	Results r = getResults(); //get the energy of the system
	//start loop (will iterate N times)
	for( unsigned long long i = 0; i < N; i++ ) {
		unsigned int a = indices[static_cast<unsigned int>( random() * indices.size() )]; //pick an atom (pseudo) randomly
		Vector s = getSpin(a);
		Vector f = getFlux(a);

		// pick the correct F coeficient to determine new flux magnitude
		unsigned int x = this->x(a);
		double F = (x < molPosL ? parameters.FL : x > molPosR ? parameters.FR : parameters.Fm);

		//"flip" that atom
		setLocalM( a, flippingAlgorithm(s, random),
				Vector::sphericalForm(F * random(), 2 * PI * random(), asin(2 * random() - 1)) );
		
		Results r2 = getResults(); //get the energy of the system (for the new state)
		if( r2.U <= r.U || random() < pow( E, (r.U - r2.U) / parameters.kT ) ) {
			//either the new system requires less energy or external energy (kT) is disrupting it
			r = r2; //in either case we keep the new system
		} else {
			//neither thing (above) happened so we revert the system
			spins[a] = s; //revert the system by flipping the atom back
			fluxes[a] = f;
			results = r;
		}
	}
	results.t += N;
}

void MSD::metropolis(unsigned long long N, unsigned long long freq) {
	if( freq == 0 ) {
		metropolis(N);
		return;
	}
	while(true) {
		record.push_back( getResults() );
		if( N >= freq ) {
			metropolis(freq);
			N -= freq;
		} else {
			if( N != 0 )
				metropolis(N);
			break;
		}
	}
}


double MSD::specificHeat() const {
	if (record.size() <= 1) {
		return 0;  // <U^2> - <U>^2 == 0 if there is only 1 data point
	}

	const Results *r0 = &record.at(0);
	double s = 0, s2 = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results *r1 = &record[i];
		
		double dU = r1->U - r0->U;
		double dt = r1->t - r0->t;
		s += (r0->U + r1->U) * dt;  // trapizoidal rule (1/2 factored out)
		s2 += ((dt/3 * dU + r0->U) * dU + sq(r0->U)) * dt;  // square of trapizoidal rule (linear interpolation)

		r0 = r1;
	}
	// Note: r0 = & last record
	double dt = r0->t - record[0].t;
	double avg = 0.5 * s / dt;
	double avgSq = s2 / dt;

	return (avgSq - sq(avg)) / (n * parameters.kT * parameters.kT);
}

double MSD::specificHeat_L() const {
	if (record.size() <= 1) {
		return 0;  // <U^2> - <U>^2 == 0 if there is only 1 data point
	}

	const Results *r0 = &record.at(0);
	double s = 0, s2 = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results *r1 = &record[i];
		
		double dU = r1->UL - r0->UL;
		double dt = r1->t - r0->t;
		s += (r0->UL + r1->UL) * dt;  // trapizoidal rule (1/2 factored out)
		s2 += ((dt/3 * dU + r0->UL) * dU + sq(r0->UL)) * dt;  // square of trapizoidal rule (linear interpolation)

		r0 = r1;
	}
	// Note: r0 = & last record
	double dt = r0->t - record[0].t;
	double avg = 0.5 * s / dt;
	double avgSq = s2 / dt;

	return (avgSq - sq(avg)) / (nL * parameters.kT * parameters.kT);
}

double MSD::specificHeat_R() const {
	if (record.size() <= 1) {
		return 0;  // <U^2> - <U>^2 == 0 if there is only 1 data point
	}

	const Results *r0 = &record.at(0);
	double s = 0, s2 = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results *r1 = &record[i];
		
		double dU = r1->UR - r0->UR;
		double dt = r1->t - r0->t;
		s += (r0->UR + r1->UR) * dt;  // trapizoidal rule (1/2 factored out)
		s2 += ((dt/3 * dU + r0->UR) * dU + sq(r0->UR)) * dt;  // square of trapizoidal rule (linear interpolation)

		r0 = r1;
	}
	// Note: r0 = & last record
	double dt = r0->t - record[0].t;
	double avg = 0.5 * s / dt;
	double avgSq = s2 / dt;

	return (avgSq - sq(avg)) / (nR * parameters.kT * parameters.kT);
}

double MSD::specificHeat_m() const {
	if (record.size() <= 1) {
		return 0;  // <U^2> - <U>^2 == 0 if there is only 1 data point
	}

	const Results *r0 = &record.at(0);
	double s = 0, s2 = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results *r1 = &record[i];
		
		double dU = r1->Um - r0->Um;
		double dt = r1->t - r0->t;
		s += (r0->Um + r1->Um) * dt;  // trapizoidal rule (1/2 factored out)
		s2 += ((dt/3 * dU + r0->Um) * dU + sq(r0->Um)) * dt;  // square of trapizoidal rule (linear interpolation)

		r0 = r1;
	}
	// Note: r0 = & last record
	double dt = r0->t - record[0].t;
	double avg = 0.5 * s / dt;
	double avgSq = s2 / dt;

	return (avgSq - sq(avg)) / (n_m * parameters.kT * parameters.kT);
}

double MSD::specificHeat_mL() const {
	if (record.size() <= 1) {
		return 0;  // <U^2> - <U>^2 == 0 if there is only 1 data point
	}

	const Results *r0 = &record.at(0);
	double s = 0, s2 = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results *r1 = &record[i];
		
		double dU = r1->UmL - r0->UmL;
		double dt = r1->t - r0->t;
		s += (r0->UmL + r1->UmL) * dt;  // trapizoidal rule (1/2 factored out)
		s2 += ((dt/3 * dU + r0->UmL) * dU + sq(r0->UmL)) * dt;  // square of trapizoidal rule (linear interpolation)

		r0 = r1;
	}
	// Note: r0 = & last record
	double dt = r0->t - record[0].t;
	double avg = 0.5 * s / dt;
	double avgSq = s2 / dt;

	return (avgSq - sq(avg)) / (n_mL * parameters.kT * parameters.kT);
}

double MSD::specificHeat_mR() const {
	if (record.size() <= 1) {
		return 0;  // <U^2> - <U>^2 == 0 if there is only 1 data point
	}

	const Results *r0 = &record.at(0);
	double s = 0, s2 = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results *r1 = &record[i];
		
		double dU = r1->UmR - r0->UmR;
		double dt = r1->t - r0->t;
		s += (r0->UmR + r1->UmR) * dt;  // trapizoidal rule (1/2 factored out)
		s2 += ((dt/3 * dU + r0->UmR) * dU + sq(r0->UmR)) * dt;  // square of trapizoidal rule (linear interpolation)

		r0 = r1;
	}
	// Note: r0 = & last record
	double dt = r0->t - record[0].t;
	double avg = 0.5 * s / dt;
	double avgSq = s2 / dt;

	return (avgSq - sq(avg)) / (n_mR * parameters.kT * parameters.kT);
}

double MSD::specificHeat_LR() const {
	if (record.size() <= 1) {
		return 0;  // <U^2> - <U>^2 == 0 if there is only 1 data point
	}

	const Results *r0 = &record.at(0);
	double s = 0, s2 = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results *r1 = &record[i];
		
		double dU = r1->ULR - r0->ULR;
		double dt = r1->t - r0->t;
		s += (r0->ULR + r1->ULR) * dt;  // trapizoidal rule (1/2 factored out)
		s2 += ((dt/3 * dU + r0->ULR) * dU + sq(r0->ULR)) * dt;  // square of trapizoidal rule (linear interpolation)

		r0 = r1;
	}
	// Note: r0 = & last record
	double dt = r0->t - record[0].t;
	double avg = 0.5 * s / dt;
	double avgSq = s2 / dt;

	return (avgSq - sq(avg)) / (nLR * parameters.kT * parameters.kT);
}

double MSD::magneticSusceptibility() const {
	if (record.size() <= 1) {
		return 0;  // <U^2> - <U>^2 == 0 if there is only 1 data point
	}

	const Results *r0 = &record.at(0);
	Vector s = Vector::ZERO;
	double s2 = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results *r1 = &record[i];
		
		Vector dM = r1->M - r0->M;
		double dt = r1->t - r0->t;
		s += (r0->M + r1->M) * dt;  // trapizoidal rule (1/2 factored out)
		s2 += ((dt/3 * dM + r0->M) * dM + r0->M * r0->M) * dt;  // square of trapizoidal rule (linear interpolation)

		r0 = r1;
	}
	// Note: r0 = & last record
	double dt = r0->t - record[0].t;
	Vector avg = 0.5 / dt * s;
	double avgSq = s2 / dt;

	return (avgSq - avg * avg) / (n * parameters.kT * parameters.kT);
}

double MSD::magneticSusceptibility_L() const {
	if (record.size() <= 1) {
		return 0;  // <U^2> - <U>^2 == 0 if there is only 1 data point
	}

	const Results *r0 = &record.at(0);
	Vector s = Vector::ZERO;
	double s2 = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results *r1 = &record[i];
		
		Vector dM = r1->ML - r0->ML;
		double dt = r1->t - r0->t;
		s += (r0->ML + r1->ML) * dt;  // trapizoidal rule (1/2 factored out)
		s2 += ((dt/3 * dM + r0->ML) * dM + r0->ML * r0->ML) * dt;  // square of trapizoidal rule (linear interpolation)

		r0 = r1;
	}
	// Note: r0 = & last record
	double dt = r0->t - record[0].t;
	Vector avg = 0.5 / dt * s;
	double avgSq = s2 / dt;

	return (avgSq - avg * avg) / (nL * parameters.kT * parameters.kT);
}

double MSD::magneticSusceptibility_R() const {
	if (record.size() <= 1) {
		return 0;  // <U^2> - <U>^2 == 0 if there is only 1 data point
	}

	const Results *r0 = &record.at(0);
	Vector s = Vector::ZERO;
	double s2 = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results *r1 = &record[i];
		
		Vector dM = r1->MR - r0->MR;
		double dt = r1->t - r0->t;
		s += (r0->MR + r1->MR) * dt;  // trapizoidal rule (1/2 factored out)
		s2 += ((dt/3 * dM + r0->MR) * dM + r0->MR * r0->MR) * dt;  // square of trapizoidal rule (linear interpolation)

		r0 = r1;
	}
	// Note: r0 = & last record
	double dt = r0->t - record[0].t;
	Vector avg = 0.5 / dt * s;
	double avgSq = s2 / dt;

	return (avgSq - avg * avg) / (nR * parameters.kT * parameters.kT);
}

double MSD::magneticSusceptibility_m() const {
	if (record.size() <= 1) {
		return 0;  // <U^2> - <U>^2 == 0 if there is only 1 data point
	}

	const Results *r0 = &record.at(0);
	Vector s = Vector::ZERO;
	double s2 = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results *r1 = &record[i];
		
		Vector dM = r1->Mm - r0->Mm;
		double dt = r1->t - r0->t;
		s += (r0->Mm + r1->Mm) * dt;  // trapizoidal rule (1/2 factored out)
		s2 += ((dt/3 * dM + r0->Mm) * dM + r0->Mm * r0->Mm) * dt;  // square of trapizoidal rule (linear interpolation)

		r0 = r1;
	}
	// Note: r0 = & last record
	double dt = r0->t - record[0].t;
	Vector avg = 0.5 / dt * s;
	double avgSq = s2 / dt;

	return (avgSq - avg * avg) / (n_m * parameters.kT * parameters.kT);
}


Vector MSD::meanM() const {
	if( record.size() <= 1 )
		return record.at(0).M;
	//compensates for t by using the trapezoidal rule; (and the same below)
	Vector s = Vector::ZERO;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.M + r1.M);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanML() const {
	if( record.size() <= 1 )
		return record.at(0).ML;
	Vector s = Vector::ZERO;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.ML + r1.ML);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanMR() const {
	if( record.size() <= 1 )
		return record.at(0).MR;
	Vector s = Vector::ZERO;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.MR + r1.MR);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanMm() const {
	if( record.size() <= 1 )
		return record.at(0).Mm;
	Vector s = Vector::ZERO;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.Mm + r1.Mm);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanMS() const {
	if( record.size() <= 1 )
		return record.at(0).MS;
	//compensates for t by using the trapezoidal rule; (and the same below)
	Vector s = Vector::ZERO;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.MS + r1.MS);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanMSL() const {
	if( record.size() <= 1 )
		return record.at(0).MSL;
	Vector s = Vector::ZERO;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.MSL + r1.MSL);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanMSR() const {
	if( record.size() <= 1 )
		return record.at(0).MSR;
	Vector s = Vector::ZERO;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.MSR + r1.MSR);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanMSm() const {
	if( record.size() <= 1 )
		return record.at(0).MSm;
	Vector s = Vector::ZERO;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.MSm + r1.MSm);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanMF() const {
	if( record.size() <= 1 )
		return record.at(0).MF;
	//compensates for t by using the trapezoidal rule; (and the same below)
	Vector s = Vector::ZERO;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.MF + r1.MF);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanMFL() const {
	if( record.size() <= 1 )
		return record.at(0).MFL;
	Vector s = Vector::ZERO;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.MFL + r1.MFL);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanMFR() const {
	if( record.size() <= 1 )
		return record.at(0).MFR;
	Vector s = Vector::ZERO;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.MFR + r1.MFR);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanMFm() const {
	if( record.size() <= 1 )
		return record.at(0).MFm;
	Vector s = Vector::ZERO;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.MFm + r1.MFm);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanU() const {
	if( record.size() <= 1 )
		return record.at(0).U;
	double s = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.U + r1.U);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanUL() const {
	if( record.size() <= 1 )
		return record.at(0).UL;
	double s = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.UL + r1.UL);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanUR() const {
	if( record.size() <= 1 )
		return record.at(0).UR;
	double s = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.UR + r1.UR);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanUm() const {
	if( record.size() <= 1 )
		return record.at(0).Um;
	double s = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.Um + r1.Um);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanUmL() const {
	if( record.size() <= 1 )
		return record.at(0).UmL;
	double s = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.UmL + r1.UmL);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanUmR() const {
	if( record.size() <= 1 )
		return record.at(0).UmR;
	double s = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.UmR + r1.UmR);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanULR() const {
	if( record.size() <= 1 )
		return record.at(0).ULR;
	double s = 0;
	for( size_t i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.ULR + r1.ULR);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}


MSD::Iterator MSD::begin() const {
	return Iterator(*this, 0);
}

MSD::Iterator MSD::end() const {
	return Iterator(*this, indices.size());
}


} //end of namespace

#endif

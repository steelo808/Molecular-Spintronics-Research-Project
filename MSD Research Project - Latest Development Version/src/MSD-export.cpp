/**
 * @file MSD-export.cpp
 * @author Christopher D'Angelo
 * @brief The extern "C" version of MSD. May not contain the full functionality of MSD.h.
 * 	To be used for Python3 (ctypes) binding, as well as any future bindings that require a C (cdecl) DLL.
 * @version 1.1
 * @date 2022-11-29
 * 
 * @copyright Copyright (c) 2022
 */

#include <cstdlib>
#include "udc.h"
#include "Vector.h"
#include "MSD.h"

using namespace std;
using namespace udc;

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned long long ulonglong;
typedef MSD::MolProto MolProto;
typedef MolProto::NodeParameters NodeParameters;
typedef MolProto::EdgeParameters EdgeParameters;
typedef MSD::Iterator MSDIter;
typedef MolProto::NodeIterable Nodes;
typedef MolProto::EdgeIterable Edges;
typedef MolProto::NodeIterator NodeIter;
typedef MolProto::EdgeIterator EdgeIter;

#define C extern "C"
#define DLL __declspec(dllexport)


// MSD Globals
C DLL const MSD::MolProtoFactory * const LINEAR_MOL = &MSD::LINEAR_MOL;
C DLL const MSD::MolProtoFactory * const CIRCULAR_MOL = &MSD::CIRCULAR_MOL;

C DLL const MSD::FlippingAlgorithm * const UP_DOWN_MODEL = &MSD::UP_DOWN_MODEL;
C DLL const MSD::FlippingAlgorithm * const CONTINUOUS_SPIN_MODEL = &MSD::CONTINUOUS_SPIN_MODEL;

// MolProto Globals
C DLL const char * const HEADER = MolProto::HEADER;
C DLL const size_t HEADER_SIZE = MolProto::HEADER_SIZE;
C DLL const uint NOT_FOUND = MolProto::NOT_FOUND;

// Vector Globals
C DLL const Vector ZERO = Vector::ZERO;
C DLL const Vector I = Vector::I;
C DLL const Vector J = Vector::J;
C DLL const Vector K = Vector::K;

// udc Globals
C DLL const double E = udc::E;
C DLL const double PI = udc::PI;


// MSD Methods
C DLL MSD* createMSD_p(
		uint width, uint height, uint depth,
		const MolProto *molProto, uint molPosL,
		uint topL, uint bottomL, uint frontR, uint backR
) {
	return new MSD(width, height, depth, *molProto, molPosL, topL, bottomL, frontR, backR);
}

C DLL MSD* createMSD_f(
		uint width, uint height, uint depth,
		const MSD::MolProtoFactory *molType, uint molPosL, uint molPosR,
		uint topL, uint bottomL, uint frontR, uint backR
) {
	return new MSD(width, height, depth, *molType, molPosL, molPosR, topL, bottomL, frontR, backR);
}

C DLL MSD* createMSD_i(
		uint width, uint height, uint depth,
		uint molPosL, uint molPosR,
		uint topL, uint bottomL, uint frontR, uint backR
) {
	return new MSD(width, height, depth, molPosL, molPosR, topL, bottomL, frontR, backR);
}

C DLL MSD* createMSD_c(
		uint width, uint height, uint depth,
		uint heightL, uint depthR
) {
	return new MSD(width, height, depth, heightL, depthR);
}

C DLL MSD* createMSD_d(
		uint width, uint height, uint depth
) {
	return new MSD(width, height, depth);
}

C DLL void destroyMSD(MSD *msd) { delete msd; }

C DLL const MSD::Results* getRecord(const MSD *msd) { return &msd->record[0]; }
C DLL size_t getRecordSize(const MSD *msd) { return msd->record.size(); }
C DLL void setRecord(MSD *msd, const MSD::Results *record, size_t len) { msd->record = std::vector<MSD::Results>(record, record + len); }
C DLL void setFlippingAlgorithm(MSD *msd, const MSD::FlippingAlgorithm *algo) { msd->flippingAlgorithm = *algo; }

C DLL MSD::Parameters getParameters(const MSD *msd) { return msd->getParameters(); }
C DLL void setParameters(MSD *msd, const MSD::Parameters *p) { msd->setParameters(*p); }
C DLL MSD::Results getResults(const MSD *msd) { return msd->getResults(); }

C DLL void set_kT(MSD *msd, double kT) { msd->set_kT(kT); }
C DLL void setB(MSD *msd, const Vector *B) { msd->setB(*B); }

C DLL MolProto* getMolProto(const MSD *msd) { return new MolProto(msd->getMolProto()); }  // allocates memory!
C DLL void setMolProto(MSD *msd, const MolProto *proto) { msd->setMolProto(*proto); }
C DLL void setMolParameters(MSD *msd, const NodeParameters *nodeParams, const EdgeParameters *edgeParams) { msd->setMolParameters(*nodeParams, *edgeParams); }

C DLL Vector getSpin_i(const MSD *msd, uint a) { return msd->getSpin(a); }
C DLL Vector getSpin_v(const MSD *msd, uint x, uint y, uint z) { return msd->getSpin(x, y, z); }
C DLL Vector getFlux_i(const MSD *msd, uint a) { return msd->getFlux(a); }
C DLL Vector getFlux_v(const MSD *msd, uint x, uint y, uint z) { return msd->getFlux(x, y, z); }
C DLL Vector getLocalM_i(const MSD *msd, uint a) { return msd->getLocalM(a); }
C DLL Vector getLocalM_v(const MSD *msd, uint x, uint y, uint z) { return msd->getLocalM(x, y, z); }
C DLL void setSpin_i(MSD *msd, uint a, const Vector *spin) {	msd->setSpin(a, *spin); }
C DLL void setSpin_v(MSD *msd, uint x, uint y, uint z, const Vector *spin) { msd->setSpin(x, y, z, *spin); }
C DLL void setFlux_i(MSD *msd, uint a, const Vector *flux) { msd->setFlux(a, *flux); }
C DLL void setFlux_v(MSD *msd, uint x, uint y, uint z, const Vector *flux) { msd->setFlux(x, y, z, *flux); }
C DLL void setLocalM_i(MSD *msd, uint a, const Vector *spin, const Vector *flux) { msd->setLocalM(a, *spin, *flux); }
C DLL void setLocalM_v(MSD *msd, uint x, uint y, uint z, const Vector *spin, const Vector *flux) { msd->setLocalM(x, y, z, *spin, *flux); }

C DLL uint getN(const MSD *msd) { return msd->getN(); }
C DLL uint getNL(const MSD *msd) { return msd->getNL(); }
C DLL uint getNR(const MSD *msd) { return msd->getNR(); }
C DLL uint getNm(const MSD *msd) { return msd->getNm(); }
C DLL uint getNmL(const MSD *msd) { return msd->getNmL(); }
C DLL uint getNmR(const MSD *msd) { return msd->getNmR(); }
C DLL uint getNLR(const MSD *msd) { return msd->getNLR(); }
C DLL uint getWidth(const MSD *msd) { return msd->getWidth(); }
C DLL uint getHeight(const MSD *msd) { return msd->getHeight(); }
C DLL uint getDepth(const MSD *msd) { return msd->getDepth(); }
C DLL void getDimensions(const MSD *msd, uint *width, uint *height, uint *depth) { msd->getDimensions(*width, *height, *depth); }
C DLL uint getMolPosL(const MSD *msd) { return msd->getMolPosL(); }
C DLL uint getMolPosR(const MSD *msd) { return msd->getMolPosR(); }
C DLL void getMolPos(const MSD *msd, uint *molPosL, uint *molPosR) { msd->getMolPos(*molPosL, *molPosR); }
C DLL uint getTopL(const MSD *msd) { return msd->getTopL(); }
C DLL uint getBottomL(const MSD *msd) { return msd->getBottomL(); }
C DLL uint getFrontR(const MSD *msd) { return msd->getFrontR(); }
C DLL uint getBackR(const MSD *msd) { return msd->getBackR(); }
C DLL void getInnerBounds(const MSD *msd, uint *topL, uint *bottomL, uint *frontR, uint *backR) { msd->getInnerBounds(*topL, *bottomL, *frontR, *backR); }
C DLL bool getFM_L_exists(const MSD *msd) { return msd->getFM_L_exists(); }
C DLL bool getFM_R_exists(const MSD *msd) { return msd->getFM_R_exists(); }
C DLL bool getMol_exists(const MSD *msd) { return msd->getMol_exists(); }
C DLL void getRegions(const MSD *msd, bool *FM_L_exists, bool *FM_R_exists, bool *mol_exists) { msd->getRegions(*FM_L_exists, *FM_R_exists, *mol_exists); }

C DLL void setSeed(MSD *msd, ulong seed) { msd->setSeed(seed); }
C DLL ulong getSeed(const MSD *msd) { return msd->getSeed(); }

C DLL void reinitialize(MSD *msd, bool reseed) { msd->reinitialize(reseed); }
C DLL void randomize(MSD *msd, bool reseed) { msd->randomize(reseed); }
C DLL void metropolis_o(MSD *msd, ulonglong N) { msd->metropolis(N); }
C DLL void metropolis_r(MSD *msd, ulonglong N, ulonglong freq) { msd->metropolis(N, freq); }

C DLL double specificHeat(const MSD *msd) { return msd->specificHeat(); }
C DLL double specificHeat_L(const MSD *msd) { return msd->specificHeat_L(); }
C DLL double specificHeat_R(const MSD *msd) { return msd->specificHeat_R(); }
C DLL double specificHeat_m(const MSD *msd) { return msd->specificHeat_m(); }
C DLL double specificHeat_mL(const MSD *msd) { return msd->specificHeat_mL(); }
C DLL double specificHeat_mR(const MSD *msd) { return msd->specificHeat_mR(); }
C DLL double specificHeat_LR(const MSD *msd) { return msd->specificHeat_LR(); }
C DLL double magneticSusceptibility(const MSD *msd) { return msd->magneticSusceptibility(); }
C DLL double magneticSusceptibility_L(const MSD *msd) { return msd->magneticSusceptibility_L(); }
C DLL double magneticSusceptibility_R(const MSD *msd) { return msd->magneticSusceptibility_R(); }
C DLL double magneticSusceptibility_m(const MSD *msd) { return msd->magneticSusceptibility_m(); }

C DLL Vector meanM(const MSD *msd) { return msd->meanM(); }
C DLL Vector meanML(const MSD *msd) { return msd->meanML(); }
C DLL Vector meanMR(const MSD *msd) { return msd->meanMR(); }
C DLL Vector meanMm(const MSD *msd) { return msd->meanMm(); }
C DLL Vector meanMS(const MSD *msd) { return msd->meanMS(); }
C DLL Vector meanMSL(const MSD *msd) { return msd->meanMSL(); }
C DLL Vector meanMSR(const MSD *msd) { return msd->meanMSR(); }
C DLL Vector meanMSm(const MSD *msd) { return msd->meanMSm(); }
C DLL Vector meanMF(const MSD *msd) { return msd->meanMF(); }
C DLL Vector meanMFL(const MSD *msd) { return msd->meanMFL(); }
C DLL Vector meanMFR(const MSD *msd) { return msd->meanMFR(); }
C DLL Vector meanMFm(const MSD *msd) { return msd->meanMFm(); }
C DLL double meanU(const MSD *msd) { return msd->meanU(); }
C DLL double meanUL(const MSD *msd) { return msd->meanUL(); }
C DLL double meanUR(const MSD *msd) { return msd->meanUR(); }
C DLL double meanUm(const MSD *msd) { return msd->meanUm(); }
C DLL double meanUmL(const MSD *msd) { return msd->meanUmL(); }
C DLL double meanUmR(const MSD *msd) { return msd->meanUmR(); }
C DLL double meanULR(const MSD *msd) { return msd->meanULR(); }


// MolProto Methods
C DLL MolProto* createMolProto_e() { return new MolProto(); }
C DLL MolProto* createMolProto_n(size_t nodeCount) { return new MolProto(nodeCount); }
C DLL MolProto* createMolProto_p(size_t nodeCount, const NodeParameters *nodeParams) { return new MolProto(nodeCount, *nodeParams); }
C DLL void destroyMolProto(MolProto *proto) { delete proto; }

C DLL void serialize(const MolProto *proto, uchar *buffer) { proto->serialize(buffer); }
C DLL void deserialize(MolProto *proto, const uchar *buffer) { proto->deserialize(buffer); }
C DLL size_t serializationSize(const MolProto *proto) { return proto->serializationSize(); }

// TODO: read, write, load ??

C DLL uint createNode_d(MolProto *proto) { return proto->createNode(); }
C DLL uint createNode_p(MolProto *proto, const NodeParameters *params) { return proto->createNode(*params); }
C DLL uint nodeCount(const MolProto *proto) { return proto->nodeCount(); }
C DLL uint connectNodes_d(MolProto *proto, uint nodeA, uint nodeB) { return proto->connectNodes(nodeA, nodeB); }
C DLL uint connectNodes_p(MolProto *proto, uint nodeA, uint nodeB, const EdgeParameters *params) { return proto->connectNodes(nodeA, nodeB, *params); }
C DLL uint edgeIndex(MolProto *proto, uint nodeA, uint nodeB) { return proto->edgeIndex(nodeA, nodeB); }

C DLL EdgeParameters getEdgeParameters(const MolProto *proto, uint index) { return proto->getEdgeParameters(index); }
C DLL void setEdgeParameters(MolProto *proto, uint index, const EdgeParameters *params) { proto->setEdgeParameters(index, *params); }
C DLL NodeParameters getNodeParameters(const MolProto *proto, uint index) { return proto->getNodeParameters(index); }
C DLL void setNodeParameters(MolProto *proto, uint index, const NodeParameters *params) { proto->setNodeParameters(index, *params); }
C DLL void setAllParameters(MolProto *proto, const NodeParameters *nodeParams, const EdgeParameters *edgeParams) { proto->setAllParameters(*nodeParams, *edgeParams); }

C DLL void setLeftLead(MolProto *proto, uint node) { proto->setLeftLead(node); }
C DLL void setRightLead(MolProto *proto, uint node) { proto->setRightLead(node); }
C DLL void setLeads(MolProto *proto, uint left, uint right) { proto->setLeads(left, right); }
C DLL uint getLeftLead(const MolProto *proto) { return proto->getLeftLead(); }
C DLL uint getRightLead(const MolProto *proto) { return proto->getRightLead(); }
C DLL void getLeads(const MolProto *proto, uint *left, uint *right) { proto->getLeads(*left, *right); }


// Iterators
C DLL MSDIter* createBeginMSDIter(const MSD *msd) { return new MSDIter(msd->begin()); }
C DLL MSDIter* createEndMSDIter(const MSD *msd) { return new MSDIter(msd->end()); }
C DLL MSDIter* copyMSDIter(const MSDIter *iter) { return new MSDIter(*iter); }
C DLL void destroyMSDIter(MSDIter *iter) { delete iter; }
C DLL unsigned int getX(const MSDIter *iter) { return iter->getX(); }
C DLL unsigned int getY(const MSDIter *iter) { return iter->getY(); }
C DLL unsigned int getZ(const MSDIter *iter) { return iter->getZ(); }
C DLL unsigned int msdIndex(const MSDIter *iter) { return iter->getIndex(); }
C DLL Vector getSpin_a(const MSDIter *iter) { return iter->getSpin(); }
C DLL Vector getFlux_a(const MSDIter *iter) { return iter->getFlux(); }
C DLL Vector getLocalM_a(const MSDIter *iter) { return iter->getLocalM(); }
C DLL bool eq_a(const MSDIter *iter1, const MSDIter *iter2) { return *iter1 == *iter2; }
C DLL bool ne_a(const MSDIter *iter1, const MSDIter *iter2) { return *iter1 != *iter2; }
C DLL bool lt_a(const MSDIter *iter1, const MSDIter *iter2) { return *iter1 < *iter2; }
C DLL bool gt_a(const MSDIter *iter1, const MSDIter *iter2) { return *iter1 > *iter2; }
C DLL bool le_a(const MSDIter *iter1, const MSDIter *iter2) { return *iter1 <= *iter2; }
C DLL bool ge_a(const MSDIter *iter1, const MSDIter *iter2) { return *iter1 >= *iter2; }
C DLL void next_a(MSDIter *iter) { ++(*iter); }
C DLL void prev_a(MSDIter *iter) { --(*iter); }
C DLL void add_a(MSDIter *iter, int offset) { *iter += offset; }
C DLL void sub_a(MSDIter *iter, int offset) { *iter -= offset; }

C DLL Nodes* createNodes(const MolProto *proto) { return new Nodes(proto->getNodes()); }
C DLL void destroyNodes(Nodes *nodes) { delete nodes; }
C DLL Edges* createEdges(const MolProto *proto) { return new Edges(proto->getEdges()); }
C DLL Edges* createAdjacencyList(const MolProto *proto, uint nodeIndex) { return new Edges(proto->getAdjacencyList(nodeIndex)); }
C DLL void destroyEdges(Nodes *nodes) { delete nodes; }

C DLL uint size_n(const Nodes *nodes) { return nodes->size(); }
C DLL uint size_e(const Edges *edges) { return edges->size(); }

C DLL NodeIter* createBeginNodeIter(const Nodes *nodes) { return new NodeIter(nodes->begin()); }
C DLL NodeIter* createEndNodeIter(const Nodes *nodes) { return new NodeIter(nodes->end()); }
C DLL NodeIter* copyNodeIter(const NodeIter *iter) { return new NodeIter(*iter); }
C DLL void destroyNodeIter(NodeIter *iter) { delete iter; }
C DLL uint nodeIndex_i(const NodeIter *iter) { return iter->getIndex(); }
C DLL NodeParameters getNodeParameters_i(const NodeIter *iter) { return iter->getParameters(); }
C DLL Edges* getNeighbors(const NodeIter *iter) { return new Edges(iter->getNeighbors()); }  // allocates memory!
C DLL bool eq_n(const NodeIter *iter1, const NodeIter *iter2) { return *iter1 == *iter2; }
C DLL bool ne_n(const NodeIter *iter1, const NodeIter *iter2) { return *iter1 != *iter2; }
C DLL bool lt_n(const NodeIter *iter1, const NodeIter *iter2) { return *iter1 < *iter2; }
C DLL bool gt_n(const NodeIter *iter1, const NodeIter *iter2) { return *iter1 > *iter2; }
C DLL bool le_n(const NodeIter *iter1, const NodeIter *iter2) { return *iter1 <= *iter2; }
C DLL bool ge_n(const NodeIter *iter1, const NodeIter *iter2) { return *iter1 >= *iter2; }
C DLL void next_n(NodeIter *iter) { ++(*iter); }
C DLL void prev_n(NodeIter *iter) { --(*iter); }
C DLL void add_n(NodeIter *iter, int offset) { *iter += offset; }
C DLL void sub_n(NodeIter *iter, int offset) { *iter -= offset; }

C DLL EdgeIter* createBeginEdgeIter(const Edges *edges) { return new EdgeIter(edges->begin()); }
C DLL EdgeIter* createEndEdgeIter(const Edges *edges) { return new EdgeIter(edges->end()); }
C DLL EdgeIter* copyEdgeIter(const EdgeIter *iter) { return new EdgeIter(*iter); }
C DLL void destroyEdgeIter(EdgeIter *iter) { delete iter; }
C DLL uint edgeIndex_i(const EdgeIter *iter) { return iter->getIndex(); }
C DLL EdgeParameters getEdgeParameters_i(const EdgeIter *iter) { return iter->getParameters(); }
C DLL uint src_e(const EdgeIter *iter) { return iter->src(); }
C DLL uint dest_e(const EdgeIter *iter) { return iter->dest(); }
C DLL double getDirection(const EdgeIter *iter) { return iter->getDirection(); }
C DLL bool eq_e(const EdgeIter *iter1, const EdgeIter *iter2) { return *iter1 == *iter2; }
C DLL bool ne_e(const EdgeIter *iter1, const EdgeIter *iter2) { return *iter1 != *iter2; }
C DLL bool lt_e(const EdgeIter *iter1, const EdgeIter *iter2) { return *iter1 < *iter2; }
C DLL bool gt_e(const EdgeIter *iter1, const EdgeIter *iter2) { return *iter1 > *iter2; }
C DLL bool le_e(const EdgeIter *iter1, const EdgeIter *iter2) { return *iter1 <= *iter2; }
C DLL bool ge_e(const EdgeIter *iter1, const EdgeIter *iter2) { return *iter1 >= *iter2; }
C DLL void next_e(EdgeIter *iter) { ++(*iter); }
C DLL void prev_e(EdgeIter *iter) { --(*iter); }
C DLL void add_e(EdgeIter *iter, int offset) { *iter += offset; }
C DLL void sub_e(EdgeIter *iter, int offset) { *iter -= offset; }


// Vector Operations
C DLL Vector createVector_3(double x, double y, double z) { return Vector(x, y, z); }
C DLL Vector createVector_2(double x, double y) { return Vector(x, y); }
C DLL Vector createVector_0() { return Vector(); }
C DLL Vector cylindricalForm(double r, double theta, double z) { return Vector::cylindricalForm(r, theta, z); }
C DLL Vector polarForm(double r, double theta) { return Vector::polarForm(r, theta); }
C DLL Vector sphericalForm(double rho, double theta, double phi) { return Vector::sphericalForm(rho, theta, phi); }

C DLL double normSq(const Vector *v) { return v->normSq(); }
C DLL double norm(const Vector *v) { return v->norm(); }
C DLL double theta(const Vector *v) { return v->theta(); }
C DLL double phi(const Vector *v) { return v->phi(); }

C DLL bool eq_v(const Vector *v1, const Vector *v2) { return *v1 == *v2; }
C DLL bool ne_v(const Vector *v1, const Vector *v2) { return *v1 != *v2; }

C DLL Vector add_v(const Vector *v1, const Vector *v2) { return *v1 + *v2; }
C DLL Vector neg_v(const Vector *v) { return -*v; }
C DLL Vector sub_v(const Vector *v1, const Vector *v2) { return *v1 - *v2; }
C DLL Vector mul_v(const Vector *v, double k) { return *v * k; }

C DLL double distanceSq(const Vector *v1, const Vector *v2) { return v1->distanceSq(*v2); }
C DLL double distance(const Vector *v1, const Vector *v2) { return v1->distance(*v2); }
C DLL double dotProduct(const Vector *v1, const Vector *v2) { return v1->dotProduct(*v2); }
C DLL double angleBetween(const Vector *v1, const Vector *v2) { return v1->angleBetween(*v2); }
C DLL Vector crossProduct(const Vector *v1, const Vector *v2) { return v1->crossProduct(*v2); }

C DLL void iadd_v(Vector *v1, const Vector *v2) { *v1 += *v2; }
C DLL void isub_v(Vector *v1, const Vector *v2) { *v1 -= *v2; }
C DLL void imul_v(Vector *v1, double k) { *v1 *= k; }

C DLL void negate(Vector *v) { -*v; }
C DLL void rotate_2d(Vector *v, double theta) { v->rotate(theta); }
C DLL void rotate_3d(Vector *v, double theta, double phi) { v->rotate(theta, phi); }
C DLL void normalize(Vector *v) { v->normalize(); }

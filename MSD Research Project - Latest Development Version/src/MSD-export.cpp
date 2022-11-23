/**
 * @file MSD-export.cpp
 * @author Christopher D'Angelo
 * @brief The extern "C" version of MSD. May not contain the full functionality of MSD.h.
 * 	To be used for Python3 (ctypes) binding, as well as any future bindings that require a C (cdecl) DLL.
 * @version 1.0
 * @date 2022-11-23
 * 
 * @copyright Copyright (c) 2022
 */

#include <cstdlib>
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
C DLL void randomize(MSD *msd, bool reseed) { msd->reinitialize(reseed); }
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

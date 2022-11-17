#include <exception>
#include "boost/python.hpp"
#include "udc.h"
#include "MSD.h"

using namespace std;
using namespace udc;
using namespace boost::python;

BOOST_PYTHON_MODULE(msd) {
 // -------------------- udc.h --------------------
	scope().attr("E") = E;
	scope().attr("PI") = PI;
	
	def("sq", sq);
	def("cube", cube);

	class_<UDCException, bases<exception>>("UDCException", init<const char *>())
			.def("what", UDCException::what)
			;

 // -------------------- Vector.h --------------------
	class_<Vector>("Vector")
			.def(init<double, double, double>())
			.def(init<double, double>())
			
			.def_readwrite("x", &Vector::x)
			.def_readwrite("y", &Vector::y)
			.def_readwrite("z", &Vector::z)
			
			.def_readonly("ZERO", &Vector::ZERO)
			.def_readonly("I", &Vector::I)
			.def_readonly("J", &Vector::J)
			.def_readonly("K", &Vector::K)
			
			.def("cylindricalForm", Vector::cylindricalForm)
			.staticmethod("cylindricalForm")
			.def("polarForm", Vector::polarForm)
			.staticmethod("polarForm")
			.def("sphericalForm", Vector::sphericalForm)
			.staticmethod("sphericalForm")
			
			.def("normSq", Vector::normSq)
			.def("norm", Vector::norm)
			.def("theta", Vector::theta)
			.def("phi", Vector::phi)

			.def(self == other<const Vector &>())
			.def(self != other<const Vector &>())

			.def(self + other<const Vector &>())
			.def(-self)
			.def(self - other<const Vector &>())
			.def(self * other<double>())
			.def(self * other<const Vector &>())

			.def("distanceSq", Vector::distanceSq)
			.def("distance", Vector::distance)
			.def("dotProduct", Vector::dotProduct)
			.def("angleBetween", Vector::angleBetween)
			.def("crossProduct", Vector::crossProduct)

			.def(self += other<const Vector &>())
			.def(self -= other<const Vector &>())
			.def(self *= other<double>())

			.def("", Vector::negate)
			.def<Vector& (Vector::*)(double, double)>("rotate", Vector::rotate)
			.def<Vector& (Vector::*)(double)>("rotate", Vector::rotate)
			.def("normalize", Vector::normalize)

			.def(other<double>() * self)
			.def(str(self))
			;

 // -------------------- MSD.h --------------------
 {  // `scope molecule`
	typedef Molecule::NodeParameters PNode;
	typedef Molecule::EdgeParameters PEdge;
	scope molecule = class_<Molecule>("Molecule")
			.def_readonly("HEADER", Molecule::HEADER)
			.def_readonly("HEADER_SIZE", Molecule::HEADER_SIZE)
			.def_readonly("NOT_FOUND", Molecule::NOT_FOUND)

			.def(init<size_t>())
			.def(init<size_t, const PNode &>())

			.def("serialize", Molecule::serialize)
			.def("deserialize", Molecule::deserialize)
			.def("serializationSize", Molecule::serializationSize)
			.def("read", Molecule::read)
			.def("write", write)
			.def("load", Molecule::load)

			.def("createNode", Molecule::createNode)
			.def("nodeCount", Molecule::nodeCount)
			.def("connectNodes", Molecule::connectNodes)
			.def("edgeIndex", Molecule::edgeIndex)

			.def("getEdgeParameters", Molecule::getEdgeParameters)
			.def("setEdgeParameters", Molecule::setEdgeParameters)
			.def("getNodeParameters", Molecule::getNodeParameters)
			.def("setNodeParameters", Molecule::setNodeParameters)
			.def("setAllParameters", Molecule::setAllParameters)

			.def("setLeftLead", Molecule::setLeftLead)
			.def("setRightLead", Molecule::setRightLead)
			.def("setLeads", Molecule::setLeads)
			.def("getLeftLead", Molecule::getLeftLead)
			.def("getRightLead", Molecule::getRightLead)
			.def("getLeads", Molecule::getLeads)
			.add_property("leftLead", Molecule::getLeftLead, Molecule::setLeftLead)
			.add_property("rightLead", Molecule::getRightLead, Molecule::setRightLead)
			.add_property("leads", [](const Molecule &mol) {
				return boost::python::make_tuple(mol.getLeftLead(), mol.getRightLead());
			})
			;
	
	class_<PEdge>("EdgeParameters")
			.def_readwrite("Jm", &PEdge::Jm)
			.def_readwrite("Je1m", &PEdge::Je1m)
			.def_readwrite("Jeem", &PEdge::Jeem)
			.def_readwrite("bm", &PEdge::bm)
			.def_readwrite("Dm", &PEdge::Dm)
			;
	
	class_<PNode>("NodeParameters")
			.def_readwrite("Sm", &PNode::Sm)
			.def_readwrite("Fm", &PNode::Fm)
			.def_readwrite("Je0m", &PNode::Je0m)
			.def_readwrite("Am", &PNode::Am)
			;
	
	class_<Molecule::DeserializationException, UDCException>("DeserializationException", init<const char *>());

 }  // end `scope molecule`

 {  // `scope msd`
	typedef unsigned int uint;
	typedef unsigned long long ullong;
	scope msd = class_<MSD>("MSD", boost::python::no_init)
			.def_readonly("UP_DOWN_MODEL", &MSD::UP_DOWN_MODEL)
			.def_readonly("CONTINUOUS_SPIN_MODEL", &MSD::CONTINUOUS_SPIN_MODEL)
			
			.def_readonly("LINEAR_MOL", &MSD::LINEAR_MOL)
			.def_readonly("CIRCULAR_MOL", &MSD::CIRCULAR_MOL)
			
			.def_readwrite("record", &MSD::record)
			.def_readwrite("flippingAlgorithm", &MSD::flippingAlgorithm)

			.def(init<uint, uint, uint,  const MSD::MolProto &, uint,  uint, uint, uint, uint>())
			.def(init<uint, uint, uint,  const MSD::MolProtoFactory &, uint, uint,  uint, uint, uint, uint>())
			.def(init<uint, uint, uint,  uint, uint,  uint, uint, uint, uint>())
			.def(init<uint, uint, uint>())

			.def("getParameters", MSD::getParameters)
			.def("setParameters", MSD::setParameters)
			.def("getResults", MSD::getResults)
			// Note: no property "parameters". Avoiding accidental pattern msd.parameters.JL = 1
			// 	which would NOT change the JL of the msd since msd.parameters is only a copy. 
			.add_property("results", MSD::getResults)


			.def("set_kT", MSD::set_kT)
			.def("setB", MSD::setB)
			.add_property("kT", [](const MSD &msd) {
				return msd.getParameters().kT;
			}, MSD::set_kT)
			.add_property("B", [](const MSD &msd) {
				return msd.getParameters().B;
			}, MSD::setB)
			
			.def("getMolProto", MSD::getMolProto)
			.def("setMolProto", MSD::setMolProto)
			.def("setMolParameters", MSD::setMolParameters)
			.add_property("molProto", MSD::getMolProto, MSD::setMolProto)
			// TODO: add property "molParameters" which use a tuple: (nodeParams, edgeParams)

			.def<Vector (MSD::*)(uint) const>("getSpin", MSD::getSpin)
			.def<Vector (MSD::*)(uint, uint, uint) const>("getSpin", MSD::getSpin)
			.def<Vector (MSD::*)(uint) const>("getFlux", MSD::getFlux)
			.def<Vector (MSD::*)(uint, uint, uint) const>("getFlux", MSD::getFlux)
			.def<Vector (MSD::*)(uint) const>("getLocalM", MSD::getLocalM)
			.def<Vector (MSD::*)(uint, uint, uint) const>("getLocalM", MSD::getLocalM)
			.def<void (MSD::*)(uint, const Vector &)>("setSpin", MSD::setSpin)
			.def<void (MSD::*)(uint, uint, uint, const Vector &)>("setSpin", MSD::setSpin)
			.def<void (MSD::*)(uint, const Vector &)>("setFlux", MSD::setFlux)
			.def<void (MSD::*)(uint, uint, uint, const Vector &)>("setFlux", MSD::setFlux)
			.def<void (MSD::*)(uint, const Vector &, const Vector &)>("setLocalM", MSD::setLocalM)
			.def<void (MSD::*)(uint, uint, uint, const Vector &, const Vector &)>("setLocalM", MSD::setLocalM)
			
			.def("getN", MSD::getN)
			.def("getNL", MSD::getNL)
			.def("getNR", MSD::getNR)
			.def("getNm", MSD::getNm)
			.def("getNmL", MSD::getNmL)
			.def("getNmR", MSD::getNmR)
			.def("getNLR", MSD::getNLR)
			.def("getWidth", MSD::getWidth)
			.def("getHeight", MSD::getHeight)
			.def("getDepth", MSD::getDepth)
			.def("getDimensions", MSD::getDimensions)
			.def("getMolPosL", MSD::getMolPosL)
			.def("getMolPosR", MSD::getMolPosR)
			.def("getMolPos", MSD::getMolPos)
			.def("getTopL", MSD::getTopL)
			.def("getBottomL", MSD::getBottomL)
			.def("getFrontR", MSD::getFrontR)
			.def("getBackR", MSD::getBackR)
			.def("getInnerBounds", MSD::getInnerBounds)
			.def("getFM_L_exists", MSD::getFM_L_exists)
			.def("getFM_R_exists", MSD::getFM_R_exists)
			.def("getMol_exists", MSD::getMol_exists)
			.def("getRegions", MSD::getRegions)
			.add_property("n", MSD::getN)
			.add_property("nL", MSD::getNL)
			.add_property("nR", MSD::getNR)
			.add_property("n_m", MSD::getNm)
			.add_property("n_mL", MSD::getNmL)
			.add_property("n_mR", MSD::getNmR)
			.add_property("nLR", MSD::getNLR)
			.add_property("width", MSD::getWidth)
			.add_property("height", MSD::getHeight)
			.add_property("depth", MSD::getDepth)
			.add_property("dimensions", [](const MSD &msd) {
				uint width, height, depth;
				msd.getDimensions(width, height, depth);
				return make_tuple(width, height, depth);
			})
			.add_property("molPosL", MSD::getMolPosL)
			.add_property("molPosR", MSD::getMolPosR)
			.add_property("molPos", [](const MSD &msd) {
				uint molPosL, molPosR;
				msd.getMolPos(molPosL, molPosR);
				return boost::python::make_tuple(molPosL, molPosR);
			})
			.add_property("topL", MSD::getTopL)
			.add_property("bottomL", MSD::getBottomL)
			.add_property("frontR", MSD::getFrontR)
			.add_property("backR", MSD::getBackR)
			.add_property("innerBounds", [](const MSD &msd) {
				uint topL, bottomL, frontR, backR;
				msd.getInnerBounds(topL, bottomL, frontR, backR);
				return boost::python::make_tuple(topL, bottomL, frontR, backR);
			})
			.add_property("FM_L_exists", MSD::getFM_L_exists)
			.add_property("FM_R_exists", MSD::getFM_R_exists)
			.add_property("mol_exists", MSD::getMol_exists)
			.add_property("regions", [](const MSD &msd) {
				bool FM_L_exists, FM_R_exists, mol_exists;
				msd.getRegions(FM_L_exists, FM_R_exists, mol_exists);
				return boost::python::make_tuple(FM_L_exists, FM_R_exists, mol_exists);
			})

			.def("setSeed", MSD::setSeed)
			.def("getSeed", MSD::getSeed)
			.add_property("seed", MSD::getSeed, MSD::setSeed)

			.def("reinitialize", MSD::reinitialize)
			.def("randomize", MSD::randomize)
			.def<void (MSD::*)(ullong)>("metropolis", MSD::metropolis)
			.def<void (MSD::*)(ullong, ullong)>("metropolis", MSD::metropolis)

			.def("specificHeat", MSD::specificHeat)
			.def("specificHeat_L", MSD::specificHeat_L)
			.def("specificHeat_R", MSD::specificHeat_R)
			.def("specificHeat_m", MSD::specificHeat_m)
			.def("specificHeat_mL", MSD::specificHeat_mL)
			.def("specificHeat_mR", MSD::specificHeat_mR)
			.def("specificHeat_LR", MSD::specificHeat_LR)
			// TODO: how to name property "specificHeat"?

			.def("meanM", MSD::meanM)
			.def("meanML", MSD::meanML)
			.def("meanMR", MSD::meanMR)
			.def("meanMm", MSD::meanMm)
			.def("meanMS", MSD::meanMS)
			.def("meanMSL", MSD::meanMSL)
			.def("meanMSR", MSD::meanMSR)
			.def("meanMSm", MSD::meanMSm)
			.def("meanMF", MSD::meanMF)
			.def("meanMFL", MSD::meanMFL)
			.def("meanMFR", MSD::meanMFR)
			.def("meanMFm", MSD::meanMFm)
			.def("meanU", MSD::meanU)
			.def("meanUL", MSD::meanU)
			.def("meanUR", MSD::meanU)
			.def("meanUm", MSD::meanU)
			.def("meanUmL", MSD::meanU)
			.def("meanUmR", MSD::meanU)
			.def("meanULR", MSD::meanU)
			
			// TODO: begin, end
			;
	
	typedef MSD::Parameters P;
	class_<P>("Parameters")
		.def_readwrite("kT", &P::kT)
		.def_readwrite("B", &P::B)
		.def_readwrite("SL", &P::SL)
		.def_readwrite("SR", &P::SR)
		.def_readwrite("FL", &P::FL)
		.def_readwrite("FR", &P::FR)
		.def_readwrite("JL", &P::JL)
		.def_readwrite("JR", &P::JR)
		.def_readwrite("JmL", &P::JmL)
		.def_readwrite("JmR", &P::JmR)
		.def_readwrite("JLR", &P::JLR)
		.def_readwrite("Je0L", &P::Je0L)
		.def_readwrite("Je0R", &P::Je0R)
		.def_readwrite("Je1L", &P::Je1L)
		.def_readwrite("Je1R", &P::Je1R)
		.def_readwrite("Je1mL", &P::Je1mL)
		.def_readwrite("Je1mR", &P::Je1mR)
		.def_readwrite("Je1LR", &P::Je1LR)
		.def_readwrite("bL", &P::bL)
		.def_readwrite("bR", &P::bR)
		.def_readwrite("bmL", &P::bmL)
		.def_readwrite("bmR", &P::bmR)
		.def_readwrite("bLR", &P::bLR)
		.def_readwrite("AL", &P::AL)
		.def_readwrite("AR", &P::AR)
		.def_readwrite("DL", &P::DL)
		.def_readwrite("DR", &P::DR)
		.def_readwrite("DmL", &P::DmL)
		.def_readwrite("DmR", &P::DmR)
		.def_readwrite("DLR", &P::DLR)

		.def(self == other<const P &>())
		.def(self != other<const P &>())
		.def(str(self))
		;
	
	typedef MSD::Results R;
	class_<R>("Results")
			.def_readwrite("t", &R::t)
			.def_readwrite("M", &R::M)
			.def_readwrite("ML", &R::ML)
			.def_readwrite("MR", &R::MR)
			.def_readwrite("Mm", &R::Mm)
			.def_readwrite("MS", &R::MS)
			.def_readwrite("MSL", &R::MSL)
			.def_readwrite("MSR", &R::MSR)
			.def_readwrite("MSm", &R::MSm)
			.def_readwrite("MF", &R::MF)
			.def_readwrite("MFL", &R::MFL)
			.def_readwrite("MFR", &R::MFR)
			.def_readwrite("MFm", &R::MFm)
			.def_readwrite("U", &R::U)
			.def_readwrite("UL", &R::UL)
			.def_readwrite("UR", &R::UR)
			.def_readwrite("Um", &R::Um)
			.def_readwrite("UmL", &R::UmL)
			.def_readwrite("UmR", &R::UmR)
			.def_readwrite("ULR", &R::ULR)

			.def(self == other<const R &>())
			.def(self != other<const R &>())
			.def(str(self))
			;
	
	typedef MSD::Iterator I;
	class_<I>("Iterator", boost::python::no_init)
			.def("getIndex", &I::getIndex)
			.def("getX", &I::getX)
			.def("getY", &I::getY)
			.def("getZ", &I::getZ)

			.def("getSpin", &I::getSpin)
			.def("getFlux", &I::getFlux)
			.def("getLocalM", &I::getLocalM)

			.def(self += other<int>())
			.def(self -= other<int>())

			.def(self + other<int>())
			.def(self - other<int>())

			.def(self == other<const I &>())
			.def(self != other<const I &>())
			.def(self > other<const I &>())
			.def(self < other<const I &>())
			.def(self >= other<const I &>())
			.def(self <= other<const I &>())
			;
	
	class_<MSD::MoleculeException, bases<UDCException>>("MoleculeException", init<const char *>())
			;
 }  // end `scope msd`

}  // end BOOST_PYTHON_MODULE(msd)

int main() {


	return 0;
}
#define BOOST_PYTHON_DYNAMIC_LIB

#include <exception>
#include <boost/python.hpp>
#include "udc.h"
#include "MSD.h"

using namespace std;
using namespace udc;
using namespace boost::python;

BOOST_PYTHON_MODULE(example_msd) {
 // -------------------- udc.h --------------------
	scope().attr("E") = E;
	scope().attr("PI") = PI;
	
	def("sq", sq);
	def("cube", cube);
}  // end BOOST_PYTHON_MODULE(msd)

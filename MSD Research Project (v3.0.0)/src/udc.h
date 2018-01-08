/*
 * udc.h
 *
 *  Last Edited: Feb 25, 2014
 *       Author: Christopher D'Angelo
 */

#ifndef UDC_H
#define UDC_H

namespace udc {

/** Mathematical constant: Euler's Number */
const double E = 2.71828182845904523536;

/** Mathematical constant */
const double PI = 3.14159265358979323846;


/** square a number fast */
inline double sq(double x) {
	return x * x;
}

/** cube a number fast */
inline double cube(double x) {
	return x * x * x;
}

/** raise a number to an integer power
    quicker then std::pow in <cmath> */
double pow(double x, int n) {
	double result = 1;
	while( n < 0 ) {
		result /= x;
		n++;
	}
	while( n > 0 ) {
		result *= x;
		n--;
	}
	return result;
}

} //end of namespace

#endif
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

/**
 * Buffer/Binary Read:
 * 
 * Reads the given number of bytes from the buffer and stores it into the destination,
 * advancing the buffer pointer in the process.
 */
inline void bread(void * destination, size_t dSize, const unsigned char * &buffer) {
	memcpy(destination, buffer, dSize);
	buffer += dSize;
}

/**
 * Templated version: determines the approprate number of bytes by calling "sizeof(T)".
 */
template <typename T> inline void bread(T &destination, const unsigned char * &buffer) {
	bread(&destination, sizeof(T), buffer);
}

/**
 * Buffer/Binary Write:
 * 
 * Writes the given number of bytes from the source and stores it into the buffer,
 * advancing the buffer pointer in the process.
 */
void bwrite(const void * source, size_t sSize, unsigned char * &buffer) {
	memcpy(buffer, source, sSize);
	buffer += sSize;
}

/**
 * Templated version: determines the approprate number of bytes by calling "sizeof(T)".
 */
template <typename T> inline void bwrite(const T &source, unsigned char * &buffer) {
	bwrite(&source, sizeof(T), buffer);
}


} //end of namespace

#endif
/*
 * iterate-util.h
 * 
 * This file provides helper functions for reading and parsing the output data
 * from the iterate.cpp program.
 * 
 * Last Edited: November 3, 2020
 *      Author: Christopher D'Angelo
 */

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "Vector.h"
#include "rapidcsv/rapidcsv.h"

namespace udc {

template <typename T>
using vector3 = std::vector<std::vector<std::vector<T>>>;

std::map<std::string, vector3<udc::Vector>> read(const std::string &filename) {
	rapidcsv::Document doc(filename);

	// read and parse x,y,z (indices)
	std::vector<unsigned int> x = doc.GetColumn<unsigned int>("x");
	std::vector<unsigned int> y = doc.GetColumn<unsigned int>("y");
	std::vector<unsigned int> z = doc.GetColumn<unsigned int>("z");
	
	size_t num_rows = std::min(std::min(x.size(), y.size()), z.size());

	unsigned int width = 0, height = 0, depth = 0;
	for (size_t row = 0; row < num_rows; row++) {
		width = std::max(width, x[row] + 1);
		height = std::max(height, y[row] + 1);
		depth = std::max(depth, z[row] + 1);
	}

	// construct vector3's m (local magnetism), s (spin), and f (spin fluctuation).
	vector3<udc::Vector> m(width, std::vector<std::vector<udc::Vector>>(
			height, std::vector<udc::Vector>(depth)));
	vector3<udc::Vector> s = m;  // copy constructor
	vector3<udc::Vector> f = m;

	// read and parse m (local magnetism)
	std::vector<double> m_x = doc.GetColumn<double>("m_x");
	std::vector<double> m_y = doc.GetColumn<double>("m_y");
	std::vector<double> m_z = doc.GetColumn<double>("m_z");

	for (size_t row = 0; row < num_rows; row++)
		m[x[row]][y[row]][z[row]] = udc::Vector(m_x[row], m_y[row], m_z[row]);
	
	std::map<std::string, vector3<udc::Vector>> map;

	// read and parse s (spin)
	std::vector<double> s_x = doc.GetColumn<double>("s_x");
	std::vector<double> s_y = doc.GetColumn<double>("s_y");
	std::vector<double> s_z = doc.GetColumn<double>("s_z");

	// read and parse f (spin fluxtuation)
	std::vector<double> f_x = doc.GetColumn<double>("f_x");
	std::vector<double> f_y = doc.GetColumn<double>("f_y");
	std::vector<double> f_z = doc.GetColumn<double>("f_z");
}

}  // end namesapce udc
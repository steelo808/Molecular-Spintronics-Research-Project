#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "MSD.h"

using namespace std;
using namespace udc;

// getline, and parse using stringstream and extraction operator >>
template <typename T> T parseline(istream &in) {
	string line;
	getline(in, line);

	T value;
	istringstream(line) >> value;
	return value;
}

template <> string parseline(istream &in) {
	string line;
	getline(in, line);
	return line;
} 

template <> istringstream parseline(istream &in) {
	string line;
	getline(in, line);
	return istringstream(line);	
}

template <> void parseline(istream &in) {
	string line;
	getline(in, line);
}

// for reading JS arrays as udc::Vector objects 
istream& operator>>(istream &in, udc::Vector &v) {
	// TODO: split on commas
	return in; // TODO: stub
}

int main(int argc, char *argv[]) {
	string src_filename;
	string bin_filename = "compiled.mmb";

	// get filename for compilation
	if (argc <= 1) {
		cout << "Enter filename: ";
		getline(cin, src_filename);
	} else {
		src_filename = argv[1];
	}

	// Construct MolProto from input (src) file.
	MSD::MolProto mol;
	{	ifstream src_file(src_filename);

		// First, read nodes section
		vector<unsigned int> nodes;  // stores all node indices in the order they appear in the src file
		{	size_t node_count = parseline<size_t>(src_file);
			while (nodes.size() < node_count) {
				MSD::MolProto::NodeParameters params;
				
				istringstream line = parseline<istringstream>(src_file);
				for (string param_str; line >> param_str; ) {
					size_t idx = param_str.find("=");
					string key = param_str.substr(0, idx);
					istringstream value = istringstream(param_str.substr(idx + 1, param_str.length() - idx - 2));
					
					if (key == "Sm")
						value >> params.Sm;
					else if (key == "Fm")
						value >> params.Fm;
					else if (key == "Je0m")
						value >> params.Je0m;
					else if (key == "Am")
						value >> params.Am;
					// TODO: Add warnings! Currently, silently ignores unrecognized/unused params.
				}

				nodes.push_back(mol.createNode(params));
			}
		}
		parseline<void>(src_file);  // skip blank line (TODO: check that line is blank)

		// Second, read edges section
		vector<unsigned int> edges;  // stores all edge indices in the order they appear in the src file
		{	size_t edge_count = parseline<size_t>(src_file);
			while (edges.size() < edge_count) {
				MSD::MolProto::EdgeParameters params;
				unsigned int src_node = 0, dest_node = 0;

				istringstream line = parseline<istringstream>(src_file);
				for (string param_str; line >> param_str; ) {
					size_t idx = param_str.find("=");
					string key = param_str.substr(0, idx);
					istringstream value = istringstream(param_str.substr(idx + 1, param_str.length() - idx - 2));
					
					if (key == "Jm")
						value >> params.Jm;
					else if (key == "Je1m")
						value >> params.Je1m;
					else if (key == "Jeem")
						value >> params.Jeem;
					else if (key == "bm")
						value >> params.bm;
					else if (key == "Dm")
						value >> params.Dm;
					else if (key == "srcNode")
						value >> src_node;
					else if (key == "destNode")
						value >> dest_node;
					// TODO: Add warnings! Currently, silently ignores unrecognized/unused params.
				}

				edges.push_back(mol.connectNodes(src_node, dest_node, params));
			}
		}
		parseline<void>(src_file);  // skip blank line (TODO: check that line is blank)

		// Last, read left and right leads
		mol.setLeftLead(parseline<unsigned int>(src_file));
		mol.setRightLead(parseline<unsigned int>(src_file));

		// Note: src_file closes automatically at end of scope
	}

	// save MolProto to file
	{	size_t size = mol.serializationSize();
		unsigned char *data = new unsigned char[size];
		mol.serialize(data);

		ofstream bin_file(bin_filename, ofstream::binary);
		bin_file.write((char *) data, size);

		delete [] data;
		// Note: bin_file closes automatically at end of scope
	}

	return 0;
}

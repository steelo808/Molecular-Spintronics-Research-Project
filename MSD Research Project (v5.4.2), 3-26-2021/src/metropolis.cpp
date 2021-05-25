
/*
 * Christopher D'Angelo
 * 5-25-2021
 */

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "MSD.h"


using namespace std;
using namespace rapidxml;
using namespace udc;


enum ARG4 {
	REINITIALIZE, RANDOMIZE
};

template <typename T> ostream& operator<<(ostream &out, const vector<T> &vec) {
	if (vec.empty())
		return cout << "{ }";
	
	cout << "{ " << vec[0];
	for( int i = 1; i < vec.size(); i++ )
		cout << "  " << vec[i];
	return cout << " }";	
}

void reportTime(time_t sec) {
	time_t min = sec / 60;
	sec %= 60;
	time_t hr = min / 60;
	min %= 60;
	time_t days = hr / 24;
	hr %= 24;
	cout << '[' << days << " days, "
	     << setw(2) << hr << ':'
	     << setw(2) << min << ':'
	     << setw(2) << sec << ']';
}

void recordVar( memory_pool<> &mem, xml_node<> &data, char *type, char *name, double value) {
	ostringstream ss;
	ss << value;
	xml_node<> *var = mem.allocate_node( node_element, "var" );
	var->append_attribute( mem.allocate_attribute("type", type) );
	var->append_attribute( mem.allocate_attribute("name", name) );
	var->append_attribute( mem.allocate_attribute("value", mem.allocate_string( ss.str().c_str() )) );
	data.append_node(var);
}

struct Atom {
	unsigned int x, y, z;
	Vector spin, flux, mag;
};

struct Spin {
	unsigned int x, y, z;
	double norm;
};

struct Info {
	unsigned int width, height, depth;
	unsigned int molPosL, molPosR;
	unsigned int topL, bottomL, frontR, backR;
	vector<Spin> spins;
	unsigned long long t_eq, simCount, freq;
	MSD::FlippingAlgorithm flippingAlgorithm;
	ARG4 initMode;
	MSD::Parameters parameters;
	MSD::Results results;
	double c, cL, cR, cm, cmL, cmR, cLR;
	double x, xL, xR, xm;
	vector<Atom> atoms;
};

Info algorithm(Info info) {
	MSD msd( info.width, info.height, info.depth, info.molPosL, info.molPosR, info.topL, info.bottomL, info.frontR, info.backR );
	
	msd.setParameters( info.parameters );
	msd.flippingAlgorithm = info.flippingAlgorithm;
	
	for (const Spin &s : info.spins) {  // custom spins
		try {
			Vector vec = msd.getSpin(s.x, s.y, s.z);
			vec = Vector::sphericalForm(s.norm, vec.theta(), vec.phi());
			msd.setSpin(s.x, s.y, s.z, vec);
		} catch(out_of_range &ex) {
			cerr << "Warning: couldn't set spin [" << s.x << " " << s.y << " " << s.z << "] = " << s.norm << ":\n"
			     << "         " << ex.what() << '\n';
		}
	}

	if (info.initMode == RANDOMIZE)
		msd.randomize();
	msd.metropolis( info.t_eq, 0 );
	msd.metropolis( info.simCount, info.freq );
	
	info.results.M = msd.meanM();
	info.results.ML = msd.meanML();
	info.results.MR = msd.meanMR();
	info.results.Mm = msd.meanMm();
	
	info.results.U = msd.meanU();
	info.results.UL = msd.meanUL();
	info.results.UR = msd.meanUR();
	info.results.Um = msd.meanUm();
	info.results.UmL = msd.meanUmL();
	info.results.UmR = msd.meanUmR();
	info.results.ULR = msd.meanULR();
	
	info.c = msd.specificHeat();
	info.cL = msd.specificHeat_L();
	info.cR = msd.specificHeat_R();
	info.cm = msd.specificHeat_m();
	info.cmL = msd.specificHeat_mL();
	info.cmR = msd.specificHeat_mR();
	info.cLR = msd.specificHeat_LR();
	
	info.x = msd.magneticSusceptibility();
	info.xL = msd.magneticSusceptibility_L();
	info.xR = msd.magneticSusceptibility_R();
	info.xm = msd.magneticSusceptibility_m();

	info.atoms.clear();
	Atom atom;
	for (atom.x = 0; atom.x < msd.getWidth(); atom.x++)
		for (atom.y = 0; atom.y < msd.getHeight(); atom.y++)
			for (atom.z = 0; atom.z < msd.getDepth(); atom.z++)
				try {
					atom.spin = msd.getSpin(atom.x, atom.y, atom.z);
					atom.flux = msd.getFlux(atom.x, atom.y, atom.z);
					atom.mag = msd.getLocalM(atom.x, atom.y, atom.z);
					info.atoms.push_back(atom);
				} catch(out_of_range &ex) {
					// skip this location: no atom
				}
	
	return info;
}


int main(int argc, char *argv[]) {
	unsigned threadCount = thread::hardware_concurrency();
	threadCount = threadCount > 1 ? threadCount : 1;

	if( argc <= 1 ) {
		cout << "Need a parameters file.\n";
		return -1;
	} else if( argc <= 2 ) {
		cout << "Need an output file.\n";
		return -2;
	} else if( argc <= 3 ) {
		cout << "Need a model type.\n";
	} else if( argc <= 4 ) {
		cout << "Need an initialization mode.\n";
	} else if( argc <= 5 ) {
		cout << "Using a default number of threads: " << threadCount << '\n';
	} else {
		stringstream ss;
		ss << argv[5];
		ss >> threadCount;
		if( ss.fail() || threadCount <= 0 ) {
			cout << "Invalid number of threads: " << argv[5] << '\n';
			return -4;
		}
	}
	
	MSD::FlippingAlgorithm flippingAlgorithm;
	string s(argv[3]);
	if( s == string("CONTINUOUS_SPIN_MODEL") )
		flippingAlgorithm = MSD::CONTINUOUS_SPIN_MODEL;
	else if( s == string("UP_DOWN_MODEL") )
		flippingAlgorithm = MSD::UP_DOWN_MODEL;
	else {
		cout << "Invalid model type: " << argv[3] << '\n';
		return -3;
	}

	ARG4 initMode;
	s = string(argv[4]);
	if (s == string("REINITIALIZE"))
		initMode = REINITIALIZE;
	else if (s == string("RANDOMIZE"))
		initMode = RANDOMIZE;
	else {
		cout << "Invalid initialization mode: " << argv[4] << '\n';
		return -5;
	}

	ofstream fout;
	string filename;
	{	//prepare output file
		stringstream ss;
		ss << argv[2];
		filename = ss.str();
		fout.open( filename, ios::out | ios::trunc );
		try {
			if( fout.fail() )
				throw 1;
			else if( !(fout << "Nope, there's only trash here.\n") )
				throw 2;
		} catch(int e) {
			cout << '(' << (e |= 0x20) << ") Error using output file: " << filename << '\n';
			return e;
		}
	}
	
	map<string, vector<double>> p;  // maps parameters names to list of values
	vector<string> labelNames;  // list of labels in the order they were given in parameters file
	map<string, set<string>> labelMap;  // maps labels to set of parameter names
	map<string, string> labelMapInv;  // maps parameter names to their label. (used for XML output)
	map<string, int> iters;  // maps labels to iterators
	map<string, int> iterLengths;  // maps labels to length (i.e. exclusive max value) of each iterator
	vector<Spin> spins;  // list of custom spin magnitudes (and their positions)
	{	//initialize parameters from file
		stringstream ss;
		ss << argv[1];
		string str = ss.str();
		ifstream fin(str);
		string key;
		try {
			while( fin >> key ) {
				// comments
				if (key[0] == '#') {  // ignore comments: any key that starts with "#" is considered the start of a single-line comment
					getline(fin, str);  // storing line into "str" variable, but the data will be ignored
					// cout << "(DEBUG) Comment: " << key << str << '\n';
					continue;  // start parsing again from the begining of the next line
				}

				// custom spin magnitudes: [x y z] = s
				if (key[0] == '[') {
					Spin spin;

					// read spin.x
					if (key.length() > 1) {
						istringstream ss(key.substr(1));
						ss >> spin.x;
					} else {
						fin >> spin.x;
					}

					// read spin.y
					fin >> spin.y;
					
					// read spin.z
					fin >> str;
					int last = str.length() - 1;
					if (str[last] == ']')
						str = str.substr(0, last);
					istringstream ss(str);
					ss >> spin.z;

					// read '='
					fin >> str;
					if (str != "=")
						throw 21;
					
					// read spin.norm
					fin >> spin.norm;

					spins.push_back(spin);
					// cout << "(DEBUG) spin: [" << spin.x << " " << spin.y << " " << spin.z << "] = " << spin.norm << '\n';
					continue;  // move on to the next parameter
				}

				vector<double> vec;
				string lbl = "";
				do {
					if( !(fin >> str) )
						throw 1;
					
					if( str == "=" ) {
						double val;
						if( !(fin >> val) )
							throw 5;
						vec.push_back(val);
					} else if( str == ":" ) {
						double val, lim, inc;
						if( !(fin >> val >> lim >> inc) || inc == 0 )
							throw 2;
						lim += inc / 256; //small compinsation for floating point error
						while( (inc > 0 && val < lim) || (inc < 0 && val > lim) ) {
							vec.push_back(val);
							val += inc;
						}
					} else if( str == "{" ) {
						double val;
						while( fin >> val ) {
							vec.push_back(val);
						}
						fin.clear();
						fin >> str;
						if( str != "}" )
							throw 3;
						if( vec.size() <= 0 )
							throw 6;
					} else if (lbl.length() == 0) {  // can't give more then one label for each parameter name (i.e. key)
						lbl = str;  // str is label
					} else {
						throw 4;
					}
				} while (vec.size() <= 0);
				
				if (lbl.length() == 0)
					lbl = key;  // use key as default label if none was given

				p[key] = vec;
				labelNames.push_back(lbl);
				labelMap[lbl].insert(key);
				labelMapInv[key] = lbl;
				iters[lbl] = 0;
				auto f = iterLengths.find(lbl);
				if (f == iterLengths.end()) {
					iterLengths[lbl] = vec.size();
				} else if (f->second != vec.size()) {
					cerr << "Label (" << lbl << ") has an inconsistant size: \n";
					cerr << key << ' ' << lbl << " = " << vec << "\n";
					cerr << "List has size " << vec.size() << ", but previously had size " << f->second << '\n';
					throw 7;
				}
				
				// cout << key << " => " << vec << endl; //DEBUG
				// cout << lbl << " => " << labels.at(lbl) << endl; //DEBUG
			}
		} catch(int e) {
			cerr << '(' << (e |= 0x10) << ") Corrupted parameters file!\n";
			return e;
		}
	}

	//run simulations
	try {
		
		double completion = 0;
		double step = 100.0;
		for (const auto &x : iterLengths)
			step /= x.second;

		// cout << "step = " << step << endl; //DEBUG
		const time_t beginning = time(NULL);
		xml_document<> doc;
		xml_node<> *root = doc.allocate_node( node_element, "msd", "" );
		
		{	//add global stuff to the XML document
			xml_node<> *dec = doc.allocate_node( node_declaration );
			dec->append_attribute( doc.allocate_attribute("version", "1.0") );
			dec->append_attribute( doc.allocate_attribute("encoding", "UTF-8") );
			doc.append_node(dec);
			
			doc.append_node( doc.allocate_node(node_doctype, "", "msd SYSTEM \"msd.dtd\"") );
			
			doc.append_node(root);
			
			xml_node<> *version = doc.allocate_node( node_element, "xml_version" );
			version->append_attribute( doc.allocate_attribute("major", "1") );
			version->append_attribute( doc.allocate_attribute("minor", "6") );
			root->append_node(version);
			
			root->append_node( doc.allocate_node(node_element, "msd_version", UDC_MSD_VERSION) );
			
			xml_node<> *gen = doc.allocate_node( node_element, "gen", "" );
			gen->append_node( doc.allocate_node(node_element, "prgm", argv[0]) );
			xml_node<> *date = doc.allocate_node( node_element, "date", "" );
			ostringstream timeout;
			timeout << beginning;
			date->append_attribute( doc.allocate_attribute("timestamp", doc.allocate_string( timeout.str().c_str() )) );
			gen->append_node(date);
			root->append_node(gen);

			xml_node<> *pargs = doc.allocate_node(node_element, "pargs", "");
			xml_node<> *parg3 = doc.allocate_node(node_element, "parg", "");
			parg3->append_attribute(doc.allocate_attribute("index", "3"));
			parg3->append_attribute(doc.allocate_attribute("name", "flippingAlgorithm"));
			parg3->append_attribute(doc.allocate_attribute("value", argv[3]));
			pargs->append_node(parg3);
			xml_node<> *parg4 = doc.allocate_node(node_element, "parg", "");
			parg4->append_attribute(doc.allocate_attribute("index", "4"));
			parg4->append_attribute(doc.allocate_attribute("name", "initMode"));
			parg4->append_attribute(doc.allocate_attribute("value", argv[4]));
			pargs->append_node(parg4);
			root->append_node(pargs);
			
			xml_node<> *global = doc.allocate_node( node_element, "global", "" );
			recordVar( doc, *global, "param", "width", p.at("width")[0] );
			recordVar( doc, *global, "param", "height", p.at("height")[0] );
			recordVar( doc, *global, "param", "depth", p.at("depth")[0] );
			recordVar( doc, *global, "param", "molPosL", p.at("molPosL")[0] );
			recordVar( doc, *global, "param", "molPosR", p.at("molPosR")[0] );
			recordVar( doc, *global, "param", "topL", p.at("topL")[0] );
			recordVar( doc, *global, "param", "bottomL", p.at("bottomL")[0] );
			recordVar( doc, *global, "param", "frontR", p.at("frontR")[0] );
			recordVar( doc, *global, "param", "backR", p.at("backR")[0] );
			recordVar( doc, *global, "param", "t_eq", p.at("t_eq")[0] );
			recordVar( doc, *global, "param", "simCount", p.at("simCount")[0] );
			recordVar( doc, *global, "param", "freq", p.at("freq")[0] );
			const unsigned int SIZE = 46;
			string inds[SIZE] = { "kT", "B_x", "B_y", "B_z",  // 4
			                      "sL", "sR", "sm", "FL", "FR", "Fm",  // 6
			                      "JL", "JmL", "Jm", "JmR", "JR", "JLR",  // 6
								  "Je0L", "Je0m", "Je0R",  // 3
								  "Je1L", "Je1mL", "Je1m", "Je1mR", "Je1R", "Je1LR",  // 6
								  "JeeL", "JeemL", "Jeem", "JeemR", "JeeR", "JeeLR",  // 6
			                      "bL", "bmL", "bm", "bmR", "bR", "bLR",  // 6
								  "AL_x", "AL_y", "AL_z",  // 3
			                      "AR_x", "AR_y", "AR_z",  // 3
			                      "Am_x", "Am_y", "Am_z" };  // 3
			for( unsigned int i = 0; i < SIZE; i++ ) {
				xml_node<> *ind = doc.allocate_node( node_element, "ind", "" );
				ind->append_attribute( doc.allocate_attribute("name", doc.allocate_string( inds[i].c_str() )) );
				auto label = labelMapInv.find(inds[i]);
				if (label != labelMapInv.end() && label->second != inds[i])
					ind->append_attribute( doc.allocate_attribute("label", doc.allocate_string( label->second.c_str() )) );
				for( auto j = p.at(inds[i]).begin(); j != p.at(inds[i]).end(); j++ ) {
					ostringstream oss;
					oss << *j;
					ind->append_node( doc.allocate_node(node_element, "val", doc.allocate_string( oss.str().c_str() )) );
				}
				global->append_node(ind);
			}

			// record custom spins
			xml_node<> *spins_node = doc.allocate_node( node_element, "spins", "" );
			for (const Spin &s : spins) {
				xml_node<> *spin_node = doc.allocate_node( node_element, "spin", "" );
				spin_node->append_attribute( doc.allocate_attribute("x", doc.allocate_string( to_string(s.x).c_str() )) );
				spin_node->append_attribute( doc.allocate_attribute("y", doc.allocate_string( to_string(s.y).c_str() )) );
				spin_node->append_attribute( doc.allocate_attribute("z", doc.allocate_string( to_string(s.z).c_str() )) );
				spin_node->append_attribute( doc.allocate_attribute("norm", doc.allocate_string( to_string(s.norm).c_str() )) );
				spins_node->append_node(spin_node);
			}
			global->append_node(spins_node);

			root->append_node(global);
		}
		
		// output XML skeleton (version, global parameters, etc...)
		fout.close();
		fout.open( filename, ios::out | ios::trunc );
		fout << doc << flush;
		
		//report starting status
		cout << completion << "% ";
		reportTime( time(NULL) - beginning );
		if( fout.fail() ) {
			cout << "\n\t- Unusual Error... Couldn't write to designated output file: " << filename;
			fout.clear();
		}
		cout << endl;
		
		//set up (pseudo) thread pool
		unique_ptr< future<Info>[] > threadPool = NULL;
		if( threadCount > 1 )
			threadPool = unique_ptr< future<Info>[] >( new future<Info>[threadCount] );
		unsigned int nextIndex = 0;
		Info postInfo;
		
		//define a lambda function
		auto recordData = [&](const Info &info) {
			ostringstream timeout;
			timeout << time(NULL);
			
			xml_node<> *data = doc.allocate_node( node_element, "data", "" );
												
			xml_node<> *date = doc.allocate_node( node_element, "date", "" );
			date->append_attribute( doc.allocate_attribute("timestamp", doc.allocate_string( timeout.str().c_str() )) );
			data->append_node(date);

			//record parameters
			recordVar( doc, *data, "param", "kT", info.parameters.kT );
			recordVar( doc, *data, "param", "B_x", info.parameters.B.x );
			recordVar( doc, *data, "param", "B_y", info.parameters.B.y );
			recordVar( doc, *data, "param", "B_z", info.parameters.B.z );
			recordVar( doc, *data, "param", "sL", info.parameters.sL );
			recordVar( doc, *data, "param", "sR", info.parameters.sR );
			recordVar( doc, *data, "param", "sm", info.parameters.sm );
			recordVar( doc, *data, "param", "FL", info.parameters.FL );
			recordVar( doc, *data, "param", "FR", info.parameters.FR );
			recordVar( doc, *data, "param", "Fm", info.parameters.Fm );
			recordVar( doc, *data, "param", "JL", info.parameters.JL );
			recordVar( doc, *data, "param", "JR", info.parameters.JR );
			recordVar( doc, *data, "param", "Jm", info.parameters.Jm );
			recordVar( doc, *data, "param", "JmL", info.parameters.JmL );
			recordVar( doc, *data, "param", "JmR", info.parameters.JmR );
			recordVar( doc, *data, "param", "JLR", info.parameters.JLR );
			recordVar( doc, *data, "param", "Je0L", info.parameters.Je0L );
			recordVar( doc, *data, "param", "Je0R", info.parameters.Je0R );
			recordVar( doc, *data, "param", "Je0m", info.parameters.Je0m );
			recordVar( doc, *data, "param", "Je1L", info.parameters.Je1L );
			recordVar( doc, *data, "param", "Je1R", info.parameters.Je1R );
			recordVar( doc, *data, "param", "Je1m", info.parameters.Je1m );
			recordVar( doc, *data, "param", "Je1mL", info.parameters.Je1mL );
			recordVar( doc, *data, "param", "Je1mR", info.parameters.Je1mR );
			recordVar( doc, *data, "param", "Je1LR", info.parameters.Je1LR );
			recordVar( doc, *data, "param", "JeeL", info.parameters.JeeL );
			recordVar( doc, *data, "param", "JeeR", info.parameters.JeeR );
			recordVar( doc, *data, "param", "Jeem", info.parameters.Jeem );
			recordVar( doc, *data, "param", "JeemL", info.parameters.JeemL );
			recordVar( doc, *data, "param", "JeemR", info.parameters.JeemR );
			recordVar( doc, *data, "param", "JeeLR", info.parameters.JeeLR );
			recordVar( doc, *data, "param", "bL", info.parameters.bL );
			recordVar( doc, *data, "param", "bR", info.parameters.bR );
			recordVar( doc, *data, "param", "bm", info.parameters.bm );
			recordVar( doc, *data, "param", "bmL", info.parameters.bmL );
			recordVar( doc, *data, "param", "bmR", info.parameters.bmR );
			recordVar( doc, *data, "param", "bLR", info.parameters.bLR );
			recordVar( doc, *data, "param", "AL_x", info.parameters.AL.x );
			recordVar( doc, *data, "param", "AL_y", info.parameters.AL.y );
			recordVar( doc, *data, "param", "AL_z", info.parameters.AL.z );
			recordVar( doc, *data, "param", "AR_x", info.parameters.AR.x );
			recordVar( doc, *data, "param", "AR_y", info.parameters.AR.y );
			recordVar( doc, *data, "param", "AR_z", info.parameters.AR.z );
			recordVar( doc, *data, "param", "Am_x", info.parameters.Am.x );
			recordVar( doc, *data, "param", "Am_y", info.parameters.Am.y );
			recordVar( doc, *data, "param", "Am_z", info.parameters.Am.z );
			
			//record results
			recordVar( doc, *data, "result", "M_x", info.results.M.x );
			recordVar( doc, *data, "result", "M_y", info.results.M.y );
			recordVar( doc, *data, "result", "M_z", info.results.M.z );
			
			recordVar( doc, *data, "result", "ML_x", info.results.ML.x );
			recordVar( doc, *data, "result", "ML_y", info.results.ML.y );
			recordVar( doc, *data, "result", "ML_z", info.results.ML.z );
			
			recordVar( doc, *data, "result", "MR_x", info.results.MR.x );
			recordVar( doc, *data, "result", "MR_y", info.results.MR.y );
			recordVar( doc, *data, "result", "MR_z", info.results.MR.z );
			
			recordVar( doc, *data, "result", "Mm_x", info.results.Mm.x );
			recordVar( doc, *data, "result", "Mm_y", info.results.Mm.y );
			recordVar( doc, *data, "result", "Mm_z", info.results.Mm.z );
			
			recordVar( doc, *data, "result", "U", info.results.U );
			recordVar( doc, *data, "result", "UL", info.results.UL );
			recordVar( doc, *data, "result", "UR", info.results.UR );
			recordVar( doc, *data, "result", "Um", info.results.Um );
			recordVar( doc, *data, "result", "UmL", info.results.UmL );
			recordVar( doc, *data, "result", "UmR", info.results.UmR );
			recordVar( doc, *data, "result", "ULR", info.results.ULR );
			
			recordVar( doc, *data, "result", "c", info.c );
			recordVar( doc, *data, "result", "cL", info.cL );
			recordVar( doc, *data, "result", "cR", info.cR );
			recordVar( doc, *data, "result", "cm", info.cm );
			recordVar( doc, *data, "result", "cmL", info.cmL );
			recordVar( doc, *data, "result", "cmR", info.cmR );
			recordVar( doc, *data, "result", "cLR", info.cLR );
			
			recordVar( doc, *data, "result", "x", info.x );
			recordVar( doc, *data, "result", "xL", info.xL );
			recordVar( doc, *data, "result", "xR", info.xR );
			recordVar( doc, *data, "result", "xm", info.xm );
			
			// record atoms
			xml_node<> *snapshot = doc.allocate_node( node_element, "snapshot", "" );
			for (const Atom &atom : info.atoms) {
				xml_node<> *atom_node = doc.allocate_node( node_element, "loc", "" );
				atom_node->append_attribute( doc.allocate_attribute("x", doc.allocate_string( to_string(atom.x).c_str() )) );
				atom_node->append_attribute( doc.allocate_attribute("y", doc.allocate_string( to_string(atom.y).c_str() )) );
				atom_node->append_attribute( doc.allocate_attribute("z", doc.allocate_string( to_string(atom.z).c_str() )) );
				atom_node->append_attribute( doc.allocate_attribute("sx", doc.allocate_string( to_string(atom.spin.x).c_str() )) );
				atom_node->append_attribute( doc.allocate_attribute("sy", doc.allocate_string( to_string(atom.spin.y).c_str() )) );
				atom_node->append_attribute( doc.allocate_attribute("sz", doc.allocate_string( to_string(atom.spin.z).c_str() )) );
				atom_node->append_attribute( doc.allocate_attribute("fx", doc.allocate_string( to_string(atom.flux.x).c_str() )) );
				atom_node->append_attribute( doc.allocate_attribute("fy", doc.allocate_string( to_string(atom.flux.y).c_str() )) );
				atom_node->append_attribute( doc.allocate_attribute("fz", doc.allocate_string( to_string(atom.flux.z).c_str() )) );
				atom_node->append_attribute( doc.allocate_attribute("mx", doc.allocate_string( to_string(atom.mag.x).c_str() )) );
				atom_node->append_attribute( doc.allocate_attribute("my", doc.allocate_string( to_string(atom.mag.y).c_str() )) );
				atom_node->append_attribute( doc.allocate_attribute("mz", doc.allocate_string( to_string(atom.mag.z).c_str() )) );
				snapshot->append_node(atom_node);
			}
			data->append_node(snapshot);

			root->append_node(data);
			
			fout.close();
			fout.open( filename, ios::out | ios::trunc );
			fout << doc << flush;
			
			//report status
			cout << (completion += step) << "% ";
			reportTime( time(NULL) - beginning );
			if( fout.fail() ) {
				cout << "\n\t- Unusual Error... Couldn't write to designated output file: " << filename;
				fout.clear();
			}
			cout << endl;
		};
		
		//start iterations
		cout << fixed << setprecision(2) << setfill('0');

		bool hasNextIter = true;
		auto nextIter = [&]() {
			static Info preInfo;
			static map<string, double*> fields = [](Info &preInfo) {
				// initialize fields map in a static block because it only needs to run once, then it can be kept in memory
				map<string, double*> fields;  // map of parameter names to pointers to Info::parameters fields
				
				fields["kT"] = &preInfo.parameters.kT;
				
				fields["B_x"] = &preInfo.parameters.B.x;
				fields["B_y"] = &preInfo.parameters.B.y;
				fields["B_z"] = &preInfo.parameters.B.z;
				
				fields["sL"] = &preInfo.parameters.sL;
				fields["sR"] = &preInfo.parameters.sR;
				fields["sm"] = &preInfo.parameters.sm ;
				fields["FL"] = &preInfo.parameters.FL;
				fields["FR"] = &preInfo.parameters.FR;
				fields["Fm"] = &preInfo.parameters.Fm;
				
				fields["JL"] = &preInfo.parameters.JL;
				fields["JR"] = &preInfo.parameters.JR;
				fields["Jm"] = &preInfo.parameters.Jm;
				fields["JmL"] = &preInfo.parameters.JmL;
				fields["JmR"] = &preInfo.parameters.JmR;
				fields["JLR"] = &preInfo.parameters.JLR;

				fields["Je0L"] = &preInfo.parameters.Je0L;
				fields["Je0R"] = &preInfo.parameters.Je0R;
				fields["Je0m"] = &preInfo.parameters.Je0m;

				fields["Je1L"] = &preInfo.parameters.Je1L;
				fields["Je1R"] = &preInfo.parameters.Je1R;
				fields["Je1m"] = &preInfo.parameters.Je1m ;
				fields["Je1mL"] = &preInfo.parameters.Je1mL;
				fields["Je1mR"] = &preInfo.parameters.Je1mR;
				fields["Je1LR"] = &preInfo.parameters.Je1LR;

				fields["JeeL"] = &preInfo.parameters.JeeL;
				fields["JeeR"] = &preInfo.parameters.JeeR;
				fields["Jeem"] = &preInfo.parameters.Jeem;
				fields["JeemL"] = &preInfo.parameters.JeemL;
				fields["JeemR"] = &preInfo.parameters.JeemR;
				fields["JeeLR"] = &preInfo.parameters.JeeLR;
				
				fields["bL"] = &preInfo.parameters.bL;
				fields["bR"] = &preInfo.parameters.bR;
				fields["bm"] = &preInfo.parameters.bm;
				fields["bmL"] = &preInfo.parameters.bmL;
				fields["bmR"] = &preInfo.parameters.bmR;
				fields["bLR"] = &preInfo.parameters.bLR;
				
				fields["AL_x"] = &preInfo.parameters.AL.x;
				fields["AL_y"] = &preInfo.parameters.AL.y;
				fields["AL_z"] = &preInfo.parameters.AL.z;
				
				fields["AR_x"] = &preInfo.parameters.AR.x;
				fields["AR_y"] = &preInfo.parameters.AR.y;
				fields["AR_z"] = &preInfo.parameters.AR.z;
				
				fields["Am_x"] = &preInfo.parameters.Am.x;
				fields["Am_y"] = &preInfo.parameters.Am.y;
				fields["Am_z"] = &preInfo.parameters.Am.z;
				
				return fields;
			}(preInfo);

			// set constant Info fields
			preInfo.flippingAlgorithm = flippingAlgorithm;
			preInfo.initMode = initMode;

			preInfo.width = p.at("width")[0];
			preInfo.height = p.at("height")[0];
			preInfo.depth = p.at("depth")[0];
			preInfo.molPosL = p.at("molPosL")[0];
			preInfo.molPosR = p.at("molPosR")[0];
			preInfo.topL = p.at("topL")[0];
			preInfo.bottomL = p.at("bottomL")[0];
			preInfo.frontR = p.at("frontR")[0];
			preInfo.backR = p.at("backR")[0];
			preInfo.t_eq = p.at("t_eq")[0];
			preInfo.simCount = p.at("simCount")[0];
			preInfo.freq = p.at("freq")[0];

			preInfo.spins = spins;

			// set Info::parameters
			for (auto iIters = iters.begin(); iIters != iters.end(); ++iIters) {
				auto params = labelMap.at(iIters->first);  // set of paramerts associated with this label
				for (auto iParams = params.begin(); iParams != params.end(); ++iParams)
					try {
						*fields.at(*iParams) = p.at(*iParams).at(iIters->second);
					
					} catch(out_of_range &ex) {
						// Field is a constant, e.g. "width", "height", "t_eq", "freq", etc.
						// it will not be mapped in "fields". This is okay. Just skip it.
						// These fields have already been set above.
					}
			}

			// iterate "iters" forward one space
			auto iLabelNames = labelNames.begin();
			while (iLabelNames != labelNames.end() && ++iters[*iLabelNames] >= iterLengths[*iLabelNames])
				iters[*iLabelNames++] = 0;
			hasNextIter = iLabelNames != labelNames.end();

			return preInfo;
		};

		while (hasNextIter) {
			
			Info preInfo = nextIter();
			bool validResult = true;
			
			if( threadCount > 1 ) {
				//get result from which ever simulation finishes next
				while(true) {
					if( !threadPool[nextIndex].valid() ) {
						validResult = false;
						break;
					}
					if( threadPool[nextIndex].wait_for(chrono::milliseconds(10)) == future_status::ready ) {
						postInfo = threadPool[nextIndex].get();
						break;
					}
					nextIndex = (nextIndex + 1) % threadCount;
				}
			
				//run next metropolis algorithm (in threadPool)
				threadPool[nextIndex] = async( launch::async, algorithm, preInfo );
			} else {
				//mono-threaded
				postInfo = algorithm(preInfo);
			}
			
			//record data...
			if( validResult )
				recordData(postInfo);
			
		} //end of iterations
		
		//clean up thread pool (wait for the last few simulations)
		if( threadCount > 1 ) {
			// nextIndex = 0;
			unsigned openThreads = 0;
			while( openThreads < threadCount ) {
				if( nextIndex == 0 )
					openThreads = 0;
				if( threadPool[nextIndex].valid() ) {
					if( threadPool[nextIndex].wait_for(chrono::milliseconds(10)) == future_status::ready ) {
						postInfo = threadPool[nextIndex].get();
						recordData(postInfo);
						openThreads++;
					}
				} else {
					openThreads++;
				}
				nextIndex = (nextIndex + 1) % threadCount;
			}
		}

	} catch(out_of_range &e) {
		cerr << "Parameter file is missing some data!\n";
		return 0x18;
	}

	return 0;
}


/*
 * Christopher D'Angelo
 * 1-25-2020
 */

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "MSD.h"


using namespace std;
using namespace rapidxml;
using namespace udc;


/* //DEBUG
template <typename T> ostream& operator<<(ostream &out, const vector<T> &vec) {
	cout << '[' << vec[0];
	for( int i = 1; i < vec.size(); i++ )
		cout << ", " << vec[i];
	return cout << ']';
}
*/

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


struct Info {
	unsigned int width, height, depth;
	unsigned int molPosL, molPosR;
	unsigned int topL, bottomL, frontR, backR;
	unsigned long long t_eq, simCount, freq;
	MSD::FlippingAlgorithm flippingAlgorithm;
	MSD::Parameters parameters;
	MSD::Results results;
	double c, cL, cR, cm, cmL, cmR, cLR;
	double x, xL, xR, xm;
};

Info algorithm(Info info) {
	MSD msd( info.width, info.height, info.depth, info.molPosL, info.molPosR,
			info.topL, info.bottomL, info.frontR, info.backR );
	
	msd.setParameters( info.parameters );
	msd.flippingAlgorithm = info.flippingAlgorithm;
	
	// msd.randomize();
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
		cout << "Using a default number of threads: " << threadCount << '\n';
	} else {
		stringstream ss;
		ss << argv[4];
		ss >> threadCount;
		if( ss.fail() || threadCount <= 0 ) {
			cout << "Invalid number of threads: " << argv[4] << '\n';
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
	
	map<string, vector<double>> p;
	{	//initialize parameters from file
		stringstream ss;
		ss << argv[1];
		string str = ss.str();
		ifstream fin(str);
		string key;
		try {
			while( fin >> key ) {
				vector<double> vec;
				double val;
				if( !(fin >> str >> val) )
					throw 1;
				if( str == "=" )
					vec.push_back(val);
				else if( str == ":" ) {
					double lim, inc;
					if( !(fin >> lim >> inc) || inc == 0 )
						throw 2;
					lim += inc / 256; //small compinsation for floating point error
					while( (inc > 0 && val < lim) || (inc < 0 && val > lim) ) {
						vec.push_back(val);
						val += inc;
					}
				} else if( str == "{" ) {
					do {
						vec.push_back(val);
					} while( fin >> val );
					fin.clear();
					fin >> str;
					if( str != "}" )
						throw 3;
				} else
					throw 4;
				p[key] = vec;
				// cout << key << " => " << vec[0] << endl; //DEBUG
			}
		} catch(int e) {
			cerr << '(' << (e |= 0x10) << ") Corrupted parameters file!\n";
			return e;
		}
	}

	//run simulations
	try {
		
		double completion = 0;
		const double step = 100.0 / ( p.at("kT").size() * p.at("B_x").size() * p.at("B_y").size() * p.at("B_z").size()
		                            * p.at("JL").size() * p.at("JR").size() * p.at("Jm").size() * p.at("JmL").size() * p.at("JmR").size() * p.at("JLR").size()
		                            * p.at("bL").size() * p.at("bR").size() * p.at("bm").size() * p.at("bmL").size() * p.at("bmR").size() * p.at("bLR").size()
									* p.at("AL").size() * p.at("AR").size() * p.at("Am").size() );
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
			
			xml_node<> *version = doc.allocate_node( node_element, "version" );
			version->append_attribute( doc.allocate_attribute("major", "1") );
			version->append_attribute( doc.allocate_attribute("minor", "4") );
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
			const unsigned int SIZE = 19;
			string inds[SIZE] = { "kT", "B_x", "B_y", "B_z",
			                      "JL", "JmL", "Jm", "JmR", "JR", "JLR",
			                      "bL", "bmL", "bm", "bmR", "bR", "bLR",
								  "AL", "AR", "Am" };
			for( unsigned int i = 0; i < SIZE; i++ ) {
				xml_node<> *ind = doc.allocate_node( node_element, "ind", "" );
				ind->append_attribute( doc.allocate_attribute("name", doc.allocate_string( inds[i].c_str() )) );
				for( auto j = p.at(inds[i]).begin(); j != p.at(inds[i]).end(); j++ ) {
					ostringstream oss;
					oss << *j;
					ind->append_node( doc.allocate_node(node_element, "val", doc.allocate_string( oss.str().c_str() )) );
				}
				global->append_node(ind);
			}
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
			recordVar( doc, *data, "param", "JL", info.parameters.JL );
			recordVar( doc, *data, "param", "JR", info.parameters.JR );
			recordVar( doc, *data, "param", "Jm", info.parameters.Jm );
			recordVar( doc, *data, "param", "JmL", info.parameters.JmL );
			recordVar( doc, *data, "param", "JmR", info.parameters.JmR );
			recordVar( doc, *data, "param", "JLR", info.parameters.JLR );
			recordVar( doc, *data, "param", "bL", info.parameters.bL );
			recordVar( doc, *data, "param", "bR", info.parameters.bR );
			recordVar( doc, *data, "param", "bm", info.parameters.bm );
			recordVar( doc, *data, "param", "bmL", info.parameters.bmL );
			recordVar( doc, *data, "param", "bmR", info.parameters.bmR );
			recordVar( doc, *data, "param", "bLR", info.parameters.bLR );
			recordVar( doc, *data, "param", "AL", info.parameters.AL );
			recordVar( doc, *data, "param", "AR", info.parameters.AR );
			recordVar( doc, *data, "param", "Am", info.parameters.Am );
			
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

		for( auto iT = p.at("kT").begin(); iT != p.at("kT").end(); iT++ ) {
		for( auto iB_x = p.at("B_x").begin(); iB_x != p.at("B_x").end(); iB_x++ ) {
		for( auto iB_y = p.at("B_y").begin(); iB_y != p.at("B_y").end(); iB_y++ ) {
		for( auto iB_z = p.at("B_z").begin(); iB_z != p.at("B_z").end(); iB_z++ ) {
		
		for( auto iJL = p.at("JL").begin(); iJL != p.at("JL").end(); iJL++ ) {
		for( auto iJR = p.at("JR").begin(); iJR != p.at("JR").end(); iJR++ ) {
		for( auto iJm = p.at("Jm").begin(); iJm != p.at("Jm").end(); iJm++ ) {
		for( auto iJmL = p.at("JmL").begin(); iJmL != p.at("JmL").end(); iJmL++ ) {
		for( auto iJmR = p.at("JmR").begin(); iJmR != p.at("JmR").end(); iJmR++ ) {
		for( auto iJLR = p.at("JLR").begin(); iJLR != p.at("JLR").end(); iJLR++ ) {
		
		for( auto ibL = p.at("bL").begin(); ibL != p.at("bL").end(); ibL++ ) {
		for( auto ibR = p.at("bR").begin(); ibR != p.at("bR").end(); ibR++ ) {
		for( auto ibm = p.at("bm").begin(); ibm != p.at("bm").end(); ibm++ ) {
		for( auto ibmL = p.at("bmL").begin(); ibmL != p.at("bmL").end(); ibmL++ ) {
		for( auto ibmR = p.at("bmR").begin(); ibmR != p.at("bmR").end(); ibmR++ ) {
		for( auto ibLR = p.at("bLR").begin(); ibLR != p.at("bLR").end(); ibLR++ ) {
		
		for( auto iAL = p.at("AL").begin(); iAL != p.at("AL").end(); iAL++ ) {
		for( auto iAR = p.at("AR").begin(); iAR != p.at("AR").end(); iAR++ ) {
		for( auto iAm = p.at("Am").begin(); iAm != p.at("Am").end(); iAm++ ) {

			bool validResult = true;
			Info preInfo;
			
			preInfo.flippingAlgorithm = flippingAlgorithm;
			
			// debug
			preInfo.width = p.at("width")[0];
			preInfo.height = p.at("height")[0];
			preInfo.depth = p.at("depth")[0];
			preInfo.molPosL = p.at("molPosL")[0];
			preInfo.molPosR = p.at("molPosR")[0];

			// TODO: Why are these lines causing an error?!!!!!!!!!!!!!!!!! WTF!
			cout << (p.find("topL") != p.end()) << '\n';  // WTF!
			preInfo.topL = 0; // preInfo.topL = p.at("topL")[0];
			preInfo.bottomL = preInfo.height - 1; // preInfo.bottomL = p.at("bottomL")[0];
			preInfo.frontR = 0; // preInfo.frontR = p.at("frontR")[0];
			preInfo.backR = preInfo.depth - 1; // preInfo.backR = p.at("backR")[0];
			
			preInfo.t_eq = p.at("t_eq")[0];
			preInfo.simCount = p.at("simCount")[0];
			preInfo.freq = p.at("freq")[0];
			
			preInfo.parameters.kT = *iT;
			preInfo.parameters.B.x = *iB_x;
			preInfo.parameters.B.y = *iB_y;
			preInfo.parameters.B.z = *iB_z;
			
			preInfo.parameters.JL = *iJL;
			preInfo.parameters.JR = *iJR;
			preInfo.parameters.Jm = *iJm;
			preInfo.parameters.JmL = *iJmL;
			preInfo.parameters.JmR = *iJmR;
			preInfo.parameters.JLR = *iJLR;
			
			preInfo.parameters.bL = *ibL;
			preInfo.parameters.bR = *ibR;
			preInfo.parameters.bm = *ibm;
			preInfo.parameters.bmL = *ibmL;
			preInfo.parameters.bmR = *ibmR;
			preInfo.parameters.bLR = *ibLR;
			
			preInfo.parameters.AL = *iAL;
			preInfo.parameters.AR = *iAR;
			preInfo.parameters.Am = *iAm;
			
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
			
		}}}}}}}}}}}}}}}}}}} //end of iterations
		
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

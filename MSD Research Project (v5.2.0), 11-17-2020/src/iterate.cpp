
/*
 * Christopher D'Angelo
 * 11-1-2020
 */

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <limits>
#include "MSD.h"

using namespace std;
using namespace udc;


void trim(string &str) {
	static const string WHITESPACE =" \n\t\r\v\f";
	str.erase(0, str.find_first_not_of(WHITESPACE));
	str.erase(str.find_last_not_of(WHITESPACE) + 1);
}

template <typename T> void ask(string msg, T &val) {
	cout << msg;
	cin >> val;
}

void ask(string msg, Vector &vec) {
	cout << msg;
	cin >> vec.x >> vec.y >> vec.z;
}

template <typename T> void getParam(map<string, string> &params, string name, T &val) {
	string trimmedName = name;
	trim(trimmedName);
	auto iter = params.find(trimmedName);
	if (iter != params.end()) {
		istringstream ss(iter->second);
		ss >> val;
		params.erase(iter);
	} else {
		ask(string("> ") + name + string(" = "), val);
	}
}

void getParam(map<string, string> &params, string name, Vector &vec) {
	string trimmedName = name;
	trim(trimmedName);
	auto iter = params.find(trimmedName);
	if (iter != params.end()) {
		istringstream ss(iter->second);
		ss >> vec.x >> vec.y >> vec.z;
		params.erase(iter);
	} else {
		ask(string("> ") + name + string(" = "), vec);
	}
}

struct Spin {
	int x, y, z;
	double norm;
};


int main(int argc, char *argv[]) {
	//get command line argument
	if( argc > 1 ) {
		ifstream file(argv[1]);
		if( file.good() ) {
			char ans;
			cout << "File \"" << argv[1] << "\" already exists. Overwrite it (Y/N)? ";
			cin >> ans;
			cin.sync();
			if( ans != 'Y' && ans != 'y' ) {
				cout << "Terminated early.\n";
				return 0;
			}
		}
	} else {
		cout << "Supply an output file as an argument.\n";
		return 1;
	}
	
	MSD::FlippingAlgorithm arg2 = MSD::CONTINUOUS_SPIN_MODEL;
	if( argc > 2 ) {
		string s(argv[2]);
		if( s == string("CONTINUOUS_SPIN_MODEL") )
			arg2 = MSD::CONTINUOUS_SPIN_MODEL;
		else if( s == string("UP_DOWN_MODEL") )
			arg2 = MSD::UP_DOWN_MODEL;
		else
			cout << "Unrecognized third argument! Defaulting to 'CONTINUOUS_SPIN_MODEL'.\n";
	} else
		cout << "Defaulting to 'CONTINUOUS_SPIN_MODEL'.\n";

	map<string, string> params;
	vector<Spin> spins;
	if (argc > 5) {
		ifstream paramsFile;
		paramsFile.exceptions( ios::badbit | ios::failbit );
		try {
			paramsFile.open(argv[5]);
		} catch(const ios::failure &e) {
			cerr << "Error opening input file \"" << argv[5] << "\": " << e.what() << '\n';
			return 5;
		}

		try {
			while (true) {  // loop exits when exception is thrown
				int c = paramsFile.peek();
				if (c == EOF)  // end of file
					break;
				if (c == '#') {  // line is a comment
					paramsFile.ignore(numeric_limits<streamsize>::max(), '\n');
					continue;
				}
				if (c == '\n' || c == '\r') {  // blank line
					paramsFile.ignore(2, '\n');
					continue;
				}

				string key, value;
				getline(paramsFile, key, '=');
				getline(paramsFile, value);
				trim(key);
				trim(value);

				if (key[0] != '[') {
					params[key] = value;
					cout << key << " = " << value << '\n';
				} else {
					Spin s;
					{	istringstream ss(key.substr(1, key.length() - 2));
						ss >> s.x >> s.y >> s.z;
					}
					{	istringstream ss(value);
						ss >> s.norm;
					}
					spins.push_back(s);
				}
			}
		} catch(const ios::failure &e) {
			if (!paramsFile.eof()) {
				cerr << "Error occured while reading from input file \"" << argv[5] << "\": " << e.what() << '\n';
				return 5;
			}
		}
	}

	ofstream file;
	file.exceptions( ios::badbit | ios::failbit );
	try {
		file.open(argv[1]);
	} catch(const ios::failure &e) {
		cerr << "Couldn't open output file \"" << argv[1] << "\" for writing: " << e.what() << '\n';
		return 4;
	}
	
	//get parameters
	unsigned int width, height, depth, molPosL, molPosR, topL, bottomL, frontR, backR;
	unsigned long long simCount, freq;
	MSD::Parameters p;
	
	cin.exceptions( ios::badbit | ios::failbit | ios::eofbit );
	try {
		getParam(params, "width ", width);
		getParam(params, "height", height);
		getParam(params, "depth ", depth);
		cout << '\n';
		getParam(params, "molPosL", molPosL);
		getParam(params, "molPosR", molPosR);
		cout << '\n';
		getParam(params, "topL   ", topL);
		getParam(params, "bottomL", bottomL);
		getParam(params, "frontR ", frontR);
		getParam(params, "backR  ", backR);
		cout << '\n';
		getParam(params, "simCount", simCount);
		getParam(params, "freq    ", freq);
		cout << '\n';
		getParam(params, "kT", p.kT);
		cout << '\n';
		getParam(params, "B", p.B);
		cout << '\n';
		getParam(params, "sL", p.sL);
		getParam(params, "sR", p.sR);
		getParam(params, "sm", p.sm);
		getParam(params, "FL", p.FL);
		getParam(params, "FR", p.FR);
		getParam(params, "Fm", p.Fm);
		cout << '\n';
		getParam(params, "JL ", p.JL);
		getParam(params, "JR ", p.JR);
		getParam(params, "Jm ", p.Jm);
		getParam(params, "JmL", p.JmL);
		getParam(params, "JmR", p.JmR);
		getParam(params, "JLR", p.JLR);
		cout << '\n';
		getParam(params, "AL", p.AL);
		getParam(params, "AR", p.AR);
		getParam(params, "Am", p.Am);
		cout << '\n';
		getParam(params, "bL ", p.bL);
		getParam(params, "bR ", p.bR);
		getParam(params, "bm ", p.bm);
		getParam(params, "bmL", p.bmL);
		getParam(params, "bmR", p.bmR);
		getParam(params, "bLR", p.bLR);
		cout << '\n';
	} catch(ios::failure &e) {
		cerr << "Invalid parameter: " << e.what() << '\n';
		return 2;
	}
	if (params.size() > 0) {
		cerr << "Warning: the following parameters are being ignored:";
		int i = 0;
		for (auto &pair : params) {
			if (i++ % 8 == 0)
				cerr << "\n         ";
			cerr << pair.first << ", ";
		}
		cerr << '\n';
	}
	
	//create MSD model
	MSD msd(width, height, depth, molPosL, molPosR, topL, bottomL, frontR, backR);
	msd.flippingAlgorithm = arg2;
	msd.setParameters(p);

	bool customSeed = argc > 4 && string(argv[4]) != string("unique");
	if (customSeed) {
		unsigned long seed;
		istringstream ss(argv[4]);
		ss >> seed;
		if (ss.bad()) {
			cerr << "Invalid seed: " << argv[4] << '\n';
			return 6;
		}
		msd.setSeed(seed);
	}

	if( argc > 3 && string(argv[3]) != string("0") )
		msd.randomize(!customSeed);

	try {
		//print info/headings
		file << "t,,M_x,M_y,M_z,M_norm,M_theta,M_phi,,ML_x,ML_y,ML_z,ML_norm,ML_theta,ML_phi,,MR_x,MR_y,MR_z,MR_norm,MR_theta,MR_phi,,"
		     << "Mm_x,Mm_y,Mm_z,Mm_norm,Mm_theta,Mm_phi,,U,UL,UR,Um,UmL,UmR,ULR,,,x,y,z,m_x,m_y,m_z,s_x,s_y,s_z,f_x,f_y,f_z,,"
			    ",width = " << msd.getWidth()
			 << ",height = " << msd.getHeight()
			 << ",depth = " << msd.getDepth()
			 << ",molPosL = " << msd.getMolPosL()
			 << ",molPosR = " << msd.getMolPosR()
			 << ",topL = " << msd.getTopL()
			 << ",bottomL = " << msd.getBottomL()
			 << ",frontR = " << msd.getFrontR()
			 << ",backR = " << msd.getBackR()
			 << ",simCount = " << simCount
			 << ",freq = " << freq
			 << ",kT = " << p.kT
			 << ",\"B = " << p.B << '"'
			 << ",sL = " << p.sL
			 << ",sR = " << p.sR
			 << ",sm = " << p.sm
			 << ",FL = " << p.FL
			 << ",FR = " << p.FR
			 << ",Fm = " << p.Fm
			 << ",JL = " << p.JL
			 << ",JR = " << p.JR
			 << ",Jm = " << p.Jm
			 << ",JmL = " << p.JmL
			 << ",JmR = " << p.JmR
			 << ",JLR = " << p.JLR
			 << ",\"AL = " << p.AL << '"'
			 << ",\"AR = " << p.AR << '"'
			 << ",\"Am = " << p.Am << '"'
			 << ",bL = " << p.bL
			 << ",bR = " << p.bR
			 << ",bm = " << p.bm
			 << ",bmL = " << p.bmL
			 << ",bmR = " << p.bmR
			 << ",bLR = " << p.bLR
			 << ",seed = " << msd.getSeed()
			 << ",,msd_version = " << UDC_MSD_VERSION
			 << '\n';
	
		//run simulations
		cout << "Starting simulation...\n";
		for (auto const &s : spins) {
			try {
				Vector vec = msd.getSpin(s.x, s.y, s.z);
				vec = Vector::sphericalForm(s.norm, vec.theta(), vec.phi());
				msd.setSpin(s.x, s.y, s.z, vec);
				cout << "[" << s.x << " " << s.y << " " << s.z << "] = " << vec.norm() << '\n';
			} catch(out_of_range &ex) {
				cerr << "Warning: couldn't set spin [" << s.x << " " << s.y << " " << s.z << "] = " << s.norm << ":\n"
				     << "         " << ex.what() << '\n';
			}
		}
		msd.metropolis(simCount, freq);
	
		//print stability info
		cout << "Saving data...\n";
		MSD::Iterator msdIter = msd.begin();
		for( auto iter = msd.record.begin(); iter != msd.record.end(); iter++ ) {
			file << iter->t << ",,"
			     << iter->M.x  << ',' << iter->M.y  << ',' << iter->M.z  << ',' << iter->M.norm()  << ',' << iter->M.theta()  << ',' << iter->M.phi()  << ",,"
				 << iter->ML.x << ',' << iter->ML.y << ',' << iter->ML.z << ',' << iter->ML.norm() << ',' << iter->ML.theta() << ',' << iter->ML.phi() << ",,"
				 << iter->MR.x << ',' << iter->MR.y << ',' << iter->MR.z << ',' << iter->MR.norm() << ',' << iter->MR.theta() << ',' << iter->MR.phi() << ",,"
				 << iter->Mm.x << ',' << iter->Mm.y << ',' << iter->Mm.z << ',' << iter->Mm.norm() << ',' << iter->Mm.theta() << ',' << iter->Mm.phi() << ",,"
			     << iter->U << ',' << iter->UL << ',' << iter->UR << ',' << iter->Um << ',' << iter->UmL << ',' << iter->UmR << ',' << iter->ULR << ",,,";
			if( msdIter != msd.end() ) {
				Vector m = msdIter.getLocalM(), s = msdIter.getSpin(), f = msdIter.getFlux();
				file << msdIter.getX() << ',' << msdIter.getY() << ',' << msdIter.getZ() << ','
				     << m.x << ',' << m.y << ',' << m.z << ','
					 << s.x << ',' << s.y << ',' << s.z << ','
					 << f.x << ',' << f.y << ',' << f.z;
				++msdIter;
			}
			file << '\n';
		}
		for( ; msdIter != msd.end(); ++msdIter ) {
			Vector m = msdIter.getLocalM(), s = msdIter.getSpin(), f = msdIter.getFlux();
			file << ",, ,,,,,,, ,,,,,,, ,,,,,,, ,,,,,,, ,,,,,,,,,"
			     << msdIter.getX() << ',' << msdIter.getY() << ',' << msdIter.getZ() << ','
			     << m.x << ',' << m.y << ',' << m.z << ','
			     << s.x << ',' << s.y << ',' << s.z << ','
			     << f.x << ',' << f.y << ',' << f.z << '\n';
		}
			
	} catch(const ios::failure &e) {
		cerr << "Couldn't write to output file \"" << argv[1] << "\": " << e.what() << '\n';
		return 3;
	}
	
	return 0;
}

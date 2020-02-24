
/*
 * Christopher D'Angelo
 * 1-23-2015
 */

#include <fstream>
#include <iostream>
#include <string>
#include "MSD.h"

using namespace std;
using namespace udc;


template <typename T> void ask(string msg, T &val) {
	cout << msg;
	cin >> val;
}

void ask(string msg, Vector &vec) {
	cout << msg;
	cin >> vec.x >> vec.y >> vec.z;
}


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
	
	ofstream file;
	file.exceptions( ios::badbit | ios::failbit );
	try {
		file.open(argv[1]);
	} catch(const ios::failure &e) {
		cerr << "Couldn't open output file \"" << argv[1] << "\" for writing: " << e.what() << '\n';
		return 4;
	}
	
	//get parameters
	unsigned int width, height, depth, molPosL, molPosR;
	unsigned long long simCount, freq;
	MSD::Parameters p;
	
	cin.exceptions( ios::badbit | ios::failbit | ios::eofbit );
	try {
		ask("> width  = ", width);
		ask("> height = ", height);
		ask("> depth  = ", depth);
		cout << '\n';
		ask("> molPosL = ", molPosL);
		ask("> molPosR = ", molPosR);
		cout << '\n';
		ask("> simCount = ", simCount);
		ask("> freq     = ", freq);
		cout << '\n';
		ask("> kT = ", p.kT);
		cout << '\n';
		ask("> B = ", p.B);
		cout << '\n';
		ask("> sL = ", p.sL);
		ask("> sR = ", p.sR);
		ask("> sm = ", p.sm);
		ask("> FL = ", p.FL);
		ask("> FR = ", p.FR);
		ask("> Fm = ", p.Fm);
		cout << '\n';
		ask("> JL  = ", p.JL);
		ask("> JR  = ", p.JR);
		ask("> Jm  = ", p.Jm);
		ask("> JmL = ", p.JmL);
		ask("> JmR = ", p.JmR);
		ask("> JLR = ", p.JLR);
		cout << '\n';
		ask("> AL = ", p.AL);
		ask("> AR = ", p.AR);
		ask("> Am = ", p.Am);
		cout << '\n';
		ask("> bL  = ", p.bL);
		ask("> bR  = ", p.bR);
		ask("> bm  = ", p.bm);
		ask("> bmL = ", p.bmL);
		ask("> bmR = ", p.bmR);
		ask("> bLR = ", p.bLR);
		cout << '\n';
	} catch(ios::failure &e) {
		cerr << "Invalid parameter: " << e.what() << '\n';
		return 2;
	}
	
	//create MSD model
	MSD msd(width, height, depth, molPosL, molPosR);
	msd.flippingAlgorithm = arg2;
	msd.setParameters(p);
	
	try {
		//print info/headings
		file << "t,,M_x,M_y,M_z,M_norm,M_theta,M_phi,,ML_x,ML_y,ML_z,ML_norm,ML_theta,ML_phi,,MR_x,MR_y,MR_z,MR_norm,MR_theta,MR_phi,,"
		     << "Mm_x,Mm_y,Mm_z,Mm_norm,Mm_theta,Mm_phi,,U,UL,UR,Um,UmL,UmR,ULR,,,x,y,z,m_x,m_y,m_z,s_x,s_y,s_z,f_x,f_y,f_z,,"
			    ",width = " << msd.getWidth()
			 << ",height = " << msd.getHeight()
			 << ",depth = " << msd.getDepth()
			 << ",molPosL = " << msd.getMolPosL()
			 << ",molPosR = " << msd.getMolPosR()
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
			 << ",,msd_version = " << UDC_MSD_VERSION
			 << '\n';
	
		//run simulations
		cout << "Starting simulation...\n";
		if( argc > 3 && string(argv[3]) != string("0") )
			msd.randomize();
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

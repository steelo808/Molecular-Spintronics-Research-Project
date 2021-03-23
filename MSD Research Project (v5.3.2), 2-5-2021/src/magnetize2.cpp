
/*
 * Christopher D'Angelo
 * 12-15-2015
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
	
	ofstream file(argv[1]);
	file.exceptions( ios::badbit | ios::failbit );
	
	//get parameters
	unsigned int width, height, depth, molPosL, molPosR, topL, bottomL, frontR, backR;
	unsigned long long t_eq, freq; // freq: how often we record a reading
	double B_min, B_max, B_rate, B_theta, B_phi; // B_rate = d |B| / d t
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
		ask("> topL    = ", topL);
		ask("> bottomL = ", bottomL);
		ask("> frontR  = ", frontR);
		ask("> backR   = ", backR);
		cout << '\n';
		ask("> t_eq = ", t_eq);
		ask("> freq = ", freq);
		cout << '\n';
		ask("> kT = ", p.kT);
		cout << '\n';
		ask("> B_min  = ", B_min);
		ask("> B_max  = ", B_max);
		ask("> B_rate = ", B_rate);
		ask("> B_theta = ", B_theta);
		ask("> B_phi = ", B_phi);
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
	MSD msd(width, height, depth, molPosL, molPosR, topL, bottomL, frontR, backR);
	msd.flippingAlgorithm = arg2;
	msd.setParameters(p);
	
	try {
		//print info/headings
		file << "B_x,B_y,B_z,,"
				"M_x,M_y,M_z,M_norm,M_theta,M_phi,,"
				"ML_x,ML_y,ML_z,ML_norm,ML_theta,ML_phi,,"
				"MR_x,MR_y,MR_z,MR_norm,MR_theta,MR_phi,,"
				"Mm_x,Mm_y,Mm_z,Mm_norm,Mm_theta,Mm_phi,,"
				"U,UL,UR,Um,UmL,UmR,ULR,"
			    ",width = " << msd.getWidth()
			 << ",height = " << msd.getHeight()
			 << ",depth = " << msd.getDepth()
			 << ",molPosL = " << msd.getMolPosL()
			 << ",molPosR = " << msd.getMolPosR()
			 << ",topL = " << msd.getTopL()
			 << ",bottomL = " << msd.getBottomL()
			 << ",frontR = " << msd.getFrontR()
			 << ",backR = " << msd.getBackR()
			 << ",t_eq = " << t_eq
			 << ",freq = " << freq
			 << ",kT = " << p.kT
			 << ",B_min = " << B_min
			 << ",B_max = " << B_max
			 << ",B_rate = " << B_rate
			 << ",B_theta = " << B_theta
			 << ",B_phi = " << B_phi
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
		
		// convert from degrees to radians
		B_theta *= PI / 180.0;
		B_phi *= PI / 180.0;
		Vector dB = Vector::sphericalForm(B_rate, B_theta, B_phi);

		// define some lambda functions
		auto recordResults = [&]() {
			cout << "B = " << p.B << '\n';
			cout << "Saving data...\n";
			
			MSD::Results r = msd.getResults();
			file << p.B.x  << ',' << p.B.y  << ',' << p.B.z  << ",,"
				 << r.M.x  << ',' << r.M.y  << ',' << r.M.z  << ',' << r.M.norm()  << ',' << r.M.theta()  << ',' << r.M.phi()  << ",,"
				 << r.ML.x << ',' << r.ML.y << ',' << r.ML.z << ',' << r.ML.norm() << ',' << r.ML.theta() << ',' << r.ML.phi() << ",,"
				 << r.MR.x << ',' << r.MR.y << ',' << r.MR.z << ',' << r.MR.norm() << ',' << r.MR.theta() << ',' << r.MR.phi() << ",,"
				 << r.Mm.x << ',' << r.Mm.y << ',' << r.Mm.z << ',' << r.Mm.norm() << ',' << r.Mm.theta() << ',' << r.Mm.phi() << ",,"
				 << r.U << ',' << r.UL << ',' << r.UR << ',' << r.Um << ',' << r.UmL << ',' << r.UmR << ',' << r.ULR << '\n';
		};
		
		unsigned long long simCount = 0L;
		auto sim = [&]() {
			msd.setB(p.B);
			msd.metropolis(1);
			
			if( ++simCount == freq ) {
				recordResults();
				simCount = 0;
			}
		};
		
		//run simulations
		cout << "Starting simulation...\n";
		if( argc > 3 && string(argv[3]) != string("0") )
			msd.randomize();
		
		// running to equilibrium
		msd.metropolis(t_eq);
		simCount = freq - 1; // record after next sim
		
		if( !(argc > 4 && string(argv[4]) != string("0")) ) {
			// running B from 0 to B_max
			p.B = Vector::ZERO;
			for( double rho = 0; rho < B_max; rho += B_rate ) {
				sim();
				p.B += dB;
			}
			simCount = freq - 1;
		}
		
		// running B_y from B_y_max to B_y_min
		p.B = Vector::sphericalForm(B_max, B_theta, B_phi);
		for( double rho = B_max; rho > B_min; rho -= B_rate ) {
			sim();
			p.B -= dB;
		}
		simCount = freq - 1;
		
		// running B_y from B_y_min to B_y_max
		double B_max2 = B_max + B_rate / 2;  // to correct for floating point errors
		p.B = Vector::sphericalForm(B_min, B_theta, B_phi);
		for( double rho = B_min; rho <= B_max2; rho += B_rate ) {
			sim();
			p.B += dB;
		}
		
	} catch(ios::failure &e) {
		cerr << "Couldn't write to output file \"" << argv[1] << "\": " << e.what() << '\n';
		return 3;
	}
	
	return 0;
}

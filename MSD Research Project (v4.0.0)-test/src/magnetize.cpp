
/*
 * Christopher D'Angelo
 * 1-22-2015
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

enum ARG3 {
	NOOP, REINITIALIZE, RANDOMIZE
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
	
	ARG3 arg3 = NOOP;
	if( argc > 3 ) {
		string s(argv[3]);
		if( s == string("reinitialize") )
			arg3 = REINITIALIZE;
		else if( s == string("randomize") )
			arg3 = RANDOMIZE;
	}
	
	ofstream file(argv[1]);
	file.exceptions( ios::badbit | ios::failbit );
	
	//get parameters
	unsigned int width, height, depth, molPosL, molPosR;
	unsigned long long t_eq, simCount, freq;
	double B_y_min, B_y_max, B_y_inc;
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
		ask("> t_eq     = ", t_eq);
		ask("> simCount = ", simCount);
		ask("> freq     = ", freq);
		cout << '\n';
		ask("> kT = ", p.kT);
		cout << '\n';
		ask("> F = ", p.F);
		cout << '\n';
		ask("> B_y_min = ", B_y_min);
		ask("> B_y_max = ", B_y_max);
		ask("> B_y_inc = ", B_y_inc);
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
	
	try {
		//print info/headings
		file << "B_x,B_y,B_z,,"
		        "<M>_x,<M>_y,<M>_z,<M>_norm,<M>_theta,<M>_phi,,"
				"<ML>_x,<ML>_y,<ML>_z,<ML>_norm,<ML>_theta,<ML>_phi,,"
				"<MR>_x,<MR>_y,<MR>_z,<MR>_norm,<MR>_theta,<MR>_phi,,"
			    "<Mm>_x,<Mm>_y,<Mm>_z,<Mm>_norm,<Mm>_theta,<Mm>_phi,,"
				"<U>,<UL>,<UR>,<Um>,<UmL>,<UmR>,<ULR>,,"
				"c,cL,cR,cm,cmL,cmR,cLR,,"
				"x,xL,xR,xm,,"
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
			 << ",t_eq = " << t_eq
			 << ",simCount = " << simCount
			 << ",freq = " << freq
			 << ",kT = " << p.kT
			 << ",F = " << p.F
			 << ",JL = " << p.JL
			 << ",JR = " << p.JR
			 << ",Jm = " << p.Jm
			 << ",JmL = " << p.JmL
			 << ",JmR = " << p.JmR
			 << ",JLR = " << p.JLR
			 << ",AL = " << p.AL
			 << ",AR = " << p.AR
			 << ",Am = " << p.Am
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
		
		auto sim = [&]() {
			if( arg3 == REINITIALIZE )
				msd.reinitialize();
			else if( arg3 == RANDOMIZE )
				msd.randomize();
			msd.record.clear();
			
			cout << "B_y = " << p.B.y << '\n';
			msd.setParameters(p);
			msd.metropolis(t_eq);
			msd.metropolis(simCount, freq);
			
			cout << "Saving data...\n";
			MSD::Results r = msd.getResults();
			Vector avgM   = msd.meanM();
			Vector avgML  = msd.meanML();
			Vector avgMR  = msd.meanMR();
			Vector avgMm  = msd.meanMm();
			double avgU   = msd.meanU();
			double avgUL  = msd.meanUL();
			double avgUR  = msd.meanUR();
			double avgUm  = msd.meanUm();
			double avgUmL = msd.meanUmL();
			double avgUmR = msd.meanUmR();
			double avgULR = msd.meanULR();
			file << p.B.x  << ',' << p.B.y  << ',' << p.B.z  << ",,"
				<< avgM.x  << ',' << avgM.y  << ',' << avgM.z  << ',' << avgM.norm()  << ',' << avgM.theta()  << ',' << avgM.phi()  << ",,"
				 << avgML.x << ',' << avgML.y << ',' << avgML.z << ',' << avgML.norm() << ',' << avgML.theta() << ',' << avgML.phi() << ",,"
				 << avgMR.x << ',' << avgMR.y << ',' << avgMR.z << ',' << avgMR.norm() << ',' << avgMR.theta() << ',' << avgMR.phi() << ",,"
				 << avgMm.x << ',' << avgMm.y << ',' << avgMm.z << ',' << avgMm.norm() << ',' << avgMm.theta() << ',' << avgMm.phi() << ",,"
				 << avgU << ',' << avgUL << ',' << avgUR << ',' << avgUm << ',' << avgUmL << ',' << avgUmR << ',' << avgULR << ",,"
				 << msd.specificHeat()    << ',' << msd.specificHeat_L()  << ',' << msd.specificHeat_R()  << ',' << msd.specificHeat_m() << ','
				 << msd.specificHeat_mL() << ',' << msd.specificHeat_mR() << ',' << msd.specificHeat_LR() << ",,"
				 << msd.magneticSusceptibility()   << ',' << msd.magneticSusceptibility_L() << ','
				 << msd.magneticSusceptibility_R() << ',' << msd.magneticSusceptibility_m() << ",,"
				 << r.M.x  << ',' << r.M.y  << ',' << r.M.z  << ',' << r.M.norm()  << ',' << r.M.theta()  << ',' << r.M.phi()  << ",,"
				 << r.ML.x << ',' << r.ML.y << ',' << r.ML.z << ',' << r.ML.norm() << ',' << r.ML.theta() << ',' << r.ML.phi() << ",,"
				 << r.MR.x << ',' << r.MR.y << ',' << r.MR.z << ',' << r.MR.norm() << ',' << r.MR.theta() << ',' << r.MR.phi() << ",,"
				 << r.Mm.x << ',' << r.Mm.y << ',' << r.Mm.z << ',' << r.Mm.norm() << ',' << r.Mm.theta() << ',' << r.Mm.phi() << ",,"
				 << r.U << ',' << r.UL << ',' << r.UR << ',' << r.Um << ',' << r.UmL << ',' << r.UmR << ',' << r.ULR << '\n';
		};
		
		for( p.B.y = B_y_max; p.B.y > B_y_min; p.B.y -= B_y_inc )
			sim();
		for( p.B.y = B_y_min; p.B.y < B_y_max; p.B.y += B_y_inc )
			sim();
		
	} catch(ios::failure &e) {
		cerr << "Couldn't write to output file \"" << argv[1] << "\": " << e.what() << '\n';
		return 3;
	}
	
	return 0;
}

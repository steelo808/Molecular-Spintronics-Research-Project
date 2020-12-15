/*
 * MSD.h
 *
 *  Last Edited: December 15, 2020
 *       Author: Christopher D'Angelo
 */

#ifndef UDC_MSD
#define UDC_MSD

#define UDC_MSD_VERSION "5.3"

#include <cstdlib>
#include <cmath>
#include <ctime>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include "Vector.h"
#include "udc.h"


namespace udc {

using std::bind;
using std::function;
using std::invalid_argument;
using std::map;
using std::mt19937_64;
using std::ostream;
using std::out_of_range;
using std::ref;
using std::string;
using std::uniform_int_distribution;
using std::uniform_real_distribution;

using udc::E;
using udc::PI;
using udc::sq;
using udc::Vector;


/**
 * An abstract Molecular Spintronic Device.
 *
 * To do: The various D (Dipolar Coupling)
 *        parameters are not taken into account yet.
 */
class MSD {
 public:
	typedef function<Vector(const Vector &, function<double()>)> FlippingAlgorithm;
	
	struct Parameters {
		double kT; //Temperature
		Vector B; //Magnetic field
		double sL, sm, sR; //spin magnitudes
		double FL, FR, Fm; //Spin fluctuation constants, flux.norm() = uniform random between [0, F)
		double JL, JR, Jm, JmL, JmR, JLR; //Heisenberg exchange coupling
		double bL, bR, bm, bmL, bmR, bLR; //Biquadratic Coupling
		Vector AL, AR, Am; //Anisotropy
		double DL, DR, Dm, DmL, DmR, DLR; //Dipolar Coupling
		
		Parameters();
		
		bool operator==(const Parameters &) const;
		bool operator!=(const Parameters &) const;
	};
	
	struct Results {
		unsigned long long t; //time, i.e. current iteration
		Vector M, ML, MR, Mm; //Magnetization
		double U, UL, UR, Um, UmL, UmR, ULR; //Internal Energy
		
		Results();
		
		bool operator==(const Results &) const;
		bool operator!=(const Results &) const;
	};
	
	class Iterator {
		friend class MSD;
		
	 private:
		const MSD &msd;
		unsigned int i;  // index in msd::indicies
		
		Iterator(const MSD &, unsigned int i);
		
	 public:
		unsigned int getIndex() const;
		unsigned int getX() const;
		unsigned int getY() const;
		unsigned int getZ() const;
		
		Vector getSpin() const;
		Vector getFlux() const;
		Vector getLocalM() const;
		
		Vector operator*() const;
		Vector operator[](int) const;
		operator unsigned int() const;
		
		Iterator& operator++();
		Iterator operator++(int);
		Iterator& operator--();
		Iterator operator--(int);
		Iterator& operator+=(int);
		Iterator& operator-=(int);
		
		Iterator operator+(int) const;
		Iterator operator-(int) const;
		
		bool operator==(const Iterator &) const;
		bool operator!=(const Iterator &) const;
		bool operator>(const Iterator &) const;
		bool operator<(const Iterator &) const;
		bool operator>=(const Iterator &) const;
		bool operator<=(const Iterator &) const;
	};
	
	static const FlippingAlgorithm UP_DOWN_MODEL;
	static const FlippingAlgorithm CONTINUOUS_SPIN_MODEL;

 private:
	static Vector initSpin; //initial spin of all atoms
	static Vector initFlux; //initial spin fluctuation (direction only) for each atom
	
	std::map<unsigned int, Vector> spins;
	std::map<unsigned int, Vector> fluxes;
	Parameters parameters;
	Results results;
	unsigned int width, height, depth;
	unsigned int molPosL, molPosR;
	unsigned int topL, bottomL, frontR, backR;  // "inner sizes/boundaries"
	unsigned int n; //number of atoms
	
	std::vector<unsigned int> indices; //valid indices
	
	mt19937_64 prng; //pseudo random number generator
	uniform_real_distribution<double> MSD::rand; //uniform probability density function on the interval [0, 1)
	unsigned long seed; //store seed so that every run can follow the same sequence
	unsigned char seed_count; //to help keep seeds from repeating because of temporal proximity
	
	unsigned int index(unsigned int x, unsigned int y, unsigned int z) const;
	unsigned int x(unsigned int a) const;
	unsigned int y(unsigned int a) const;
	unsigned int z(unsigned int a) const;
	
	unsigned long genSeed(); //generates a new seed
	
	MSD& operator=(const MSD&); //undefined, do not use!
	MSD(const MSD &m); //undefined, do not use!

	void init();
	
 public:
	std::vector<Results> record;
	FlippingAlgorithm flippingAlgorithm; //algorithm used to "flip" an atom in metropolis
	
	MSD(unsigned int width, unsigned int height, unsigned int depth,
			unsigned int molPosL, unsigned int molPosR,
			unsigned int topL, unsigned int bottomL, unsigned int frontR, unsigned int backR);
	MSD(unsigned int width, unsigned int height, unsigned int depth,
			unsigned int heightL, unsigned depthR);
	MSD(unsigned int width, unsigned int height, unsigned int depth);
	
	Parameters getParameters() const;
	void setParameters(const Parameters &);
	Results getResults() const;
	
	void setB(const Vector &B);
	
	Vector getSpin(unsigned int a) const;
	Vector getSpin(unsigned int x, unsigned int y, unsigned int z) const;
	Vector getFlux(unsigned int a) const;
	Vector getFlux(unsigned int x, unsigned int y, unsigned int z) const;
	Vector getLocalM(unsigned int a) const;
	Vector getLocalM(unsigned int x, unsigned int y, unsigned int z) const;
	void setSpin(unsigned int a, const Vector &);
	void setSpin(unsigned int x, unsigned int y, unsigned int z, const Vector &);
	void setFlux(unsigned int a, const Vector &);
	void setFlux(unsigned int x, unsigned int y, unsigned int z, const Vector &);
	void setLocalM(unsigned int a, const Vector &, const Vector &);
	void setLocalM(unsigned int x, unsigned int y, unsigned int z, const Vector &, const Vector &);
	
	unsigned int getN() const;
	unsigned int getWidth() const;
	unsigned int getHeight() const;
	unsigned int getDepth() const;
	void getDimensions(unsigned int &width, unsigned int &height, unsigned int &depth) const;
	unsigned int getMolPosL() const;
	unsigned int getMolPosR() const;
	void getMolPos(unsigned int &molPosL, unsigned int &molPosR) const;
	unsigned int getTopL() const;
	unsigned int getBottomL() const;
	unsigned int getFrontR() const;
	unsigned int getBackR() const;
	void getInnerBounds(unsigned int &topL, unsigned int &bottomL, unsigned int &frontR, unsigned int &backR) const;
	
	void setSeed(unsigned long seed);  // change the seed of the prng, and restart the pseudo-random sequence
	unsigned long getSeed() const;  // get the seed currently being used

	void reinitialize(bool reseed = true); //reseed iff you want a new seed, true by default
	void randomize(bool reseed = true); //similar to reinitialize, but initial state is random
	void metropolis(unsigned long long N);
	void metropolis(unsigned long long N, unsigned long long freq);
	
	double specificHeat() const;
	double specificHeat_L() const;
	double specificHeat_R() const;
	double specificHeat_m() const;
	double specificHeat_mL() const;
	double specificHeat_mR() const;
	double specificHeat_LR() const;
	double magneticSusceptibility() const;
	double magneticSusceptibility_L() const;
	double magneticSusceptibility_R() const;
	double magneticSusceptibility_m() const;
	
	Vector meanM() const;
	Vector meanML() const;
	Vector meanMR() const;
	Vector meanMm() const;
	double meanU() const;
	double meanUL() const;
	double meanUR() const;
	double meanUm() const;
	double meanUmL() const;
	double meanUmR() const;
	double meanULR() const;
	
	Iterator begin() const;
	Iterator end() const;
};


ostream& operator <<(ostream &out, const MSD::Parameters &p) {
	return out
		<< "kT  = " << p.kT  << '\n' << "B   = " << p.B   << "\n\n"
		<< "sL  = " << p.sL  << '\n' << "SR  = " << p.sR  << '\n' << "sm  = " << p.sm  << '\n'
		<< "FL  = " << p.FL  << '\n' << "FR  = " << p.FR  << '\n' << "Fm  = " << p.Fm  << "\n\n"
		<< "JL  = " << p.JL  << '\n' << "JR  = " << p.JR  << '\n' << "Jm  = " << p.Jm  << '\n'
		<< "JmL = " << p.JmL << '\n' << "JmR = " << p.JmR << '\n' << "JLR = " << p.JLR << "\n\n"
		<< "bL  = " << p.bL  << '\n' << "bR  = " << p.bR  << '\n' << "bm  = " << p.bm  << '\n'
		<< "bmL = " << p.bmL << '\n' << "bmR = " << p.bmR << '\n' << "bLR = " << p.bLR << "\n\n"
		<< "AL  = " << p.AL  << '\n' << "AR  = " << p.AR  << '\n' << "Am  = " << p.Am  << "\n\n"
		<< "DL  = " << p.DL  << '\n' << "DR  = " << p.DR  << '\n' << "Dm  = " << p.Dm  << '\n'
		<< "DmL = " << p.DmL << '\n' << "DmR = " << p.DmR << '\n' << "DLR = " << p.DLR << '\n';
}

ostream& operator <<(ostream &out, const MSD::Results &r) {
	return out
		<< "t   = " << r.t   << "\n\n"
		<< "M   = " << r.M   << '\n' << "ML  = " << r.ML  << '\n' << "MR  = " << r.MR  << '\n' << "Mm  = " << r.Mm  << "\n\n"
		<< "U   = " << r.U   << '\n' << "UL  = " << r.UL  << '\n' << "UR  = " << r.UR  << '\n' << "Um  = " << r.Um  << '\n'
		<< "UmL = " << r.UmL << '\n' << "UmR = " << r.UmR << '\n' << "ULR = " << r.ULR << '\n';
}


//--------------------------------------------------------------------------------


MSD::Parameters::Parameters()
: kT(0.25), B(Vector::ZERO),
  sL(1), sR(1), sm(1), FL(0), FR(0), Fm(0),
  JL(1), JR(1), Jm(0), JmL(1), JmR(-1), JLR(0),
  bL(0), bR(0), bm(0), bmL(0), bmR(0), bLR(0),
  AL(Vector::ZERO), AR(Vector::ZERO), Am(Vector::ZERO),
  DL(0), DR(0), Dm(0), DmL(0), DmR(0), DLR(0) {
}

bool MSD::Parameters::operator==(const Parameters &p) const {
	return kT  == p.kT  && B   == p.B
	    && sL  == p.sL  && sR  == p.sR  && sm  == p.sm
	    && FL  == p.FL  && FR  == p.FR  && Fm  == p.Fm
	    && JL  == p.JL  && JR  == p.JR  && Jm  == p.Jm
	    && JmL == p.JmL && JmR == p.JmR && JLR == p.JLR
	    && bL  == p.bL  && bR  == p.bR  && bm  == p.bm
	    && bmL == p.bmL && bmR == p.bmR && bLR == p.bLR
	    && AL  == p.AL  && AR  == p.AR  && Am  == p.Am
	    && DL  == p.DL  && DR  == p.DR  && Dm  == p.Dm
	    && DmL == p.DmL && DmR == p.DmR && DLR == p.DLR;
}

bool MSD::Parameters::operator!=(const Parameters &p) const {
	return !(*this == p);
}


MSD::Results::Results()
: t(0), M(Vector::ZERO), ML(Vector::ZERO), MR(Vector::ZERO), Mm(Vector::ZERO),
  U(0), UL(0), UR(0), Um(0), UmL(0), UmR(0), ULR(0) {
}

bool MSD::Results::operator==(const Results &r) const {
	return t   == r.t
	    && M   == r.M   && ML  == r.ML  && MR  == r.MR && Mm == r.Mm
	    && U   == r.U   && UL  == r.UL  && UR  == r.UR && Um == r.Um
		&& UmL == r.UmL && UmR == r.UmR && ULR == r.ULR;
}

bool MSD::Results::operator!=(const Results &r) const {
	return !(*this == r);
}


MSD::Iterator::Iterator(const MSD &msd, unsigned int i) : msd(msd), i(i) {
}

unsigned int MSD::Iterator::getIndex() const {
	return msd.indices.at(i);
}

unsigned int MSD::Iterator::getX() const {
	return msd.x( getIndex() );
}

unsigned int MSD::Iterator::getY() const {
	return msd.y( getIndex() );
}

unsigned int MSD::Iterator::getZ() const {
	return msd.z( getIndex() );
}

Vector MSD::Iterator::getSpin() const {
	return msd.getSpin( this->getIndex() );
}

Vector MSD::Iterator::getFlux() const {
	return msd.getFlux( this->getIndex() );
}

Vector MSD::Iterator::getLocalM() const {
	return msd.getLocalM( this->getIndex() );
}

Vector MSD::Iterator::operator*() const {
	return getLocalM();
}

Vector MSD::Iterator::operator[](int n) const {
	return *(*this + n);
}

MSD::Iterator::operator unsigned int() const {
	return getIndex();
}

MSD::Iterator& MSD::Iterator::operator++() {
	unsigned int next = i + 1;
	if (next >= msd.indices.size())
		out_of_range("can't increment MSD::Iterator == msd.end()");
	i = next;
	return *this;
}

MSD::Iterator MSD::Iterator::operator++(int) {
	Iterator prev = *this;
	++(*this);
	return prev;
}

MSD::Iterator& MSD::Iterator::operator--() {
	if (i <= 0)
		throw out_of_range("can't decrement MSD::Iterator == msd.begin()");
	
	--i;
	return *this;
}

MSD::Iterator MSD::Iterator::operator--(int) {
	Iterator prev = *this;
	--(*this);
	return prev;
}

MSD::Iterator& MSD::Iterator::operator+=(int n) {
	if( n < 0 )
		return *this -= -n;
	
	unsigned int sum = i + n; 
	if (sum >= msd.indices.size())
		throw out_of_range("can't increment past msd.end()");
	i = sum;
	return *this;
}

MSD::Iterator& MSD::Iterator::operator-=(int n) {
	if( n < 0 )
		return *this += -n;
	
	if (n > i)
		throw out_of_range("can't decrement past msd.begin()");
	i -= n;
	return *this;
}

MSD::Iterator MSD::Iterator::operator+(int n) const {
	Iterator iter = *this;
	iter += n;
	return iter;
}

MSD::Iterator MSD::Iterator::operator-(int n) const {
	Iterator iter = *this;
	iter -= n;
	return iter;
}

bool MSD::Iterator::operator==(const Iterator &iter) const {
	if( &msd != &iter.msd )
		throw invalid_argument("must compare two iterators from the same MSD");
	return i == iter.i;
}

bool MSD::Iterator::operator!=(const Iterator &iter) const {
	return !(*this == iter);
}

bool MSD::Iterator::operator>(const Iterator &iter) const {
	if( &msd != &iter.msd )
		throw invalid_argument("must compare two iterators from the same MSD");
	return i > iter.i;
}

bool MSD::Iterator::operator<(const Iterator &iter) const {
	if( &msd != &iter.msd )
		throw invalid_argument("must compare two iterators from the same MSD");
	return i < iter.i;
}

bool MSD::Iterator::operator>=(const Iterator &iter) const {
	return !(*this < iter);
}

bool MSD::Iterator::operator<=(const Iterator &iter) const {
	return !(*this > iter);
}


const MSD::FlippingAlgorithm MSD::UP_DOWN_MODEL = [](const Vector &spin, function<double()> rand) {
	return -spin;
};

const MSD::FlippingAlgorithm MSD::CONTINUOUS_SPIN_MODEL = [](const Vector &spin, function<double()> rand) {
	return Vector::sphericalForm( spin.norm(), 2 * PI * rand(), asin(2 * rand() - 1) );
};


Vector MSD::initSpin = Vector::J;
Vector MSD::initFlux = Vector::ZERO;


unsigned int MSD::index(unsigned int x, unsigned int y, unsigned int z) const {
	return (z * height + y) * width + x;
}

unsigned int MSD::x(unsigned int a) const {
	return a % width;
}

unsigned int MSD::y(unsigned int a) const {
	return a % (width * height) / width;
}

unsigned int MSD::z(unsigned int a) const {
	return a / (width * height);
}


unsigned long MSD::genSeed() {
	return (  static_cast<unsigned long>(time(NULL))      << 16 )
	     | ( (static_cast<unsigned long>(clock()) & 0xFF) << 8  )
	     | ( (static_cast<unsigned long>(seed_count++) & 0xFF) );
}


void MSD::init() {
	// preconditions:
	if (molPosR >= width)   molPosR = width - 1;
	if (molPosL > width)    molPosL = width;
	if (molPosR < molPosL)  molPosR = molPosL - 1;
	if (bottomL >= height)  bottomL = height - 1;
	if (topL > height)      topL = height;
	if (bottomL < topL)     topL = bottomL - 1;
	if (backR >= depth)     backR = depth - 1;
	if (frontR > depth)     frontR = depth;
	if (backR < frontR)     backR = frontR - 1;

	seed = genSeed();
	prng.seed(seed);
	
	n = 0;
	unsigned int a;
	for( unsigned int z = 0; z < depth; z++ )
		for( unsigned int y = 0; y < height; y++ ) {
			// left
			for( unsigned int x = 0; x < molPosL; x++ )
				if (topL <= y && y <= bottomL) {
					a = index(x, y, z);
					indices.push_back(a);
					spins[a] = initSpin;
					fluxes[a] = initFlux;
					n++;
				}
			// mol
			if( ((y == topL || y == bottomL) && frontR <= z && z <= backR) || ((z == frontR || z == backR) && topL <= y && y <= bottomL) )
				for( unsigned int x = molPosL; x <= molPosR; x++ ) {
					a = index(x, y, z);
					indices.push_back(a);
					spins[a] = initSpin;
					fluxes[a] = initFlux;
					n++;
				}
			// right
			for( unsigned int x = molPosR + 1; x < width; x++ )
				if (frontR <= z && z <= backR) {
					a = index(x, y, z);
					indices.push_back(a);
					spins[a] = initSpin;
					fluxes[a] = initFlux;
					n++;
				}
		}
	
	setParameters(parameters); //calculate initial state (results)
	flippingAlgorithm = CONTINUOUS_SPIN_MODEL; //set default "flipping" algorithm
}


MSD::MSD(unsigned int width, unsigned int height, unsigned int depth,
		unsigned int molPosL, unsigned int molPosR,
		unsigned int topL, unsigned int bottomL, unsigned int frontR, unsigned int backR)
: width(width), height(height), depth(depth), molPosL(molPosL), molPosR(molPosR),
		topL(topL), bottomL(bottomL), frontR(frontR), backR(backR)
{
	init();
}


MSD::MSD(unsigned int width, unsigned int height, unsigned int depth,
		unsigned int heightL, unsigned depthR)
: width(width), height(height), depth(depth), molPosL((width - 1) / 2), molPosR((width - 1) / 2),
		topL( (unsigned int) ceil((height - 1 - heightL) / 2.0) ),
		bottomL( (unsigned int) floor((height - 1 + heightL) / 2.0) ),
		frontR( (unsigned int) ceil((depth - 1 - depthR) / 2.0) ),
		backR( (unsigned int) floor((depth - 1 + depthR) / 2.0) )
{
	init();
}


MSD::MSD(unsigned int width, unsigned int height, unsigned int depth)
: width(width), height(height), depth(depth), molPosL((width - 1) / 2), molPosR(width / 2),
		topL(0), bottomL(height - 1), frontR(0), backR(depth - 1)
{
	init();
}


MSD::Parameters MSD::getParameters() const {
	return parameters;
}

void MSD::setParameters(const MSD::Parameters &p) {
	MSD::Parameters p0 = parameters;  // old parameters
	parameters = p;  // update to new parameters
	
	//Spin and Spin Flux Magnitudes
	for( auto iter = begin(); iter != end(); ++iter ) {
		unsigned int i = iter.getIndex();
		unsigned int x = iter.getX();
		if( x < molPosL ) {
			spins[i].normalize() *= parameters.sL;
			fluxes[i].normalize() *= p0.FL != 0 ? parameters.FL / p0.FL : 0;
		} else if( x > molPosR ) {
			spins[i].normalize() *= parameters.sR;
			fluxes[i].normalize() *= p0.FR != 0 ? parameters.FR / p0.FR : 0;
		} else {
			spins[i].normalize() *= parameters.sm;
			fluxes[i].normalize() *= p0.Fm != 0 ? parameters.Fm / p0.Fm : 0;
		}
	}
	
	//Magnetization and Anisotropy
	results.ML = results.MR = results.Mm = Vector::ZERO;
	Vector anisotropy_L = Vector::ZERO, anisotropy_R = Vector::ZERO, anisotropy_m = Vector::ZERO;
	for( auto iter = begin(); iter != end(); ++iter ) {
		unsigned int x = iter.getX();
		Vector localM = iter.getLocalM();
		if( x < molPosL ) {
			results.ML += localM;
			anisotropy_L.x += sq( localM.x );
			anisotropy_L.y += sq( localM.y );
			anisotropy_L.z += sq( localM.z );
		} else if( x > molPosR ) {
			results.MR += iter.getLocalM();
			anisotropy_R.x += sq( localM.x );
			anisotropy_R.y += sq( localM.y );
			anisotropy_R.z += sq( localM.z );
		} else {
			results.Mm += iter.getLocalM();
			anisotropy_m.x += sq( localM.x );
			anisotropy_m.y += sq( localM.y );
			anisotropy_m.z += sq( localM.z );
		}
	}
	results.M = results.ML + results.MR + results.Mm;
	

	//Internal Energy
	results.UL = 0;
	double biquad_L = 0;
	for( unsigned int z = 0; z < depth; z++ )
		for( unsigned int y = topL; y <= bottomL; y++ )
			for( unsigned int x = 0; x < molPosL; x++ ) {
				Vector m = getLocalM(x, y, z);
				if( x + 1 < molPosL ) {
					double bond = m * getLocalM(x + 1, y, z);
					results.UL += bond;
					biquad_L += sq(bond);
				}
				if( y + 1 < bottomL ) {
					double bond = m * getLocalM(x, y + 1, z);
					results.UL += bond;
					biquad_L += sq(bond);
				}
				if( z + 1 < depth ) {
					double bond = m * getLocalM(x, y, z + 1);
					results.UL += bond;
					biquad_L += sq(bond);
				}
			}
	results.UL *= -parameters.JL;
	results.UL -= parameters.B * results.ML;
	results.UL -= parameters.AL * anisotropy_L;
	results.UL -= parameters.bL * biquad_L;
	
	results.UR = 0;
	double biquad_R = 0;
	for( unsigned int z = frontR; z <= backR; z++ )
		for( unsigned int y = 0; y < height; y++ )
			for( unsigned int x = molPosR + 1; x < width; x++ ) {
				Vector m = getLocalM(x, y, z);
				if( x + 1 < width ) {
					double bond = m * getLocalM(x + 1, y, z);
					results.UR += bond;
					biquad_R += sq(bond);
				}
				if( y + 1 < height ) {
					double bond = m * getLocalM(x, y + 1, z);
					results.UR += bond;
					biquad_R += sq(bond);
				}
				if( z + 1 < backR ) {
					double bond = m * getLocalM(x, y, z + 1);
					results.UR += bond;
					biquad_R += sq(bond);
				}
			}
	results.UR *= -parameters.JR;
	results.UR -= parameters.B * results.MR;
	results.UR -= parameters.AR * anisotropy_R;
	results.UR -= parameters.bR * biquad_R;
	
	results.Um = 0;
	double biquad_m = 0;
	for( unsigned int x = molPosL; x < molPosR; x++ )
		for( unsigned int y = topL; y <= bottomL; y++ )
			for( unsigned int z = frontR; z <= backR; z++ )
				if( y == topL || z == frontR || y == bottomL || z == backR ) {
					double bond = getLocalM(x, y, z) * getLocalM(x + 1, y, z);
					results.Um += bond;
					biquad_m += sq(bond);
				}
	results.Um *= -parameters.Jm;
	results.Um -= parameters.B * results.Mm;
	results.Um -= parameters.Am * anisotropy_m;
	results.Um -= parameters.bm * biquad_m;
	
	results.UmL = 0;
	double biquad_mL = 0;
	const unsigned int x1 = molPosL - 1; //only useful if( molPosL != 0 )
	if( molPosL != 0 && molPosL <= molPosR )
		for( unsigned int y = topL; y <= bottomL; y++ )
			for( unsigned int z = frontR; z <= backR; z++ )
				if( y == topL || z == frontR || y == bottomL || z == backR ) {
					double bond = getLocalM(x1, y, z) * getLocalM(molPosL, y, z);
					results.UmL += bond;
					biquad_mL += sq(bond);
				}
	results.UmL *= -parameters.JmL;
	results.UmL -= parameters.bmL * biquad_mL;
	
	results.UmR = 0;
	double biquad_mR = 0;
	const unsigned int x2 = molPosR + 1; //only useful if( molPosR != width - 1 )
	if( x2 != width && molPosR >= molPosL )
		for( unsigned int y = topL; y <= bottomL; y++ )
			for( unsigned int z = frontR; z <= backR; z++ )
				if( y == topL || z == frontR || y == bottomL || z == backR ) {
					double bond = getLocalM(molPosR, y, z) * getLocalM(x2, y, z);
					results.UmR += bond;
					biquad_mR += sq(bond);
				}
	results.UmR *= -parameters.JmR;
	results.UmR -= parameters.bmR * biquad_mR;
	
	results.ULR = 0;
	double biquad_LR = 0;
	if( molPosL != 0 && x2 != width )
		for( unsigned int z = frontR; z <= backR; z++ )
			for( unsigned int y = topL; y <= bottomL; y++ ) {
				double bond = getLocalM(x1, y, z) * getLocalM(x2, y, z);
				results.ULR += bond;
				biquad_LR += sq(bond);
			}
	results.ULR *= -parameters.JLR;
	results.ULR -= parameters.bLR * biquad_LR;
	 
	results.U = results.UL + results.UR + results.Um + results.UmL + results.UmR + results.ULR;
}

MSD::Results MSD::getResults() const {
	return results;
}


void MSD::setB(const Vector &B) {
	// optimized energy recalculation when only changing magnetic field
	Vector deltaB = B - parameters.B;
	
	results.UL -= deltaB * results.ML;
	results.UR -= deltaB * results.MR;
	results.Um -= deltaB * results.Mm;
	results.U = results.UL + results.UR + results.Um + results.UmL + results.UmR + results.ULR;
	
	parameters.B = B;
}


Vector MSD::getSpin(unsigned int a) const {
	return spins.at(a);
}

Vector MSD::getSpin(unsigned int x, unsigned int y, unsigned int z) const {
	if( x >= width || y >= height || z >= depth )
		throw out_of_range("(x,y,z) coordinate not in range");
	return getSpin( index(x, y, z) );
}

Vector MSD::getFlux(unsigned int a) const {
	return fluxes.at(a);
}

Vector MSD::getFlux(unsigned int x, unsigned int y, unsigned int z) const {
	if( x >= width || y >= height || z >= depth )
		throw out_of_range("(x,y,z) coordinate not in range");
	return getFlux( index(x, y, z) );
}

Vector MSD::getLocalM(unsigned int a) const {
	return getSpin(a) + getFlux(a);
}

Vector MSD::getLocalM(unsigned int x, unsigned int y, unsigned int z) const {
	if( x >= width || y >= height || z >= depth )
		throw out_of_range("(x,y,z) coordinate not in range");
	return getLocalM( index(x, y, z) );
}

void MSD::setSpin(unsigned int a, const Vector &spin) {
	setLocalM( a, spin, getFlux(a) );
}

void MSD::setSpin(unsigned int x, unsigned int y, unsigned int z, const Vector &spin) {
	if( x >= width || y >= height || z >= depth )
		throw out_of_range("(x,y,z) coordinate not in range");
	setSpin( index(x, y, z), spin );
}

void MSD::setFlux(unsigned int a, const Vector &flux) {
	setLocalM( a, getSpin(a), flux );
}

void MSD::setFlux(unsigned int x, unsigned int y, unsigned int z, const Vector &flux) {
	if( x >= width || y >= height || z >= depth )
		throw out_of_range("(x,y,z) coordinate not in range");
	setFlux( index(x, y, z), flux );
}

void MSD::setLocalM(unsigned int a, const Vector &spin, const Vector &flux) {
	try {

	Vector &s = spins.at(a); //previous spin
	Vector &f = fluxes.at(a); //previous spin fluctuation
	
	if( s == spin && f == flux )
		return;
	
	Vector m = s + f; // previous local magnetization
	Vector mag = spin + flux; // new local magnetization
	
	unsigned int x = MSD::x(a);
	unsigned int y = MSD::y(a);
	unsigned int z = MSD::z(a);
	
	Vector deltaM = mag - m;
	results.M += deltaM;
	
	// delta U's are actually negative, simply grouping the negatives in front of each energy coefficient into deltaU -= ... (instead of +=)
	double deltaU_B = parameters.B * deltaM;
	results.U -= deltaU_B;
	
	if( x < molPosL ) {
	
		results.ML += deltaM;
		results.UL -= deltaU_B;
		
		{	double deltaU = parameters.AL * ( Vector(sq(mag.x), sq(mag.y), sq(mag.z)) - Vector(sq(m.x), sq(m.y), sq(m.z)) );
			results.U -= deltaU;
			results.UL -= deltaU;
		}
		
		if( x != 0 ) {
			Vector neighbor = getLocalM(x - 1, y, z);
			double deltaU = parameters.JL * ( neighbor * deltaM )
			              + parameters.bL * ( sq(neighbor * mag) - sq(neighbor * m) );
			results.U -= deltaU;
			results.UL -= deltaU;
		} // else, x - 1 neighbor doesn't exist
		if( y != topL ) {
			Vector neighbor = getLocalM(x, y - 1, z);
			double deltaU = parameters.JL * ( neighbor * deltaM )
			              + parameters.bL * ( sq(neighbor * mag) - sq(neighbor * m) );
			results.U -= deltaU;
			results.UL -= deltaU;
		} // else, y - 1 neighbor doesn't exist
		if( y != bottomL ) {
			Vector neighbor = getLocalM(x, y + 1, z);
			double deltaU = parameters.JL * ( neighbor * deltaM )
			              + parameters.bL * ( sq(neighbor * mag) - sq(neighbor * m) );
			results.U -= deltaU;
			results.UL -= deltaU;
		} // else, y + 1 neighbor doesn't exist
		if( z != 0 ) {
			Vector neighbor = getLocalM(x, y, z - 1);
			double deltaU = parameters.JL * ( neighbor * deltaM )
			              + parameters.bL * ( sq(neighbor * mag) - sq(neighbor * m) );
			results.U -= deltaU;
			results.UL -= deltaU;
		} // else, z - 1 neighbor doesn't exist
		if( z + 1 != depth ) {
			Vector neighbor = getLocalM(x, y, z + 1);
			double deltaU = parameters.JL * ( neighbor * deltaM )
			              + parameters.bL * ( sq(neighbor * mag) - sq(neighbor * m) );
			results.U -= deltaU;
			results.UL -= deltaU;
		} // else, z + 1 neighbor doesn't exist
		
		if( x + 1 != width )
			if( x + 1 == molPosL ) {
				if( molPosR >= molPosL )
					try {
						Vector neighbor = getLocalM(x + 1, y, z);
						double deltaU = parameters.JmL * ( neighbor * deltaM )
						              + parameters.bmL * ( sq(neighbor * mag) - sq(neighbor * m) );
						results.U -= deltaU;
						results.UmL -= deltaU;
					} catch(const out_of_range &e) {} // x + 1 neighbor doesn't exist because it's in the buffer zone
				// else, the molecule doesn't exist
				
				if( molPosR + 1 != width ) {
					try {
						Vector neighbor = getLocalM(molPosR + 1, y, z);
						double deltaU = parameters.JLR * ( neighbor * deltaM )
						              + parameters.bLR * ( sq(neighbor * mag) - sq(neighbor * m) );
						results.U -= deltaU;
						results.ULR -= deltaU;
					} catch(const out_of_range &e) {} // molPosR + 1 atom doesn't exist because we're not in the center
				} // else, the right ferromagnet doesn't exist
			} else {
				Vector neighbor = getLocalM(x + 1, y, z);
				double deltaU = parameters.JL * ( neighbor * deltaM )
				              + parameters.bL * ( sq(neighbor * mag) - sq(neighbor * m) );
				results.U -= deltaU;
				results.UL -= deltaU;
			}
		// else, x + 1 neighbor doesn't exist (because molPosL == width)
		
	} else if( x > molPosR ) {
	
		results.MR += deltaM;
		results.UR -= deltaU_B;
		
		{	double deltaU = parameters.AR * ( Vector(sq(mag.x), sq(mag.y), sq(mag.z)) - Vector(sq(m.x), sq(m.y), sq(m.z)) );
			results.U -= deltaU;
			results.UR -= deltaU;
		}
		
		if( x + 1 != width ) {
			Vector neighbor = getLocalM(x + 1, y, z);
			double deltaU = parameters.JR * ( neighbor * deltaM )
			              + parameters.bR * ( sq(neighbor * mag) - sq(neighbor * m) );
			results.U -= deltaU;
			results.UR -= deltaU;
		} // else, x + 1 neighbor doesn't exist
		if( y != 0 ) {
			Vector neighbor = getLocalM(x, y - 1, z);
			double deltaU = parameters.JR * ( neighbor * deltaM )
			              + parameters.bR * ( sq(neighbor * mag) - sq(neighbor * m) );
			results.U -= deltaU;
			results.UR -= deltaU;
		} // else, y - 1 neighbor doesn't exist
		if( y + 1 != height ) {
			Vector neighbor = getLocalM(x, y + 1, z);
			double deltaU = parameters.JR * ( neighbor * deltaM )
			              + parameters.bR * ( sq(neighbor * mag) - sq(neighbor * m) );
			results.U -= deltaU;
			results.UR -= deltaU;
		} // else, y + 1 neighbor doesn't exist
		if( z != frontR ) {
			Vector neighbor = getLocalM(x, y, z - 1);
			double deltaU = parameters.JR * ( neighbor * deltaM )
			              + parameters.bR * ( sq(neighbor * mag) - sq(neighbor * m) );
			results.U -= deltaU;
			results.UR -= deltaU;
		} // else, z - 1 neighbor doesn't exist
		if( z != backR ) {
			Vector neighbor = getLocalM(x, y, z + 1);
			double deltaU = parameters.JR * ( neighbor * deltaM )
			              + parameters.bR * ( sq(neighbor * mag) - sq(neighbor * m) );
			results.U -= deltaU;
			results.UR -= deltaU;
		} // else, z + 1 neighbor doesn't exist
		
		// x != 0, because x > (unsigned molPosR) >= 0
		if( x - 1 == molPosR ) {
			if( molPosR >= molPosL )
				try {
					Vector neighbor = getLocalM(x - 1, y, z);
					double deltaU = parameters.JmR * ( neighbor * deltaM )
					              + parameters.bmR * ( sq(neighbor * mag) - sq(neighbor * m) );
					results.U -= deltaU;
					results.UmR -= deltaU;
				} catch(const out_of_range &e) {} // x - 1 neighbor doesn't exist because it's in the buffer zone
			// else, the molecule doesn't exist
			
			if( molPosL != 0 ) {
				try {
					Vector neighbor = getLocalM(molPosL - 1, y, z);
					double deltaU = parameters.JLR * ( neighbor * deltaM )
					              + parameters.bLR * ( sq(neighbor * mag) - sq(neighbor * m) );
					results.U -= deltaU;
					results.ULR -= deltaU;
				} catch(const out_of_range &e) {} // molPos - 1 atom doesn't exist because we're not in the center
			} // else, the left ferromagnet doesn't exist
		} else {
			Vector neighbor = getLocalM(x - 1, y, z);
			double deltaU = parameters.JR * ( neighbor * deltaM )
			              + parameters.bR * ( sq(neighbor * mag) - sq(neighbor * m) );
			results.U -= deltaU;
			results.UR -= deltaU;
		}
		
	} else { // molPosL <= x <= molPosR
	
		results.Mm += deltaM;
		results.Um -= deltaU_B;
		
		{	double deltaU = parameters.Am * ( Vector(sq(mag.x), sq(mag.y), sq(mag.z)) - Vector(sq(m.x), sq(m.y), sq(m.z)) );
			results.U -= deltaU;
			results.Um -= deltaU;
		}
		
		if( x != 0 ) {
			Vector neighbor = getLocalM(x - 1, y, z);
			if( x == molPosL ) {
				double deltaU = parameters.JmL * ( neighbor * deltaM )
				              + parameters.bmL * ( sq(neighbor * mag) - sq(neighbor * m) );
				results.U -= deltaU;
				results.UmL -= deltaU;
			} else {
				double deltaU = parameters.Jm * ( neighbor * deltaM )
				              + parameters.bm * ( sq(neighbor * mag) - sq(neighbor * m) );
				results.U -= deltaU;
				results.Um -= deltaU;
			}
		}
		
		if( x + 1 != width ) {
			Vector neighbor = getLocalM(x + 1, y, z);
			if( x == molPosR ) {
				double deltaU = parameters.JmR * ( neighbor * deltaM )
				              + parameters.bmR * ( sq(neighbor * mag) - sq(neighbor * m) );
				results.U -= deltaU;
				results.UmR -= deltaU;
			} else {
				double deltaU = parameters.Jm * ( neighbor * deltaM )
				              + parameters.bm * ( sq(neighbor * mag) - sq(neighbor * m) );
				results.U -= deltaU;
				results.Um -= deltaU;
			}
		}
		
	}
	
	s = spin;
	f = flux;
	// setParameters(parameters); //recalculate results (the long way!!!)

	} catch(const out_of_range &ex) {
		// For debugging. This exception should not happen in production!
		std::cerr << "ERROR in MSD::setLocalM(unsigned int, udc::Vector, udc::Vector)\n";
		std::cerr << a << " == (" << x(a) << ", " << y(a) << ", " << z(a) << ")\n";
		std::cerr << ex.what() << "\n\n";
		std::cerr << "topL=" << topL << ", bottomL=" << bottomL << ", frontR=" << frontR << ", backR=" << backR << '\n';
		std::cerr << "Valid Indicies:\n";
		for (auto iter = indices.begin(); iter != indices.end(); ++iter)
			std::cerr << *iter << " == (" << x(*iter) << ", " << y(*iter) << ", " << z(*iter) << ")\n";
		exit(200);
	}
}

void MSD::setLocalM(unsigned int x, unsigned int y, unsigned int z, const Vector &spin, const Vector &flux) {
	if( x >= width || y >= height || z >= depth )
		throw out_of_range("(x,y,z) coordinate not in range");
	setLocalM( index(x, y, z), spin, flux );
}


unsigned int MSD::getN() const {
	return n;
}

unsigned int MSD::getWidth() const {
	return width;
}

unsigned int MSD::getHeight() const {
	return height;
}

unsigned int MSD::getDepth() const {
	return depth;
}

void MSD::getDimensions(unsigned int &width, unsigned int &height, unsigned int &depth) const {
	width = this->width;
	height = this->height;
	depth = this->depth;
}

unsigned int MSD::getMolPosL() const {
	return molPosL;
}

unsigned int MSD::getMolPosR() const {
	return molPosR;
}

void MSD::getMolPos(unsigned int &molPosL, unsigned int &molPosR) const {
	molPosL = this->molPosL;
	molPosR = this->molPosR;
}

unsigned int MSD::getTopL() const {
	return topL;
}

unsigned int MSD::getBottomL() const {
	return bottomL;
}

unsigned int MSD::getFrontR() const {
	return frontR;
}

unsigned int MSD::getBackR() const {
	return backR;
}

void MSD::getInnerBounds(unsigned int &topL, unsigned int &bottomL, unsigned int &frontR, unsigned int &backR) const{
	topL = this->topL;
	bottomL = this->bottomL;
	frontR = this->frontR;
	backR = this->backR;
}


void MSD::setSeed(unsigned long seed) {
	this->seed = seed;
	prng.seed(seed);
}

unsigned long MSD::getSeed() const {
	return seed;
}


void MSD::reinitialize(bool reseed) {
	if( reseed )
		seed = genSeed();
	prng.seed(seed);
	for( auto i = begin(); i != end(); i++ )
		setLocalM( i, initSpin, initFlux );
	record.clear();
	setParameters(parameters);
	results.t = 0;
}

void MSD::randomize(bool reseed) {
	if( reseed )
		seed = genSeed();
	prng.seed(seed);

	// so that setParameters scales the flux magnitudes correctly
	Parameters p0 = getParameters();
	Parameters pTemp = p0;
	pTemp.FL = pTemp.FR = pTemp.Fm = 1;
	setParameters(pTemp);

	for( auto i = begin(); i != end(); i++ )
		setLocalM( i,
				Vector::sphericalForm(1, 2 * PI * rand(prng), asin(2 * rand(prng) - 1)),
				Vector::sphericalForm(rand(prng), 2 * PI * rand(prng), asin(2 * rand(prng) - 1)) );
	record.clear();
	setParameters(p0);
	results.t = 0;
}

void MSD::metropolis(unsigned long long N) {
	function<double()> random = bind( rand, ref(prng) );
	Results r = getResults(); //get the energy of the system
	//start loop (will iterate N times)
	for( unsigned long long i = 0; i < N; i++ ) {
		unsigned int a = indices[static_cast<unsigned int>( random() * indices.size() )]; //pick an atom (pseudo) randomly
		Vector s = getSpin(a);
		Vector f = getFlux(a);

		// pick the correct F coeficient to determine new flux magnitude
		unsigned int x = this->x(a);
		double F = (x < molPosL ? parameters.FL : x > molPosR ? parameters.FR : parameters.Fm);

		//"flip" that atom
		setLocalM( a, flippingAlgorithm(s, random),
				Vector::sphericalForm(F * random(), 2 * PI * random(), asin(2 * random() - 1)) );
		
		Results r2 = getResults(); //get the energy of the system (for the new state)
		if( r2.U <= r.U || random() < pow( E, (r.U - r2.U) / parameters.kT ) ) {
			//either the new system requires less energy or external energy (kT) is disrupting it
			r = r2; //in either case we keep the new system
		} else {
			//neither thing (above) happened so we revert the system
			spins[a] = s; //revert the system by flipping the atom back
			fluxes[a] = f;
			results = r;
		}
	}
	results.t += N;
}

void MSD::metropolis(unsigned long long N, unsigned long long freq) {
	if( freq == 0 ) {
		metropolis(N);
		return;
	}
	while(true) {
		record.push_back( getResults() );
		if( N >= freq ) {
			metropolis(freq);
			N -= freq;
		} else {
			if( N != 0 )
				metropolis(N);
			break;
		}
	}
}


double MSD::specificHeat() const {
	double avg = 0, avgSq = 0;
	for( auto iter = record.begin(); iter != record.end(); iter++ ) {
		avg += iter->U;
		avgSq += iter->U * iter->U;
	}
	avg /= record.size();
	avgSq /= record.size();
	return (avgSq - avg * avg) / n / parameters.kT / parameters.kT;
}

double MSD::specificHeat_L() const {
double avg = 0, avgSq = 0;
	for( auto iter = record.begin(); iter != record.end(); iter++ ) {
		avg += iter->UL;
		avgSq += iter->UL * iter->UL;
	}
	avg /= record.size();
	avgSq /= record.size();
	return (avgSq - avg * avg) / n / parameters.kT / parameters.kT;
}

double MSD::specificHeat_R() const {
	double avg = 0, avgSq = 0;
	for( auto iter = record.begin(); iter != record.end(); iter++ ) {
		avg += iter->UR;
		avgSq += iter->UR * iter->UR;
	}
	avg /= record.size();
	avgSq /= record.size();
	return (avgSq - avg * avg) / n / parameters.kT / parameters.kT;
}

double MSD::specificHeat_m() const {
	double avg = 0, avgSq = 0;
	for( auto iter = record.begin(); iter != record.end(); iter++ ) {
		avg += iter->Um;
		avgSq += iter->Um * iter->Um;
	}
	avg /= record.size();
	avgSq /= record.size();
	return (avgSq - avg * avg) / n / parameters.kT / parameters.kT;
}

double MSD::specificHeat_mL() const {
	double avg = 0, avgSq = 0;
	for( auto iter = record.begin(); iter != record.end(); iter++ ) {
		avg += iter->UmL;
		avgSq += iter->UmL * iter->UmL;
	}
	avg /= record.size();
	avgSq /= record.size();
	return (avgSq - avg * avg) / n / parameters.kT / parameters.kT;
}

double MSD::specificHeat_mR() const {
	double avg = 0, avgSq = 0;
	for( auto iter = record.begin(); iter != record.end(); iter++ ) {
		avg += iter->UmR;
		avgSq += iter->UmR * iter->UmR;
	}
	avg /= record.size();
	avgSq /= record.size();
	return (avgSq - avg * avg) / n / parameters.kT / parameters.kT;
}

double MSD::specificHeat_LR() const {
	double avg = 0, avgSq = 0;
	for( auto iter = record.begin(); iter != record.end(); iter++ ) {
		avg += iter->ULR;
		avgSq += iter->ULR * iter->ULR;
	}
	avg /= record.size();
	avgSq /= record.size();
	return (avgSq - avg * avg) / n / parameters.kT / parameters.kT;
}

double MSD::magneticSusceptibility() const {
	Vector avg = Vector::ZERO;
	double avgSq = 0;
	for( auto iter = record.begin(); iter != record.end(); iter++ ) {
		avg += iter->M;
		avgSq += iter->M * iter->M;
	}
	avg *= 1.0 / record.size();
	avgSq /= record.size();
	return n * (avgSq - avg * avg) / parameters.kT;
}

double MSD::magneticSusceptibility_L() const {
	Vector avg = Vector::ZERO;
	double avgSq = 0;
	for( auto iter = record.begin(); iter != record.end(); iter++ ) {
		avg += iter->ML;
		avgSq += iter->ML * iter->ML;
	}
	avg *= 1.0 / record.size();
	avgSq /= record.size();
	return n * (avgSq - avg * avg) / parameters.kT;
}

double MSD::magneticSusceptibility_R() const {
	Vector avg = Vector::ZERO;
	double avgSq = 0;
	for( auto iter = record.begin(); iter != record.end(); iter++ ) {
		avg += iter->MR;
		avgSq += iter->MR * iter->MR;
	}
	avg *= 1.0 / record.size();
	avgSq /= record.size();
	return n * (avgSq - avg * avg) / parameters.kT;
}

double MSD::magneticSusceptibility_m() const {
	Vector avg = Vector::ZERO;
	double avgSq = 0;
	for( auto iter = record.begin(); iter != record.end(); iter++ ) {
		avg += iter->Mm;
		avgSq += iter->Mm * iter->Mm;
	}
	avg *= 1.0 / record.size();
	avgSq /= record.size();
	return n * (avgSq - avg * avg) / parameters.kT;
}


Vector MSD::meanM() const {
	if( record.size() <= 1 )
		return record.at(0).M;
	//compensates for t by using the trapezoidal rule; (and the same below)
	Vector s = Vector::ZERO;
	for( unsigned int i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.M + r1.M);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanML() const {
	if( record.size() <= 1 )
		return record.at(0).ML;
	Vector s = Vector::ZERO;
	for( unsigned int i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.ML + r1.ML);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanMR() const {
	if( record.size() <= 1 )
		return record.at(0).MR;
	Vector s = Vector::ZERO;
	for( unsigned int i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.MR + r1.MR);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

Vector MSD::meanMm() const {
	if( record.size() <= 1 )
		return record.at(0).Mm;
	Vector s = Vector::ZERO;
	for( unsigned int i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.Mm + r1.Mm);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanU() const {
	if( record.size() <= 1 )
		return record.at(0).U;
	double s = 0;
	for( unsigned int i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.U + r1.U);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanUL() const {
	if( record.size() <= 1 )
		return record.at(0).UL;
	double s = 0;
	for( unsigned int i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.UL + r1.UL);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanUR() const {
	if( record.size() <= 1 )
		return record.at(0).UR;
	double s = 0;
	for( unsigned int i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.UR + r1.UR);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanUm() const {
	if( record.size() <= 1 )
		return record.at(0).Um;
	double s = 0;
	for( unsigned int i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.Um + r1.Um);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanUmL() const {
	if( record.size() <= 1 )
		return record.at(0).UmL;
	double s = 0;
	for( unsigned int i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.UmL + r1.UmL);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanUmR() const {
	if( record.size() <= 1 )
		return record.at(0).UmR;
	double s = 0;
	for( unsigned int i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.UmR + r1.UmR);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}

double MSD::meanULR() const {
	if( record.size() <= 1 )
		return record.at(0).ULR;
	double s = 0;
	for( unsigned int i = 1; i < record.size(); i++ ) {
		const Results &r0 = record[i - 1];
		const Results &r1 = record[i];
		s += (r1.t - r0.t) * (r0.ULR + r1.ULR);
	}
	return (0.5 / (record[record.size() - 1].t - record[0].t)) * s;
}


MSD::Iterator MSD::begin() const {
	return Iterator(*this, 0);
}

MSD::Iterator MSD::end() const {
	return Iterator(*this, indices.size());
}

} //end of namespace

#endif

(() => {  // define IIFE

// ---- Imports: --------------------------------------------------------------
const { defineExports } = MSDBuilder.util;


// ---- Classes: --------------------------------------------------------------
class Vector extends Array {
	/**
	 * @param {...Number} xyz - Contains 3 elements: [x, y, z].
	 * 	Extra elements will get stored, but ignored otherwise.
	 *  Missing elements will be filled with 0.
	 *  All parameters will be converted to the Number type.
	 */
	constructor(...xyz) {
		for (let x of xyz)
			this.push(+x);
		while (this.length < 3)
			this.push(0);
	}

	static zero = () => { Vector(); }
	static i = () => { Vector(1, 0, 0); }
	static j = () => { Vector(0, 1, 0); }
	static k = () => { Vector(0, 0, 1); }

	get x() { return this[0]; }
	get y() { return this[1]; }
	get z() { return this[2]; }
	set x(x) { this[0] = x; }
	set y(y) { this[1] = y; }
	set z(z) { this[2] = z; }
}

/**
 * Contains data in a format that can be set to MSD.py for execution.
 * Also stores data send from MSD.py upon completion.
 */
class Simulation {
	constructor({
		width, height, depth,
		molPosL = null, molPosR = null,
		topL = null, bottomL = null, frontR = null, backR = null,
		molLen = null, heightL = null, depthR = null
	}) {
		// assign each of the parameters 
		Object.assign(this, (obj => {
			for (let prop in obj)
				if (obj[prop] !== null)
					obj[prop] = +obj[prop];
		})({
			width, height, depth,
			molPosL, molPosR,
			topL, bottomL, frontR, backR,
			molLen, heightL, depthR
		}));
		this.parameters = {
			kT: 0.25,
			B: Vector(0, 0, 0),
			SL: 1,   SR: 1,
			FL: 1,   FR: 1,
			JL: 1,   JR: 1,   JmL: 1,   JmR: -1,  JLR: 0,
			Je0L: 0, Je0R: 0,
			Je1L: 0, Je1R: 0, Je1mL: 0, Je1mR: 0, Je1LR: 0,
			JeeL: 0, JeeR: 0, JeemL: 0, JeemR: 0, JeeLR: 0,
			bL: 0,   bR: 0,   bmL: 0,   bmR: 0,   bLR: 0,
			AL: Vector.zero(), AR: Vector.zero(),
			DL: Vector.zero(), DR: Vector.zero(), DmL: Vector.zero(), DmR: Vector.zero(), DLR: Vector.zero()
		};
		this.mol = {
			nodes: [],
			edges: []
		};
};


// ---- Exports: --------------------------------------------------------------
defineExports("MSDBuilder.simulation", { /* TODO */ });

})();  // run IIFE
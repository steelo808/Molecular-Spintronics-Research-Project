(() => {  // define IIFE

// ---- Imports: --------------------------------------------------------------
const { defineExports, ajax, sleep } = MSDBuilder.util;


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
		// TODO: ...
		this.mol = {
			nodes: [],
			edges: []
		};
	}
}

/**
 * An asynchronous interface bewteen the frontend (Javascript) and the backend (MSD Server).
 * TODO: Store/cache retrieved info?
 */
class MSD {
	server;
	id = null;

	/** @private */
	constructor(server) {
		this.server = server;
	}

	/**
	 * @private
	 * @param {String} method - HTTP Method; same as ajax()
	 * @param {String} path - Appended to <code>this.server</code> root.
	 * @param {Object?} body - Stringified into JSON
	 * @return {Object?} respnseText parsed into JSON, if a response body exists.
	 */
	async request(method, path, body) {
		// set up request
		let args = { url: this.server + path, method };
		if (body) {
			Object.assign(args, {
				content_type: "application/json",
				body: JSON.stringify(body) });
		}

		// send request. parse response as JSON.
		return await ajax(args)
			.catch(({ status, statusText, responseText }) => {
				throw new Error(`${status} ${statusText}\n${responseText}`)
			
			}).then(xmlHttpRequest => {
				let { status, responseText } = xmlHttpRequest;
				if (Math.floor(status / 100) !== 2)
					throw xmlHttpRequest;
				if (responseText)
					return JSON.parse(responseText);
			});
	}

	/**
	 * @public
	 */
	static async create(args, server = "http://localhost:8080") {
		let msd = new MSD(server);
		await msd.request("POST", "/msd", args)
			.then(({ id }) => msd.id = id);
		return msd;
	}

	/**
	 * @public
	 * @async
	 * @param {Object} args
	 * @param {Number} args.simCount - (int) (required) Number of iterations to run
	 * @param {Number} args.freq - (int) (optional) How often (in iterations) to record data
	 * @param {Number} args.dkT - (float) (optional) Linear change in tempurature (kT) per iteration
	 * @param {Number[]} args.dB - (float[]) (optional) Linear change in external magnetic field (B) per iteration
	 */
	async run(args) {
		await this.request("POST", "/run?id=" + this.id, args);
	}

	/**
	 * @public
	 */
	get record() {
		let self = this;
		return {
			async length() {
				return await self.request("GET", "/results?id=" + self.id)
					.then(({ length }) => length);
			},

			async get(index) {
				return await self.request("GET", `/results?id=${self.id}&index=${index}`);
			},

			async range(start, end) {
				return await self.request("GET", `/results?id=${self.id}&start=${start}&end=${end}`);
			},

			async all() {
				return await self.request("GET", `/results?id=${self.id}&all`);
			}
		};
	};

	/**
	 * @public
	 * @returns {Object} Current state of MSD (TODO: add details of JSON structure)
	 */
	async getState() {
		return await this.request("GET", "/msd?id=" + this.id);
	}

	/**
	 * @public
	 * @param {Object} p - Parameters to change
	 */
	async setParameters(p) {
		return await this.request("PATCH", "/msd?id=" + this.id, p);
	}

	/**
	 * @public
	 */
	async destory() {
		await this.request("DELETE", "/msd?id=" + this.id)
			.catch(MSD.throwError);
	}
}


// ---- Functions: ------------------------------------------------------------
const iterate = async (createArgs, runArgs) => {
	console.log(createArgs, runArgs);
	const id = `[iterate, ${++iterate.nextId}]`;
	console.time(id);
	try {
		console.log(`${id} Workload started...`);
		let msd = await MSD.create(createArgs);
		console.log(id, "Created MSD:", msd);
		console.log(id, "Start running simulation.");
		await msd.run(runArgs);
		let index = -1;
		let t = -1;	
		while (t < simCount) {
			await sleep(1000);
			let length = await msd.record.length();
			while(true) {
				let next = index + 1;
				if (next < length) {
					index = next;
					let state = await msd.record.get(index);
					console.log(id, `Result [${index}]`, state);
					t = state.results.t;
				} else {
					break;
				}
			}
		}
		console.log(id, "Destoryed MSD:", msd);
		msd.destory();
		console.log(id, "Workload Complete.");
	} catch(ex) {
		console.error(ex);
	}
	console.timeEnd(id);
};
iterate.nextId = 0;

const demo = async () => {
	console.time("MSD workload");
	try {
		console.log("Workload started...");
		let msd = await MSD.create({
			width: 11,  height: 10,  depth: 10,
			
			molPosL: 5,  molPosR: 5,
			topL: 0,  bottomL: 9,
			frontR: 0,  backR: 9,

			kT: 0.3,
			B: [0, 0, 0],

			SL: 1,  SR: 1,  Sm: 1,
			FL: 0,  FR: 0,  Fm: 0,

			JL: 1,  JR: 1,  Jm: 1,  JmL: 0.75,  JmR: -0.75,  JLR: 0,
			Je0L: 0,  Je0R: 0,  Je0m: 0,
			Je1L: 0,  Je1R: 0,  Je1m: 0,  Je1mL: 0,  Je1mR: 0, Je1LR: 0,
			JeeL: 0,  JeeR: 0,  Jeem: 0,  JeemL: 0,  JeemR: 0, JeeLR: 0,
			bL: 0,  bR: 0,  bm: 0,  bmL: 0,  bmR: 0,  bLR: 0,

			AL:[0,0,0], AR:[0,0,0], Am:[0,0,0],
			DL:[0,0,0], DR:[0,0,0], Dm:[0,0,0], DmL:[0,0,0], DmR:[0,0,0], DLR:[0,0,0],
			
			flippingAlgorithm: "CONTINUOUS_SPIN_MODEL",
			molType: "LINEAR",
			randomize: true,
			seed: 0
		});
		console.log("-- Created MSD:", msd);
		console.log("-- Start running simulation.");
		const simCount = 50_000_000;
		await msd.run({
			simCount,
			freq: 1_000_000
		});
		let prevIndex = -1;
		let t = -1;	
		while (t < simCount) {
			await sleep(1000);
			let index = await msd.record.length() - 1;
			if (index > prevIndex) {
				prevIndex = index;
				let state = await msd.record.get(index);
				console.log(`Result [${index}]`, state);
				t = state.results.t;
			}
		}
		console.log("-- Destoryed MSD:", msd);
		msd.destory();
		console.log("==== Workload Complete. ====");
	} catch(ex) {
		console.error(ex);
	}
	console.timeEnd("MSD workload");
};


// ---- Exports: --------------------------------------------------------------
defineExports("MSDBuilder.simulation", { MSD, iterate, demo });

})();  // run IIFE
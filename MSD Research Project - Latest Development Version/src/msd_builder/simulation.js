/**
 * @file simulation.js
 * @brief Contains logic related to comunicating with the MSD Server.
 * @author Christopher D'Angelo
 */

(function() {  // IIFE

	// ---- Imports: --------------------------------------------------------------
const { AsyncPool, defineExports, ajax, sleep } = MSDBuilder.util;


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
 * An asynchronous interface bewteen the frontend (Javascript) and the backend (MSD Server).
 * TODO: Store/cache retrieved info?
 */
class MSD {
	server;
	id = null;
	t = 0;
	timeoutMap = new Map();

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
	static async create(args, server = "http://localhost:8082") {
		let msd = new MSD(server);
		await msd.request("POST", "/msd", args)
			.then(({ id }) => msd.id = id);
		return msd;
	}

	/**
	 * Dispatches a job to the server. Returns once the job has been
	 * confirmed, but before the simulation is finished.
	 * @public
	 * 
	 * @param {Object} args
	 * @param {Number} args.simCount - (int) (required) Number of iterations to run
	 * @param {Number} args.freq - (int) (optional) How often (in iterations) to record data
	 * @param {Number} args.dkT - (float) (optional) Linear change in tempurature (kT) per iteration
	 * @param {Number[]} args.dB - (float[]) (optional) Linear change in external magnetic field (B) per iteration
	 */
	async run(args) {
		this.t += args.simCount;
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

			/**
			 * @param {Number} start (inclusive) index
			 * @param {Number} end (exclusive) index
			 * @returns JSON Array of states
			 */
			async range(start, end) {
				return await self.request("GET", `/results?id=${self.id}&start=${start}&end=${end}`);
			},

			async all() {
				return await self.request("GET", `/results?id=${self.id}&all`);
			},

			[Symbol.asyncIterator]: async function*() {
				const length = await this.length();
				for (let i = 0; i < length; i++)
					yield this.get(i);
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
	 * Reset this MSD to an intial state. Also clears the record.
	 * @public
	 * 
	 * @param {Object} options
	 * 	Defaults to empty Object, {}
	 * 	By default the MSD is not randomized, but is reseeded.
	 * @param {Boolean?} options.randomize
	 * 	Set <code>true</code> if the system should be reset to a random state
	 * @param {Boolean?} options.reseed
	 * 	Set <code>false</code> if the PRNG seed should not be changed.
	 * 	Otherwise a new unique seed will be generated.
	 * @param {Number?} options.seed
	 * 	Use if PRNG should be reset with a specific seed.
	 * 	Will override the reseed option.
	 * @returns {Object} { seed } Contains whatever seed is now being used
	 */
	async reset(options = {}) {
		this.t = 0;
		return await this.request("POST", "/reset?id=" + this.id, options);
	}

	/**
	 * @public
	 */
	async destory() {
		await this.request("DELETE", "/msd?id=" + this.id)
			.catch(MSD.throwError);
	}

	/**
	 * @public
	 * @param {Function} callback A function that's called when the simulation reaches the specified "time".
	 * 	By default, this is set to the sum of all previous calls to {@link #run}.
	 * @param {Object?} options
	 * @param {Number?} options.until What simulation time the MSD is expected to end at.
	 * @param {Number?} options.delay How often (in milliseconds) to query the server for new states.
	 * @param {Number?} options.start What index to start processing.
	 * 	Default is 0 which means all previous states are processed.
	 * @param {Function?} options.process A function to call on each new state as they come in.
	 */
	addFinishListener(callback, { until = this.t, delay = 1000, start = 0, process = null } = {}) {
		if (!callback)
			throw new Error("No callback function given: " + callback);

		let prevLen = start;
		const loop = async () => {
			let len = await this.record.length();
			if (len > prevLen)  {  // has the record gotten longer?

				// get missing records for processing
				let lastState;
				if (process) {
					let states = await this.record.range(prevLen, len);
					states.forEach((s, i) => process(s, prevLen + i));
					lastState = states[states.length - 1];
				} else {
					lastState = await this.record.get(-1);
				}
				prevLen = len;

				if (lastState.results.t >= until) {  // have we reached time = "until"?
					callback(lastState, len - 1);  // onFinish
					this.timeoutMap.delete(callback);  // delete tid
					return;  // don't setTimeout
				}
			}

			// still waiting; setTimeout, and store tid in Map
			this.timeoutMap.set(callback, setTimeout(loop, delay));
		};
		loop();  // check immediately
	}

	/**
	 * @public
	 * @param {Function} callback 
	 */
	removeFinishListener(callback) {
		let tid = this.timeoutMap.get(callback);
		if (tid)
			clearTimeout(tid);
	}

	/**
	 * @public
	 * @async
	 * @param {Function?} options.process A function to call on each new state as they come in.
	 * @param {Object?} options
	 * @param {Number?} options.until What simulation time the MSD is expected to end at.
	 * @param {Number?} options.delay How often (in milliseconds) to query the server for new states.
	 */
	wait(process = null, options = {}) {
		return new Promise((resolve, reject) =>
			this.addFinishListener(resolve, {...options, process}) );
	}
}

/**
 * Base class for running multiple related simulations as a single "group".
 */
class MSDGroup {
	threadCount;  // Max number of threads
	msds;  // Array of MSD objects

	/** @private */
	constructor() {}

	/**
	 * @public
	 * @param {Number} threadCount - Maximum number of active run requests for this group.
	 * @param {Array[Object]} createArgs - Array of createArgs
	 * @return {MSDGroup}
	 */
	static async create(threadCount, createArgs) {
		let group = new MSDGroup();

		group.threadCount = threadCount;

		let len = createArgs.length;
		group.msds = new Array(len);
		for (let i = 0; i < len; i++)
			group.msds[i] = MSD.create(createArgs[i])  // store Promise
		for (let i = 0; i < len; i++)
			group.msds[i] = await group.msds[i];  // await for all MSDs to be created

		return group;
	}

	/**
	 * @public
	 * Depth-First: Run to completion this.threadCount MSD simulations.
	 * Only run new simulations as previous ones finish.
	 *  
	 * @param {Array[Object]} runArgs - Array of runArgs
	 * @param {Number} delay - (optional) Time (in ms) to wait between
	 * 	HTTP requests while waiting for simulations to finish.
	 */
	async runD(runArgs, delay = 1000) {
		let pool = new AsyncPool(this.threadCount);

		// queue runs
		for (let msd of this.msds)
			pool.do(() => msd.run(runArgs[i]));

		await pool.join();  // await for pool to finish all tasks
	}

	// TODO: can we add and remove from the group after it's constructed and potentially run simulations??

	// TODO: add methods for aggrigating results??
}

/**
 * Runs multiple repatitions of the same system (different seeds)
 * so averages can be calculated.
 */
class Ensemble extends MSDGroup {
	// TODO ...
}

/**
 * Runs multiple repatitions of the system with some variables controlled
 * (i.e., constant), and some independant (i.e., varied) to study their effect.
 */
class ExperimentalGroup extends MSDGroup {
	// TODO ...
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
		await msd.wait((state, index) => console.log(id, `Result [${index}]`, state));
		console.log(id, "Destoryed MSD:", msd);
		msd.destory();
		console.log(id, "Complete.");
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

})();  // end IIFE
/**
 * @file util.js
 * @brief Utilities
 * @author Christopher D'Angelo
 */

(function() {  // IIFE

// ---- Imports: --------------------------------------------------------------
// (None)


// ---- Classes: ------------------------------------------------------------
/**
 * Controlls a set of async tasks, allowing only a limited number to run at any
 * one time. As each async functions finishes, the next will automatically, and
 * concurrently be executed.
 */
class AsyncPool {

	/**
	 * @public
	 * @param {Number} size - Max number of async tasks that can simultaneously execute.
	 * @param {Function} then - Used to respond to completing tasks as they complete.
	 * 	Takes (return, func, ...args)
	 * @param {Function} _catch - (optional) Used to respond to taaks as they fail.
	 * 	Takes (error, func, ...args)
	 */
	constructor(size, then, _catch = (ex, f, ...args) => {
		throw new Error(`Unhandled ${ex} while executing ${f.name} of ${args}`); }
	) {
		this._size = size;  // total number of tasks that can be run concurrently
		this._count = 0;    // how many tasks are running?
		this._queue = [];   // tasks that are not yet running
		this._promises = [];  // Array of Promise objects, one for each _executeQueue invocation

		this.then = then;
		this.catch = _catch;
	}

	/** @public */
	get availble() {
		return Math.min(this.size - this.count, this.queue.length);
	}

	/** @private */
	async _executeQueue() {
		do {
			this._count++;
			let { f, args } = this.queue.shift();
			try {
				let result = await f(...args);
				this.then(result, f, ...args);
			} catch(ex) {
				this.catch(ex, f, ...args);
			}
			this._count--;
		} while(this.available > 0);
	}

	/** @private */ _runOne() { this._promises.push(this._executeQueue()); }
	/** @private */ _run(n) { for (let i = 0; i < n; i++)  this._runOne(); }

	/** @public */ runOne() { if (this.availble > 0)  this._runOne(); }
	/** @public */ run(n) { this._run(Math.min(n, this.availble)); }
	/** @public */ runAll() { this._run(this.availble); }

	/** @param {Function} f */
	do(f, ...args) {
		this.queue.push({ f, args } );
		this.runOne();
	}

	/** @public */
	async join() {
		await Promise.allSettled(this._promises);
		this._promises = [];
	}

	/** @public */
	set size(size) {
		let growth = size - this.size;  // are we growing?
		this.size = size;  // update size
		this.run(growth);  // start up n new threads if they are now available
	}

	/** @public */ get size() { return this.size; }
	/** @public */ get count() { return this.count; }
	/** @public */ get queue() { return this.queue; }
}

class Map2 extends Map {
	constructor(...args) {
		super(...args);
	}

	/** @public */
	remove(key) {
		let value = this.get(key);
		this.delete(key);
		return value;
	}
}

// ---- Functions: ------------------------------------------------------------
/**
 * Define exports for a JS library.
 * The given exports will be stored in the global object; i.e., window.
 * 
 * @param {String} name - The package or library name.
 * @param {Object} exports - A set of identifiers (e.g., classes, functions, variables) to export.
 */
const defineExports = (name, exports, global = window) => {
	if (!name)  throw Error("Must provide a package or library name when defining exports.");
	for (let x of name.split(".")) {
		if (!global[x])  global[x] = {};
		global = global[x];
	}
	Object.assign(global, exports);
};

/**
 * @async
 * 
 * @param {Object} args
 * @param {String} args.url
 * @param {String} args.method - HTTP Method. E.g., GET, POST, DELETE
 * @param {String?} args.body - (optional) HTTP Request body. Used in (e.g.) POST requests.
 * @param {String?} args.content_type - (optional) Should be included if <code>(body)</code>. E.g., application/json
 * @param {String?} args.username
 * @param {String?} args.password
 * 
 * @returns {Promise}
 * 	Which will either <code>resolve</code> or <code>reject</code>
 * 	an <code>XMLHttpRequest</code> object.
 */
function ajax({ url, method, body, content_type, username, password }) {
	return new Promise((resolve, reject) => {
		let req = new XMLHttpRequest();
		req.open(method, url, true, username, password);
		req.addEventListener("load", () => resolve(req));
		req.addEventListener("error", () => reject(req));
		req.addEventListener("abort", () => reject(req));
		if (body && content_type)
			req.setRequestHeader("Content-type", content_type);
		req.send(body);
	});
}

/**
 * @async
 * @param {Number} ms
 */
function sleep(ms) {
	let { promise, resolve, reject } = Promise.withResolvers();
	promise.timeoutID = setTimeout(resolve, ms);
	promise.interrupt = () => {
		clearTimeout(promise.timeoutID);
		reject(new Error("Sleep interrupted"));
	};
	return promise;
}

// ---- Exports: --------------------------------------------------------------
defineExports("MSDBuilder.util", { AsyncPool, Map2, defineExports, ajax, sleep });

})();  // end IIFE
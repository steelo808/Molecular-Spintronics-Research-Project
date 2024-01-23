(function() {  // IIFE

// ---- Imports: --------------------------------------------------------------
// (None)


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
defineExports("MSDBuilder.util", { defineExports, ajax, sleep });

})();  // end IIFE
(() => {  // define IIFE

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


// ---- Exports: --------------------------------------------------------------
defineExports("MSDBuilder.util", { defineExports });

})();  // run IIFE
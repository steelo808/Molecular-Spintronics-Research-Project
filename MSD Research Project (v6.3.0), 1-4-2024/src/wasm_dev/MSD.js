// Node.js module for MSD
const Module = require("./MSD-emscripten-module.js");

const createMSD_i = Module.cwrap("createMSD_i", "number", [
		"number", "number", "number",
		"number", "number",
		"number", "number","number", "number"
]);
const destroyMSD = Module.cwrap("destroyMSD", null, ["number"]);

// const metropolis_o = Module.cwrap("metropolis_o", null, ["number", "number"]);


let msd = createMSD_i(11, 10, 10, 5, 5, 3, 7, 3, 7);
// metropolis_o(msd, 10);
// destroyMSD(msd);

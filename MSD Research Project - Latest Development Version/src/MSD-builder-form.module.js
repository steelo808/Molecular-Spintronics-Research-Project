(() => {  // define IIFE

// ---- Imports: --------------------------------------------------------------
const { defineExports } = MSDBuilder.util;
const { updateCamera } = MSDBuilder.render;


// ---- Classes: --------------------------------------------------------------
class SavedMap extends Map {
	/**
	 * @param {Storage} storage Either localStorage or sessionStorage, or similar object.
	 * @param {String} name Key for saving this object in given storage.
	 */ 
	constructor(storage, name, { autoSave = true, autoLoad = true } = {}) {
		super();
		this.storage = storage;
		this.name = name;
		this.autoSave = autoSave;
		if (autoLoad)
			this.load();
	}

	save() {
		this.storage.setItem(this.name, JSON.stringify([...this]));
	}

	load() {
		const map = this.storage.getItem(this.name);
		if (map !== null)
			JSON.parse(map)?.forEach(([ key, value ]) => { super.set(key, value) });
	}

	clear() {
		super.clear();
		if (this.autoSave)
			this.save();
	}

	set(key, value) {
		super.set(key, value);
		if (this.autoSave)
			this.save();
	}

	delete(key) {
		super.delete(key);
		if (this.autoSave)
			this.save();
	}
};


// ---- Globals: --------------------------------------------------------------
const DEFAULTS = {
	DIM_FIELDS: new Map([
		["FML-width", 5], ["FML-height", 4], ["FML-depth", 10], ["FML-y", 0],
		["FMR-width", 5], ["FMR-height", 10], ["FMR-depth", 4], ["FMR-z", 0],
		["mol-width", 1], ["mol-height", 4], ["mol-depth", 4], ["mol-y", 0], ["mol-z", 0]
	]),

	PARAM_FIELDS: new Map([
		["kT", 0.1],
		["B_x", 0], ["B_y", 0], ["B_z", 0],
		
		["SL", 1], ["Sm", 1], ["SR", 1],
		["FL", 1], ["Fm", 0], ["FR", 0],
	
		["JL", 1], ["JmL", 1], ["Jm", 1], ["JmR", -1], ["JR", 1], ["JLR", 0],
		["Je0L", 0], ["Je0m", 0], ["Je0R", 0],
		["Je1L", 0], ["Je1m", 0], ["Je1R", 0], ["Je1mL", 0], ["Je1mR", 0], ["Je1LR", 0],
		["JeeL", 0], ["Jeem", 0], ["JeeR", 0], ["JeemL", 0], ["JeemR", 0], ["JeeLR", 0],
	
		["bL", 0], ["bm", 0], ["bR", 0], ["bmL", 0], ["bmR", 0], ["bLR", 0],
		
		["AL_x", 0], ["Am_x", 0], ["AR_x", 0],
		["AL_y", 0], ["Am_y", 0], ["AR_y", 0],
		["AL_z", 0], ["Am_z", 0], ["AR_z", 0],
	
		["DL_x", 0], ["Dm_x", 0], ["DR_x", 0], ["DmL_x", 0], ["DmR_x", 0], ["DLR_x", 0],
		["DL_y", 0], ["Dm_y", 0], ["DR_y", 0], ["DmL_y", 0], ["DmR_y", 0], ["DLR_y", 0],
		["DL_z", 0], ["Dm_z", 0], ["DR_z", 0], ["DmL_z", 0], ["DmR_z", 0], ["DLR_z", 0],
	])
};

const valueCache = new SavedMap(localStorage, "valueCache");


// ---- Functions: ------------------------------------------------------------
const forEachDimField = (f) => {
	for(let id of DEFAULTS.DIM_FIELDS.keys())
	{
		const [region, prop] = id.split("-", 2);	
		const input = document.getElementById(id);
		f({ id, input, region, prop });
	}
};

/**
 * Resets the 3D {@link Scene} and {@link MSDView} object with default values.
 * @param {MSDView} msd - will be modified
 * @param {Map} DIM_FIELDS - Contains the dimensions of a default MSD
 */
const resetView = (msd, { DIM_FIELDS }) => {
	msd.FML.width = DIM_FIELDS.get("FML-width");
	msd.FMR.width = DIM_FIELDS.get("FMR-width");
	msd.mol.width = DIM_FIELDS.get("mol-width");

	msd.FML.height = 0;
	msd.FMR.height = DIM_FIELDS.get("FMR-height");
	msd.FML.height = DIM_FIELDS.get("FML-height");

	msd.FMR.depth = 0;
	msd.FML.depth = DIM_FIELDS.get("FML-depth");
	msd.FMR.depth = DIM_FIELDS.get("FMR-depth");

	msd.mol.y = DIM_FIELDS.get("mol-y");
	msd.mol.z = DIM_FIELDS.get("mol-z");
};

/**
 * Load the {@link MSDView} dimensions from previously saved info in {@link valueCache}.
 * @param {MSDView} msd
 */
const loadView = (msd) => {
	// (First reset dims so setting properties out-of-order doesn't cause errors.)
	msd.FML.width = msd.FMR.width = msd.mol.width = 0; 
	msd.FMR.depth = msd.FML.height = 0;
	msd.mol.y = msd.mol.z = 0;
	forEachDimField(({ id, region, prop }) => {
		let value = valueCache.get(id);
		if (value !== undefined && value !== null)
			msd[region][prop] = value;
		else
			msd[region][prop] = DEFAULTS.DIM_FIELDS.get(id);
	});
};

/**
 * Load HTML (param) inputs with cached or default info.
 */
const loadHTMLParamFields = () => {
	DEFAULTS.PARAM_FIELDS.forEach((default_value, param_name) => {
		console.log(param_name);
		let value = valueCache.get(param_name);
		document.getElementById(param_name).value =
			(value !== undefined && value != null ? value : default_value);

		// TODO: handle _rho, _theta, _phi fields because they are not in PARAM_FIELDS
		// let [ prefix, suffix ] = splitParam(param_name);
		// if (suffix)
		// 	updateRhoThetaPhi(prefix);
	});
};

/**
 * Update HTML (dimension) fields with info in {@link MSDView} object.
 * @param {MSDView} msd
 */
const syncHTMLDimFields = (msd) => {
	forEachDimField(({ input, region, prop }) => {
		input.value = Math.floor(msd[region][prop]);
		input.min = msd[region].bounds[prop]?.min;
		input.max = msd[region].bounds[prop]?.max;
	})
};


// ---- TODO: Unused ----------------------------------------------------------
const splitParam = (param_name) => param_name.split("_", 2);

// update _x, _y, _z fields for a parameter given _phi, _theta, _rho. Inverse of updateRhoThetaPhi.
// Note: using ISO standard definitions for phi, theta, rho
const updateXYZ = (prefix) => {
	let [ rho, theta, phi ] = ["rho", "theta", "phi"].map(suffix =>
		+document.getElementById(`${prefix}_${suffix}`).value );
	let r = rho * Math.sin(theta);
	document.getElementById(`${prefix}_x`).value = r * Math.cos(phi);
	document.getElementById(`${prefix}_y`).value = r * Math.sin(phi);
	document.getElementById(`${prefix}_z`).value = rho * Math.cos(theta);
};

// Update _rho, _theta, _phi fields for a parameter given _x, _y, _z. Inverse of updateXYZ.
// Note: using ISO standard definitions for phi, theta, rho
const updateRhoThetaPhi = (prefix) => {
	let [ x, y, z ] = ["x", "y", "z"].map(suffix =>
		+document.getElementById(`${prefix}_${suffix}`).value );
	let r2 = x*x + y*y;
	let rho = Math.sqrt(r2 + z*z);
	document.getElementById(`${prefix}_rho`).value = rho;
	document.getElementById(`${prefix}_theta`).value = Math.acos(z / rho) * 180 / Math.PI;
	document.getElementById(`${prefix}_phi`).value = Math.sign(y) * Math.acos(x / Math.sqrt(r2)) * 180 / Math.PI;
};


// ---- Main: -----------------------------------------------------------------
const initForm = ({ camera, msdView }) => {
	loadView(msdView);
	updateCamera(camera, msdView);
	syncHTMLDimFields(msdView);
	loadHTMLParamFields();

	// Event Listeners
	forEachDimField(({ id, input, region, prop }) => {
		// onfocus: Store values of each field before they are changed,
		// 	so they can be reverted incase invalid values are entered.
		input.addEventListener("focus", (event) => {
			let value = +event.currentTarget.value;
			valueCache.set(id, value);
		});

		// onchange:
		input.addEventListener("change", (event) => {
			const input = event.currentTarget;
			let value = Math.round(+input.value);

			input.value = value;
			try {
				msdView[region][prop] = value;
				valueCache.set(id, value);
				syncHTMLDimFields(msdView);
				updateCamera(camera, msdView);
			} catch(ex) {
				document.querySelector(`#${id}`).value = valueCache.get(id);
				console.log(ex);
				alert(ex);
			}
		});
	});
	
	for (let param_name of DEFAULTS.PARAM_FIELDS.keys()) {
		// let [ prefix, suffix ] = splitParam(param_name);
		// if (!suffix) {
			document.getElementById(param_name).addEventListener("change", (event) => {
				valueCache.set(param_name, +event.currentTarget.value);
			});
		// } else {
			// TODO: How to save Vectors, and how do we update vectors as one represntation is modified?
		// }
	};

	const paramsForm = document.getElementById("msd-params-form");
	paramsForm.addEventListener("submit", (event) => {
		event.preventDefault();
		// TODO: export data as iterate parameters file
	});
	paramsForm.addEventListener("reset", (event) => {
		event.preventDefault();
		if (confirm("Reset all parameters to a default state?")) {
			valueCache.clear();
			resetView(msdView, DEFAULTS);
			updateCamera(camera, msdView);
			syncHTMLDimFields(msdView);
			loadHTMLParamFields();
		}
	});

	document.getElementById("FML-legend").innerText = `[${msdView.FML.name}]`;
	document.getElementById("FMR-legend").innerText = `[${msdView.FMR.name}]`;
	document.getElementById("mol-legend").innerText = `[${msdView.mol.name}]`;
	document.getElementById("LR-legend").innerText = `[${msdView.FML.name}~~${msdView.FMR.name}]`;
	document.getElementById("mL-legend").innerText = `[${msdView.FML.name}~~${msdView.mol.name}]`;
	document.getElementById("mR-legend").innerText = `[${msdView.mol.name}~~${msdView.FMR.name}]`;
};


// ---- Exports: --------------------------------------------------------------
defineExports("MSDBuilder.form", { initForm });

})();  // run IIFE
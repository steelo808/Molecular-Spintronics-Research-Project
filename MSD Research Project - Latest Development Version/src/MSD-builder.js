// ---- Imports: --------------------------------------------------------------
const {
	Scene, PerspectiveCamera, WebGLRenderer,
	BufferGeometry, BoxGeometry,
	MeshBasicMaterial,
	Mesh, Group
} = window.Three;


// ---- Classes: --------------------------------------------------------------
class MSDRegion {
	constructor({ x = 0, y = 0, z = 0, width = 1, height = 1, depth = 1, color = 0xffffff, wireframe = true }) {
		this._x = x;
		this._y = y;
		this._z = z;
		this._width = width;
		this._height = height;
		this._depth = depth;
		this._color = color;
		this._wireframe = wireframe;

		this.mesh = new Mesh();
		this.mesh.material = new MeshBasicMaterial({
			color: color,
			wireframe: MSDRegion._hasVolume(width, height, depth) && wireframe });
		this._updateGeometry();
	}

	static _hasVolume(w, h, d) {
		return w > 0 && h > 0 && d > 0;
	}

	_hasVolume() {
		return MSDRegion._hasVolume(this.width, this.height, this.depth);
	}

	_updateGeometry() {
		const [w, h, d] = [this.width, this.height, this.depth];
		const hasVolume = MSDRegion._hasVolume(w, h, d);
		this.mesh.material.wireframe = hasVolume && this.wireframe;
		this.mesh.geometry = hasVolume ? new BoxGeometry(w, h, d, w, h, d) : new BufferGeometry();
		// console.log(this.width, this.height, this.depth, this.mesh.geometry)
	}

	set x(x) {
		this._x = x;
		this.mesh.position.x = x;
	}

	set y(y) {
		this._y = y;
		this.mesh.position.y = y;
	}

	set z(z) {
		this._z = z;
		this.mesh.position.z = z;
	}

	set width(width) {
		this._width = width;
		this._updateGeometry();
	}

	set height(height) {
		this._height = height;
		this._updateGeometry();
	}

	set depth(depth) {
		this._depth = depth;
		this._updateGeometry();
	}

	set color(color) {
		this._color = color;
		this.mesh.material.color = color;
	}

	set wireframe(wireframe) {
		this._wireframe = wireframe;
		this.mesh.material.wireframe = this._hasVolume() && wireframe;
	}

	get x() { return this._x; }
	get y() { return this._y; }
	get z() { return this._z; }
	get width() { return this._width; }
	get height() { return this._height; }
	get depth() { return this._depth; }
	get color() { return this._color; }
	get wireframe() { return this._wireframe; }
}

class MSDView {
	constructor() {
		const wireframe = true;
		this._FML = new MSDRegion({ width: 5, height: 4, depth: 10, color: 0x0000FF, wireframe: wireframe });
		this._FMR = new MSDRegion({ width: 5, height: 10, depth: 4, color: 0xFF0000, wireframe: wireframe });
		this._mol = new MSDRegion({ width: 1, height: 4, depth: 4, color: 0xFF00FF, wireframe: wireframe });

		this.objects = new Group();
		this.objects.add(this._FML.mesh);
		this.objects.add(this._FMR.mesh);
		this.objects.add(this._mol.mesh);

		this.FML._updateX();
		this.FMR._updateX();
	}

	_genNonNegError({ name }, dim) {
		return `${dim} of ${name} must be non-negative`;
	}

	_ensureBound(bounds, val) {
		if (val < bounds.min)
			throw new Error(`${bounds.minError}: ${val}`);
		if (val > bounds.max)
			throw new Error(`${bounds.maxError}: ${val}`);
	}

	_ensureWidth({ bounds }, width) { this._ensureBound(bounds.width, width); }
	_ensureHeight({ bounds }, height) { this._ensureBound(bounds.height, height); }
	_ensureDepth({ bounds }, depth) { this._ensureBound(bounds.depth, depth); }
	_ensureX({ bounds }, x) { this._ensureBound(bounds.x, x); }
	_ensureY({ bounds }, y) { this._ensureBound(bounds.y, y); }
	_ensureZ({ bounds }, z) { this._ensureBound(bounds.z, z); }

	get FML() {
	 const self = this;
	 return {
		name: "FML",

		_updateX() { self._FML.x = -(this.width + self.mol.width) / 2; },

		get bounds() {
		 const fml = this;
		 const fmr = self.FMR;
		 const mol = self.mol;

		 return {
			width: {
				min: 0,
				max: Infinity,

				minError: self._genNonNegError(fml, "Width")
			},

			height: {
				min: 0,
				max: fmr.height,

				minError: self._genNonNegError(fml, "Height"),
				maxError: `Height of ${fml.name} must non exceed height of ${fmr.name} (${fmr.height})`
			},

			depth: {
				min: fmr.depth,
				max: Infinity,

				minError: `Depth of FML must at least equal depth of FMR (${fmr.depth})`,
			},

			x: {},
			y: mol.bounds.y,
			z: {}
		 }
		},

		set width(width) {
			self._ensureWidth(this, width);
			self._FML.width = width;
			this._updateX();
		},

		set height(height) {
			self._ensureHeight(this, height);
			self.mol.height = height;
		},

		set depth(depth) {
			self._ensureDepth(this, depth);
			self._FML.depth = depth;
			self.mol.justifyZ();
		},

		set x(x) { throw new Error("Can't set x-offset of individual MSD regions. Set MSDView.objects.position.x instead?"); },
		set y(y) { self.mol.y = y; },
		set z(z) { throw new Error("Can't set z-offset of FML. Did you mean FMR?"); },

		get width() { return self._FML.width; },
		get height() { return self._FML.height; },
		get depth() { return self._FML.depth; },

		get x() { return 0; },
		get y() { return self._FML.y; },
		get z() { return self._FML.z; }
	 };
	}

	get FMR() {
	 const self = this;
	 return {
		name: "FMR",

		_updateX() { self._FMR.x = (self.mol.width + this.width) / 2; },

		get bounds() {
		 const fmr = this;
		 const fml = self.FML;
		 const mol = self.mol;

		 return { 
			width: {
				min: 0,
				max: Infinity,

				minError: self._genNonNegError(fmr, "Width"),
			},

			height: {
				min: fml.height,
				max: Infinity,

				minError: `Height of ${fmr.name} must at least equal height of ${fml.name} (${fml.height})`
			},

			depth: {
				min: 0,
				max: fml.depth,

				minError: self._genNonNegError(fmr, "Depth"),
				maxError: `Depth of FMR must not exceed depth of ${fml.name} (${fml.depth})`,
			},

			x: {},
			y: {},
			z: mol.bounds.z
		 }
		},

		set width(width) {
			self._ensureWidth(this, width);
			self._FMR.width = width;
			this._updateX();
		},

		set height(height) {
			self._ensureHeight(this, height);
			self._FMR.height = height;
			self.mol.justifyY();
		},

		set depth(depth) {
			self._ensureDepth(this, depth);
			self.mol.depth = depth;
		},

		set x(x) { throw new Error("Can't set x-offset of individual MSD regions. Set MSDView.objects.position.x instead?"); },
		set y(y) { throw new Error("Can't set y-offset of FMR. Did you mean FML?") },
		set z(z) { self.mol.z = z; },

		get width() { return self._FMR.width; },
		get height() { return self._FMR.height; },
		get depth() { return self._FMR.depth; },

		get x() { return 0; },
		get y() { return self._FMR.y; },
		get z() { return self._FMR.z; }
	 };
	}

	get mol() {
	 const self = this;
	 return {
		name: "mol.",
		
		get _maxYOffset() { return (self.height - self.mol.height) / 2; },
		get _maxZOffset() { return (self.depth - self.mol.depth) / 2; },

		_isWhole(n) { return (n * 2) % 2 == 0; },

		justifyY() {
			const { y } = this;
			try {
				self._ensureY(this, y);
			} catch(ex) {
				const { min, max } = self.mol.bounds.y;
				const distMin = Math.abs(y - min);
				const distMax = Math.abs(max - y);
				self._mol.y = (distMin <= distMax ? min : max);
			}
			self._mol.y = self._FML.y = Math.floor(this.y) +
					(this._isWhole(this._maxYOffset) ? 0 : 0.5);
		},

		justifyZ() {
			const { z } = this;
			try {
				self._ensureZ(this, z);
			} catch(ex) {
				const { min, max } = self.mol.bounds.z;
				const distMin = Math.abs(z - min);
				const distMax = Math.abs(max - z);
				self._mol.z = (distMin <= distMax ? min : max);
			}
			self._mol.z = self._FMR.z = Math.floor(this.z) +
					(this._isWhole(this._maxZOffset) ? 0 : 0.5);
		},

		get bounds() {
		 const mol = this;
		 const fml = self.FML;
		 const fmr = self.FMR;
		 
		 return { 
			width: {
				min: 0,
				max: Infinity,

				minError: self._genNonNegError(mol, "Width"),
			},

			height: {
				min: 0,
				max: fmr.height,

				minError: self._genNonNegError(mol, "Height"),
				maxError: `Height of ${mol.name} must not exceed height of ${fmr.name} (${fmr.height})`
			},

			depth: {
				min: 0,
				max: fml.depth,

				minError: self._genNonNegError(mol, "Depth"),
				maxError: `Depth of ${mol.name} must not exceed depth of ${fml.name} (${fml.depth})`,
			},

			x: {},

			get y() {
				const { _maxYOffset } = mol;
				let obj = {
					min: Math.floor(-_maxYOffset),
					max: Math.floor(_maxYOffset)
				};
				obj.minError = `Given the current dimensions, y-offset must be between [${obj.min}, and ${obj.max}]`;
				obj.maxError = `Given the current dimensions, y-offset must be between [${obj.min}, and ${obj.max}]`;
				return obj;
			},

			get z() {
				const { _maxZOffset } = mol;
				let obj = {
					min: Math.floor(-_maxZOffset),
					max: Math.floor(_maxZOffset)
				};
				obj.minError = `Given the current dimensions, z-offset must be between [${obj.min}, and ${obj.max}]`;
				obj.maxError = `Given the current dimensions, z-offset must be between [${obj.min}, and ${obj.max}]`;
				return obj;
			}
		 }
		},

		set width(width) {
			self._ensureWidth(this, width);
			self._mol.width = width;
			self.FML._updateX();
			self.FMR._updateX();
		},

		set height(height) {
			self._ensureHeight(this, height);
			self._mol.height = height;
			self._FML.height = height;
			this.justifyY();
		},

		set depth(depth) {
			self._ensureDepth(this, depth);
			self._mol.depth = depth;
			self._FMR.depth = depth;
			this.justifyZ();
		},

		set x(x) {
			throw new Error("Can't set x of individual MSD regions. Set MSDView.objects.position.x instead?");
		},

		set y(y) {
			self._ensureY(this, y);
			self._mol.y = y;
			self._FML.y = y;
			this.justifyY();
		},

		set z(z) {
			self._ensureZ(this, z);
			self._mol.z = z;
			self._FMR.z = z;
			this.justifyZ();
		},

		get width() { return self._mol.width; },
		get height() { return self._mol.height; },
		get depth() { return self._mol.depth; },

		get x() { return 0; },
		get y() { return self._mol.y; },
		get z() { return self._mol.z; }
	 };
	}

	set width(width) { throw new Error("Width of each region must be set individually."); }
	set height(height) { this.FMR.height = height; }
	set depth(depth) { this.FML.depth = depth; }

	get width() { return this.FML.width + this._mol.width + this._FMR.width; }
	get height() { return this.FMR.height; }
	get depth() { return this.FML.depth; }
};

class AnimationLoop {
	constructor(renderer, scene, camera) {
		this.renderer = renderer;
		this.scene = scene;
		this.camera = camera;
		this.isRunning = false;
		this.update = null;

		this._t0 = 0;  // timestamp when animation loop started (in milliseconds)
		this.time = 0;  // time since animation loop started (in milliseconds)
		this.deltaTime = 0;  // time since last frame (in milliseconds)
	}

	renderFrame() {
		let time = Date.now() - this._t0;
		this.deltaTime = time - this.time;
		this.time = time;
		if (this.update)
			this.update();
		this.renderer.render(this.scene, this.camera);
		if (this.isRunning)
			requestAnimationFrame(() => this.renderFrame());
	}

	start(update = null) {
		this.update = update;
		this.isRunning = true;
		this.time = 0;
		this._t0 = Date.now();
		this.renderFrame();
	}

	stop() {
		this.isRunning = false;
	}
}


// ---- Main: -----------------------------------------------------------------
const main = () => {
	const canvasWidth = 900;
	const canvasHeight = 450;
	const scene = new Scene();
	const camera = new PerspectiveCamera(90, canvasWidth / canvasHeight);  // params: fov, aspect ratio
	const renderer = new WebGLRenderer();
	renderer.setSize(canvasWidth, canvasHeight /*, false */);
	document.querySelector("#msdBuilder").append(renderer.domElement);

	const DIM_FIELDS = new Map([
		["FML-width", 5], ["FML-height", 4], ["FML-depth", 10], ["FML-y", 0],
		["FMR-width", 5], ["FMR-height", 10], ["FMR-depth", 4], ["FMR-z", 0],
		["mol-width", 1], ["mol-height", 4], ["mol-depth", 4], ["mol-y", 0], ["mol-z", 0]
	]);
	const PARAM_FIELDS = new Map([
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
	]);

	let msd = new MSDView();

	// resets the 3D scene and msd object
	const reset = () => {
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

		msd.objects.rotation.x = Math.PI / 6;
		msd.objects.rotation.y = -Math.PI / 24;
	};
	reset();
	scene.add(msd.objects);

	const loop = new AnimationLoop(renderer, scene, camera);
	// const update = () => {
	// 	msd.objects.rotation.y += 0.0001 * loop.deltaTime;
	// 	console.log(loop.time, loop.deltaTime);
	// };
	loop.start(/* update */);
	// renderer.domElement.addEventListener("click", (event) => {
	// 	if (loop.isRunning)
	// 		loop.stop();
	// 	else
	// 		loop.start(update);
	// });

	const forEachDimField = (f) => {
		for(let id of DIM_FIELDS.keys())
		{
			const [region, prop] = id.split("-", 2);	
			const input = document.getElementById(id);
			f({ id, input, region, prop });
		}
	};

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

	// Given a key from PARAM_FIELDS,
	// returns the prefix if the parameter is a vector (ends with _x, _y, or _z),
	// or return null.
	const splitParam = (param_name) => param_name.split("_", 2);

	class SavedMap extends Map {
		/**
		 * @param storage Either localStorage or sessionStorage, or similar object.
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
	const valueCache = new SavedMap(localStorage, "valueCache");

	// Update the msd object with previously saved info in the valueCache.
	// (First reset dims so setting props. out-of-order doesn't cause errors.)
	msd.FML.width = msd.FMR.width = msd.mol.width = 0; 
	msd.FMR.depth = msd.FML.height = 0;
	msd.mol.y = msd.mol.z = 0;
	forEachDimField(({ id, region, prop }) => {
		let value = valueCache.get(id);
		if (value !== undefined && value !== null)
			msd[region][prop] = value;
		else
			msd[region][prop] = DIM_FIELDS.get(id);
	});

	// update HTML (dimension) fields with info in msd object 
	const updateDimFields = () => {
		forEachDimField(({ input, region, prop }) => {
			input.value = Math.floor(msd[region][prop]);
			input.min = msd[region].bounds[prop]?.min;
			input.max = msd[region].bounds[prop]?.max;
		})
	};
	updateDimFields();
	
	// update HTML (param) inputs with cached or default info
	const updateParamFields = () => {
		PARAM_FIELDS.forEach((default_value, param_name) => {
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
	updateParamFields();

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
				msd[region][prop] = value;
				valueCache.set(id, value);
				updateDimFields();
				updateCamera();
			} catch(ex) {
				document.querySelector(`#${id}`).value = valueCache.get(id)
				console.log(ex);
				alert(ex);
			}
		});
	});

	for (let param_name of PARAM_FIELDS.keys()) {
		// let [ prefix, suffix ] = splitParam(param_name);
		// if (!suffix) {
			document.getElementById(param_name).addEventListener("change", (event) => {
				valueCache.set(param_name, +event.currentTarget.value);
			});
		// } else {
			// TODO: How to save Vectors, and how do we update vectors as one represntation is modified?
		// }
	};

	const updateCamera = () => {
		camera.position.z = 0.85 * Math.max(msd.width, msd.height, msd.depth);
	};
	updateCamera();

	const paramsForm = document.getElementById("msd-params-form");
	paramsForm.addEventListener("submit", (event) => {
		event.preventDefault();
		// TODO: export data as iterate parameters file
	});
	paramsForm.addEventListener("reset", (event) => {
		event.preventDefault();
		if (confirm("Reset all parameters to a default state?")) {
			reset();
			updateDimFields();
			updateParamFields();
			updateCamera();
			valueCache.clear();
		}
	});

	document.getElementById("FML-legend").innerText = `[${msd.FML.name}]`;
	document.getElementById("FMR-legend").innerText = `[${msd.FMR.name}]`;
	document.getElementById("mol-legend").innerText = `[${msd.mol.name}]`;
	document.getElementById("LR-legend").innerText = `[${msd.FML.name}~~${msd.FMR.name}]`;
	document.getElementById("mL-legend").innerText = `[${msd.FML.name}~~${msd.mol.name}]`;
	document.getElementById("mR-legend").innerText = `[${msd.mol.name}~~${msd.FMR.name}]`;
};

document.addEventListener("DOMContentLoaded", main);

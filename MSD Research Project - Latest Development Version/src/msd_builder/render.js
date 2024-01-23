(function() {  // IEFF

// ---- Imports: --------------------------------------------------------------
const { defineExports } = MSDBuilder.util;
const {
	Scene, Camera, PerspectiveCamera, WebGLRenderer,
	BufferGeometry, BoxGeometry, SphereGeometry,
	MeshBasicMaterial,
	Mesh, Group,
	SceneLoader
} = Three;


// ---- Classes: --------------------------------------------------------------
/**
 * 3D model (geometry & material) of an MSD region; e.g., FML, FMR, mol.
 * @abstract Override: {@link #_updateGeometry()}, {@link #_updateMaterial()}
 */
class MSDRegion extends Group {
	constructor({ x = 0, y = 0, z = 0, width = 1, height = 1, depth = 1, color = 0xffffff, wireframe = true }) {
		super();

		// @protected - fields
		this._x = x;
		this._y = y;
		this._z = z;
		this._width = width;
		this._height = height;
		this._depth = depth;
		this._color = color;
		this._wireframe = wireframe;
	}

	/** @private */
	static _hasVolume = (w, h, d) => w > 0 && h > 0 && d > 0;

	/** @protected */
	_hasVolume = () =>
		MSDRegion._hasVolume(this.width, this.height, this.depth);

	/**
	 * @protected
	 * @abstract Needs to be overriden in subclass!
	 */
	_updateGeometry() {
		throw new Error("Unimplemented method: MSDRegion._updateGeometry()");
	}

	/**
	 * @protected
	 * @abstract Needs to be overriden in subclass!
	 */
	_updateMaterial() {
		throw new Error("Unimplemented method: MSDRegion._updateMaterial()");
	}

	set x(x) {
		this._x = x;
		this.position.x = x;  // Three.js Group.position
	}

	set y(y) {
		this._y = y;
		this.position.y = y;  // // Three.js Group.position
	}

	set z(z) {
		this._z = z;
		this.position.z = z;  // Three.js Group.position
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
		this._updateMaterial();
	}

	/**
	 * @param {Boolean} wireframe Wireframe material true or false?
	 */
	set wireframe(wireframe) {
		this._wireframe = wireframe;
		this._updateMaterial();
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

/**
 * Displays the region as a single box with width, height, and depth.
 */
class BoxMSDRegion extends MSDRegion {
	constructor(args) {
		super(args);

		this.mesh = new Mesh();
		this._updateMaterial();
		this._updateGeometry();
		this.add(this.mesh);  // add to this Three.js Group
	}

	/** @Override */
	_updateGeometry() {
		const [w, h, d] = [this.width, this.height, this.depth];
		const hasVolume = this._hasVolume();
		this.mesh.material.wireframe = hasVolume && this.wireframe;
		this.mesh.geometry = hasVolume ? new BoxGeometry(w, h, d, w, h, d) : new BufferGeometry();
	}

	/** @Override */
	_updateMaterial() {
		this.mesh.material = new MeshBasicMaterial({
			color: this.color,
			wireframe: this._hasVolume() && this.wireframe });
	}
}

class LatticeMSDRegion extends MSDRegion {
	constructor({r = 1/6, detail = 8, ...args}) {
		super(args);
		this._r = r;
		this._detail = detail;  // TODO: better name? (Controls polygon count)

		this.nodes = new Group();
		this.edges = new Group();
		this._updateGeometry();
		this.add(this.nodes, this.edges);  // add to this Three.js Group

		// overriding defaults
		if (!args.wireframe)  this.wireframe = false;
	}

	/** @Override */
	_updateGeometry() {
		// TODO: optimize. do we really need to delete all the node meshs?
		this.nodes.clear();

		let dims = [this.width, this.height, this.depth];
		let [w, h, d] = dims;
		let [halfW, halfH, halfD] = dims.map(dim => (dim - 1) / 2);

		for (let z = -halfD; z <= halfD; z++)
		for (let y = -halfH; y <= halfH; y++)
		for (let x = -halfW; x <= halfW; x++) {
			let geo = new SphereGeometry(this.r, this.detail, this.detail);
			let mat = new MeshBasicMaterial({
				color: this.color,
				wireframe: this.wireframe });
			let mesh = new Mesh(geo, mat);
			Object.assign(mesh.position, {x, y, z});
			this.nodes.add(mesh);
		}
	}

	/** @Override */
	_updateMaterial() {
		this.nodes.traverse(mesh =>
			mesh.material = new MeshBasicMaterial({
				color: this.color,
				wireframe: this.wireframe }) );
	}

	set r(r) {
		this._r = r;
		this._updateGeometry();
	}

	set detail(detail) {
		this._detail = detail;
		this._updateGeometry();
	}

	get r() { return this._r; }
	get detail() { return this._detail; }
}

/**
 * GUI logic for 3D redering an MSD.
 */
class MSDView {
	/**
	 * @param {...<? extends MSDRegion>} RegionTypes
	 * 	(optional) one to three region types (classes) to use for
	 * 	FML, mol, and FMR region views respectively.
	 */
	// TODO: what is the correct interface for this so the called can configure some of the parameters??
	// Maybe a getter of each MSDRegion (view) where they can edit the properties?
	constructor(...RegionTypes) {
		let FMLType = RegionTypes.length > 0 ? RegionTypes[0] : BoxMSDRegion;
		let FMRType = RegionTypes.length > 2 ? RegionTypes[2] : FMLType;
		let MolType = RegionTypes.length > 1 ? RegionTypes[1] : FMLType;

		const wireframe = true;  // TODO: set undefinded - use defaults
		this._FML = new FMLType({ width: 5, height: 4, depth: 10, color: 0x0000FF, wireframe: wireframe });
		this._FMR = new FMRType({ width: 5, height: 10, depth: 4, color: 0xFF0000, wireframe: wireframe });
		this._mol = new MolType({ width: 1, height: 4, depth: 4, color: 0xFF00FF, wireframe: wireframe });

		this.objects = new Group();
		this.objects.add(this._FML);
		this.objects.add(this._FMR);
		this.objects.add(this._mol);

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
		 const [fml, fmr, mol] = [this, self.FMR, self.mol];
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
		 const [fml, fmr, mol] = [self.FML, this, self.mol];
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
		 const [fml, fmr, mol] = [self.FML, self.FMR, this];
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
	/**
	 * @param {WebGLRenderer} renderer 
	 * @param {Scene} scene 
	 * @param {Camera} camera
	 */
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
		requestAnimationFrame(() => this.renderFrame());
	}

	stop() {
		this.isRunning = false;
	}
}


// ---- Functions: ------------------------------------------------------------
/**
 * Updates the {@link Camera} position position (distance) based on the {@link MSDView}.
 * @param {Camera} camera - will be modified
 * @param {MSDView} msd
 */
const updateCamera = (camera, msd) => {
	camera.position.z = 0.85 * Math.max(msd.width, msd.height, msd.depth);
};

/**
 * Constructs and returns all the relavent objects to render the 3D scene.
 * 
 * @param {Object} obj
 * @param {Number} obj.aspectRatio -
 * 	Aspect ratio of the canvas created by the {@link WebGLRenderer}.
 * @param {Number} obj.maxCanvasWidth -
 * 	Maximum width of the canvas created by the {@link WebGLRenderer}.
 * @param {Number} obj.maxCanvasHeight -
 * 	Maximum height of the canvas created by the {@link WebGLRenderer}.
 * @param {Array[MSDRegion]} obj.MSDRegionTypes -
 * 	What type(s) of {@link MSDRegion} classes to used for the {@link MSDView}
 * @param {Function} obj.onAnimationFrame -
 * 	Called once per frame, before redering.
 * 	Takes an optional parameter containing the same data returned from this function.
 * 
 * @return {{
 * 	scene: Scene,
 * 	camera: Camera,
 * 	renderer, WebGLRenderer,
 * 	loop: AnimationLoop,
 * 	msdView: MSDView
 * }}
 */
const startRendering = ({
	aspectRatio = 16/9,
	maxCanvasWidth = Infinity,
	maxCanvasHeight = Infinity,
	MSDRegionTypes = [],
	onAnimationFrame = null
} = {}) => {
	const scene = new Scene();
	const camera = new PerspectiveCamera(90, aspectRatio);  // params: fov, aspect ratio
	const renderer = new WebGLRenderer();
	// renderer.setSize(canvasWidth, canvasHeight, false);
	
	// add canvas to DOM
	let container = document.querySelector("#msdBuilder");
	let canvas = renderer.domElement;
	container.append(canvas);

	// response canvas resolution
	const resize = () => {
		let { width, height } = container.getBoundingClientRect();
		width = Math.min(width, maxCanvasWidth);
		height = Math.min(height, maxCanvasWidth);
		
		let expectedWidth = height * aspectRatio;
		if (width > expectedWidth)  // too wide, make smaller
			width = expectedWidth;
		else                        // too tall, make smaller
			height = width / aspectRatio;
		
		renderer.setSize(width, height, false);
	};
	resize();
	window.addEventListener("resize", resize);
	
	// continuously redraw canvas (once per "frames")
	const loop = new AnimationLoop(renderer, scene, camera);
	const msdView = new MSDView(...MSDRegionTypes);
	scene.add(msdView.objects);
	const _vars = { scene, camera, renderer, loop, msdView };

	if (onAnimationFrame)
		loop.start(() => onAnimationFrame(_vars));
	else
		loop.start();

	// TODO: testing FBX file (need to import from Addons.js)
	// new FBXLoader().load("../assets/Test.fbx", object => {
	// 	scene.add(object);
	// });

	return _vars;
	// const update = () => {
	// 	msd.objects.rotation.y += 0.0001 * loop.deltaTime;
	// 	console.log(loop.time, loop.deltaTime);
	// };
	// loop.start(update);
	// renderer.domElement.addEventListener("click", (event) => {
	// 	if (loop.isRunning)
	// 		loop.stop();
	// 	else
	// 		loop.start(update);
	// });
};


// ---- Exports ---------------------------------------------------------------
defineExports("MSDBuilder.render", {
	MSDRegion, BoxMSDRegion, LatticeMSDRegion, MSDView, AnimationLoop,
	updateCamera, startRendering
});

})();  // end IEFF
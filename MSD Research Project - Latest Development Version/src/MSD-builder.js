// ---- Imports: --------------------------------------------------------------
const {
	Scene, PerspectiveCamera, WebGLRenderer,
	Geometry, BoxGeometry,
	Mesh, MeshBasicMaterial,
	Group
} = window.Three;


// ---- Classes: --------------------------------------------------------------
class MSDRegion {
	constructor({ x = 0, y = 0, z = 0, width = 1, height = 1, depth = 1, color = 0xffffff }) {
		// init. state
		let geometry = new BoxGeometry(width, height, depth, width, height, depth);  // dim(3), segments(3)
		let material = new MeshBasicMaterial({ color: color });
		this.mesh = new Mesh(geometry, material);

		this._x = x;
		this._y = y;
		this._z = z;
		this._width = width;
		this._height = height;
		this._depth = depth;
		this._color = color;
	}

	_updateGeometry() {
		this.mesh.geometry = new BoxGeometry(this.width, this.height, this.depth, this.width, this.height, this.depth);
	}

	_updateMaterial() {
		this.mesh.material = new MeshBasicMaterial({ color: this.color });
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
		this._updateMaterial();
	}

	get x() { return this._x; }
	get y() { return this._y; }
	get z() { return this._z; }
	get width() { return this._width; }
	get height() { return this._height; }
	get depth() { return this._depth; }
}

class MSDView {
	constructor() {
		this._FML = new MSDRegion({ width: 5, height: 10, depth: 10, color: 0x0000FF });
		this._FMR = new MSDRegion({ width: 5, height: 10, depth: 10, color: 0xFF0000 });
		this._mol = new MSDRegion({ width: 1, height: 4, depth: 4, color: 0xFF00FF });

		this.objects = new Group();
		this.objects.add(this._FML.mesh);
		this.objects.add(this._FMR.mesh);
		this.objects.add(this._mol.mesh);

		this.FML._updateX();
		this.FMR._updateX();

		for (let r of [this.FML, this.mol, this.FMR])
			console.log(r.name, r.x, r.y, r.z, r.width, r.height, r.depth);
	}

	get FML() {
	 const self = this;
	 return {
		name: "FML",

		_updateX() { self._FML.x = -(self.mol.width / 2 + self.FML.width); },

		set width(width) {
			self._FML.width = width;
			self.FML._updateX();
		},

		set height(height) {
			if (height > self.height)
				throw new Error(`Height of FML (${height}) must not exceed height of FMR (${self.height})`);
			self._FML.height = height;
			self._mol.height = height;
		},

		set depth(depth) {
			if (depth < self.depth)
				throw new Error(`Depth of FML (${depth}) must at least equal depth of FMR (${self.depth})`);
			self._FML.depth = depth;
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

		_updateX() { self._FMR.x = self.mol.width + self.FMR.width; },

		set width(width) {
			self._FMR.width = width;
			self.FMR._updateX();
		},

		set height(height) {
			if (height < self.FML.height)
				throw new Error(`Height of FMR (${height}) must at least equal height of FML (${self.FML.height})`);
			self._FMR.height = height;
		},

		set depth(depth) {
			if (depth > self.FML.depth)
				throw new Error(`Depth of FMR (${depth}) must not exceed depth of FML (${self.FML.height})`);
			self._FMR.depth = depth;
			self._mol.depth = depth;
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
		name: "mol",

		set width(width) {
			self._mol.width = width;
			self.FML._updateX();
			self.FMR._updateX();
		},

		set height(height) {
			if (height > self.FMR.height)
				throw new Error(`Height of mol. (${height}) must not exceed height of FMR (${self.FMR.height})`);
			self._mol.height = height;
			self._FML.height = height;
		},

		set depth(depth) {
			if (depth > self.FML.depth)
				throw new Error(`Depth of mol. (${depth}) must not exceed depth of FML (${self.FML.depth})`);
			self._mol.depth = depth;
			self._FMR.depth = depth;
		},

		set x(x) {
			throw new Error("Can't set x of individual MSD regions. Set MSDView.objects.position.x instead?");
		},

		set y(y) {
			const maxOffset = self.height - self.mol.height;
			if (Math.abs(y) > maxOffset)
				throw new Error(`Given the current dimensions, y-offset can't exceed ${maxOffset}`);
			self._mol.y = y;
			self._FML.y = y;
		},

		set z(z) {
			const maxOffset = self.depth - self.mol.depth;
			if (Math.abs(z) > maxOffset)
				throw new Error(`Given the current dimension, z-offset can't exceed ${maxOffset}`);
			self._mol.z = z;
			self._FML.z = z;
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
	const scene = new Scene();
	const camera = new PerspectiveCamera(75, 1 / 1, 0.1, 1000);  // params: fov, aspect ratio, near and far clipping plane
	const renderer = new WebGLRenderer();
	renderer.setSize(500, 500 /*, false */);
	document.querySelector("#msdBuilder").append(renderer.domElement);

	let msd = new MSDView();
	scene.add(msd.objects);

	camera.position.z = 25;

	const loop = new AnimationLoop(renderer, scene, camera);
	const update = () => {
		msd.objects.rotation.y += 0.001 * loop.deltaTime;
		console.log(loop.time, loop.deltaTime);
	};
	loop.start(update);
	renderer.domElement.addEventListener("click", (event) => {
		if (loop.isRunning)
			loop.stop();
		else
			loop.start(update);
	})
};

document.addEventListener("DOMContentLoaded", main);

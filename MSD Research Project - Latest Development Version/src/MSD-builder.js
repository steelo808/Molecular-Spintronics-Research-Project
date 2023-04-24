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

	get FML() {
	 const self = this;
	 return {
		name: "FML",

		_updateX() { self._FML.x = -(this.width + self.mol.width) / 2; },

		set width(width) {
			self._FML.width = width;
			this._updateX();
		},

		set height(height) {
			if (height > self.height)
				throw new Error(`Height of FML (${height}) must not exceed height of FMR (${self.height})`);
			self._FML.height = height;
			self._mol.height = height;
			self.mol.justifyY();
		},

		set depth(depth) {
			if (depth < self.FMR.depth)
				throw new Error(`Depth of FML (${depth}) must at least equal depth of FMR (${self.FMR.depth})`);
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

		_updateX() { self._FMR.x = (self.mol.width + this.width) / 2; },

		set width(width) {
			self._FMR.width = width;
			this._updateX();
		},

		set height(height) {
			if (height < self.FML.height)
				throw new Error(`Height of FMR (${height}) must at least equal height of FML (${self.FML.height})`);
			self._FMR.height = height;
		},

		set depth(depth) {
			if (depth > self.depth)	
				throw new Error(`Depth of FMR (${depth}) must not exceed depth of FML (${self.depth})`);
			self._FMR.depth = depth;
			self._mol.depth = depth;
			self.mol.justifyZ();
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

		get _maxYOffset() { return (self.height - self.mol.height) / 2; },
		get _maxZOffset() { return (self.depth - self.mol.depth) / 2; },
		
		_isWhole(n) { return (n * 2) % 2 == 0; },

		justifyY() {
			self._mol.y = self._FML.y = Math.floor(this.y) +
					(this._isWhole(this._maxYOffset) ? 0 : 0.5);
		},

		justifyZ() {
			self._mol.z = self._FMR.z = Math.floor(this.z) +
					(this._isWhole(this._maxZOffset) ? 0 : 0.5);
		},

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
			this.justifyY();
		},

		set depth(depth) {
			if (depth > self.FML.depth)
				throw new Error(`Depth of mol. (${depth}) must not exceed depth of FML (${self.FML.depth})`);
			self._mol.depth = depth;
			self._FMR.depth = depth;
			this.justifyZ();
		},

		set x(x) {
			throw new Error("Can't set x of individual MSD regions. Set MSDView.objects.position.x instead?");
		},

		set y(y) {
			let max = this._maxYOffset;
			let min = -max;
			max = Math.floor(max);
			min = Math.floor(min);
			if (y > max || y < min)
				throw new Error(`Given the current dimensions, y-offset must be between [${min}, and ${max}]`);
			self._mol.y = y;
			self._FML.y = y;
			this.justifyY();
		},

		set z(z) {
			let max = this._maxZOffset;
			let min = -max;
			max = Math.floor(max);
			min = Math.floor(min);
			if (z > max || z < min)
				throw new Error(`Given the current dimension, z-offset must be between [${min}, and ${max}]`);
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
	const camera = new PerspectiveCamera(75, canvasWidth / canvasHeight, 0.1, 1000);  // params: fov, aspect ratio, near and far clipping plane
	const renderer = new WebGLRenderer();
	renderer.setSize(canvasWidth, canvasHeight /*, false */);
	document.querySelector("#msdBuilder").append(renderer.domElement);

	let msd = new MSDView();
	msd.objects.rotation.x = Math.PI / 6;
	msd.objects.rotation.y = -Math.PI / 24;
	scene.add(msd.objects);

	camera.position.z = 15;

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

	for (let id of [
			"FML-width", "FML-height", "FML-depth", "FML-y",
			"FMR-width", "FMR-height", "FMR-depth", "FMR-z",
			"mol-width", "mol-height", "mol-depth", "mol-y", "mol-z" ])
	{
		let [region, prop] = id.split("-", 2);	
		const input = document.querySelector(`#${id}`);
		input.value = msd[region][prop];
		input.addEventListener("change", (event) => {
			let { value } = event.currentTarget;
			// console.log(`${id}:`, value);
			try {
				msd[region][prop] = Number(value);
			} catch(ex) {
				console.log(ex);
				alert(ex);
			}
		});
	}
};

document.addEventListener("DOMContentLoaded", main);

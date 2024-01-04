// MSD properties
let FML = { width: 5, height: 5, depth: 11 };
let FMR = { width: 5, height: 11, depth: 5 };
let mol = { width: 1, top: 4, front: 4 };  // number of nodes in molecule

// Graphics
const { Illustration, Anchor, Rect, Line, TAU } = Zdog;  // using Zdog

const msdIllustration = new Illustration({
	element: "#msdIllustration"
});

class FM {
	static get FACES() {
		return {
			"left": {},
			"right": ,
			"top": TODO,
			"bottom": TODO,
			"front": {},
			"back": TODO
		};
	};

	constructor({ x = 0, y = 0, z = 0, width = 1, height = 1, depth = 1, color = "black" }) {
		this.faces = {};
		for (let face of FM.FACES) {
			let a = new Anchor({ addTo:  })
			this.faces[face] = new Rect({ addTo: msdIllustration });
		}
		this.x = x;
		this.y = y;
		this.z = z;
		this.width = width;
		this.height = height;
		this.depth = depth;
		this.color = color;
	}

	set x(x) {

	}

	set y(y) {

	}

	set z(z) {

	}

	set width(width) {

	}

	set height(height) {

	}

	set depth(depth) {

	}

	set color(color) {

	}

	get x() { return this.faces.front.translate.x; }
	get y() { return this.faces.front.translate.y; }
	get z() { return this.faces.}
};

const FML_g = new FM("blue");
const FMR_g = new FM("red");

const mol_g = new Rect({
	addTo: msdIllustration,
	color: "purple"
})

const update = function() {
	FML_g.width = FML.width;
	FML_g.height = FML.height;
	FML_g.depth = FML.depth;

	mol_g.width = mol.width;
	mol_g.height = FML.height;
	mol_g.depth = FMR.depth;
	mol_g.translate = { x: FML.width, y: mol.top, z: mol.front };

	FMR_g.width = FMR.width;
	FMR_g.height = FMR.height;
	FMR_g.depth = FMR.depth;
	FMR_g.translate = { x: FML.width + mol.width, y: 0, z: mol.front };

	msdIllustration.updateRenderGraph();
};

// Test
update();

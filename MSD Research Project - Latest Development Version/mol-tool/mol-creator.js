const R = 7.5;  // radius of each node in SVG units 

class Node {
	constructor(svg, x, y) {
		this.svgCircle = svg.myCreate("circle", { cx: x, cy: y, r: R });

		this.Sm = 1;
		this.Fm = 0;
		this.Je0m = 0;
		this.Am = 0;
	}

	get x() { return +this.svgCircle.getAttribute("cx"); }
	get y() { return +this.svgCircle.getAttribute("cy"); }
}

class Edge {
	constructor(svgLine, srcNode, destNode) {
		this.svgLine = svgLine;
		
		this.srcNode = srcNode;
		this.destNode = destNode;

		this.Jm = 0;
		this.Je1 = 0;
		this.Jee = 0;
		this.bm = 0;
		this.Dm = [0, 0, 0];
	}
}



let svg = document.querySelector("svg#mol-canvas");
let form = document.querySelector("#form");  // a container for the parameter update form
let nodes = [];
let edges = [];  // edge matrix
let selected = null;
let dragging = null;

/**
 * Transform client-space to SVG-space.
 * @param clientPoint An {x, y} object in client-space.
 * @returns An {x, y} object in SVG space.
 */
svg.clientToSVG = function(clientPoint) {
	let svgPoint = this.createSVGPoint();
	svgPoint.x = clientPoint.x;
	svgPoint.y = clientPoint.y;
	return svgPoint.matrixTransform(this.getScreenCTM().inverse());
};

/**
 * Creates an SVG element with the given tag name, and attributes, and appends it to this parent SVGElement.
 * @param name Tag name (e.g. "circle", "rect")
 * @param attrs A object containing the attributes to copy over to the newly created SVG element
 * 	(e.g. {cx: 0, cy: 0, r: 10})
 * @param append (optional) Add the newly created element to the SVG if true (default); or don't append if false.
 * @returns The newly created element.
 */
svg.myCreate = function(name, attrs, append) {
	if (append === undefined)
		append = true;
	
	let ele = document.createElementNS("http://www.w3.org/2000/svg", name);
	for (let key in attrs)  // Note: old-style "for-in" loop
		ele.setAttribute(key, attrs[key]);
	if (append)
		this.append(ele);
	return ele;
};

// "shadows" are displayed when dragging
let shadowNode = svg.myCreate("circle", { cx: 0, cy: 0, r: R }, false);
shadowNode.classList.add("shadow");
let shadowEdge = svg.myCreate("line", { x1: 0, y1: 0, x2: 0, y2: 0 }, false);
shadowEdge.classList.add("shadow");


/**
 * Create an MSD mol. Node and add it to the SVG canvas and the "nodes" list.
 * @param svgPoint Where the user clicked (in SVG-space)
 * @see svg.clientToSVG()
 */
const createNode = function({x, y}) {
	let node = new Node(svg, x, y);
	nodes.push(node);

	// event handlers
	node.svgCircle.addEventListener("mousedown", function(event) {
		dragging = node;
		node.svgCircle.classList.add("dragging");
		let {x, y} = svg.clientToSVG({ x: event.clientX, y: event.clientY });
		shadowNode.setAttribute("cx", x);
		shadowNode.setAttribute("cy", y);
		shadowEdge.setAttribute("x1", node.x);
		shadowEdge.setAttribute("y1", node.y);
	});
};

const createEdge = function(srcNode, destNode) {
	let line = svg.myCreate("line", { x1: srcNode.x, y1: srcNode.y, x2: destNode.x, y2: destNode.y });
	let edge = new Edge(line, srcNode, destNode);
	edges.push(edge);
	
	// event handlers
	line.addEventListener("mouseup", function(event) {
		if (event.button !== 0)  // 0: primary ("left") mouse button
			return;
		
		let index = edges.indexOf(edge);
		if (event.ctrlKey)
			removeEdge(index);
		else
			selectEdge(index);
		event.stopPropagation();
	});
};

/**
 * Looks for a node at the given client-space point in the "nodes" list.
 * The given point can be anywhere "inside" of the found node, not nessesarily at the exact center.
 * @param svgPoint Where the user clicked (in svg-space)
 * @returns The first found node, or "null" if no node is found at/near this location,
 * 	and that node's index (or -1 if not found) as an object: { node: ..., index: ... }
 * @see svg.clientToSVG(clientPoint)
 */
const findNode = function({x, y}) {
	const R2 = R * R;

	for (let i = 0; i < nodes.length; i++) {
		const node = nodes[i];
		const dx = node.x - x, dy = node.y - y;
		if (dx * dx + dy * dy <= R2)
			return { node: node, index: i };
	}
	return { node: null, index: -1 };
};

const findEdge = function(nodeA, nodeB) {
	for (let i = 0; i < edges.length; i++) {
		let edge = edges[i];
		let {srcNode, destNode} = edge;
		if ((nodeA === srcNode && nodeB === destNode) || (nodeA === destNode && nodeB === srcNode))
			return { edge: edge, index: i };
	}
	return { edge: null, index: -1 };
};

const clearSelected = function() {
	if (selected instanceof Node)
		selected.svgCircle.classList.remove("selected");
	else if (selected instanceof Edge)
		selected.svgLine.classList.remove("selected");
	
	selected = null;  // update global
	form.innerHTML = "(blank)";  // TODO: is this what we want?
};

const updateFormWithNode = function(node, index) {
	let form = document.createElement("form");
	form.action = "";
	form.addEventListener("submit", function(event) {
		event.preventDefault();
	});

	let h1 = document.createElement("h1");
	h1.innerText = `Node ${index}`;
	form.append(h1);

	for (let prop of ["Sm", "Fm", "Am", "Je0m"]) {
		let div = document.createElement("div");

		let label = document.createElement("label");
		label.innerText = `${prop}: `;
		label.for = prop;
		div.append(label);

		let input = document.createElement("input");
		input.type = "number";
		input.id = prop;
		input.value = node[prop];
		const onChange = function(event) {
			node[prop] = input.value;
		};
		input.addEventListener("change", onChange);
		input.addEventListener("keypress", onChange);
		div.append(input);

		form.append(div);
	}

	let section = document.getElementById("form");
	section.innerHTML = "";
	section.append(form);
};

const updateFormWithEdge = function(edge, index) {
	// TODO: allow values to be updated
	let html = `<h1> Edge ${index} </h1>`;
	for (let key in edge)
		html += `<p> <b>${key}</b>: ${edge[key]} </p>`;
	form.innerHTML = html;
}

/**
 * "Select" at the given index, the node from the "nodes" list.
 * It's information will be displayed in the "#form" section, and
 * it's parameter's can be updated. 
 */
const selectNode = function(index) {
	let node = nodes[index];
	let wasSelected = (node === selected);
	clearSelected();

	if (!wasSelected) {
		selected = node;  // update global
		node.svgCircle.classList.add("selected");
		updateFormWithNode(node, index);
	}
};

/**
 * "Select" at the given index the edge from the "edges" list.
 * It's information will be displayed in the "#form" sectiom, and
 * it's parameter's can be updated.
 */
const selectEdge = function(index) {
	let edge = edges[index];
	let wasSelected = (edge === selected);
	clearSelected();

	if (!wasSelected) {
		selected = edge;
		edge.svgLine.classList.add("selected");
		updateFormWithEdge(edge, index);
	}
};

/**
 * Remove a node from both the SVG canvas and "nodes" list, along with any connected edges.
 * @param index The index of the node to remove from the "nodes" list.
 * @returns The removed Node.
 */
const removeNode = function(index) {
	let [node] = nodes.splice(index, 1);
	if (node === selected)
		clearSelected()
	node.svgCircle.remove();

	for (let i = 0; i < edges.length; /* iterate in loop */) {
		let edge = edges[i];
		if (node === edge.srcNode || node === edge.destNode)
			removeEdge(i);
		else
			i++;
	}

	return node;
};

/**
 * Remove an edge from both the SVG canvas and "edges" list.
 * @param index The index of the edge to remove from the "edges" list.
 * @returns The removed Edge.
 */
const removeEdge = function(index) {
	let [edge] = edges.splice(index, 1);
	if (edge === selected)
		clearSelected();
	edge.svgLine.remove();
	return edge;
};

// event handler
svg.addEventListener("mousemove", function(event) {
	if (event.button !== 0 || dragging == null)  // 0: primary ("left") mouse button
		return;
	
	let p = svg.clientToSVG({ x: event.clientX, y: event.clientY });
	shadowNode.setAttribute("cx", p.x);
	shadowNode.setAttribute("cy", p.y);
	
	// display or hide shadow node
	let dx = +dragging.svgCircle.getAttribute("cx") - p.x;
	let dy = +dragging.svgCircle.getAttribute("cy") - p.y;
	let dist2 = dx * dx + dy * dy;
	const R2 = R * R;
	if (shadowNode.parentNode === null) {
		if (dist2 > R2)
			svg.append(shadowNode);
	} else {
		if (dist2 < R2)
			shadowNode.remove();
	}

	// display or hide shadow edge
	let {node, index} = findNode(p);
	if (shadowEdge.parentNode === null) {
		if (node !== null && node !== dragging) {
			shadowEdge.setAttribute("x2", node.x);
			shadowEdge.setAttribute("y2", node.y);
			svg.append(shadowEdge);
		}
	} else {
		if (node === null || node === dragging)
			shadowEdge.remove();
	}
});

svg.addEventListener("mouseup", function(event) {
	if (event.button !== 0)  // 0: primary ("left") mouse button
		return;

	let p = svg.clientToSVG({ x: event.clientX, y: event.clientY });
	let {node, index} = findNode(p);
	
	if (node === null) {
		// let go in empty space
		if (dragging === null) {
			createNode(p);
		} else {
			if (p.x >= 80 && p.y <= -80)
				removeNode(nodes.indexOf(dragging));
			else {
				// move node
				dragging.svgCircle.setAttribute("cx", p.x);
				dragging.svgCircle.setAttribute("cy", p.y);
				// move connected edges
				for (let edge of edges)
					if (edge.srcNode === dragging) {
						edge.svgLine.setAttribute("x1", p.x);
						edge.svgLine.setAttribute("y1", p.y);
					} else if (edge.destNode === dragging) {
						edge.svgLine.setAttribute("x2", p.x);
						edge.svgLine.setAttribute("y2", p.y);
					}
			}
		}

	} else if (dragging === node) {
		// clicked on a single node
		selectNode(index);
	
	} else if (dragging !== null) {
		// dragged between two different nodes
		if (findEdge(dragging, node).edge === null)
			createEdge(dragging, node);
	}

	dragging?.svgCircle.classList.remove("dragging");
	dragging = null;
	shadowNode.remove();
	shadowEdge.remove();
});

document.addEventListener("keydown", function(event) {
	if (selected === null)
		return;
	
	let updateInfo = null;
	if (event.key === "ArrowUp")
		updateInfo = { dir: "y", delta: -1, clamp: Math.max, limit: -100 };
	else if (event.key === "ArrowDown")
		updateInfo = { dir: "y", delta: 1, clamp: Math.min, limit: 100 };
	else if (event.key === "ArrowLeft")
		updateInfo = { dir: "x", delta: -1, clamp: Math.max, limit: -100 };
	else if (event.key === "ArrowRight")
		updateInfo = { dir: "x", delta: 1, clamp: Math.min, limit: 100 };8
	if (updateInfo !== null) {
		let { dir, delta, clamp, limit } = updateInfo;
		event.preventDefault();
		selected[dir] = clamp(Math.round(selected[dir]) + delta, limit);
		selected.svgCircle.setAttribute("c" + dir, selected[dir]);
		updateFormWithNode(selected, nodes.indexOf(selected));
	}
});

// TOOD: drag to move nodes

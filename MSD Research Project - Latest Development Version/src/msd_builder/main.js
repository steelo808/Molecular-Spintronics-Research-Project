/**
 * @file main.js
 * @brief Entry point. Called when page loads.
 * @author Christopher D'Angelo
 */

(function() {  // IIFE

// ---- Imports: --------------------------------------------------------------
const { startRendering, BoxRegion, LatticeRegion, YZFaceLatticeRegion } = MSDBuilder.render;
const { initForm } = MSDBuilder.form;
const { Timeline } = MSDBuilder.timeline;


// ---- Main: -----------------------------------------------------------------
const main = () => {
	const { camera, msdView, /* DEBUG: */ scene } = startRendering({
		MSDRegionTypes: [LatticeRegion, YZFaceLatticeRegion],
		// onAnimationFrame: ({ loop }) => {
		// 	camera.rotation.y += 0.0001 * 10 * loop.deltaTime;
		// 	console.log(loop.time, loop.deltaTime);
 		// }
	});
	let timeline = new Timeline("timeline", msdView);
	initForm({ camera, msdView, timeline });

	msdView.rotation.x = Math.PI / 6;
	msdView.rotation.y = -Math.PI / 24;
	
	// renderer.domElement.addEventListener("click", (event) => {
	// 	if (loop.isRunning)
	// 		loop.stop();
	// 	else
	// 		loop.start(update);
	// });

	// DEBUG: make global for testing
	Object.assign(window, { msdView, camera, scene });

	// DEBUG: FBX test
	const { FBXLoader } = Three;  // import
	// TODO: need to server FBX files via server because of CORS
	// console.log(MSDBuilder.assets.TestFBX);
	// new FBXLoader().load(MSDBuilder.assets.TestFBX, fbx => {
	// 	console.log("FBX:", fbx);
	// 	scene.add(fbx);
	// })
};

document.addEventListener("DOMContentLoaded", main);

})();  // end IIFE


let toggleBtn = document.querySelector(".toggle-btn");

toggleBtn.addEventListener("click", () => {
	let dataPanel = document.querySelector(".data-panel");
	console.log("working")

	if (!dataPanel.classList.contains("show")){
		dataPanel.classList.add("show")
	}else{
		dataPanel.classList.remove("show")
	}
})


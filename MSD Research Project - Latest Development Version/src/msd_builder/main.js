(function() {  // IIFE

// ---- Imports: --------------------------------------------------------------
const { startRendering, BoxMSDRegion, LatticeMSDRegion, YZFaceLatticeMSDRegion } = MSDBuilder.render;
const { initForm } = MSDBuilder.form;


// ---- Main: -----------------------------------------------------------------
const main = () => {
	const { camera, msdView, /* DEBUG: */ scene } = startRendering({
		MSDRegionTypes: [LatticeMSDRegion, YZFaceLatticeMSDRegion],
		// onAnimationFrame: ({ loop }) => {
		// 	camera.rotation.y += 0.0001 * 10 * loop.deltaTime;
		// 	console.log(loop.time, loop.deltaTime);
 		// }
	});
	msdView.objects.rotation.x = Math.PI / 6;
	msdView.objects.rotation.y = -Math.PI / 24;
	initForm({ camera, msdView });
	
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
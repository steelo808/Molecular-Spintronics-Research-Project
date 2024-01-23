(function() {  // IIFE

// ---- Imports: --------------------------------------------------------------
const { startRendering, BoxMSDRegion, LatticeMSDRegion } = MSDBuilder.render;
const { initForm } = MSDBuilder.form;


// ---- Main: -----------------------------------------------------------------
const main = () => {
	const { camera, msdView } = startRendering({
		MSDRegionTypes: [LatticeMSDRegion, BoxMSDRegion],
		/* onAnimationFrame: ({ loop }) => console.log(loop.deltaTime) */ });
	msdView.objects.rotation.x = Math.PI / 6;
	msdView.objects.rotation.y = -Math.PI / 24;
	initForm({ camera, msdView });
	// const { camera, renderer, msd: msdView } = startRendering({ onAnimationFrame: ({ loop, msd: msdView }) => {
	// 	msd.objects.rotation.y += 0.0001 * loop.deltaTime;
	// 	console.log(loop.time, loop.deltaTime);
	// } });
	
	// renderer.domElement.addEventListener("click", (event) => {
	// 	if (loop.isRunning)
	// 		loop.stop();
	// 	else
	// 		loop.start(update);
	// });
};

document.addEventListener("DOMContentLoaded", main);

})();  // end IIFE
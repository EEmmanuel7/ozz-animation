ozz-animation provides many samples to help with this first integration steps:
  * **[playback](SamplePlayback.md)** : Uses dae2skel and dae2anim command line tools to convert Collada data to ozz run-time data, as part of the build process. The samples then loads/de-serializes run-time data from the files produced by the tools, samples and displays the animated skeleton.
  * **[blending](SampleBlend.md)** : Load, samples and blends two animations, exposing all run-time parameters through the gui.
  * **[partial blending](SamplePartialBlend.md)** : Uses partial animation blending technique to animate the lower and upper part of the skeleton with different animations.
  * **[attach](SampleAttach.md)** : Demonstrates how to attach an object to an animated skeleton's joint.
  * **[multithreads](SampleMultithread.md)** : Distributes sampling and local-to-model jobs across multiple threads, using OpenMp.
  * **[millipede](SampleMillipede.md)** : Demonstrates usage of ozz offline data structures and utilities.
  * **[optimize](SampleOptimize.md)** : Executes ozz::animation::offline::AnimationOptimizer on a raw animation object to remove redundant keyframes (within a tolerance).
  * **[skinning](SampleSkin.md)** : Executes ozz::geometry::SkinningJob on a loaded mesh to perform matrix palette skinning transformation.
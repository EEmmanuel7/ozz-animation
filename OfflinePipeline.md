# Pipeline description #
ozz-animation provides full support for Collada and Fbx formats. Those formats are heavily used by the animation industry and supported by all major Digitatl Content Creation tools (Maya, Max, MotionBuilder, Blender...). ozz-animation offline pipeline aims to convert from these DCC offline formats (or any proprietary format) to ozz internal runtime format, as illustrated below:

<img src='http://ozz.qualipilote.fr/images/pipeline.png' alt='ozz-animation offline pipeline'>

In a way or another, the aim of this pipeline and importer tools is to end up with runtime data structures (ozz::animation::Skeleton, ozz::animation::Animation) that can be used with the run-time libraries, on any platform. Run-time libraries provide jobs and data structures to process runtime operations like sampling, blending...<br>
<br>
<h2>Standard pipeline</h2>
ozz-animation provides a standard pipeline to import skeletons and animations from Collada documents and Fbx files. This pipeline is based on command-line tools: dae2skel and dae2anim respectively for Collada skeleton and animation, fbx2skel and fbx2anim for Fbx.<br>
<br>
dae2skel and fbx2skel command line tools require as input a file containing the skeleton to import (dae or fbx file), using --file argument. Output filename is specified with --skeleton argument.<br>
<pre><code>dae2skel --file="path_to_source_skeleton.dae" --skeleton="path_to_output_skeleton.ozz"  <br>
</code></pre>
The extension of the output file is not important though.<br>
<br>
dae2anim and fbx2anim command line tools require two inputs:<br>
<ol><li>A dae or fbx file containing the animation to import, specified with --file argument.<br>
</li><li>A file containing an ozz runtime skeleton (usually outputted by dae2skel or fbx2skel), specified with --skeleton argument.<br>
Output filename is specified with --animation argument. Like for skeleton, the extension of the output file is not important.<br>
<pre><code>dae2anim --file="path_to_source_animation.dae" --skeleton="path_to_skeleton.ozz" --animation="path_to_output_animation.ozz"  <br>
</code></pre></li></ol>

Some other optional arguments can be provided to the animation importer tools, to setup keyframe optimization ratios, sampling rate... The detailed usage for all the tools can be obtained with --help argument.<br>
<br>
Collada and Fbx importer c++ sources and libraries (ozz_animation_collada and ozz_animation_fbx) are also provided to integrate their features to any application.<br>
<br>
<h2>Custom pipeline</h2>
Offline ozz-animation c++ libraries (ozz_animation_offline) are available, in order to implement custom format importers. This library exposes offline skeleton and animation data structures. They are then converted to runtime data structures using respectively SkeletonBuilder and AnimtionBuilder utilities. Animations can also be optimized (removing redundant keyframes) using AnimationOptimizer. Usage of ozz_animation_offline library is demonstrated by <a href='SampleMillipede.md'>SampleMillipede</a> and <a href='SampleOptimize.md'>SampleOptimize</a> samples.<br>
<br>
<h1>ozz-animation offline data structures</h1>
Offline c++ data structure are designed to be setup and modified programmatically: add/remove joints, change animation duration and keyframes, validate data integrity... These structures define a clear public API to emphasize modifiable elements. They are intended to be used on the tool side, offline, to prepare and build runtime structures.<br>
<h2>ozz::animation::offline::RawSkeleton</h2>
This structure declared in ozz/animation/offline/skeleton_builder.h. It is used to define the offline skeleton object that can be converted to the runtime skeleton using the SkeletonBuilder.<br>
This skeleton structure exposes joints' hierarchy. A joint is defined with a name, a transformation (its bind pose), and its children. Children are exposed as a public std::vector of joints. This same type is used for skeleton roots, also exposed from the public API.<br>
The public API exposed through std:vector's of joints can be used freely with the only restriction that the total number of joints does not exceed Skeleton::kMaxJoints.<br>
<h2>ozz::animation::offline::RawAnimation</h2>
This structure declared in ozz/animation/offline/animation_builder.h. It is used to define the offline animation object that can be converted to the runtime animation using the AnimationBuilder.<br>
This animation structure exposes tracks of keyframes. Keyframes are defined with a time and a value which can either be a translation (3 float x, y, z), a rotation (a quaternion) or scale coefficient (3 floats x, y, z). Tracks are defined as a set of three std::vectors (translation, rotation, scales). Animation structure is then a vector of tracks, along with a duration value.<br>
Finally the RawAnimation structure exposes Validate() function to check that it is valid, meaning that all the following rules are respected:<br>
<ol><li>Animation duration is greater than 0.<br>
</li><li>Keyframes' time are sorted in a strict ascending order.<br>
</li><li>Keyframes' time are all within [0,animation duration] range.</li></ol>

Animations that would fail this validation will fail to be converted by the AnimationBuilder.<br>
A valid RawAnimation can also be optimized (removing redundant keyframes) using AnimationOptimizer. See <a href='SampleOptimize.md'>optimize sample</a> for more details.<br>
<br>
<h1>ozz-animation offline utilities</h1>
ozz offline utilities are usually conversion functions, like ozz::animation::offline::SkeletonBuilder and ozz::animation::offline::animationBuilder are. Using the "builder" design approach frees the user from understanding internal details about the implementation (compression, memory packing...). It also allows to modify ozz internal implementation, without affecting existing user code.<br>
<h2>ozz::animation::offline::SkeletonBuilder</h2>
The SkeletonBuilder utility class purpose is to build a <a href='RuntimePipeline#ozz::animation::Skeleton.md'>runtime skeleton</a> from an offline raw skeleton. Raw data structures are suited for offline task, but are not optimized for runtime constraints.<br>
<br>
<ul><li><b>Inputs</b>
<ol><li>ozz::animation::offline::RawSkeleton <i>raw_skeleton</i>
</li></ol></li><li><b>Processing</b>
<ol><li>Validates input.<br>
</li><li>Builds skeleton breadth-first joints tree.<br>
</li><li>Packs bind poses to soa data structures.<br>
</li><li>Fills other runtime skeleton data structures (array of names...).<br>
</li></ol></li><li><b>Outputs</b>
<ol><li>ozz::animation::Skeleton <i>skeleton</i></li></ol></li></ul>

<h2>ozz::animation::offline::AnimationBuilder</h2>
The AnimationBuilder utility class purpose is to build a runtime animation from an offline raw animation. The raw animation has a simple API based on vectors of traks and key frames, whereas the <a href='RuntimePipeline#ozz::animation::Animation.md'>runtime animation</a> has compressed key frames strucutred and organised to maximize performance and cache coherency.<br>
<br>
<ul><li><b>Inputs</b>
<ol><li>ozz::animation::offline::RawAnimation <i>raw_animation</i>
</li></ol></li><li><b>Processing</b>
<ol><li>Validates input.<br>
</li><li>Creates required first and last keys for all tracks (if missing).<br>
</li><li>Sorts keys to match runtime sampling job requirements, maximizing cache coherency.<br>
</li><li>Compresses translation and scales to three half floating point values, and quantizes quaternions to three 16bit integers (4th component is recomputed at runtime).<br>
</li></ol></li><li><b>Outputs</b>
<ol><li>ozz::animation::Animation <i>animation</i>
<h2>ozz::animation::offline::AnimationOptimizer</h2>
The AnimationOptimizer strips redudant key frames (within a toleracne) from a raw animation. It doesn't actually modify input animtion, but builds a second one. Toleances are provided as input arguments, for every key frame type: translation, rotation and scale.</li></ol></li></ul>

<ul><li><b>Inputs</b>
<ol><li>ozz::animation::offline::RawAnimation <i>raw_animation</i>
</li><li>float <i>translation_tolerance</i>: Translation optimization tolerance, defined as the distance between two values in meters.<br>
</li><li>float <i>rotation_tolerance</i>: Rotation optimization tolerance, ie: the angle between two rotation values in radian.<br>
</li><li>float scale_tolerance: Scale optimization tolerance, ie: the norm of the difference of two scales.<br>
</li></ol></li><li><b>Processing</b>
<ol><li>Validates input.<br>
</li><li>Filters all key frames that can be interpolated within the specified tolerance.<br>
</li><li>Builds output raw animation.<br>
</li></ol></li><li><b>Outputs</b>
<ol><li>ozz::animation::offline::RawAnimation <i>raw_animation</i>
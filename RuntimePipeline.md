# ozz-animation runtime data structures #
Runtime structures are optimized, cache friendly, c++ data structures. Their data-oriented design aims to organize them (memory wise) according to the way they are consumed at runtime, maximizing cpu capabilities, trading-off memory footprint.
Ozz-animation runtime data structures cannot be modified programmatically, all data accessors are const. They are converted from offline data structures (RawAnimation and RawSkeleton), or read (de-serialized) from a stream. They are intended to be used by ozz animation runtime jobs.

## ozz::animation::Skeleton ##
The runtime skeleton data structure represents a skeletal hierarchy of joints, with a bind-pose (rest pose of the skeleton). This structure is filled by the SkeletonBuilder (usually offline) and deserialized/loaded at runtime.
Joint names, joint bind-poses and hierarchical informations are all stored in separate arrays of data (as opposed to joint structures for the RawSkeleton), in order to closely match with the way runtime algorithms use them. Joint hierarchy is packed as an array of 16 bits element per joint (see JointProperties below), stored in breadth-first order.
```
  struct JointProperties {
    // Parent's index, kNoParentIndex for the root.
    uint16 parent: Skeleton::kMaxJointsNumBits;
    
    // Set to 1 for a leaf, 0 for a branch.
    uint16 is_leaf: 1;
  };
```
JointProperties::parent member is enough to traverse the whole joint hierarchy in breadth-first order. JointProperties::is\_leaf is a helper that is used to speed-up some algorithms: See IterateJointsDF() from skeleton\_utils.h that implements a depth-first traversal utility.

## ozz::animation::Animation ##
The runtime animation data structure stores compressed animation keyframes, for all the joints of a skeleton. Translations and scales are stored using 3 half floats (3`*`16bits) while rotations (a quaternion) are quantized on 3 16bits integers (the 4th component is recomputed at runtime). This structure is usually filled by the AnimationBuilder and deserialized/loaded at runtime.
For each transformation type (translation, rotation and scale), Animation structure stores a single array of keyframes that contains all the tracks required to animate all the joints of a skeleton, matching breadth-first joints order of the runtime skeleton structure. Keyframes in this array are sorted by time, then by track number. This is the comparison function "less" used be the sorting algorithm of the AnimationBuilder implementation:
```
bool SortingKeyLess(const _Key& _left, const _Key& _right) {
  return _left.prev_key_time < _right.prev_key_time
         || (_left.prev_key_time == _right.prev_key_time
             && _left.track < _right.track);
}
```
Note that in fact "previous keyframe time" (which is only available in the offline sorting data structure) is used, instead of actual keyframe's time. This allows to optimize the search loop while sampling, as explained later.

This table shows the data layout of an animation of 1 second duration, 4 tracks (track0-3) and 17 keyframes (0-16)

![http://ozz.qualipilote.fr/images/animation-tracks.png](http://ozz.qualipilote.fr/images/animation-tracks.png)

Note that keyframes are mandatory at t=0 and t=duration, for all tracks, in order to optimize sampling algorithm. This constrain is automatically handled by the AnimationBuilder utility though.

The color of each cell (as well as key numbers) refers to its index in the array, and thus its location in memory. This sorting strategy has 2 goals:
  * This is a cache-friendly strategy that groups keyframes (for all tracks/joints) such that all keyframes needed at a time t (during sampling) are close one to each other in memory.
  * Moving forward in the animation time only requires to move the time-cursor further, without traversing all tracks.

The following diagrams show "hot" keys, the one that are accessed during sampling. These keys are decompressed and stored in the ozz::animation::SamplingCache in a SoA format suited for interpolation computations. The cache stores 2 keyframes per track, as they are required for interpolation when sampling-time is between 2 keyframes.

  * To sampling at time = 0, the sampling algorithm goes through the 8 first keys and finds all the keys needed. The SamplingCache also stores the sampling-cursor (the red arrow) at key 8, to be able to start again from there if sampling-time is moved further.
![http://ozz.qualipilote.fr/images/animation-tracks-t0.png](http://ozz.qualipilote.fr/images/animation-tracks-t0.png)
  * When sampling at time = 0.1, the cache is already filled (as shown by orange keys) with all the required data (decompressed SoA keyframes), the same one used at time = 0. The sampling-cursor doesn't have to be moved either.
![http://ozz.qualipilote.fr/images/animation-tracks-t0.1.png](http://ozz.qualipilote.fr/images/animation-tracks-t0.1.png)
  * Sampling-time 0.3 is higher that latest sampling-cursor, so it has to be incremented up to key 10. Note that very few increments are needed, and that hot keys are still grouped.
![http://ozz.qualipilote.fr/images/animation-tracks-t0.3.png](http://ozz.qualipilote.fr/images/animation-tracks-t0.3.png)
  * Moving a bit further to time = 0.7 requires to increment the sampling-cursor again. Note that hot keys are also quite grouped, despite a few holes.
![http://ozz.qualipilote.fr/images/animation-tracks-t0.7.png](http://ozz.qualipilote.fr/images/animation-tracks-t0.7.png)
  * Finally moving to time = 1, the cursor is pushed up to the last key, with most of the hot keys close one to each other in memory, already cached and decompressed (orange keys).
![http://ozz.qualipilote.fr/images/animation-tracks-t1.png](http://ozz.qualipilote.fr/images/animation-tracks-t1.png)

The sampling algorithm needs to access very few keyframes when moving forward in the timeline, thanks to the way keyframe are stored.

However this strategy doesn't apply to move the time cursor backward. The current implementation restarts sampling from the beginning when the animation is played backward (similar behavior to reseting the cache).

## Mathematical structures ##
The runtime pipeline also refers to generic c++ mathematical structures like vectors, quaternions, matrices, ozz:math::Transform (translation, rotation and scale). It especially makes a heavy use of Struct-of-Array mathematical structures available in ozz for optimum memory access patterns and maximize SIMD instruction (SSE2, AVX...) throughput. This [document from Intel](http://software.intel.com/en-us/articles/how-to-manipulate-data-structure-to-optimize-memory-use-on-32-bit-intel-architecture) gives more details about the AoS/SoA layouts and their benefit.

## Local-space and model-space coordinate systems ##
ozz-animation represents transformations as either in local or model-space coordinate systems:
  * Local-space: The transformation of a joint in local space is relative to its parent.
  * Model-space: The transformation of a joint in model space is relative to the root of the hierarchy.

Most of the processes in animation are done in local space (sampling, blending...). Local-space to model-space conversion is usually done by [LocalToModelJob](https://code.google.com/p/ozz-animation/wiki/RuntimePipeline#LocalToModelJob) at the end of the animation runtime pipeline, to prepare transformations for rendering. Data in ozz are structured according to the processing done on the data. Local space transforms are thus stored as separate SoA affine components (translation, rotation, scale), suitable for sampling, blending and global (whole skeleton) processes. Model space transforms are AoS matrices, suitable for rendering and per joint processes.

# Runtime jobs #
This section describes c++ runtime jobs, aka data processing units, provided by ozz-animation library.

All jobs follow the same principals: They process inputs (read-only or read-write) and fill outputs (write only). Both inputs and outputs are provided to jobs by the user. Jobs never access any memory outside of these inputs and outputs, making them intrinsically thread safe (race condition wise).

## SamplingJob ##
The SamplingJob is in charge of computing the local-space transformations (for all tracks) at a given time of an animation. The output could be considered as a pose, a snapshot of all the tracks of an animation at a specific time.
The SamplingJob uses a cache (aka SamplingCache) to store intermediate values (decompressed animation keyframes...) while sampling, which are likely to be used for the subsequent frames thanks to interpolation. This cache also stores pre-computed values that allows drastic optimization while playing/sampling the animation forward. Backward sampling works of course but isn't optimized through the cache though, favoring memory footprint over performance in this case.

  * **Inputs**
    1. ozz::animation::Animation _animation_
    1. float _time_
    1. ozz::animation::SamplingCache _sampling\_cache_
  * **Processing**
    1. Validates job.
    1. Finds relevant keyframes for the sampling _time_, for all tracks, for all trasnsformations type (translation, rotation and scale). Finding will take advantage of the _sampling\_cache_ if possible.
    1. Decompress relevant animation keyframes.
    1. Pack keyframes in SoA structures (ozz::math::SoATransform).
    1. Updates the _sampling\_cache_ with intermediary results.
    1. Interpolates keyframes (SoA form) and fills local-space transformations output _transforms_ (SoA form also).
  * **Outputs**
    1. ozz::math::SoATransform`*` _transforms_

Relying on the fact that the same set of keyframes can be relevant from an update to the next (because keyframes are interpolated), the _sampling\_cache_ stores data as decompressed SoA structures. This factorizes decompression and SoA conversion cost. Furthermore, interpolating SoA structures is optimal.

See [SamplePlayback](SamplePlayback.md) for a simple use case.

## BlendingJob ##
The BlendingJob is in charge of blending (mixing) multiple poses (the result of a sampled animation) according to their respective weight, into one output pose. The number of transforms/joints blended by the job is defined by the number of transforms of the bind pose (note that this is a SoA format). This means that all buffers must be at least as big as the bind pose buffer.
Partial animation blending is supported through optional joint weights that can be specified for each layer's joint\_weights buffer. Unspecified joint weights are considered as a unit weight of 1.f, allowing to mix full and partial blend operations in a single pass.

  * **Inputs**
    1. BlendingJob::Layer
      1. float _weight_: Overall layer weight.
      1. ozz::math::SoATransform`*` _transforms_: Posture in local space.
      1. ozz::math::SimdFloat4`*` _joint\_weights_: Optional range of blending weight for each joint.
    1. float _threshold_: Minimum weight before blending to the bind pose.
    1. ozz::math::SoATransform`*` _bind\_pose_
  * **Processing**
    1. Validates job.
    1. Blends all layers together, according to layer and joint's weight.
    1. Blends bind pose to all joints whose weight is lower than thresold.
    1. Normalizes ouput.
  * **Outputs**
    1. ozz::math::SoATransform`*` _ouput_

See [SampleBlend](SampleBlend.md) that blends 3 locomotion animations (walk/jog/run) depending on a target spedd factor, and [SamplePartialBlend](SamplePartialBlend.md) that blends a specific animation to the character upper-body.

## LocalToModelJob ##
The LocalToModelJob is in charge of converting [local-space to model-space coordinate system](https://code.google.com/p/ozz-animation/wiki/RuntimePipeline#Local-space_and_model-space_coordinate_systems). It uses the skeleton to define joints parent-child hierarchy. The job iterates through all joints to compute their transform relatively to the skeleton root.
Job inputs is an array of SoaTransform objects (in local-space), ordered like skeleton's joints. Job output is an array of matrices (in model-space), also ordered like skeleton's joints. Output are matrices, because the combination of affine transformations can contain shearing or complex transformation that cannot be represented as affine transform objects.

Note that this job also intrinsically unpack SoA input data (ozz::math::SoATransform) to standard AoS matrices (ozz::math::Float4x4) on output.

  * **Inputs**
    1. ozz::animation::Skeleton`*` _skeleton_: The skeleton object defining joints hierarchy.
    1. ozz::math::SoATransform`*` _input_: Input SoA transforms in model space.
  * **Processing**
    1. Validates job.
    1. Builds joints matrices from input affine transformations (SoATransform).
    1. Concatenate matrices according to skeleton joints hierarchy.
  * **Outputs**
    1. ozz::math::Float4x4`*` _output_: Output matrices in model spaces.

All samples use LocalToModelJob. See [SamplePlayback](SamplePlayback.md) for a simple use case.

## SkinningJob ##
The SkinningJob is part of ozz\_geometry library in ozz::geometry namespace and provides per-vertex matrix palette skinning job implementation. Skinning is the process of creating the association of skeleton joints with some vertices of a mesh. Portions of the mesh's skin can normally be associated with multiple joints, each one having a weight. The sum of the weights for a vertex is equal to 1. To calculate the final position of the vertex, each joint transformation is applied to the vertex position, scaled by its corresponding weight. This algorithm is called matrix palette skinning because of the set of joint transformations (stored as transform matrices) form a palette for the skin vertex to choose from.

This job iterates and transforms vertices (points and vectors) provided as input using the matrix palette skinning algorithm. The implementation supports any number of joints influences per vertex, and can transform up to one point (vertex position) and two vectors (vertex normal and tangent) per loop (aka per vertex). It assumes bi-normals aren't needed as they can be rebuilt from the normal and tangent with a lower cost than skinning (a single cross product). Input and output buffers must be provided with a stride value (aka the number of bytes from a vertex to the next). This allows the job to support vertices packed as array-of-structs (array of vertices with positions, normals...) or struct-of-arrays (buffers of points, buffer of normals...).

Note that ozz does not propose any production ready tool to load meshes or any mesh format. See [SampleSkin](SampleSkin.md) fbx2skin for a example of how to import a skinned mesh from fbx.

The skinning job optimizes every code path at maximum. The per vertex loop is indeed not the same depending on the number of joints influencing a vertex (or if there are normals to transform). To maximize performances, application should partition its vertices based on their number of joints influences, and run a different job for every vertices set. Joint matrices are accessed using the per-vertex joints indices provided as input. These matrices must be pre-multiplied with the inverse of the skeleton bind-pose model-space matrices. This allows to transform vertices to joints local space. In case of non-uniform-scale matrices, the job proposes to transform vectors using an optional set of matrices, whose are usually inverse transpose of joints matrices (see http://www.glprogramming.com/red/appendixf.html). This code path is less efficient than the one without this matrices set, and should only be used when input matrices have non uniform scaling or shearing.

  * **Inputs**
    1. int vertex\_count: Number of vertices to transform. All input and output arrays must store at least this number of vertices.
    1. int influences\_count: Maximum number of joints influencing each vertex. The number of influences drives how joint\_indices and joint\_weights are sampled. influences\_count joint indices are red from joint\_indices for each vertex,influences\_count - 1 joint weights are red from joint\_weightrs for each vertex. The weight of the last joint is restored (as weights are normalized).
    1. Range<const math::Float4x4> joint\_matrices: Array of matrices for each joint. Joint are indexed through indices array.
    1. Range<const math::Float4x4> joint\_inverse\_transpose\_matrices: Optional array of inverse transposed matrices for each joint. If provided, this array is used to transform vectors (normals and tangents), otherwise joint\_matrices array is used. Transforming vectors requires a special attention when the transformation matrix has scaling or shearing. In this case the right transformation is done by the inverse transpose of the transformation that transforms points. Any rotation matrix is good though. These matrices are optional as they might by costly to compute, and also fall into a more costly code path in the skinning algorithm.
    1. Range<const uint16\_t> joint\_indices: Array of joints indices. This array is used to indexes matrices in joints array. Each vertex has influences\_max number of indices, meaning that the size of this array must be at least influences\_max `*` vertex\_count.
    1. size\_t joint\_indices\_stride: This is the stride (offset) in byte in joint\_indices buffer to move from one vertex to the next.
    1. Range<const float> joint\_weights: Array of joints weights. This array is used to associate a weight to every joint that influences a vertex. The number of weights required per vertex is "influences\_max - 1". The weight for the last joint (for each vertex) is restored at runtime thanks to the fact that the sum of the weights for each vertex is 1. Each vertex has (influences\_max - 1) number of weights, meaning that the size of this array must be at least (influences\_max - 1) `*` vertex\_count.
    1. size\_t joint\_weights\_stride: This is the stride (offset) in byte in joint\_weights buffer to move from one vertex to the next.
    1. Range<const float> in\_positions: Input vertex positions array made of 3 float values per vertex. Array length must be at least vertex\_count `*` in\_positions\_stride.
    1. size\_t in\_positions\_stride: This is the stride (offset) in byte in in\_positions buffer to move from one vertex to the next.
    1. ...
    1. Remaining inputs are optional normal and tangent buffers. Note that tangents buffer cannot be transformed if normals aren't. If this case is required, tangents should be provided in the normals buffer, as they are transformed the same way.
  * **Processing**
    1. Validates job.
    1. Deduce optimal skinning algorithm based on job inputs (influences, inverse transpose matrices, normals...) and run it.
    1. For each vertex
      1. Computes the transform matrix based on joint indices and weights.
      1. If a joint\_inverse\_transpose\_matrices is provided, computes the normal and tangent transform matrix.
      1. Load, transform (based on vertex transformation matrix) and store vertex in the output buffers.
  * **Outputs**
    1. Range`<`float`>` out\_positions: Output vertex positions (3 float values per vertex) array. Array length must be at least vertex\_count `*` out\_positions\_stride.
    1. size\_t out\_positions\_stride: This is the stride (offset) in byte in out\_positions buffer to move from one vertex to the next.
    1. ...
    1. Remaining outputs are optional normal and tangent buffers. Note that this buffers must be provided if there corresponding input one are.
See [SampleSkin](SampleSkin.md) for a simple use case.
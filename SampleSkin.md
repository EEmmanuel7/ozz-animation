## Description ##
Skinning is the process of creating the association of skeleton joints with some vertices of a mesh. This sample demonstrates ozz SkinningJob usage which transforms mesh vertices using the matrix palette skinning algorithm.

&lt;wiki:gadget url="http://ozz.qualipilote.fr/gadgets/samples/skin/sample\_skin.xml" width="800" height="450" border="1" /&gt;

## Concept ##
This sample is based on the [playback sample](SamplePlayback.md) to load and animate a skeleton. Animated model-space matrices (outputted from the animation stage) are used to build input matrices for the SkinningJob. Skinning job input vertices come from a mesh loaded by the sample. Note that this is just an example, as the skinning job is generic and not bound to a specific mesh format. Skinning job output (skinned) vertices are directly mapped to the graphic API vertex buffer.
The sample also implement a tool (fbx2skin) to import and serialize a skinned mesh from a Fbx file. Note that this tool isn't production ready and only targets this sample needs.

## Sample usage ##
Some parameters can be tuned from the sample UI:
  * Limit the maximum number of joints influencing a vertex.
  * Display joint influences.
  * Animation control
    * Play/pause animation.
    * Fix animation time.
    * Set playback speed, which can be negative to go backward.

## Sample implementation ##
Skeleton and animation loading, animation playback, are based on the [playback sample](SamplePlayback.md) and are not discussed here.
  1. During initialization
    1. Load the mesh.
      1. Open a ozz::io::OArchive object with a valid ozz::io::Stream as argument. The stream can be a ozz::io::File, or your custom io read capable object that inherits from ozz::io::Stream.
      1. Check that the stream stores the mesh object type using ozz::io::OArchive::TestTag() function. Mesh type is ozz::Sample::SkinnedMesh, part of ozz sample namespace (but not ozz libraries).
      1. Deserialize the SkinnedMesh object with stream >> operator.
    1. Compute joints inverse bind-pose matrices. Bind-poses are obtained from the skeleton which stores them in local-space, where we need them in model-space. ozz::animation::LocalToModelJob is used to convert skeleton bind-pose SoaTransfrom to model-space matrices, before inverting them. This is only done once at initialisation stage, as bind-pose is static.
  1. Every frame
    1. Compute skinning matrices. Skinning matrices are computed at the end of the animation stage. They are the result of the multiplication of model-space matrices with joints inverse bind-pose matrices. This step is required as vertices are stored in model-space.
    1. Run skinning job. The job is perfomed on all vertices of the mesh. As an optimisation, the mesh is partitionned according to the number of joints influencing each vertex. So one job is ran per partition, aka per number of influencinng joints. Partitionning is done offline by the mesh importer tool used by the sample.

## Sample's mesh importer tool (fbx2skin) implementation ##
This command line tool loads a mesh from a fbx file and serialize it to an ozz archive.
  1. Loads the mesh from the fbx file, with positions, normals, as well as skinning informations aka joint and weights.
  1. Loads the skeleton object given as argument to the tool, and build a mapping table from joint names to joint indices. This is used to convert fbx joint names to joint indices.
  1. Partitions the mesh according to the number of joint influencing each vertex. This pre-process allows to run the optimum skinning job variant for each vertex set.
  1. Serialize the mesh to the output stream.
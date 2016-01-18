> Note for windows users: On Windows, this sample requires "VC++ Redistribuable Packages" to be installed. OpenMp runtime libraries are only available as Dlls (vcomp`*`.dll) indeed. Latest packages are available here: https://support.microsoft.com/kb/2019667

## Description ##
The sample takes advantage of ozz jobs thread-safety to distribute sampling and local-to-model jobs across multiple threads, using OpenMp.
User can tweak the number of characters and the number of threads. Animation control is automatically handled by the sample for all characters.

<img src='http://ozz.qualipilote.fr/images/multithread.jpg' alt='ozz-animation offline pipeline'>

<h2>Concept</h2>
All ozz jobs are thread-safe: ozz::animation::SamplingJob, ozz::animation::BlendingJob, ozz::animation::LocalToModelJob... This is an effect of the data-driven architecture, which makes a clear distinction between data and processes (aka jobs). Jobs' execution can thus be distributed to multiple threads safely, as long as the data provided as inputs and outputs do not create any race conditions.<br>
As a proof of concept, this sample uses a naive strategy: All characters' update (execution of their sampling and local-to-model stages, as demonstrated in playback sample) are distributed using an OpenMp parallel-for, every frame. During initialization, every character is allocated all the data required for their own update, eliminating any dependency and race condition risk.<br>
<br>
<h2>Sample usage</h2>
The sample allows to switch OpenMp multi-threading on/off and set the number of threads used to distribute characters' update. The number of characters can also be set from the GUI.<br>
<br>
<h2>Implementation</h2>
<ol><li>This sample extends "playback" sample, and uses the same procedure to load skeleton and animation objects:<br>
<ol><li>Open a ozz::io::OArchive object with a valid ozz::io::Stream as argument. The stream can be a ozz::io::File, or your custom io read capable object that inherits from ozz::io::Stream.<br>
</li><li>Check that the stream stores the expected object type using ozz::io::OArchive::TestTag() function. Object type is specified as a template argument.<br>
</li><li>De-serialize the object with >> operator.<br>
</li></ol></li><li>For each character, allocates runtime buffers (local-space transforms of type ozz::math::SoaTransform, model-space matrices of type ozz::math::Float4x4) with the number of elements required for your skeleton, and a sampling cache (ozz::animation::SamplingCache). Only the skeleton and the animation are shared amongst all characters, as they are read only objects, not modified during jobs execution.<br>
</li><li>Update function uses an OpenMp parallel-for loop to split up characters' update loop amongst OpenMp threads (sampling and local-to-model jobs execution), allowing all characters' update to be executed concurrently. See "playback" sample for more details about each character update function.
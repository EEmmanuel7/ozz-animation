# What is ozz-animation #
ozz-animation is an open source **c++ 3d skeletal animation engine**. It provides runtime **character animation playback** fonctionnalities (loading, sampling, blending...), with full support for **Collada** and **Fbx** import formats. It proposes a low-level **renderer agnostic** and **game-engine agnostic** implementation, focusing on performance and memory constraints with a data-oriented design.

ozz-animation comes with **Collada** and **Fbx** toolchains to convert from major Digital Content Creation formats to ozz optimized runtime structures. Offline libraries are also provided to implement the conversion from any other animation and skeleton formats.

# What ozz-animation is not #
ozz-animation is not a high level animation blend tree. It proposes the mathematical background required for playing animations and a pipeline to build/import animations. It lets you organise your data and your blending logic the way you want.

ozz-animation doesn't do any rendering either. You're in charge of applying the output of the animation stage (one matrix per joint) to your skins, or to your scene graph nodes.

Finally ozz does not propose the pipeline to load meshes and materials either.

# Pipeline #
ozz-animation comes with **Collada** and **Fbx** tool chains to convert from major Digital Content Creation tools' format to ozz optimized runtime structures. Offline libraries are also provided to implement the convertion from any other animation (and skeleton) formats.

![http://ozz.qualipilote.fr/images/pipeline.png](http://ozz.qualipilote.fr/images/pipeline.png)

Jump to [offline pipeline](OfflinePipeline.md) for a description of ozz-animation offline pipeline tools and features.

Jump to [runtime pipeline](RuntimePipeline.md) for a description of ozz-animation runtime features.

# License #
ozz-animation is published under the very permissive zlib/libpng license, which allows to modify and redistribute sources or binaries with very few constraints. The license only has the following points to be accounted for ([wikipedia](http://en.wikipedia.org/wiki/Zlib_License)):
  * Software is used on 'as-is' basis. Authors are not liable for any damages arising from its use.
  * The distribution of a modified version of the software is subject to the following restrictions:
    1. The authorship of the original software must not be misrepresented,
    1. Altered source versions must not be misrepresented as being the original software, and
    1. The license notice must not be removed from source distributions.
The license does not require source code to be made available if distributing binary code.

# Reporting issues or feature requests #
[This page](FeatureMap.md) lists implemented features and plans for future releases. Please use [Issues tab](http://code.google.com/p/ozz-animation/issues/list) to report bugs and feature requests.

<img src='http://upload.wikimedia.org/wikipedia/commons/thumb/a/ad/Logo_of_Google_Drive.svg/260px-Logo_of_Google_Drive.svg.png' alt='Feedback form' height='24' width='24'> Use the anonymous <a href='https://docs.google.com/forms/d/1RscU59w7TkCvOsGUXKM7Lg0Ud0eGpl_CQnryDID91Ws/viewform'>user feedback form</a> to share your thoughts and needs.<br>
<br>
<h1>Contributing</h1>
Any contribution is welcome: code review, bug report or fix, feature request or implementation. Don't hesitate...
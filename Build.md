## Building ozz librairies, tools and samples ##
The build process relies on cmake (www.cmake.org), which allows to propose a portable build system. It is setup to build ozz-animation libraries, tools and samples (along with their data), but also to run unit-tests and package sources/binary distributions. See [the feature-map page](FeatureMap.md) for a list of [supported OS](FeatureMap#Os_support.md) and [compilers](FeatureMap#Compiler_support.md).

You can run CMake as usual from ozz-animation root directory. It is recommended to build out-of-source though (creating "build" directory and running CMake from there):
```
mkdir build
cd build
cmake ..
cmake --build ./ --config Release
```
An optional python script _build-helper.py_ is available in the root directory to help performing all operations:
  * Build from sources.
  * Run unit-tests.
  * Package sources and binaries.
  * Configure CMake.
  * Clean build directory.
  * ...

<img src='http://my.cdash.org/images/cdash.gif' alt='CDash' height='24' width='24'> The nightly build dashboard (based on CDash) is <a href='http://ozz.qualipilote.fr/dashboard/cdash/'>available there</a>. It allows to monitor main branch build status for all supported OS.<br>
<br>
<h2>Integrating ozz in your project</h2>
To use ozz animation in an application, you'll need to add ozz <i>include/</i> path to your project's header search path so that your compiler can include ozz files (paths starting with ozz/...).<br>
Then you'll have to setup the project to link with ozz libraries:<br>
<ul><li>ozz_base: Required for any other library. Integrates io, math...<br>
</li><li>ozz_animation: Integrates all animation runtime jobs, sampling, blending...<br>
</li><li>ozz_animation_offline: Integrates ozz offline libraries, animation builder, optimizer...<br>
</li><li>ozz_geometry: Integrates mesh/geometry runtime jobs, skinning...</li></ul>

Some other libraries can also be found for tools, Collada and Fbx... Note that pre-built libraries, tools and samples can be found on the <a href='Downloads.md'>download</a> section.<br>
<br>
<h2>Integrating ozz sources in your build process</h2>
Offline and runtime sources can also be integrated to your own build process. ozz does not rely on any configuration file, so you'll only need to add sources files to your build system and add ozz <i>include/</i> path as an include directory. It should be straightforward and compatible with all modern c++ compilers.<br>
<br>
This solution is interesting for ozz runtime features as it ensures compilers compatibility and allows to trace into ozz code. It works for offline libraries as well.<br>
Integrating Collada or Fbx sources could be more problematic (tinyxml, fbx sdk) as they have dependencies.
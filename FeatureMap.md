This page lists implemented features and plans for future releases.

<img src='http://upload.wikimedia.org/wikipedia/commons/thumb/a/ad/Logo_of_Google_Drive.svg/260px-Logo_of_Google_Drive.svg.png' alt='Feedback form' height='24' width='24'> Use the anonymous <a href='https://docs.google.com/forms/d/1RscU59w7TkCvOsGUXKM7Lg0Ud0eGpl_CQnryDID91Ws/viewform'>user feedback form</a> to share your thoughts and needs.<br>
<br>
<h1>Features</h1>
<table><thead><th> <b>Feature name</b> </th><th> <b>Description</b> </th><th> <b>Availability</b> </th></thead><tbody>
<tr><td> Animation building  </td><td> Building an ozz run-time (optimized, compressed) animation from a raw (simple, editable, offline) animation format. </td><td> <del>0.1.x</del>    </td></tr>
<tr><td> Skeleton building   </td><td> Building an ozz run-time (optimized, compressed) skeleton from a raw (simple, editable, offline) skeleton format. </td><td> <del>0.1.x</del>    </td></tr>
<tr><td> Sampling            </td><td> Sampling an animation at a given time, in local space. </td><td> <del>0.1.x</del>    </td></tr>
<tr><td> Local to model space conversion </td><td> Converting from local space (bone transform relatively to its parent) to model space (bone transform relatively to the root). </td><td> <del>0.1.x</del>    </td></tr>
<tr><td> Serialization       </td><td> Serialization of skeleton and animation data structures, endiannes support. </td><td> <del>0.1.x</del>    </td></tr>
<tr><td> Key-frame optimizations </td><td> Optimizes redundant key-frames. </td><td> <del>0.1.x</del>    </td></tr>
<tr><td> Blending            </td><td> N-way blending of local space skeleton postures. </td><td> <del>0.2.x</del>    </td></tr>
<tr><td> Partial animation   </td><td> Support building of animation masks, sampling and blending masked animations. </td><td> <del>0.3.x</del>    </td></tr>
<tr><td> Fbx support         </td><td> Adds full pipeline support for Fbx files (skeletons and animations). </td><td> <del>0.4.x</del>    </td></tr>
<tr><td> Key-frame compression </td><td> Compress animation memory footprint. </td><td> <del>0.6.x</del>    </td></tr>
<tr><td> Skinning            </td><td> Supports for matrix palette skinning. </td><td> <del>0.7.0</del>    </td></tr>
<tr><td> Additive animation  </td><td> Supports building, sampling and blending of additive animations. </td><td> 0.8.0               </td></tr>
<tr><td> Hierarchical key-frame optimizations </td><td> Improves key-frame optimization quality by considering the full joints hierarchy while evaluating optimization error. </td><td> 0.8.0               </td></tr>
<tr><td> Motion extraction   </td><td> Supports building of motion tracks, extracted from full animations. </td><td> To be planned       </td></tr></tbody></table>

<h1>Samples</h1>
<table><thead><th> <b>Feature name</b> </th><th> <b>Description</b> </th><th> <b>Availability</b> </th></thead><tbody>
<tr><td> Millipede           </td><td> Demonstrates how to build an animation "by hand" using ozz_animation_offline library, convert it to ozz run-time format and samples it. </td><td> <del>0.1.x</del>    </td></tr>
<tr><td> Collada Animation   </td><td> Demonstrates how to use ozz_animation_collada library to load a collada animation and skeleton. </td><td> <del>0.1.x</del>    </td></tr>
<tr><td> Load Animation      </td><td> Demonstrates how to load animations and skeletons built using dae2anim and dae2skel tools. </td><td> <del>0.1.x</del>    </td></tr>
<tr><td> Blending            </td><td> Demonstrates how to blend animations. </td><td> <del>0.2.x</del>    </td></tr>
<tr><td> Partial blending    </td><td> Demonstrates partial animation blending. </td><td> <del>0.3.x</del>    </td></tr>
<tr><td> Multithread         </td><td> Demonstrates how to distribute job's workload using openmp. </td><td> <del>0.3.x</del>    </td></tr>
<tr><td> Attachment          </td><td> Demonstrates how to attach transformed objects to animated joints. </td><td> <del>0.3.x</del>    </td></tr>
<tr><td> Skinning            </td><td> Demonstrates how to perform skinning transformation to a mesh. </td><td> <del>0.7.0</del>    </td></tr>
<tr><td> Additive            </td><td> Demonstrates how to use additive blending. </td><td> 0.8.x               </td></tr>
<tr><td> Motion extraction   </td><td> Demonstrates motion extraction sampling. </td><td> To be planned       </td></tr>
<tr><td> LookAt              </td><td> Demonstrates how to implement procedural look at. </td><td> To be planned       </td></tr></tbody></table>

<h1>Os support</h1>
The run-time code (ozz_base, ozz_animation) only refers to the standard CRT and have no OS specific code. Os support should be considered for samples, tools and tests only.<br>
<table><thead><th> <b>Feature name</b> </th><th> <b>Description</b> </th><th> <b>Availability</b> </th></thead><tbody>
<tr><td> Linux               </td><td> All samples and full pipeline support. </td><td> <del>0.1.x</del>    </td></tr>
<tr><td> Windows             </td><td> All samples and full pipeline support. </td><td> <del>0.1.x</del>    </td></tr>
<tr><td> OS X                </td><td> All samples and full pipeline support. </td><td> <del>0.3.1</del>    </td></tr>
<tr><td> emscripten          </td><td> Runtime and samples support. </td><td> <del>0.5.x</del>    </td></tr>
<tr><td> Google Nacl         </td><td> Runtime and samples support. </td><td> Not planned         </td></tr>
<tr><td> iOS                 </td><td> Runtime and samples support. </td><td> Not planned         </td></tr></tbody></table>

<h1>Compiler support</h1>
<table><thead><th> <b>Feature name</b> </th><th> <b>Description</b> </th><th> <b>Availability</b> </th></thead><tbody>
<tr><td> GCC support         </td><td> Support latest gcc. </td><td> <del>0.1.x</del>    </td></tr>
<tr><td> MSVC support        </td><td> Support Visual Studio 2012 - 2013. </td><td> <del>0.5.x</del>    </td></tr>
<tr><td> Clang support       </td><td> Support latest clang. </td><td> <del>0.1.x</del>    </td></tr>
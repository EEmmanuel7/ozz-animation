//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
//                                                                            //
// This software is provided 'as-is', without any express or implied          //
// warranty. In no event will the authors be held liable for any damages      //
// arising from the use of this software.                                     //
//                                                                            //
// Permission is granted to anyone to use this software for any purpose,      //
// including commercial applications, and to alter it and redistribute it     //
// freely, subject to the following restrictions:                             //
//                                                                            //
// 1. The origin of this software must not be misrepresented; you must not    //
// claim that you wrote the original software. If you use this software       //
// in a product, an acknowledgment in the product documentation would be      //
// appreciated but is not required.                                           //
//                                                                            //
// 2. Altered source versions must be plainly marked as such, and must not be //
// misrepresented as being the original software.                             //
//                                                                            //
// 3. This notice may not be removed or altered from any source               //
// distribution.                                                              //
//                                                                            //
//============================================================================//

#include "ozz/animation/runtime/skeleton.h"
#include "ozz/animation/runtime/animation.h"
#include "ozz/animation/runtime/sampling_job.h"
#include "ozz/animation/runtime/local_to_model_job.h"

#include "ozz/animation/offline/animation_builder.h"
#include "ozz/animation/offline/animation_optimizer.h"
#include "ozz/animation/offline/raw_animation.h"
#include "ozz/animation/offline/raw_animation_utils.h"

#include "ozz/base/memory/allocator.h"

#include "ozz/base/io/archive.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/soa_transform.h"

#include "ozz/options/options.h"

#include "framework/profile.h"
#include "framework/application.h"
#include "framework/renderer.h"
#include "framework/imgui.h"
#include "framework/utils.h"

// Collada skeleton and animation file can be specified as an option.
OZZ_OPTIONS_DECLARE_STRING(
  skeleton,
  "Path to the runtime skeleton file.",
  "media/skeleton.ozz",
  false)

OZZ_OPTIONS_DECLARE_STRING(
  animation,
  "Path to the raw animation file.",
  "media/raw_animation.ozz",
  false)

namespace {
bool LoadAnimation(const char* _filename,
                   ozz::animation::offline::RawAnimation* _animation) {
  assert(_filename && _animation);
  ozz::log::Out() << "Loading raw animation archive: " << _filename <<
    "." << std::endl;
  ozz::io::File file(_filename, "rb");
  if (!file.opened()) {
    ozz::log::Err() << "Failed to open animation file " << _filename <<
      "." << std::endl;
    return false;
  }
  ozz::io::IArchive archive(&file);
  if (!archive.TestTag<ozz::animation::offline::RawAnimation>()) {
    ozz::log::Err() << "Failed to load rawvanimation instance from file " <<
      _filename << "." << std::endl;
    return false;
  }

  // Once the tag is validated, reading cannot fail.
  archive >> *_animation;

  return true;
}
}  // namespace

class OptimizeSampleApplication : public ozz::sample::Application {
 public:
  OptimizeSampleApplication()
    : selected_display_(eRuntimeAnimation),
      optimize_(true),
      cache_(NULL),
      animation_rt_(NULL),
      error_record_(64) {
  }

 protected:
  // Updates current animation time.
  virtual bool OnUpdate(float _dt) {
    // Updates current animation time.
    controller_.Update(*animation_rt_, _dt);

    // Prepares sampling job.
    ozz::animation::SamplingJob sampling_job;
    sampling_job.cache = cache_;
    sampling_job.time = controller_.time();

    // Samples optimized animation (_according to the display mode).
    sampling_job.animation = animation_rt_;
    sampling_job.output = locals_rt_;
    if (!sampling_job.Run()) {
      return false;
    }

    // Also samples non-optimized animation, from the raw animation.
    if (!SampleRawAnimation(raw_animation_, controller_.time(), locals_raw_)) {
      return false;
    }

    // Computes difference between the optimized and non-optimized animations
    // in local space, and rebinds it to the bind pose.
    {
      const ozz::math::SoaTransform* locals_raw = locals_raw_.begin;
      const ozz::math::SoaTransform* locals_rt = locals_rt_.begin;
      ozz::math::SoaTransform* locals_diff = locals_diff_.begin;
      ozz::Range<const ozz::math::SoaTransform> bind_poses =
        skeleton_.bind_pose();
      const ozz::math::SoaTransform* bind_pose = bind_poses.begin;
      for (;
           bind_pose < bind_poses.end;
           ++locals_raw, ++locals_rt, ++locals_diff, ++bind_pose) {
        assert(locals_raw < locals_raw_.end &&
               locals_rt < locals_rt_.end &&
               locals_diff < locals_diff_.end &&
               bind_pose < bind_poses.end);

        // Computes difference.
        const ozz::math::SoaTransform diff = {
          locals_rt->translation - locals_raw->translation,
          locals_rt->rotation * Conjugate(locals_raw->rotation),
          locals_rt->scale / locals_raw->scale
        };

        // Rebinds to the bind pose in the diff buffer.
        locals_diff->translation = bind_pose->translation + diff.translation;
        locals_diff->rotation = bind_pose->rotation * diff.rotation;
        locals_diff->scale = bind_pose->scale * diff.scale;
      }
    }

    // Converts from local space to model space matrices.
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = &skeleton_;

    // Optimized samples.
    ltm_job.input = locals_rt_;
    ltm_job.output = models_rt_;
    if (!ltm_job.Run()) {
      return false;
    }

    // Non-optimized samples (from the raw animation).
    ltm_job.input = locals_raw_;
    ltm_job.output = models_raw_;
    if (!ltm_job.Run()) {
      return false;
    }

    // Difference between optimized and non-optimized samples.
    ltm_job.input = locals_diff_;
    ltm_job.output = models_diff_;
    if (!ltm_job.Run()) {
      return false;
    }

    // Computes the absolute error, aka the difference between the raw and
    // runtime model space transformation.
    float error = 0.f;
    const ozz::math::Float4x4* models_rt = models_rt_.begin;
    const ozz::math::Float4x4* models_raw = models_raw_.begin;
    for (;
         models_rt < models_rt_.end;
         ++models_rt, ++models_raw) {

      // Computes the difference 
      const ozz::math::SimdFloat4 diff =
        models_rt->cols[3] - models_raw->cols[3];

      // Stores maximum error.
      error = ozz::math::Max(error, ozz::math::GetX(ozz::math::Length3(diff)));
    }
    error_record_.Push(error * 1000.f);  // Error is in millimeters.

    return true;
  }

  bool SampleRawAnimation(
    const ozz::animation::offline::RawAnimation& _animation,
    float _time,
    ozz::Range<ozz::math::SoaTransform> _locals) {

    // Ensure output is big enough.
    if (_locals.Count() * 4 < _animation.num_tracks()) {
      return false;
    }

    for (int i = 0; i < _animation.num_tracks(); i += 4) {
      ozz::math::SimdFloat4 translations[4];
      ozz::math::SimdFloat4 rotations[4];
      ozz::math::SimdFloat4 scales[4];
      // Samples 4 consecutive tracks.
      const int jmax = ozz::math::Min(_animation.num_tracks() - i, 4);
      for (int j = 0; j < jmax; ++j) {
        // Sample track.
        ozz::math::Transform transform =
          SampleTrack(_animation.tracks[i + j], _time);
        // Convert transform to AoS SimdFloat4 values.
        translations[j] =
          ozz::math::simd_float4::Load3PtrU(&transform.translation.x);
        rotations[j] =
          ozz::math::simd_float4::LoadPtrU(&transform.rotation.x);
        scales[j] =
          ozz::math::simd_float4::Load3PtrU(&transform.scale.x);
      }
      // Fills remaining transforms.
      for (int j = jmax; j < 4; ++j) {
        translations[j] = ozz::math::simd_float4::zero();
        rotations[j] = ozz::math::simd_float4::w_axis();
        scales[j] = ozz::math::simd_float4::one();
      }
      // Stores AoS keyframes to the SoA output.
      ozz::math::SoaTransform& output = _locals[i/4];
      ozz::math::Transpose4x3(translations, &output.translation.x);
      ozz::math::Transpose4x4(rotations, &output.rotation.x);
      ozz::math::Transpose4x3(scales, &output.scale.x);
    }

    return true;
  }


  // Selects model space matrices according to the display mode.
  ozz::Range<const ozz::math::Float4x4> models() const {
    switch(selected_display_) {
      case eRuntimeAnimation: return models_rt_;
      case eRawAnimation: return models_raw_;
      case eAbsoluteError: return models_diff_;
      default: {
        assert(false && "Invalid display mode");
        return models_rt_;
      }
    }
  }

  // Samples animation, transforms to model space and renders.
  virtual bool OnDisplay(ozz::sample::Renderer* _renderer) {

    // Renders posture.
    return _renderer->
      DrawPosture(skeleton_, models(), ozz::math::Float4x4::identity());
  }

  virtual bool OnInitialize() {
    // Imports offline skeleton from a binary file.
    if (!ozz::sample::LoadSkeleton(OPTIONS_skeleton, &skeleton_)) {
      return false;
    }

    // Imports offline animation from a binary file.
    if (!LoadAnimation(OPTIONS_animation, &raw_animation_)) {
      return false;
    }

    // Builds the runtime animation from the raw one imported from Collada.
    if (!BuildAnimations()) {
      return false;
    }

    // Allocates runtime buffers.
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    const int num_joints = skeleton_.num_joints();
    const int num_soa_joints = skeleton_.num_soa_joints();

    locals_rt_ =
      allocator->AllocateRange<ozz::math::SoaTransform>(num_soa_joints);
    models_rt_ = allocator->AllocateRange<ozz::math::Float4x4>(num_joints);
    locals_raw_ =
      allocator->AllocateRange<ozz::math::SoaTransform>(num_soa_joints);
    models_raw_ = allocator->AllocateRange<ozz::math::Float4x4>(num_joints);
    locals_diff_ =
      allocator->AllocateRange<ozz::math::SoaTransform>(num_soa_joints);
    models_diff_ = allocator->AllocateRange<ozz::math::Float4x4>(num_joints);

    // Allocates a cache that matches animation requirements.
    cache_ = allocator->New<ozz::animation::SamplingCache>(num_joints);

    return true;
  }

  virtual bool OnGui(ozz::sample::ImGui* _im_gui) {
    // Exposes animation runtime playback controls.
    {
      static bool open = true;
      ozz::sample::ImGui::OpenClose occ(_im_gui, "Animation control", &open);
      if (open) {
        controller_.OnGui(*animation_rt_, _im_gui);
      }
    }

    // Exposes optimizer's tolerances.
    {
      static bool open_tol = true;
      ozz::sample::ImGui::OpenClose ocb(_im_gui, "Optimization tolerances", &open_tol);
      if (open_tol) {
        bool rebuild = false;
        char label[64];

        rebuild |= _im_gui->DoCheckBox("Enable optimzations", &optimize_);

        std::sprintf(label,
                     "Translation : %0.2f cm",
                     optimizer_.translation_tolerance * 100);
        rebuild |= _im_gui->DoSlider(
          label, 0.f, .1f, &optimizer_.translation_tolerance, .5f, optimize_);

        std::sprintf(label,
                     "Rotation : %0.2f degree",
                     optimizer_.rotation_tolerance * 180.f / ozz::math::kPi);

        rebuild |= _im_gui->DoSlider(
          label, 0.f, 10.f * ozz::math::kPi / 180.f,
          &optimizer_.rotation_tolerance, .5f, optimize_);

        std::sprintf(label,
                     "Scale : %0.2f %%", optimizer_.scale_tolerance * 100.f);
        rebuild |= _im_gui->DoSlider(
          label, 0.f, .1f, &optimizer_.scale_tolerance, .5f, optimize_);

        std::sprintf(label, "Animation size : %dKB",
          static_cast<int>(animation_rt_->size()>>10));

        _im_gui->DoLabel(label);

        if (rebuild) {
          // Delete current animation and rebuild one with the new tolerances.
          ozz::memory::default_allocator()->Delete(animation_rt_);
          animation_rt_ = NULL;

          // Invalidates the cache in case the new animation has the same
          // address as the previous one. Other cases are automatic handled by
          // the cache. See SamplingCache::Invalidate for more details.
          cache_->Invalidate();

          // Rebuilds a new runtime animation.
          if (!BuildAnimations()) {
            return false;
          }
        }
      }

      // Selects display mode.
      static bool open_mode = true;
      ozz::sample::ImGui::OpenClose mode(
        _im_gui, "Display mode", &open_mode);
      if (open_mode) {
        _im_gui->DoRadioButton(eRuntimeAnimation, "Rutime animation", &selected_display_);
        _im_gui->DoRadioButton(eRawAnimation, "Raw animation", &selected_display_);
        _im_gui->DoRadioButton(eAbsoluteError, "Absolute error", &selected_display_);
      }

      // Show absolute error.
      { // FPS
        char szLabel[64];
        ozz::sample::Record::Statistics stats = error_record_.GetStatistics();
        static bool error_open = true;
        ozz::sample::ImGui::OpenClose oc_stats(_im_gui, szLabel, &error_open);
        if (error_open) {
          std::sprintf(szLabel, "Absolute error: %.2f mm", stats.mean);
          _im_gui->DoGraph(
            szLabel, 0.f, stats.max, stats.latest,
            error_record_.cursor(),
            error_record_.record_begin(), error_record_.record_end());
        }
      }
    }
    return true;
  }

  virtual void OnDestroy() {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    allocator->Delete(animation_rt_);
    allocator->Deallocate(locals_rt_);
    allocator->Deallocate(models_rt_);
    allocator->Deallocate(locals_raw_);
    allocator->Deallocate(models_raw_);
    allocator->Deallocate(locals_diff_);
    allocator->Deallocate(models_diff_);
    allocator->Delete(cache_);
  }

  bool BuildAnimations() {
    assert(!animation_rt_);

    // Instantiate an aniation builder.
    ozz::animation::offline::AnimationBuilder animation_builder;

    // Builds the optimized animation.
    if (optimize_) {
      // Optimzes the raw animation.
      ozz::animation::offline::RawAnimation optimized_animation;
      if (!optimizer_(raw_animation_, &optimized_animation)) {
        return false;
      }
      // Builds runtime aniamtion from the optimized one.
      animation_rt_ = animation_builder(optimized_animation);
    } else {
      // Builds runtime aniamtion from the brut one.
      animation_rt_ = animation_builder(raw_animation_);
    }

    // Check if building runtime animation was successful.
    if (!animation_rt_) {
      return false;
    }

    return true;
  }

  virtual void GetSceneBounds(ozz::math::Box* _bound) const {
    ozz::sample::ComputePostureBounds(models(), _bound);
  }

 private:

  // Selects which animation is displayed.
  enum DisplayMode {
    eRuntimeAnimation,
    eRawAnimation,
    eAbsoluteError,
  };
  int selected_display_;

  // Select whether optimization should be perfomed.
  bool optimize_;

  // Imported non-optimized animation.
  ozz::animation::offline::RawAnimation raw_animation_;

  // Stores the optimizer in order to expose its parameters.
  ozz::animation::offline::AnimationOptimizer optimizer_;

  // Playback animation controller. This is a utility class that helps with
  // controlling animation playback time.
  ozz::sample::PlaybackController controller_;

  // Runtime skeleton.
  ozz::animation::Skeleton skeleton_;

  // Sampling cache, shared accros optimized and non-optimized animations. This
  // is not optimal, but it's not an issue either.
  ozz::animation::SamplingCache* cache_;

  // Runtime optimized animation.
  ozz::animation::Animation* animation_rt_;

  // Buffer of local and model space transformations as sampled from the
  // rutime (optimized and compressed) animation.
  ozz::Range<ozz::math::SoaTransform> locals_rt_;
  ozz::Range<ozz::math::Float4x4> models_rt_;

  // Buffer of local and model space transformations as sampled from the
  // non-optimized (raw) animation.
  ozz::Range<ozz::math::SoaTransform> locals_raw_;
  ozz::Range<ozz::math::Float4x4> models_raw_;

  // Buffer of local and model space transformations storing samples from the
  // difference between optimized and non-optimized animations.
  ozz::Range<ozz::math::SoaTransform> locals_diff_;
  ozz::Range<ozz::math::Float4x4> models_diff_;

  // Record of accuracy errors produced by animation compression and
  // optimization.
  ozz::sample::Record error_record_;
};

int main(int _argc, const char** _argv) {
  const char* title = "Ozz-animation sample: Animation keyframe optimization";
  return OptimizeSampleApplication().Run(_argc, _argv, "1.0", title);
}

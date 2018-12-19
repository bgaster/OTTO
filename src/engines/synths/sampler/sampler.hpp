#pragma once

#include <atomic>

#include <OP1Aif.hpp>

#include "core/engine/engine.hpp"

#include "core/audio/faust.hpp"
#include "core/audio/voice_manager.hpp"

namespace otto::engines {

  using namespace core;
  using namespace core::engine;
  using namespace props;

  struct Sampler : SynthEngine, EngineWithEnvelope {
    struct Props : Properties<> {
      Property<float> gain = {
          this, "GAIN", 0.5, has_limits::init(0, 1), steppable::init(0.01)};

      Property<std::string> aif = {
        this, "AIF", "LO_CH_DRM01.aif"
      };

      Property<bool> previous_load = {this, "PreviousLoad", false };

      Property<std::array<uint32_t, 24>> start_times =
      {
          this, "StartTimes", std::array<uint32_t, 24>{0}
      };

      Property<std::array<uint32_t, 24>> end_times = {
          this, "EndTimes", std::array<uint32_t, 24>{0}
      };
    } props;

    Sampler();

    audio::ProcessData<1> process(audio::ProcessData<1>) override;

    ui::Screen& envelope_screen() override {
      return voice_mgr_.envelope_screen();
    }

    ui::Screen& voices_screen() override {
      return voice_mgr_.settings_screen();
    }

private:
    audio::VoiceManager<1> voice_mgr_;
    //std::unique_ptr<audio::FaustWrapper<0, 1>> sampler_dsp_;
    audio::FaustWrapper<0, 1> sampler_dsp_;

    // op-1 AIF drum loader
    OP1Aif<float> op1aif_;
  };

} // namespace otto::engines

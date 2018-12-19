#include "sampler.hpp"

#include "services/application.hpp"
#include "core/ui/vector_graphics.hpp"

#include "util/iterator.hpp"
#include "util/utility.hpp"

#include "sampler_dsp.hpp"

namespace otto::engines {

  using namespace ui;
  using namespace ui::vg;

  struct SamplerScreen : EngineScreen<Sampler> {
    void draw(Canvas& ctx) override;
    void rotary(RotaryEvent e) override;

    using EngineScreen<Sampler>::EngineScreen;
  };

  Sampler::Sampler()
    : Engine("Sampler", props, std::make_unique<SamplerScreen>(this)),
      voice_mgr_(props),
      sampler_dsp_(std::make_unique<FAUSTCLASS>(), props) {
          // load current sample AFI
          // FIXME: should move all this to the sample_dsp constructor
          op1aif_.load(Application::current().data_dir / "samples" / props.aif.get());

          // assume this is save as process would not be value to call yet...
          std::unique_ptr<sampler_dsp> fDSP{static_cast<sampler_dsp*>(
            sampler_dsp_.fDSP.release())};

          fDSP->setKit(&op1aif_);

          sampler_dsp_.fDSP = std::move(fDSP);

          // if not previously loaded, then set up end and start times
          if (!props.previous_load.get()) {
              std::array<uint32_t, 24> tmp1;
              std::array<uint32_t, 24> tmp2;
              for (int i = 0; i < 24; i++) {
                  tmp1[i] = op1aif_.start(i);
                  tmp2[i] = op1aif_.end(i);
              }
              props.start_times.set(tmp1);
              props.end_times.set(tmp2);

              props.previous_load.set(true);
          }
  }

  audio::ProcessData<1> Sampler::process(audio::ProcessData<1> data)
  {
      voice_mgr_.process_before(data.midi_only());
      auto res = sampler_dsp_.process(data.midi_only());
      voice_mgr_.process_after(data.midi_only());
      return res;
  }

  // SCREEN //

  void SamplerScreen::rotary(ui::RotaryEvent ev)
  {
    auto& props = engine.props;
    props.gain.step(ev.clicks);
  }

  void SamplerScreen::draw(ui::vg::Canvas& ctx)
  {
    using namespace ui::vg;

    auto& props = engine.props;

    ctx.font(Fonts::Bold, 40);

    ctx.beginPath();
    ctx.fillStyle(Colours::Blue);
    ctx.textAlign(HorizontalAlign::Center, VerticalAlign::Baseline);
    ctx.fillText("Sampler IN", {160, 60});

    ctx.font(Fonts::Bold, 100);

    ctx.beginPath();
    ctx.fillStyle(Colours::Blue);
    ctx.textAlign(HorizontalAlign::Center, VerticalAlign::Top);
    ctx.fillText(fmt::format("{}", (int) std::round(props.gain* 100.0)), {160, 70});

    ctx.beginPath();
    ctx.moveTo({40, 200});
    //ctx.lineTo({40 + 240 * engine.graph.average, 200});
    ctx.lineWidth(6);
    ctx.stroke(Colours::White);

    //engine.graph.clear();

  }

} // namespace otto::engines

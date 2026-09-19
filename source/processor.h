#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include <array>
#include <vector>
#include <cstdint>

namespace UglyReverb {

class Processor final : public Steinberg::Vst::AudioEffect
{
public:
    Processor();
    static Steinberg::FUnknown* createInstance(void*) { return (Steinberg::Vst::IAudioProcessor*)new Processor(); }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API terminate() override;
    Steinberg::tresult PLUGIN_API setActive(Steinberg::TBool state) override;
    Steinberg::tresult PLUGIN_API setupProcessing(Steinberg::Vst::ProcessSetup& setup) override;
    Steinberg::tresult PLUGIN_API canProcessSampleSize(Steinberg::int32 symbolicSampleSize) override;
    Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
                                                     Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) override;
    Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
    Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
    Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;

#ifdef UGLY_REVERB_TESTING
    void setTestParameter(Steinberg::Vst::ParamID id, float value) { applyParameter(id, value); }
#endif

private:
    static constexpr int kLines = 8;

    struct DelayLine
    {
        std::vector<float> data;
        int write = 0;
        float lp = 0.f;

        void resize(int samples);
        void clear();
        float read(float delaySamples) const;
        void push(float x);
    };

    void resetDsp();
    void updateDelayLengths();
    void applyParameter(Steinberg::Vst::ParamID id, float value);
    float processDigital(float x) const;
    float noise();

    double sampleRate_ = 44100.0;
    std::array<DelayLine, kLines> lines_;
    std::array<int, kLines> delays_ {};
    std::array<float, kLines> phase_ {};
    std::array<float, kLines> rattlePhase_ {};

    std::vector<float> preL_, preR_;
    int preWrite_ = 0;

    uint32_t rng_ = 0x125A5EEDu;

    float material_ = 0.f;
    float size_ = 0.55f;
    float decay_ = 0.58f;
    float preDelay_ = 0.08f;
    float diffusion_ = 0.45f;
    float damping_ = 0.48f;
    float metal_ = 0.68f;
    float clang_ = 0.55f;
    float rattle_ = 0.12f;
    float body_ = 0.55f;
    float width_ = 0.75f;
    float mix_ = 0.28f;
    float output_ = 0.5f;
    float digital_ = 0.f;
    bool bypass_ = false;
};

} // namespace UglyReverb

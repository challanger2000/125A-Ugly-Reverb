#include "processor.h"
#include "controller.h"
#include "ids.h"
#include "parameters.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include <algorithm>
#include <cmath>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace UglyReverb {

namespace {
constexpr float kPi = 3.14159265358979323846f;

inline float clamp1(float x)
{
    return std::max(-1.f, std::min(1.f, x));
}

inline float softClip(float x)
{
    return x / (1.f + std::fabs(x));
}
}

void Processor::DelayLine::resize(int samples)
{
    data.assign(std::max(samples, 8), 0.f);
    write = 0;
    lp = 0.f;
}

void Processor::DelayLine::clear()
{
    std::fill(data.begin(), data.end(), 0.f);
    write = 0;
    lp = 0.f;
}

float Processor::DelayLine::read(int delaySamples) const
{
    if (data.empty()) return 0.f;
    int d = std::max(1, std::min(delaySamples, (int)data.size() - 1));
    int idx = write - d;
    if (idx < 0) idx += (int)data.size();
    return data[(size_t)idx];
}

void Processor::DelayLine::push(float x)
{
    if (data.empty()) return;
    data[(size_t)write] = x;
    if (++write >= (int)data.size()) write = 0;
}

Processor::Processor()
{
    setControllerClass(ControllerUID);
}

tresult PLUGIN_API Processor::initialize(FUnknown* context)
{
    auto r = AudioEffect::initialize(context);
    if (r != kResultOk) return r;
    addAudioInput(STR16("Stereo In"), SpeakerArr::kStereo);
    addAudioOutput(STR16("Stereo Out"), SpeakerArr::kStereo);
    return kResultOk;
}

tresult PLUGIN_API Processor::terminate()
{
    return AudioEffect::terminate();
}

tresult PLUGIN_API Processor::setupProcessing(ProcessSetup& setup)
{
    sampleRate_ = setup.sampleRate > 1.0 ? setup.sampleRate : 44100.0;
    resetDsp();
    return AudioEffect::setupProcessing(setup);
}

tresult PLUGIN_API Processor::setActive(TBool state)
{
    if (state) resetDsp();
    return AudioEffect::setActive(state);
}

tresult PLUGIN_API Processor::canProcessSampleSize(int32 symbolicSampleSize)
{
    return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
}

tresult PLUGIN_API Processor::setBusArrangements(SpeakerArrangement* inputs, int32 numIns,
                                                  SpeakerArrangement* outputs, int32 numOuts)
{
    if (numIns == 1 && numOuts == 1 &&
        inputs[0] == SpeakerArr::kStereo && outputs[0] == SpeakerArr::kStereo)
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    return kResultFalse;
}

void Processor::resetDsp()
{
    const int maxDelay = (int)(sampleRate_ * 0.35) + 16;
    for (auto& l : lines_) l.resize(maxDelay);
    const int maxPre = (int)(sampleRate_ * 0.25) + 16;
    preL_.assign(maxPre, 0.f);
    preR_.assign(maxPre, 0.f);
    preWrite_ = 0;
    phase_.fill(0.f);
    updateDelayLengths();
}

void Processor::updateDelayLengths()
{
    static constexpr float plate[kLines] = {29.7f, 37.1f, 41.1f, 43.7f, 53.1f, 59.7f, 67.9f, 73.1f};
    static constexpr float steel[kLines] = {17.0f, 23.0f, 31.0f, 41.0f, 47.0f, 61.0f, 71.0f, 89.0f};
    static constexpr float tank [kLines] = {43.0f, 47.0f, 59.0f, 67.0f, 79.0f, 97.0f, 109.0f, 127.0f};

    const int mode = std::max(0, std::min(2, (int)std::lround(material_ * 2.f)));
    const float* base = mode == 0 ? plate : (mode == 1 ? steel : tank);

    const float sizeScale = 0.45f + size_ * 1.85f;
    const float bodyScale = 0.72f + body_ * 0.62f;

    for (int i = 0; i < kLines; ++i)
    {
        float ms = base[i] * sizeScale;
        if ((i & 1) == 0) ms *= bodyScale;
        else ms /= std::max(0.55f, bodyScale);
        int d = (int)std::lround(ms * 0.001f * (float)sampleRate_);
        delays_[i] = std::max(3, std::min(d, (int)lines_[i].data.size() - 2));
    }
}

float Processor::noise()
{
    rng_ ^= rng_ << 13;
    rng_ ^= rng_ >> 17;
    rng_ ^= rng_ << 5;
    return ((rng_ & 0x00FFFFFFu) / 8388607.5f) - 1.f;
}

float Processor::processDigital(float x) const
{
    const int mode = std::max(0, std::min(2, (int)std::lround(digital_ * 2.f)));
    if (mode == 0) return x;
    const float levels = mode == 1 ? 2048.f : 128.f;
    return std::round(clamp1(x) * levels) / levels;
}

tresult PLUGIN_API Processor::process(ProcessData& data)
{
    if (data.inputParameterChanges)
    {
        const int32 count = data.inputParameterChanges->getParameterCount();
        for (int32 i = 0; i < count; ++i)
        {
            if (auto* q = data.inputParameterChanges->getParameterData(i))
            {
                if (q->getPointCount() <= 0) continue;
                int32 offset = 0; ParamValue v = 0.0;
                if (q->getPoint(q->getPointCount() - 1, offset, v) != kResultTrue) continue;
                const float f = (float)v;
                switch (q->getParameterId())
                {
                    case kMaterial: material_ = f; updateDelayLengths(); break;
                    case kSize: size_ = f; updateDelayLengths(); break;
                    case kDecay: decay_ = f; break;
                    case kPreDelay: preDelay_ = f; break;
                    case kDiffusion: diffusion_ = f; break;
                    case kDamping: damping_ = f; break;
                    case kMetal: metal_ = f; break;
                    case kClang: clang_ = f; break;
                    case kRattle: rattle_ = f; break;
                    case kBody: body_ = f; updateDelayLengths(); break;
                    case kWidth: width_ = f; break;
                    case kMix: mix_ = f; break;
                    case kOutput: output_ = f; break;
                    case kDigital: digital_ = f; break;
                    case kBypass: bypass_ = f > 0.5f; break;
                    default: break;
                }
            }
        }
    }

    if (data.numInputs < 1 || data.numOutputs < 1 || data.numSamples <= 0)
        return kResultOk;

    if (data.symbolicSampleSize != kSample32)
        return kResultFalse;

    auto** in = data.inputs[0].channelBuffers32;
    auto** out = data.outputs[0].channelBuffers32;
    if (!in || !out || data.inputs[0].numChannels < 2 || data.outputs[0].numChannels < 2)
        return kResultFalse;

    const float outGain = std::pow(10.f, ((output_ * 24.f) - 12.f) / 20.f);
    const float wet = mix_;
    const float dry = 1.f - wet;
    const float feedback = std::min(0.985f, 0.42f + decay_ * 0.53f + metal_ * 0.025f);
    const float dampCoef = 0.04f + (1.f - damping_) * 0.82f;
    const float diff = 0.15f + diffusion_ * 0.82f;
    const float drive = 1.f + metal_ * 2.5f + clang_ * 1.4f;
    const int preSamp = std::max(0, std::min((int)preL_.size() - 1,
                      (int)std::lround(preDelay_ * 0.18f * (float)sampleRate_)));

    for (int32 s = 0; s < data.numSamples; ++s)
    {
        const float xL = in[0][s];
        const float xR = in[1][s];

        if (bypass_)
        {
            out[0][s] = xL;
            out[1][s] = xR;
            continue;
        }

        preL_[(size_t)preWrite_] = xL;
        preR_[(size_t)preWrite_] = xR;
        int pr = preWrite_ - preSamp;
        while (pr < 0) pr += (int)preL_.size();
        const float pL = preL_[(size_t)pr];
        const float pR = preR_[(size_t)pr];
        if (++preWrite_ >= (int)preL_.size()) preWrite_ = 0;

        float y[kLines] {};
        float sum = 0.f;
        for (int i = 0; i < kLines; ++i)
        {
            const float jitter = rattle_ * 0.0035f * (float)sampleRate_ * noise();
            const int dj = std::max(2, std::min((int)lines_[i].data.size() - 2, delays_[i] + (int)jitter));
            y[i] = lines_[i].read(dj);
            lines_[i].lp += dampCoef * (y[i] - lines_[i].lp);
            y[i] = lines_[i].lp;
            sum += y[i];
        }

        const float mean = sum * 0.125f;
        float fb[kLines];
        for (int i = 0; i < kLines; ++i)
        {
            float scattered = (mean * 2.f - y[i]) * diff + y[(i + 3) & 7] * (1.f - diff);
            const float ring = std::sin(phase_[i]) * clang_ * 0.055f * y[i];
            phase_[i] += 2.f * kPi * (180.f + 37.f * i + 520.f * metal_) / (float)sampleRate_;
            if (phase_[i] > 2.f * kPi) phase_[i] -= 2.f * kPi;
            fb[i] = processDigital(softClip((scattered + ring) * drive)) * feedback;
        }

        const float mono = 0.5f * (pL + pR);
        const float side = 0.5f * (pL - pR);
        for (int i = 0; i < kLines; ++i)
        {
            const float inject = mono * 0.20f + ((i & 1) ? side : -side) * 0.12f;
            lines_[i].push(inject + fb[i]);
        }

        float wetL = (y[0] + y[2] - y[5] + y[7]) * 0.30f;
        float wetR = (y[1] + y[3] - y[4] + y[6]) * 0.30f;
        const float wmid = 0.5f * (wetL + wetR);
        const float wside = 0.5f * (wetL - wetR) * (0.15f + width_ * 1.85f);
        wetL = wmid + wside;
        wetR = wmid - wside;

        out[0][s] = (xL * dry + wetL * wet) * outGain;
        out[1][s] = (xR * dry + wetR * wet) * outGain;
    }

    return kResultOk;
}

tresult PLUGIN_API Processor::setState(IBStream* state)
{
    IBStreamer s(state, kLittleEndian);
    float values[] = {material_, size_, decay_, preDelay_, diffusion_, damping_, metal_, clang_,
                      rattle_, body_, width_, mix_, output_, digital_};
    for (float& v : values) if (!s.readFloat(v)) return kResultFalse;
    int32 bp = 0; if (!s.readInt32(bp)) return kResultFalse;
    material_=values[0]; size_=values[1]; decay_=values[2]; preDelay_=values[3];
    diffusion_=values[4]; damping_=values[5]; metal_=values[6]; clang_=values[7];
    rattle_=values[8]; body_=values[9]; width_=values[10]; mix_=values[11];
    output_=values[12]; digital_=values[13]; bypass_=bp!=0;
    updateDelayLengths();
    return kResultOk;
}

tresult PLUGIN_API Processor::getState(IBStream* state)
{
    IBStreamer s(state, kLittleEndian);
    const float values[] = {material_, size_, decay_, preDelay_, diffusion_, damping_, metal_, clang_,
                            rattle_, body_, width_, mix_, output_, digital_};
    for (float v : values) if (!s.writeFloat(v)) return kResultFalse;
    if (!s.writeInt32(bypass_ ? 1 : 0)) return kResultFalse;
    return kResultOk;
}

} // namespace UglyReverb

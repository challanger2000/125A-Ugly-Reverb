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

inline float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
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

float Processor::DelayLine::read(float delaySamples) const
{
    if (data.empty()) return 0.f;
    const float d = std::max(1.f, std::min(delaySamples, (float)data.size() - 2.f));
    const int d0 = (int)std::floor(d);
    const float frac = d - (float)d0;

    int i0 = write - d0;
    while (i0 < 0) i0 += (int)data.size();
    int i1 = i0 - 1;
    if (i1 < 0) i1 += (int)data.size();

    return data[(size_t)i0] * (1.f - frac) + data[(size_t)i1] * frac;
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
    const int maxComb = (int)(sampleRate_ * 0.18) + 32;
    const int maxAp = (int)(sampleRate_ * 0.045) + 32;
    for (auto& x : combL_) x.resize(maxComb);
    for (auto& x : combR_) x.resize(maxComb);
    for (auto& x : apL_) x.resize(maxAp);
    for (auto& x : apR_) x.resize(maxAp);

    const int maxPre = (int)(sampleRate_ * 0.25) + 16;
    preL_.assign(maxPre, 0.f);
    preR_.assign(maxPre, 0.f);
    preWrite_ = 0;

    for (int i = 0; i < kCombs; ++i)
        rattlePhase_[i] = (2.f * kPi * (float)i) / (float)kCombs;

    resetSmoothers();
}

void Processor::resetSmoothers()
{
    smDecay_ = decay_;
    smPreDelay_ = preDelay_;
    smDiffusion_ = diffusion_;
    smDamping_ = damping_;
    smMetal_ = metal_;
    smClang_ = clang_;
    smRattle_ = rattle_;
    smWidth_ = width_;
    smMix_ = mix_;
    smOutput_ = output_;
}

float Processor::processDigital(float x) const
{
    const int mode = std::max(0, std::min(2, (int)std::lround(digital_ * 2.f)));
    if (mode == 0) return x;
    const float scale = mode == 1 ? 2047.f : 127.f;
    return std::round(clamp1(x) * scale) / scale;
}

float Processor::processAllpass(DelayLine& line, float input, float delaySamples, float feedback)
{
    const float delayed = line.read(delaySamples);
    const float y = delayed - input;
    line.push(input + delayed * feedback);
    return y;
}

void Processor::applyParameter(ParamID id, float value)
{
    const float f = std::max(0.f, std::min(1.f, value));
    switch (id)
    {
        case kMaterial: material_ = f; break;
        case kSize: size_ = f; break;
        case kDecay: decay_ = f; break;
        case kPreDelay: preDelay_ = f; break;
        case kDiffusion: diffusion_ = f; break;
        case kDamping: damping_ = f; break;
        case kMetal: metal_ = f; break;
        case kClang: clang_ = f; break;
        case kRattle: rattle_ = f; break;
        case kBody: body_ = f; break;
        case kWidth: width_ = f; break;
        case kMix: mix_ = f; break;
        case kOutput: output_ = f; break;
        case kDigital: digital_ = f; break;
        case kBypass: bypass_ = f > 0.5f; break;
        default: break;
    }
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
                int32 offset = 0;
                ParamValue v = 0.0;
                if (q->getPoint(q->getPointCount() - 1, offset, v) != kResultTrue) continue;
                applyParameter(q->getParameterId(), (float)v);
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

    const float smoothCoef = 1.f - std::exp(-1.f / std::max(1.f, 0.012f * (float)sampleRate_));

    // Delays intentionally avoid the evenly-spaced, highly-decorrelated design of modern reverbs.
    // Three related sets create Plate / Steel / Tank flavours while preserving obvious modes.
    static constexpr float baseMs[3][kCombs] = {
        {23.1f, 27.8f, 31.7f, 36.4f, 41.9f, 47.3f, 53.6f, 61.2f},
        {17.8f, 22.6f, 28.3f, 34.7f, 42.1f, 51.8f, 63.4f, 78.6f},
        {31.6f, 39.3f, 48.7f, 59.8f, 73.1f, 88.4f, 104.7f, 126.3f}
    };
    static constexpr float uglyMs[3][kCombs] = {
        {7.3f,  9.8f, 12.7f, 16.9f, 22.4f, 29.1f, 37.8f, 49.6f},
        {5.9f,  8.4f, 11.6f, 15.7f, 21.3f, 28.9f, 39.4f, 54.1f},
        {9.1f, 12.8f, 17.6f, 24.3f, 33.7f, 46.2f, 63.9f, 86.7f}
    };
    static constexpr float apMs[3][kAllpasses] = {
        {4.7f, 7.1f, 10.9f, 15.8f},
        {3.8f, 6.2f,  9.6f, 14.1f},
        {5.9f, 8.7f, 13.2f, 18.6f}
    };

    // Each material deliberately has a different failure mode:
    // Plate = dense metallic sheet, Steel = hard modal clang, Tank = coarse hollow ring.
    static constexpr float clangShape[3][kCombs] = {
        {0.42f, 0.74f, 0.18f, 0.66f, 0.82f, 0.28f, 0.71f, 0.12f},
        {0.08f, 1.00f,-0.16f, 0.72f, 1.00f, 0.03f, 0.91f,-0.12f},
        {0.22f, 0.81f,-0.08f, 1.00f, 0.58f, 0.18f, 0.97f, 0.05f}
    };
    static constexpr float combWeight[3][kCombs] = {
        {0.92f,0.88f,0.96f,0.90f,0.94f,0.89f,0.93f,0.91f},
        {0.62f,1.16f,0.58f,0.96f,1.22f,0.66f,1.08f,0.61f},
        {0.78f,1.02f,0.70f,1.12f,0.76f,0.94f,1.18f,0.72f}
    };
    static constexpr float rt60Scale[3] = {0.92f, 1.00f, 1.18f};
    static constexpr float diffusionBias[3] = {0.10f, -0.05f, -0.10f};
    static constexpr float rawLeakScale[3] = {0.72f, 1.18f, 1.06f};
    static constexpr float materialGain[3] = {1.00f, 1.14f, 1.08f};

    const int mat = std::max(0, std::min(2, (int)std::lround(material_ * 2.f)));

    for (int32 s = 0; s < data.numSamples; ++s)
    {
        const float xL = in[0][s];
        const float xR = in[1][s];

        smDecay_ += smoothCoef * (decay_ - smDecay_);
        smPreDelay_ += smoothCoef * (preDelay_ - smPreDelay_);
        smDiffusion_ += smoothCoef * (diffusion_ - smDiffusion_);
        smDamping_ += smoothCoef * (damping_ - smDamping_);
        smMetal_ += smoothCoef * (metal_ - smMetal_);
        smClang_ += smoothCoef * (clang_ - smClang_);
        smRattle_ += smoothCoef * (rattle_ - smRattle_);
        smWidth_ += smoothCoef * (width_ - smWidth_);
        smMix_ += smoothCoef * (mix_ - smMix_);
        smOutput_ += smoothCoef * (output_ - smOutput_);

        const float outGain = std::pow(10.f, ((smOutput_ * 24.f) - 12.f) / 20.f);
        const float wet = smMix_;
        const float dry = 1.f - wet;

        const float preSamples = std::max(0.f, std::min((float)preL_.size() - 2.f,
                               smPreDelay_ * 0.18f * (float)sampleRate_));
        preL_[(size_t)preWrite_] = xL;
        preR_[(size_t)preWrite_] = xR;

        const int pre0 = (int)std::floor(preSamples);
        const float preFrac = preSamples - (float)pre0;
        int pr0 = preWrite_ - pre0;
        while (pr0 < 0) pr0 += (int)preL_.size();
        int pr1 = pr0 - 1;
        if (pr1 < 0) pr1 += (int)preL_.size();

        const float pL = preL_[(size_t)pr0] * (1.f - preFrac) + preL_[(size_t)pr1] * preFrac;
        const float pR = preR_[(size_t)pr0] * (1.f - preFrac) + preR_[(size_t)pr1] * preFrac;
        if (++preWrite_ >= (int)preL_.size()) preWrite_ = 0;

        // Old-style excitation: mostly mono, preserving the artificial "one box" feel.
        const float mono = 0.5f * (pL + pR);
        const float side = 0.5f * (pL - pR);
        const float exciteL = mono + side * (0.10f + 0.22f * smWidth_);
        const float exciteR = mono - side * (0.10f + 0.22f * smWidth_);

        float combSumL = 0.f;
        float combSumR = 0.f;

        const float sizeScale = 0.58f + size_ * 1.22f;
        const float bodySkew = 0.82f + body_ * 0.36f;
        const float rt60 = 0.45f * std::pow(28.f, smDecay_) * rt60Scale[mat];
        const float dampingCoef = 0.10f + (1.f - smDamping_) * 0.82f;

        for (int i = 0; i < kCombs; ++i)
        {
            rattlePhase_[i] += (2.f * kPi * (0.13f + 0.037f * i)) / (float)sampleRate_;
            if (rattlePhase_[i] >= 2.f * kPi) rattlePhase_[i] -= 2.f * kPi;

            float ms = lerp(baseMs[mat][i], uglyMs[mat][i], smMetal_);
            ms *= sizeScale;
            ms *= (i & 1) ? (1.f / bodySkew) : bodySkew;

            // Rattle deliberately affects only a few paths strongly, like loose hardware.
            const float rattleMask = (i == 1 || i == 4 || i == 6) ? 1.f : 0.25f;
            const float jitter = smRattle_ * rattleMask * 0.0035f * (float)sampleRate_
                               * (std::sin(rattlePhase_[i]) + 0.31f * std::sin(rattlePhase_[i] * 2.7f + i));
            const float delayL = ms * 0.001f * (float)sampleRate_ + jitter;
            const float delayR = delayL + (17.f + 3.f * (float)i);

            float yL = combL_[i].read(delayL);
            float yR = combR_[i].read(delayR);

            combL_[i].lp += dampingCoef * (yL - combL_[i].lp);
            combR_[i].lp += dampingCoef * (yR - combR_[i].lp);
            const float fL = combL_[i].lp;
            const float fR = combR_[i].lp;

            const float delaySeconds = std::max(0.001f, ms * 0.001f);
            float fb = std::pow(10.f, -3.f * delaySeconds / rt60);

            // CLANG intentionally makes selected modes dominate instead of equalising them away.
            const float clangDepth = mat == 0 ? 0.050f : (mat == 1 ? 0.074f : 0.066f);
            fb += smClang_ * clangShape[mat][i] * clangDepth;
            fb = std::max(0.20f, std::min(0.991f, fb));

            const float drive = 1.f + smMetal_ * 1.9f + smClang_ * 1.4f;
            // Do not normalise the deliberate ugliness back out.  sqrt(drive) keeps
            // the loop bounded while allowing high METAL/CLANG to become audibly fierce.
            const float driveNorm = std::sqrt(drive);
            const float writeL = std::tanh((exciteL * (0.20f + 0.055f * i) + fL * fb) * drive) / driveNorm;
            const float writeR = std::tanh((exciteR * (0.20f + 0.055f * i) + fR * fb) * drive) / driveNorm;

            combL_[i].push(processDigital(writeL));
            combR_[i].push(processDigital(writeR));

            const float weight = combWeight[mat][i];
            combSumL += fL * weight;
            combSumR += fR * weight;
        }

        combSumL *= 0.17f;
        combSumR *= 0.17f;

        // Short serial allpasses make the parallel echoes fuse into reverb,
        // but intentionally stop before the tail becomes modern/smooth.
        float apOutL = combSumL;
        float apOutR = combSumR;
        const float apFeedback = 0.34f + smDiffusion_ * 0.34f + smMetal_ * 0.10f + diffusionBias[mat];
        for (int i = 0; i < kAllpasses; ++i)
        {
            const float uglyScale = 1.f - smMetal_ * (0.10f + 0.035f * i);
            const float dL = apMs[mat][i] * uglyScale * 0.001f * (float)sampleRate_;
            const float dR = dL + 11.f + 4.f * (float)i;
            apOutL = processAllpass(apL_[i], apOutL, dL, std::min(0.82f, apFeedback));
            apOutR = processAllpass(apR_[i], apOutR, dR, std::min(0.82f, apFeedback));
        }

        // At high METAL/CLANG the raw comb bank is deliberately leaked back in.
        // This is the "too metallic for a good reverb" control range.
        const float rawLeak = std::min(0.82f,
            smMetal_ * (0.18f + 0.50f * smClang_) * rawLeakScale[mat]);
        float wetL = apOutL * (1.f - rawLeak) + combSumL * rawLeak;
        float wetR = apOutR * (1.f - rawLeak) + combSumR * rawLeak;

        const float characterGain = (1.12f + 1.25f * smMetal_ + 0.90f * smClang_) * materialGain[mat];
        wetL *= characterGain;
        wetR *= characterGain;

        const float wmid = 0.5f * (wetL + wetR);
        const float wside = 0.5f * (wetL - wetR) * (0.18f + smWidth_ * 1.82f);
        wetL = wmid + wside;
        wetR = wmid - wside;

        if (bypass_)
        {
            out[0][s] = xL;
            out[1][s] = xR;
        }
        else
        {
            out[0][s] = (xL * dry + wetL * wet) * outGain;
            out[1][s] = (xR * dry + wetR * wet) * outGain;
        }
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

    const ParamID ids[14] = {kMaterial,kSize,kDecay,kPreDelay,kDiffusion,kDamping,kMetal,kClang,
                             kRattle,kBody,kWidth,kMix,kOutput,kDigital};
    for (int i = 0; i < 14; ++i) applyParameter(ids[i], values[i]);
    bypass_ = bp != 0;
    resetSmoothers();
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

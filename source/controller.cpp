#include "controller.h"
#include "parameters.h"
#include "base/source/fstreamer.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace UglyReverb {

tresult PLUGIN_API Controller::initialize(FUnknown* context)
{
    auto r = EditControllerEx1::initialize(context);
    if (r != kResultOk) return r;

    parameters.addParameter(STR16("Material"), nullptr, 10, 0.0, ParameterInfo::kCanAutomate, kMaterial);
    parameters.addParameter(STR16("Size"), nullptr, 0, 0.55, ParameterInfo::kCanAutomate, kSize);
    parameters.addParameter(STR16("Decay"), nullptr, 0, 0.58, ParameterInfo::kCanAutomate, kDecay);
    parameters.addParameter(STR16("Pre-Delay"), nullptr, 0, 0.08, ParameterInfo::kCanAutomate, kPreDelay);
    parameters.addParameter(STR16("Diffusion"), nullptr, 0, 0.45, ParameterInfo::kCanAutomate, kDiffusion);
    parameters.addParameter(STR16("Damping"), nullptr, 0, 0.48, ParameterInfo::kCanAutomate, kDamping);
    parameters.addParameter(STR16("Metal"), nullptr, 0, 0.68, ParameterInfo::kCanAutomate, kMetal);
    parameters.addParameter(STR16("Clang"), nullptr, 0, 0.55, ParameterInfo::kCanAutomate, kClang);
    parameters.addParameter(STR16("Rattle"), nullptr, 0, 0.12, ParameterInfo::kCanAutomate, kRattle);
    parameters.addParameter(STR16("Body"), nullptr, 0, 0.55, ParameterInfo::kCanAutomate, kBody);
    parameters.addParameter(STR16("Width"), nullptr, 0, 0.75, ParameterInfo::kCanAutomate, kWidth);
    parameters.addParameter(STR16("Mix"), nullptr, 0, 0.28, ParameterInfo::kCanAutomate, kMix);
    parameters.addParameter(STR16("Output"), STR16("dB"), 0, 0.5, ParameterInfo::kCanAutomate, kOutput);
    parameters.addParameter(STR16("Digital Color"), nullptr, 2, 0.0, ParameterInfo::kCanAutomate, kDigital);
    parameters.addParameter(STR16("Bypass"), nullptr, 1, 0.0,
                            ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass, kBypass);
    return kResultOk;
}

tresult PLUGIN_API Controller::setComponentState(IBStream* state)
{
    if (!state) return kResultFalse;
    IBStreamer s(state, kLittleEndian);
    float v[14] {};
    for (float& x : v) if (!s.readFloat(x)) return kResultFalse;
    int32 bp = 0; if (!s.readInt32(bp)) return kResultFalse;

    const ParamID ids[14] = {kMaterial,kSize,kDecay,kPreDelay,kDiffusion,kDamping,kMetal,kClang,
                             kRattle,kBody,kWidth,kMix,kOutput,kDigital};
    for (int i=0;i<14;++i) setParamNormalized(ids[i], v[i]);
    setParamNormalized(kBypass, bp ? 1.0 : 0.0);
    return kResultOk;
}

} // namespace UglyReverb

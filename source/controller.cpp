#include "controller.h"
#include "parameters.h"
#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace UglyReverb {

tresult PLUGIN_API Controller::initialize(FUnknown* context)
{
    auto r = EditControllerEx1::initialize(context);
    if (r != kResultOk) return r;

    auto* material = new StringListParameter(STR16("Material"), kMaterial);
    material->appendString(STR16("Plate"));
    material->appendString(STR16("Thin Plate"));
    material->appendString(STR16("Heavy Plate"));
    material->appendString(STR16("Sheet"));
    material->appendString(STR16("Spring"));
    material->appendString(STR16("Steel"));
    material->appendString(STR16("Pipe"));
    material->appendString(STR16("Metal Drum"));
    material->appendString(STR16("Oil Can"));
    material->appendString(STR16("Chamber"));
    material->appendString(STR16("Tank"));
    parameters.addParameter(material);

    parameters.addParameter(new RangeParameter(STR16("Size"), kSize, STR16("%"), 0.0, 100.0, 55.0));
    parameters.addParameter(new RangeParameter(STR16("Decay"), kDecay, STR16("%"), 0.0, 100.0, 58.0));
    parameters.addParameter(new RangeParameter(STR16("Pre-Delay"), kPreDelay, STR16("ms"), 0.0, 180.0, 14.4));
    parameters.addParameter(new RangeParameter(STR16("Diffusion"), kDiffusion, STR16("%"), 0.0, 100.0, 45.0));
    parameters.addParameter(new RangeParameter(STR16("Damping"), kDamping, STR16("%"), 0.0, 100.0, 48.0));
    parameters.addParameter(new RangeParameter(STR16("Metal"), kMetal, STR16("%"), 0.0, 100.0, 68.0));
    parameters.addParameter(new RangeParameter(STR16("Clang"), kClang, STR16("%"), 0.0, 100.0, 55.0));
    parameters.addParameter(new RangeParameter(STR16("Rattle"), kRattle, STR16("%"), 0.0, 100.0, 12.0));
    parameters.addParameter(new RangeParameter(STR16("Body"), kBody, STR16("%"), 0.0, 100.0, 55.0));
    parameters.addParameter(new RangeParameter(STR16("Width"), kWidth, STR16("%"), 0.0, 100.0, 75.0));
    parameters.addParameter(new RangeParameter(STR16("Mix"), kMix, STR16("%"), 0.0, 100.0, 28.0));
    parameters.addParameter(new RangeParameter(STR16("Output"), kOutput, STR16("dB"), -12.0, 12.0, 0.0));

    auto* digital = new StringListParameter(STR16("Digital Color"), kDigital);
    digital->appendString(STR16("Clean"));
    digital->appendString(STR16("12-bit"));
    digital->appendString(STR16("8-bit"));
    parameters.addParameter(digital);

    auto* bypass = new StringListParameter(STR16("Bypass"), kBypass, nullptr,
        ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass | ParameterInfo::kIsList);
    bypass->appendString(STR16("Off"));
    bypass->appendString(STR16("On"));
    parameters.addParameter(bypass);
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

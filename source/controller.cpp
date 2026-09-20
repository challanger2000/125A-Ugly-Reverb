#include "controller.h"
#include "parameters.h"
#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "base/source/fstring.h"
#include "UglyControls.h"
#include <cstring>
#include <string>
#include <vector>

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

Steinberg::IPlugView* PLUGIN_API Controller::createView(const char* name)
{
    Steinberg::ConstString viewName(name);
    if (viewName == Steinberg::Vst::ViewType::kEditor)
        {
        auto* editor = new VSTGUI::VST3Editor(this, "view", "ugly_reverb.uidesc");
        editor->setAllowedZoomFactors({1.0, 1.25, 1.5, 1.75, 2.0});
        return editor;
    }
    return nullptr;
}

VSTGUI::CView* Controller::createCustomView(VSTGUI::UTF8StringPtr name,
    const VSTGUI::UIAttributes& a,const VSTGUI::IUIDescription* d,VSTGUI::VST3Editor* e)
{
    if(!name||!e||!d) return nullptr;
    VSTGUI::CPoint o{0,0},s{60,60}; a.getPointAttribute("origin",o); a.getPointAttribute("size",s);
    VSTGUI::CRect r(o.x,o.y,o.x+s.x,o.y+s.y);
    if(std::strcmp(name,"Faceplate")==0) return new UglyFaceplate(r);
    if(std::strcmp(name,"BrandLogo")==0) return new UglyLogo(r);
    auto* knobBody=d->getBitmap("KnobMaster");
    auto* toggleOff=d->getBitmap("ToggleOff");
    auto* toggleOn=d->getBitmap("ToggleOn");
    auto knob=[&](const char* n,ParamID id)->VSTGUI::CView*{
        return std::strcmp(name,n)==0?new UglyKnob(r,e,id,knobBody):nullptr;
    };
    if(auto*v=knob("Material",kMaterial))return v; if(auto*v=knob("Size",kSize))return v;
    if(auto*v=knob("Decay",kDecay))return v; if(auto*v=knob("PreDelay",kPreDelay))return v;
    if(auto*v=knob("Diffusion",kDiffusion))return v; if(auto*v=knob("Damping",kDamping))return v;
    if(auto*v=knob("Body",kBody))return v; if(auto*v=knob("Metal",kMetal))return v;
    if(auto*v=knob("Clang",kClang))return v; if(auto*v=knob("Rattle",kRattle))return v;
    if(auto*v=knob("Width",kWidth))return v;
    if(std::strcmp(name,"Digital")==0)
        return new UglySelector(r,e,kDigital,std::vector<std::string>{"CLEAN","12-BIT","8-BIT"});
    if(auto*v=knob("Mix",kMix))return v; if(auto*v=knob("Output",kOutput))return v;
    if(std::strcmp(name,"Bypass")==0) return new UglyToggle(r,e,kBypass,toggleOff,toggleOn);
    return nullptr;
}

} // namespace UglyReverb

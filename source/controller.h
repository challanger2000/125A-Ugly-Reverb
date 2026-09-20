#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"

namespace UglyReverb {

class Controller final : public Steinberg::Vst::EditControllerEx1
{
public:
    static Steinberg::FUnknown* createInstance(void*) { return (Steinberg::Vst::IEditController*)new Controller(); }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override;
    Steinberg::IPlugView* PLUGIN_API createView(const char* name) override;
};

} // namespace UglyReverb

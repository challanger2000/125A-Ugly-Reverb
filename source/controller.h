#pragma once
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "vstgui/uidescription/iviewcreator.h"

namespace UglyReverb {

class Controller final : public Steinberg::Vst::EditControllerEx1,
                         public VSTGUI::VST3EditorDelegate
{
public:
    static Steinberg::FUnknown* createInstance(void*) { return (Steinberg::Vst::IEditController*)new Controller(); }
    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
    Steinberg::tresult PLUGIN_API setComponentState(Steinberg::IBStream* state) override;
    Steinberg::IPlugView* PLUGIN_API createView(const char* name) override;
    VSTGUI::CView* createCustomView(VSTGUI::UTF8StringPtr name,
        const VSTGUI::UIAttributes& attributes,
        const VSTGUI::IUIDescription* description,
        VSTGUI::VST3Editor* editor) override;
};

} // namespace UglyReverb

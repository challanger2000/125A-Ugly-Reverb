#pragma once
#include "vstgui/lib/controls/cknob.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/lib/cview.h"

namespace UglyReverb {

class UglyFaceplate final : public VSTGUI::CView {
public:
    explicit UglyFaceplate(const VSTGUI::CRect& r);
    UglyFaceplate(const UglyFaceplate& o) : VSTGUI::CView(o) {}
    VSTGUI::CBaseObject* newCopy() const override { return new UglyFaceplate(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
};

class UglyKnob final : public VSTGUI::CKnob {
public:
    UglyKnob(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag);
    UglyKnob(const UglyKnob& o);
    VSTGUI::CBaseObject* newCopy() const override { return new UglyKnob(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
};

class UglyToggle final : public VSTGUI::COnOffButton {
public:
    UglyToggle(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag);
    UglyToggle(const UglyToggle& o);
    VSTGUI::CBaseObject* newCopy() const override { return new UglyToggle(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
};

} // namespace UglyReverb

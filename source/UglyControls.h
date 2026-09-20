#pragma once
#include "vstgui/lib/controls/cknob.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/lib/cview.h"
#include "vstgui/lib/cbitmap.h"

namespace UglyReverb {

class UglyFaceplate final : public VSTGUI::CView {
public:
    explicit UglyFaceplate(const VSTGUI::CRect& r);
    UglyFaceplate(const UglyFaceplate& o) : VSTGUI::CView(o) {}
    VSTGUI::CBaseObject* newCopy() const override { return new UglyFaceplate(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
};

class UglyLogo final : public VSTGUI::CView {
public:
    explicit UglyLogo(const VSTGUI::CRect& r);
    UglyLogo(const UglyLogo& o) : VSTGUI::CView(o) {}
    VSTGUI::CBaseObject* newCopy() const override { return new UglyLogo(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
};

class UglyKnob final : public VSTGUI::CKnob {
public:
    UglyKnob(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag,VSTGUI::CBitmap* body);
    UglyKnob(const UglyKnob& o);
    VSTGUI::CBaseObject* newCopy() const override { return new UglyKnob(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
private:
    VSTGUI::CBitmap* body_ {nullptr};
};

class UglyToggle final : public VSTGUI::COnOffButton {
public:
    UglyToggle(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag,
               VSTGUI::CBitmap* offBitmap,VSTGUI::CBitmap* onBitmap);
    UglyToggle(const UglyToggle& o);
    VSTGUI::CBaseObject* newCopy() const override { return new UglyToggle(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
private:
    VSTGUI::CBitmap* off_ {nullptr};
    VSTGUI::CBitmap* on_ {nullptr};
};

} // namespace UglyReverb

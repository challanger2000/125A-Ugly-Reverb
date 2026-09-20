#pragma once
#include "vstgui/lib/controls/cknob.h"
#include "vstgui/lib/controls/cbuttons.h"
#include "vstgui/lib/controls/ccontrol.h"
#include "vstgui/lib/cview.h"
#include "vstgui/lib/cbitmap.h"
#include <string>
#include <vector>

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

class UglySelector final : public VSTGUI::CControl {
public:
    UglySelector(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag,
                 std::vector<std::string> labels);
    UglySelector(const UglySelector& o);
    VSTGUI::CBaseObject* newCopy() const override { return new UglySelector(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
    VSTGUI::CMouseEventResult onMouseDown(VSTGUI::CPoint& where,
                                           const VSTGUI::CButtonState& buttons) override;
private:
    std::vector<std::string> labels_;
};

class UglyToggle final : public VSTGUI::COnOffButton {
public:
    UglyToggle(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag);
    UglyToggle(const UglyToggle& o);
    VSTGUI::CBaseObject* newCopy() const override { return new UglyToggle(*this); }
    void draw(VSTGUI::CDrawContext* c) override;
};

} // namespace UglyReverb

#include "UglyControls.h"
#include "vstgui/lib/cdrawcontext.h"
#include <algorithm>
#include <cmath>

namespace UglyReverb {
namespace {
constexpr double pi=3.14159265358979323846;
void screw(VSTGUI::CDrawContext* c,double x,double y){
    c->setFillColor({76,74,68,255}); c->drawEllipse({x-4,y-4,x+4,y+4},VSTGUI::kDrawFilled);
    c->setFrameColor({18,17,15,255}); c->setLineWidth(1.2); c->drawLine({x-2.4,y},{x+2.4,y});
}
}
UglyFaceplate::UglyFaceplate(const VSTGUI::CRect& r):CView(r){setMouseEnabled(false);}
void UglyFaceplate::draw(VSTGUI::CDrawContext* c){
    const auto r=getViewSize(); c->setDrawMode(VSTGUI::kAntiAliasing);
    c->setFillColor({31,30,27,255}); c->drawRect(r,VSTGUI::kDrawFilled);
    for(int y=3;y<430;y+=4){ c->setFrameColor({112,108,96,(uint8_t)((y%16)?10:18)}); c->drawLine({r.left+8.,r.top+y},{r.right-8.,r.top+y}); }
    c->setFrameColor({8,8,7,255}); c->setLineWidth(3); c->drawRect({r.left+5,r.top+5,r.right-5,r.bottom-5},VSTGUI::kDrawStroked);
    c->setFrameColor({92,88,78,100}); c->setLineWidth(1); c->drawRect({r.left+9,r.top+9,r.right-9,r.bottom-9},VSTGUI::kDrawStroked);
    screw(c,r.left+16,r.top+16); screw(c,r.right-16,r.top+16); screw(c,r.left+16,r.bottom-16); screw(c,r.right-16,r.bottom-16);
    setDirty(false);
}
UglyKnob::UglyKnob(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag):CKnob(r,l,tag,nullptr,nullptr){
    setStartAngle((float)(135*pi/180)); setRangeAngle((float)(270*pi/180)); setTransparency(true); setWantsFocus(true);
}
UglyKnob::UglyKnob(const UglyKnob& o):CKnob(o){}
void UglyKnob::draw(VSTGUI::CDrawContext* c){
    const auto r=getViewSize(); const auto p=r.getCenter(); const double rad=std::min(r.getWidth(),r.getHeight())*.34;
    const double v=std::clamp((double)getValueNormalized(),0.,1.); const double a=(135.+270.*v)*pi/180.;
    c->setDrawMode(VSTGUI::kAntiAliasing);
    c->setFillColor({0,0,0,105}); c->drawEllipse({p.x-rad-5,p.y-rad,p.x+rad+6,p.y+rad+9},VSTGUI::kDrawFilled);
    c->setFillColor({183,174,145,255}); c->setFrameColor({63,59,49,255}); c->setLineWidth(2);
    c->drawEllipse({p.x-rad,p.y-rad,p.x+rad,p.y+rad},VSTGUI::kDrawFilledAndStroked);
    c->setFillColor({211,200,164,110}); c->drawEllipse({p.x-rad*.72,p.y-rad*.76,p.x+rad*.20,p.y+rad*.05},VSTGUI::kDrawFilled);
    c->setFrameColor({35,31,25,255}); c->setLineWidth(3);
    c->drawLine({p.x+std::cos(a)*rad*.25,p.y+std::sin(a)*rad*.25},{p.x+std::cos(a)*rad*.83,p.y+std::sin(a)*rad*.83});
    setDirty(false);
}
UglyToggle::UglyToggle(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag):COnOffButton(r,l,tag,nullptr){setTransparency(true);setWantsFocus(true);}
UglyToggle::UglyToggle(const UglyToggle& o):COnOffButton(o){}
void UglyToggle::draw(VSTGUI::CDrawContext* c){
    const auto r=getViewSize(); const bool on=getValueNormalized()>=.5; const auto p=r.getCenter();
    c->setDrawMode(VSTGUI::kAntiAliasing); c->setFillColor({8,8,7,180}); c->drawEllipse({p.x-17,p.y-17,p.x+17,p.y+17},VSTGUI::kDrawFilled);
    c->setFillColor({112,106,89,255}); c->setFrameColor({32,30,25,255}); c->setLineWidth(2); c->drawEllipse({p.x-12,p.y-12,p.x+12,p.y+12},VSTGUI::kDrawFilledAndStroked);
    c->setFrameColor({205,193,158,255}); c->setLineWidth(6);
    c->drawLine({p.x,p.y+3},{p.x+(on?8:-8),p.y-13});
    setDirty(false);
}
} // namespace UglyReverb

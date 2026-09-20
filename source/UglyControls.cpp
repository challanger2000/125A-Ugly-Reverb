#include "UglyControls.h"
#include "branding_master.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string_view>
#include <utility>
#include <vector>

namespace UglyReverb {
namespace {
constexpr double kPi=3.14159265358979323846;
constexpr VSTGUI::CColor kLogoSilver {217,217,217,255};
constexpr VSTGUI::CColor kLogoRed {215,25,32,255};

void screw(VSTGUI::CDrawContext* c,double x,double y)
{
    c->setFillColor({83,79,69,255});
    c->drawEllipse({x-4,y-4,x+4,y+4},VSTGUI::kDrawFilled);
    c->setFrameColor({18,17,15,255});
    c->setLineWidth(1.1);
    c->drawEllipse({x-4,y-4,x+4,y+4},VSTGUI::kDrawStroked);
    c->drawLine({x-2.4,y},{x+2.4,y});
}

struct LogoSubpath { std::vector<VSTGUI::CPoint> points; };
struct LogoPath { std::vector<LogoSubpath> subpaths; bool red {false}; };

std::vector<LogoPath> parseMasterLogo()
{
    std::vector<LogoPath> result;
    result.reserve(Branding::kMasterPathCount);
    for(const auto& source:Branding::kMasterPaths) {
        const std::string_view d {source.d};
        LogoPath path;
        path.red=source.red;
        const char* p=d.data();
        const char* end=d.data()+d.size();
        char command=0;
        LogoSubpath* current=nullptr;
        while(p<end) {
            while(p<end&&(std::isspace(static_cast<unsigned char>(*p))||*p==',')) ++p;
            if(p>=end) break;
            if(std::isalpha(static_cast<unsigned char>(*p))) {
                command=*p++;
                if(command=='Z'||command=='z') {
                    command=0;
                    current=nullptr;
                    continue;
                }
            }
            if(command!='M'&&command!='m'&&command!='L'&&command!='l') {
                ++p;
                continue;
            }
            char* next=nullptr;
            const double x=std::strtod(p,&next);
            if(next==p||next>end) break;
            p=next;
            while(p<end&&(std::isspace(static_cast<unsigned char>(*p))||*p==',')) ++p;
            const double y=std::strtod(p,&next);
            if(next==p||next>end) break;
            p=next;
            if(command=='M'||command=='m') {
                path.subpaths.emplace_back();
                current=&path.subpaths.back();
                current->points.emplace_back(x,y);
                command=(command=='M')?'L':'l';
            } else if(current) {
                current->points.emplace_back(x,y);
            }
        }
        if(!path.subpaths.empty()) result.emplace_back(std::move(path));
    }
    return result;
}

void drawScaledBitmap(VSTGUI::CDrawContext* c,VSTGUI::CBitmap* bitmap,const VSTGUI::CRect& dst)
{
    if(!bitmap || !bitmap->isLoaded()) return;
    const VSTGUI::CRect src(0,0,bitmap->getWidth(),bitmap->getHeight());
    c->fillRectWithBitmap(bitmap,src,dst,1.f);
}
}

UglyFaceplate::UglyFaceplate(const VSTGUI::CRect& r):CView(r)
{
    setMouseEnabled(false);
}

void UglyFaceplate::draw(VSTGUI::CDrawContext* c)
{
    const auto r=getViewSize();
    c->setDrawMode(VSTGUI::kAntiAliasing);

    c->setFillColor({24,23,21,255});
    c->drawRect(r,VSTGUI::kDrawFilled);

    // One neutral worn metal plate. No baked control boxes.
    for(int y=0;y<static_cast<int>(r.getHeight());++y) {
        const uint8_t shade=static_cast<uint8_t>(31 + ((y/7)%3));
        c->setFrameColor({shade,shade,static_cast<uint8_t>(shade-2),255});
        c->setLineWidth(1.0);
        c->drawLine({r.left,r.top+y},{r.right,r.top+y});
    }

    // Sparse scratches and abrasion: deliberately subtle, not rusty/cyberpunk.
    const struct {double x1,y1,x2,y2; uint8_t a;} marks[] = {
        {44,74,183,72,35},{234,52,322,54,22},{404,92,517,88,24},
        {596,68,704,70,30},{68,207,168,204,24},{273,218,357,220,20},
        {438,196,575,198,26},{90,349,226,346,22},{306,371,413,369,26},
        {487,338,604,341,22},{623,362,716,359,26},{176,116,214,114,18}
    };
    for(const auto& m:marks) {
        c->setFrameColor({164,157,139,m.a});
        c->setLineWidth(1.0);
        c->drawLine({r.left+m.x1,r.top+m.y1},{r.left+m.x2,r.top+m.y2});
    }

    c->setFrameColor({7,7,6,255});
    c->setLineWidth(3.0);
    c->drawRect({r.left+4,r.top+4,r.right-4,r.bottom-4},VSTGUI::kDrawStroked);
    c->setFrameColor({104,97,82,90});
    c->setLineWidth(1.0);
    c->drawRect({r.left+9,r.top+9,r.right-9,r.bottom-9},VSTGUI::kDrawStroked);

    screw(c,r.left+17,r.top+17);
    screw(c,r.right-17,r.top+17);
    screw(c,r.left+17,r.bottom-17);
    screw(c,r.right-17,r.bottom-17);
    setDirty(false);
}

UglyLogo::UglyLogo(const VSTGUI::CRect& r):CView(r)
{
    setMouseEnabled(false);
}

void UglyLogo::draw(VSTGUI::CDrawContext* c)
{
    static const auto logo=parseMasterLogo();
    const auto r=getViewSize();
    constexpr double masterWidth=1774.0;
    constexpr double masterHeight=887.0;
    const double scale=std::min(r.getWidth()/masterWidth,r.getHeight()/masterHeight);
    const double x0=r.left+(r.getWidth()-masterWidth*scale)*0.5;
    const double y0=r.top+(r.getHeight()-masterHeight*scale)*0.5;

    c->setDrawMode(VSTGUI::kAntiAliasing);
    for(const auto& sourcePath:logo) {
        auto* path=c->createGraphicsPath();
        if(!path) continue;
        for(const auto& subpath:sourcePath.subpaths) {
            if(subpath.points.empty()) continue;
            const auto toView=[&](const VSTGUI::CPoint& p) {
                return VSTGUI::CPoint{x0+p.x*scale,y0+p.y*scale};
            };
            path->beginSubpath(toView(subpath.points.front()));
            for(std::size_t i=1;i<subpath.points.size();++i)
                path->addLine(toView(subpath.points[i]));
            path->closeSubpath();
        }
        c->setFillColor(sourcePath.red?kLogoRed:kLogoSilver);
        c->drawGraphicsPath(path,VSTGUI::CDrawContext::kPathFilledEvenOdd);
        path->forget();
    }
    setDirty(false);
}

UglyKnob::UglyKnob(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag,VSTGUI::CBitmap* body)
: CKnob(r,l,tag,nullptr,nullptr),body_(body)
{
    setStartAngle(static_cast<float>(135.0*kPi/180.0));
    setRangeAngle(static_cast<float>(270.0*kPi/180.0));
    setTransparency(true);
    setWantsFocus(true);
}

UglyKnob::UglyKnob(const UglyKnob& o):CKnob(o),body_(o.body_) {}

void UglyKnob::draw(VSTGUI::CDrawContext* c)
{
    const auto r=getViewSize();
    const auto p=r.getCenter();
    const double v=std::clamp(static_cast<double>(getValueNormalized()),0.0,1.0);
    const double a=(135.0+270.0*v)*kPi/180.0;

    c->setDrawMode(VSTGUI::kAntiAliasing);

    // Small contact shadow only; the visible hardware itself is the approved PNG.
    const double shadowR=std::min(r.getWidth(),r.getHeight())*0.34;
    c->setFillColor({0,0,0,72});
    c->drawEllipse({p.x-shadowR-2,p.y-shadowR,p.x+shadowR+4,p.y+shadowR+6},VSTGUI::kDrawFilled);

    drawScaledBitmap(c,body_,r);

    // Dynamic VSTGUI marker: the PNG never rotates.
    const double markerR=std::min(r.getWidth(),r.getHeight())*0.31;
    c->setFrameColor({38,33,26,255});
    c->setLineWidth(2.4);
    c->drawLine({p.x+std::cos(a)*markerR*0.40,p.y+std::sin(a)*markerR*0.40},
                {p.x+std::cos(a)*markerR*0.86,p.y+std::sin(a)*markerR*0.86});
    setDirty(false);
}

UglyToggle::UglyToggle(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag,
                       VSTGUI::CBitmap* offBitmap,VSTGUI::CBitmap* onBitmap)
: COnOffButton(r,l,tag,nullptr),off_(offBitmap),on_(onBitmap)
{
    setTransparency(true);
    setWantsFocus(true);
}

UglyToggle::UglyToggle(const UglyToggle& o):COnOffButton(o),off_(o.off_),on_(o.on_) {}

void UglyToggle::draw(VSTGUI::CDrawContext* c)
{
    const auto r=getViewSize();
    c->setDrawMode(VSTGUI::kAntiAliasing);
    drawScaledBitmap(c,getValueNormalized()>=0.5?on_:off_,r);
    setDirty(false);
}

} // namespace UglyReverb

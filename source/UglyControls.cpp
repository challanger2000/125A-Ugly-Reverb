#include "UglyControls.h"
#include "branding_master.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgradient.h"
#include "vstgui/lib/cgraphicspath.h"
#include "vstgui/plugin-bindings/vst3editor.h"
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

void fillRound(VSTGUI::CDrawContext* c,const VSTGUI::CRect& r,double radius,
               const VSTGUI::CColor& fill,const VSTGUI::CColor& frame,double lineWidth=1.0)
{
    auto* path=c->createRoundRectGraphicsPath(r,radius);
    if(!path) return;
    c->setFillColor(fill);
    c->drawGraphicsPath(path,VSTGUI::CDrawContext::kPathFilled);
    c->setFrameColor(frame);
    c->setLineWidth(lineWidth);
    c->drawGraphicsPath(path,VSTGUI::CDrawContext::kPathStroked);
    path->forget();
}

void gradientRound(VSTGUI::CDrawContext* c,const VSTGUI::CRect& r,double radius,
                   const VSTGUI::CColor& top,const VSTGUI::CColor& bottom,
                   const VSTGUI::CColor& frame,double lineWidth=1.0)
{
    auto* path=c->createRoundRectGraphicsPath(r,radius);
    if(!path) return;
    auto* gradient=VSTGUI::CGradient::create(0.0,1.0,top,bottom);
    if(gradient) {
        c->fillLinearGradient(path,*gradient,r.getTopLeft(),r.getBottomLeft(),false);
        gradient->forget();
    } else {
        c->setFillColor(bottom);
        c->drawGraphicsPath(path,VSTGUI::CDrawContext::kPathFilled);
    }
    c->setFrameColor(frame);
    c->setLineWidth(lineWidth);
    c->drawGraphicsPath(path,VSTGUI::CDrawContext::kPathStroked);
    path->forget();
}

void modulePanel(VSTGUI::CDrawContext* c,const VSTGUI::CRect& r,
                 const VSTGUI::CColor& top,const VSTGUI::CColor& bottom)
{
    VSTGUI::CRect shadow=r;
    shadow.offset(3.0,4.0);
    fillRound(c,shadow,8.0,{0,0,0,118},{0,0,0,0},0.0);
    gradientRound(c,r,8.0,top,bottom,{42,82,124,235},1.0);
    c->setFrameColor({112,159,204,48});
    c->setLineWidth(1.0);
    c->drawLine({r.left+12.0,r.top+2.0},{r.right-12.0,r.top+2.0});
}

void panelWear(VSTGUI::CDrawContext* c,const VSTGUI::CRect& r,int variant)
{
    // Deliberately sparse, deterministic damage.  The goal is used industrial
    // hardware, not a full-screen grunge texture.
    const VSTGUI::CColor bright {184,202,214,72};
    const VSTGUI::CColor dark   {1,7,12,120};
    const VSTGUI::CColor oxide {126,74,47,64};

    c->setLineWidth(1.0);

    auto nick=[&](double x,double y,double len,bool vertical,const VSTGUI::CColor& color) {
        c->setFrameColor(color);
        if(vertical)
            c->drawLine({x,y},{x+0.8,y+len});
        else
            c->drawLine({x,y},{x+len,y+0.8});
    };

    const double shift=static_cast<double>(variant)*7.0;
    nick(r.left+25.0+shift,r.top+1.5,9.0,false,bright);
    nick(r.left+78.0+shift*0.6,r.top+2.2,5.0,false,dark);
    nick(r.right-58.0-shift*0.4,r.top+1.0,11.0,false,bright);
    nick(r.left+1.5,r.top+92.0+shift,8.0,true,dark);
    nick(r.right-2.0,r.top+168.0-shift*0.5,7.0,true,bright);
    nick(r.left+48.0+shift,r.bottom-2.0,13.0,false,dark);
    nick(r.right-84.0-shift*0.5,r.bottom-1.5,8.0,false,oxide);

    // A few irregular hairline scratches, all kept near the panel perimeter.
    c->setFrameColor({196,211,221,42});
    c->drawLine({r.left+18.0,r.top+31.0+shift},{r.left+31.0,r.top+27.0+shift});
    c->drawLine({r.right-42.0,r.bottom-27.0-shift*0.3},{r.right-27.0,r.bottom-31.0-shift*0.3});

    c->setFrameColor(oxide);
    c->drawLine({r.left+10.0,r.bottom-52.0+shift*0.25},{r.left+17.0,r.bottom-49.0+shift*0.25});
}

void wornControlHalo(VSTGUI::CDrawContext* c,const VSTGUI::CPoint& p,double radius,int variant)
{
    // Controls in the deliberately ugly section look handled more often than
    // the rest: faint rub marks, one dark scuff, and a tiny oxidised nick.
    const double d=radius*2.0;
    VSTGUI::CRect ring(p.x-radius,p.y-radius,p.x+radius,p.y+radius);
    c->setLineWidth(1.0);
    c->setFrameColor({205,214,220,26});
    c->drawArc(ring,198.f+variant*9.f,292.f+variant*7.f,VSTGUI::kDrawStroked);

    VSTGUI::CRect outer(p.x-radius-2.0,p.y-radius-2.0,p.x+radius+2.0,p.y+radius+2.0);
    c->setFrameColor({0,0,0,56});
    c->drawArc(outer,18.f+variant*11.f,112.f+variant*8.f,VSTGUI::kDrawStroked);

    c->setFrameColor({132,78,49,58});
    c->drawLine({p.x-radius*0.78,p.y+radius*0.58},
                {p.x-radius*0.55,p.y+radius*0.48});
}

void degradedPlate(VSTGUI::CDrawContext* c,const VSTGUI::CRect& r)
{
    // Small battered service plate.  It makes the "ugly" concept intentional
    // without turning the whole UI into a novelty graphic.
    VSTGUI::CRect shadow=r;
    shadow.offset(1.5,2.0);
    fillRound(c,shadow,3.0,{0,0,0,92},{0,0,0,0},0.0);
    gradientRound(c,r,3.0,{21,31,40,222},{8,14,20,232},{111,128,140,104},1.0);

    // Uneven worn edge/highlight.
    c->setLineWidth(1.0);
    c->setFrameColor({213,220,224,54});
    c->drawLine({r.left+12.0,r.top+1.0},{r.left+49.0,r.top+1.0});
    c->drawLine({r.right-64.0,r.bottom-1.0},{r.right-23.0,r.bottom-1.0});
    c->setFrameColor({128,74,45,70});
    c->drawLine({r.left+5.0,r.bottom-7.0},{r.left+17.0,r.bottom-4.0});

    // Two old fasteners, deliberately not perfectly identical.
    c->setFillColor({58,64,69,230});
    c->drawEllipse({r.left+6.0,r.top+6.0,r.left+11.0,r.top+11.0},VSTGUI::kDrawFilled);
    c->drawEllipse({r.right-11.0,r.bottom-11.0,r.right-6.0,r.bottom-6.0},VSTGUI::kDrawFilled);
    c->setFrameColor({9,11,13,220});
    c->drawLine({r.left+6.8,r.top+8.7},{r.left+10.1,r.top+7.4});
    c->drawLine({r.right-10.0,r.bottom-9.5},{r.right-6.8,r.bottom-7.2});

    c->setFont(VSTGUI::kNormalFont,8.0,VSTGUI::kBoldFace);
    c->setFontColor({170,181,188,175});
    VSTGUI::CRect title=r;
    title.inset(18.0,4.0);
    title.bottom=title.top+12.0;
    c->drawString(VSTGUI::UTF8String("DEGRADED SPACE"),title,VSTGUI::kCenterText);

    c->setFont(VSTGUI::kNormalFont,6.5,VSTGUI::kNormalFace);
    c->setFontColor({111,126,137,150});
    VSTGUI::CRect sub=r;
    sub.inset(18.0,4.0);
    sub.top+=13.0;
    c->drawString(VSTGUI::UTF8String("SERVICE UNIT // 125A"),sub,VSTGUI::kCenterText);

    // One intentional scrape through the lower print.
    c->setFrameColor({205,213,218,48});
    c->drawLine({r.left+54.0,r.bottom-7.0},{r.left+96.0,r.bottom-10.0});
}

} // namespace

UglyFaceplate::UglyFaceplate(const VSTGUI::CRect& r):VSTGUI::CView(r)
{
    setMouseEnabled(false);
}

void UglyFaceplate::draw(VSTGUI::CDrawContext* c)
{
    const auto r=getViewSize();
    c->setDrawMode(VSTGUI::kAntiAliasing|VSTGUI::kNonIntegralMode);

    // Almost-black chassis. It remains visible only as the outer frame,
    // header and gutters between the large blue modules.
    c->setFillColor({5,7,10,255});
    c->drawRect(r,VSTGUI::kDrawFilled);

    VSTGUI::CRect header(r.left,r.top,r.right,r.top+60.0);
    gradientRound(c,header,0.0,{18,22,28,255},{7,9,13,255},{20,25,31,255},1.0);
    c->setFrameColor({48,109,164,180});
    c->setLineWidth(1.0);
    c->drawLine({r.left+16.0,r.top+59.0},{r.right-16.0,r.top+59.0});

    modulePanel(c,{r.left+16.0,r.top+70.0,r.left+278.0,r.top+414.0},
                {22,67,112,255},{10,34,61,255});
    modulePanel(c,{r.left+288.0,r.top+70.0,r.left+618.0,r.top+414.0},
                {18,59,103,255},{8,30,56,255});
    modulePanel(c,{r.left+628.0,r.top+70.0,r.left+744.0,r.top+414.0},
                {24,74,121,255},{10,37,67,255});

    const VSTGUI::CRect leftPanel  {r.left+16.0,r.top+70.0,r.left+278.0,r.top+414.0};
    const VSTGUI::CRect uglyPanel  {r.left+288.0,r.top+70.0,r.left+618.0,r.top+414.0};
    const VSTGUI::CRect masterPanel{r.left+628.0,r.top+70.0,r.left+744.0,r.top+414.0};
    panelWear(c,leftPanel,0);
    panelWear(c,uglyPanel,1);
    panelWear(c,masterPanel,2);

    // The character section looks most "handled": these marks sit behind the
    // actual controls and remain subtle at normal viewing size.
    wornControlHalo(c,{r.left+340.0,r.top+295.0},32.0,0); // METAL
    wornControlHalo(c,{r.left+441.0,r.top+295.0},32.0,1); // CLANG
    wornControlHalo(c,{r.left+542.0,r.top+295.0},32.0,2); // RATTLE

    // Free space below the left controls becomes a small battered service plate.
    degradedPlate(c,{r.left+36.0,r.top+366.0,r.left+258.0,r.top+401.0});

    // A couple of isolated chassis scratches stop the outer black shell from
    // looking factory-new while keeping the branding/header readable.
    c->setLineWidth(1.0);
    c->setFrameColor({126,137,145,48});
    c->drawLine({r.left+123.0,r.top+18.0},{r.left+147.0,r.top+15.0});
    c->drawLine({r.right-86.0,r.top+49.0},{r.right-55.0,r.top+46.0});
    c->setFrameColor({116,68,43,46});
    c->drawLine({r.left+8.0,r.top+201.0},{r.left+12.0,r.top+218.0});

    // Small chassis feet/details keep the black frame from looking flat.
    c->setFillColor({23,27,33,255});
    c->drawEllipse({r.left+7,r.top+7,r.left+13,r.top+13},VSTGUI::kDrawFilled);
    c->drawEllipse({r.right-13,r.top+7,r.right-7,r.top+13},VSTGUI::kDrawFilled);
    c->drawEllipse({r.left+7,r.bottom-13,r.left+13,r.bottom-7},VSTGUI::kDrawFilled);
    c->drawEllipse({r.right-13,r.bottom-13,r.right-7,r.bottom-7},VSTGUI::kDrawFilled);
    setDirty(false);
}

UglyLogo::UglyLogo(const VSTGUI::CRect& r):VSTGUI::CView(r)
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

UglyKnob::UglyKnob(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag)
: VSTGUI::CKnob(r,l,tag,nullptr,nullptr)
{
    setStartAngle(static_cast<float>(135.0*kPi/180.0));
    setRangeAngle(static_cast<float>(270.0*kPi/180.0));
    setTransparency(true);
    setWantsFocus(true);
}

UglyKnob::UglyKnob(const UglyKnob& o):VSTGUI::CKnob(o) {}

void UglyKnob::draw(VSTGUI::CDrawContext* c)
{
    const auto r=getViewSize();
    const auto p=r.getCenter();
    const double size=std::min(r.getWidth(),r.getHeight());
    const double v=std::clamp(static_cast<double>(getValueNormalized()),0.0,1.0);
    const double a=(135.0+270.0*v)*kPi/180.0;

    c->setDrawMode(VSTGUI::kAntiAliasing|VSTGUI::kNonIntegralMode);

    const double arcR=size*0.455;
    const VSTGUI::CRect arcBox(p.x-arcR,p.y-arcR,p.x+arcR,p.y+arcR);
    c->setLineWidth(2.0);
    c->setFrameColor({15,25,36,255});
    c->drawArc(arcBox,135.f,405.f,VSTGUI::kDrawStroked);
    if(v>0.001) {
        c->setFrameColor({70,159,226,235});
        c->drawArc(arcBox,135.f,static_cast<float>(135.0+270.0*v),VSTGUI::kDrawStroked);
    }

    // Deep mounting shadow.
    const double shadowR=size*0.355;
    c->setFillColor({0,0,0,125});
    c->drawEllipse({p.x-shadowR+2.0,p.y-shadowR+4.0,
                    p.x+shadowR+4.0,p.y+shadowR+8.0},VSTGUI::kDrawFilled);

    // Black recessed bezel.
    const double bezelR=size*0.375;
    c->setFillColor({2,4,6,255});
    c->drawEllipse({p.x-bezelR,p.y-bezelR,p.x+bezelR,p.y+bezelR},VSTGUI::kDrawFilled);
    c->setFrameColor({65,74,84,255});
    c->setLineWidth(1.0);
    c->drawEllipse({p.x-bezelR,p.y-bezelR,p.x+bezelR,p.y+bezelR},VSTGUI::kDrawStroked);

    // Steel collar with radial knurling.
    const double collarR=size*0.325;
    VSTGUI::CRect collar(p.x-collarR,p.y-collarR,p.x+collarR,p.y+collarR);
    auto* collarPath=c->createRoundRectGraphicsPath(collar,collarR);
    if(collarPath) {
        auto* steel=VSTGUI::CGradient::create(0.0,1.0,
            VSTGUI::CColor{205,212,219,255},VSTGUI::CColor{74,82,91,255});
        if(steel) {
            c->fillLinearGradient(collarPath,*steel,collar.getTopLeft(),collar.getBottomLeft(),false);
            steel->forget();
        }
        c->setFrameColor({26,31,37,255});
        c->drawGraphicsPath(collarPath,VSTGUI::CDrawContext::kPathStroked);
        collarPath->forget();
    }

    c->setFrameColor({44,50,57,190});
    c->setLineWidth(1.0);
    for(int i=0;i<18;++i) {
        const double ka=(2.0*kPi*static_cast<double>(i))/18.0;
        const double r1=collarR*0.80;
        const double r2=collarR*0.97;
        c->drawLine({p.x+std::cos(ka)*r1,p.y+std::sin(ka)*r1},
                    {p.x+std::cos(ka)*r2,p.y+std::sin(ka)*r2});
    }

    // Black separator between collar and cap.
    const double separatorR=size*0.278;
    c->setFillColor({8,11,14,255});
    c->drawEllipse({p.x-separatorR,p.y-separatorR,p.x+separatorR,p.y+separatorR},
                   VSTGUI::kDrawFilled);

    // Raised brushed-metal cap.
    const double capR=size*0.246;
    VSTGUI::CRect cap(p.x-capR,p.y-capR,p.x+capR,p.y+capR);
    auto* capPath=c->createRoundRectGraphicsPath(cap,capR);
    if(capPath) {
        auto* silver=VSTGUI::CGradient::create(0.0,1.0,
            VSTGUI::CColor{239,242,245,255},VSTGUI::CColor{118,128,138,255});
        if(silver) {
            c->fillLinearGradient(capPath,*silver,cap.getTopLeft(),cap.getBottomLeft(),false);
            silver->forget();
        }
        c->setFrameColor({38,44,50,255});
        c->setLineWidth(1.0);
        c->drawGraphicsPath(capPath,VSTGUI::CDrawContext::kPathStroked);
        capPath->forget();
    }

    // Specular metal highlights.
    c->setFrameColor({255,255,255,105});
    c->setLineWidth(1.0);
    c->drawArc({cap.left+1.5,cap.top+1.5,cap.right-1.5,cap.bottom-1.5},
               202.f,323.f,VSTGUI::kDrawStroked);
    c->setFrameColor({42,47,53,115});
    c->drawArc({cap.left+2.5,cap.top+2.5,cap.right-2.5,cap.bottom-2.5},
               20.f,145.f,VSTGUI::kDrawStroked);

    // Engraved black position line.
    c->setFrameColor({5,8,11,255});
    c->setLineWidth(2.3);
    c->drawLine({p.x+std::cos(a)*capR*0.28,p.y+std::sin(a)*capR*0.28},
                {p.x+std::cos(a)*capR*0.84,p.y+std::sin(a)*capR*0.84});
    c->setFrameColor({255,255,255,80});
    c->setLineWidth(0.8);
    c->drawLine({p.x+std::cos(a)*capR*0.30-0.7,p.y+std::sin(a)*capR*0.30-0.7},
                {p.x+std::cos(a)*capR*0.82-0.7,p.y+std::sin(a)*capR*0.82-0.7});
    setDirty(false);
}

UglySelector::UglySelector(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag,
                           std::vector<std::string> labels)
: VSTGUI::CControl(r,l,tag,nullptr),labels_(std::move(labels))
{
    setTransparency(true);
    setWantsFocus(true);
}

UglySelector::UglySelector(const UglySelector& o)
: VSTGUI::CControl(o),labels_(o.labels_) {}

void UglySelector::draw(VSTGUI::CDrawContext* c)
{
    const auto r=getViewSize();
    if(labels_.empty()) { setDirty(false); return; }

    c->setDrawMode(VSTGUI::kAntiAliasing|VSTGUI::kNonIntegralMode);
    VSTGUI::CRect shadow=r;
    shadow.offset(1.5,2.0);
    fillRound(c,shadow,5.0,{0,0,0,100},{0,0,0,0},0.0);
    fillRound(c,r,5.0,{4,7,10,255},{61,78,94,255},1.0);

    VSTGUI::CRect inner=r;
    inner.inset(3.0,3.0);
    const int count=static_cast<int>(labels_.size());
    const int selected=std::clamp(
        static_cast<int>(std::lround(getValueNormalized()*static_cast<double>(std::max(1,count-1)))),
        0,count-1);
    const double segW=inner.getWidth()/static_cast<double>(count);

    c->setFont(VSTGUI::kNormalFont,7.5,VSTGUI::kBoldFace);
    for(int i=0;i<count;++i) {
        VSTGUI::CRect seg(inner.left+i*segW,inner.top,
                          i==count-1?inner.right:inner.left+(i+1)*segW,inner.bottom);
        VSTGUI::CRect face=seg;
        face.inset(1.0,1.0);
        if(i==selected) {
            gradientRound(c,face,3.0,{226,231,236,255},{116,127,139,255},
                          {239,244,248,120},1.0);
            c->setFontColor({10,16,22,255});
        } else {
            fillRound(c,face,3.0,{15,23,31,255},{37,52,66,255},1.0);
            c->setFontColor({150,170,188,255});
        }
        c->drawString(VSTGUI::UTF8String(labels_[i].c_str()),face,VSTGUI::kCenterText);
    }
    setDirty(false);
}

VSTGUI::CMouseEventResult UglySelector::onMouseDown(VSTGUI::CPoint& where,
                                                     const VSTGUI::CButtonState& buttons)
{
    if(!buttons.isLeftButton()||labels_.empty())
        return VSTGUI::kMouseEventNotHandled;
    const auto r=getViewSize();
    if(!r.pointInside(where))
        return VSTGUI::kMouseEventNotHandled;

    const int count=static_cast<int>(labels_.size());
    const double normalized=(where.x-r.left)/std::max(1.0,r.getWidth());
    const int index=std::clamp(static_cast<int>(normalized*count),0,count-1);
    const float value=count<=1?0.f:static_cast<float>(index)/static_cast<float>(count-1);

    beginEdit();
    setValueNormalized(value);
    valueChanged();
    endEdit();
    invalid();
    return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}

UglyToggle::UglyToggle(const VSTGUI::CRect& r,VSTGUI::IControlListener* l,int32_t tag)
: VSTGUI::COnOffButton(r,l,tag,nullptr)
{
    setTransparency(true);
    setWantsFocus(true);
}

UglyToggle::UglyToggle(const UglyToggle& o):VSTGUI::COnOffButton(o) {}

void UglyToggle::draw(VSTGUI::CDrawContext* c)
{
    const auto r=getViewSize();
    const bool on=getValueNormalized()>=0.5;
    c->setDrawMode(VSTGUI::kAntiAliasing|VSTGUI::kNonIntegralMode);

    VSTGUI::CRect shadow=r;
    shadow.offset(1.5,2.5);
    fillRound(c,shadow,r.getHeight()*0.5,{0,0,0,110},{0,0,0,0},0.0);
    fillRound(c,r,r.getHeight()*0.5,{3,6,9,255},{72,86,100,255},1.0);

    VSTGUI::CRect track=r;
    track.inset(3.0,3.0);
    fillRound(c,track,track.getHeight()*0.5,
              on?VSTGUI::CColor{18,62,99,255}:VSTGUI::CColor{11,17,23,255},
              on?VSTGUI::CColor{65,150,213,220}:VSTGUI::CColor{31,43,55,255},1.0);

    const double d=track.getHeight()-4.0;
    const double cy=track.getCenter().y;
    const double cx=on?(track.right-2.0-d*0.5):(track.left+2.0+d*0.5);
    VSTGUI::CRect thumb(cx-d*0.5,cy-d*0.5,cx+d*0.5,cy+d*0.5);
    auto* thumbPath=c->createRoundRectGraphicsPath(thumb,d*0.5);
    if(thumbPath) {
        auto* silver=VSTGUI::CGradient::create(0.0,1.0,
            VSTGUI::CColor{235,239,243,255},VSTGUI::CColor{112,122,133,255});
        if(silver) {
            c->fillLinearGradient(thumbPath,*silver,thumb.getTopLeft(),thumb.getBottomLeft(),false);
            silver->forget();
        }
        c->setFrameColor({28,33,39,255});
        c->setLineWidth(1.0);
        c->drawGraphicsPath(thumbPath,VSTGUI::CDrawContext::kPathStroked);
        thumbPath->forget();
    }

    c->setFont(VSTGUI::kNormalFont,7.0,VSTGUI::kBoldFace);
    c->setFontColor(on?VSTGUI::CColor{126,198,246,255}:VSTGUI::CColor{118,132,145,255});
    VSTGUI::CRect textRect=track;
    if(on) textRect.right=thumb.left-1.0;
    else textRect.left=thumb.right+1.0;
    c->drawString(VSTGUI::UTF8String(on?"ON":"OFF"),textRect,VSTGUI::kCenterText);
    setDirty(false);
}

UglyZoomControl::UglyZoomControl(const VSTGUI::CRect& r,VSTGUI::VST3Editor* editor,int* zoomIndex)
: VSTGUI::CControl(r,nullptr,-1,nullptr),editor_(editor),zoomIndex_(zoomIndex)
{
    setTransparency(true);
    setWantsFocus(true);
}

UglyZoomControl::UglyZoomControl(const UglyZoomControl& o)
: VSTGUI::CControl(o),editor_(o.editor_),zoomIndex_(o.zoomIndex_) {}

void UglyZoomControl::draw(VSTGUI::CDrawContext* c)
{
    const auto r=getViewSize();
    c->setDrawMode(VSTGUI::kAntiAliasing|VSTGUI::kNonIntegralMode);

    const double gap=4.0;
    const double w=(r.getWidth()-gap)*0.5;
    VSTGUI::CRect minus(r.left,r.top,r.left+w,r.bottom);
    VSTGUI::CRect plus(r.right-w,r.top,r.right,r.bottom);

    auto drawButton=[&](const VSTGUI::CRect& b,const char* text) {
        VSTGUI::CRect sh=b; sh.offset(1.2,1.8);
        fillRound(c,sh,4.0,{0,0,0,110},{0,0,0,0},0.0);
        gradientRound(c,b,4.0,{66,74,83,255},{24,29,35,255},{117,128,139,210},1.0);
        c->setFrameColor({255,255,255,45});
        c->drawLine({b.left+4,b.top+2},{b.right-4,b.top+2});
        c->setFont(VSTGUI::kNormalFont,11.0,VSTGUI::kBoldFace);
        c->setFontColor({225,232,238,255});
        c->drawString(VSTGUI::UTF8String(text),b,VSTGUI::kCenterText);
    };
    drawButton(minus,"-");
    drawButton(plus,"+");
    setDirty(false);
}

VSTGUI::CMouseEventResult UglyZoomControl::onMouseDown(
    VSTGUI::CPoint& where,const VSTGUI::CButtonState& buttons)
{
    if(!buttons.isLeftButton() || !editor_ || !getViewSize().pointInside(where))
        return VSTGUI::kMouseEventNotHandled;

    static constexpr double zooms[] {1.0,1.25,1.5,1.75,2.0};
    const double current=editor_->getZoomFactor();
    int index=0;
    double best=std::abs(current-zooms[0]);
    for(int i=1;i<5;++i) {
        const double d=std::abs(current-zooms[i]);
        if(d<best) { best=d; index=i; }
    }

    const bool plus=where.x>=getViewSize().getCenter().x;
    index=std::clamp(index+(plus?1:-1),0,4);
    if (zoomIndex_) *zoomIndex_=index;
    editor_->setZoomFactor(zooms[index]);
    invalid();
    return VSTGUI::kMouseDownEventHandledButDontNeedMovedOrUpEvents;
}

} // namespace UglyReverb

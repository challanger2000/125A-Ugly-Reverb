#include "processor.h"
#include "parameters.h"

#include "pluginterfaces/vst/ivstaudioprocessor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;
using UglyReverb::Processor;

namespace {

struct RenderResult
{
    std::vector<float> left;
    std::vector<float> right;
};

bool finiteBuffer(const std::vector<float>& x)
{
    for (float v : x)
        if (!std::isfinite(v)) return false;
    return true;
}

double energy(const std::vector<float>& x, size_t a, size_t b)
{
    b = std::min(b, x.size());
    a = std::min(a, b);
    double e = 0.0;
    for (size_t i = a; i < b; ++i) e += (double)x[i] * (double)x[i];
    return e;
}

double difference(const std::vector<float>& a, const std::vector<float>& b)
{
    const size_t n = std::min(a.size(), b.size());
    double d = 0.0;
    for (size_t i=0;i<n;++i) d += std::fabs((double)a[i]-(double)b[i]);
    return d / std::max<size_t>(1,n);
}

RenderResult render(double sr, double seconds, float material, float preDelay, float digital,
                    bool bypass=false, bool impulse=true)
{
    Processor p;
    if (p.initialize(nullptr) != kResultOk)
        throw std::runtime_error("Processor initialize failed");

    ProcessSetup setup {};
    setup.processMode = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = 128;
    setup.sampleRate = sr;
    if (p.setupProcessing(setup) != kResultOk)
        throw std::runtime_error("setupProcessing failed");
    if (p.setActive(true) != kResultOk)
        throw std::runtime_error("setActive failed");

    p.setTestParameter(UglyReverb::kMaterial, material);
    p.setTestParameter(UglyReverb::kPreDelay, preDelay);
    p.setTestParameter(UglyReverb::kDigital, digital);
    p.setTestParameter(UglyReverb::kMix, bypass ? 0.28f : 1.f);
    p.setTestParameter(UglyReverb::kOutput, 0.5f);
    p.setTestParameter(UglyReverb::kBypass, bypass ? 1.f : 0.f);

    const size_t total=(size_t)std::llround(sr*seconds);
    RenderResult rr;
    rr.left.assign(total,0.f);
    rr.right.assign(total,0.f);

    const int block=128;
    std::vector<float> inL(block,0.f), inR(block,0.f), outL(block,0.f), outR(block,0.f);
    float* inPtrs[2]={inL.data(),inR.data()};
    float* outPtrs[2]={outL.data(),outR.data()};

    AudioBusBuffers inBus {};
    inBus.numChannels=2;
    inBus.channelBuffers32=inPtrs;
    AudioBusBuffers outBus {};
    outBus.numChannels=2;
    outBus.channelBuffers32=outPtrs;

    bool sent=false;
    size_t pos=0;
    while(pos<total)
    {
        const int n=(int)std::min<size_t>(block,total-pos);
        std::fill(inL.begin(),inL.end(),0.f);
        std::fill(inR.begin(),inR.end(),0.f);
        std::fill(outL.begin(),outL.end(),0.f);
        std::fill(outR.begin(),outR.end(),0.f);
        if (impulse && !sent)
        {
            inL[0]=1.f;
            inR[0]=1.f;
            sent=true;
        }

        ProcessData data {};
        data.processMode=kRealtime;
        data.symbolicSampleSize=kSample32;
        data.numSamples=n;
        data.numInputs=1;
        data.numOutputs=1;
        data.inputs=&inBus;
        data.outputs=&outBus;

        if (p.process(data) != kResultOk)
            throw std::runtime_error("process failed");

        for(int i=0;i<n;++i)
        {
            rr.left[pos+i]=outL[i];
            rr.right[pos+i]=outR[i];
        }
        pos+=(size_t)n;
    }

    p.setActive(false);
    p.terminate();
    return rr;
}

void require(bool cond, const std::string& msg, int& failures)
{
    if(cond) std::cout << "[PASS] " << msg << "\n";
    else { std::cout << "[FAIL] " << msg << "\n"; ++failures; }
}

}

int main()
{
    int failures=0;
    try
    {
        for(double sr : {44100.0,48000.0,96000.0})
        {
            auto r=render(sr,4.0,0.5f,0.f,0.f);
            require(finiteBuffer(r.left)&&finiteBuffer(r.right),
                    "Finite output at "+std::to_string((int)sr)+" Hz", failures);

            float peak=0.f;
            for(float v:r.left) peak=std::max(peak,std::fabs(v));
            require(peak < 2.0f, "Bounded peak at "+std::to_string((int)sr)+" Hz", failures);

            const size_t s50=(size_t)(sr*0.05);
            const size_t s500=(size_t)(sr*0.5);
            const size_t s3000=(size_t)(sr*3.0);
            const double early=energy(r.left,s50,s500);
            const double late=energy(r.left,s3000,r.left.size());
            std::cout << "[INFO] " << (int)sr << " Hz early_energy=" << early
                      << " late_energy=" << late
                      << " ratio=" << (early > 0.0 ? late / early : 0.0) << "\n";
            require(early > 1e-8, "Audible reverb tail energy", failures);
            require(late < early, "Tail decays over time", failures);
        }

        auto silence=render(48000.0,1.0,0.5f,0.f,0.f,false,false);
        require(energy(silence.left,0,silence.left.size()) < 1e-20 &&
                energy(silence.right,0,silence.right.size()) < 1e-20,
                "Silence in -> silence out", failures);

        auto delayed=render(48000.0,1.0,0.5f,0.5f,0.f);
        const size_t first70=(size_t)(48000.0*0.070);
        require(energy(delayed.left,0,first70) < 1e-12,
                "Pre-delay prevents premature wet output", failures);

        auto plate=render(48000.0,1.5,0.f,0.f,0.f);
        auto steel=render(48000.0,1.5,0.5f,0.f,0.f);
        auto tank=render(48000.0,1.5,1.f,0.f,0.f);
        require(difference(plate.left,steel.left) > 1e-5, "Plate differs from Steel", failures);
        require(difference(steel.left,tank.left) > 1e-5, "Steel differs from Tank", failures);

        auto clean=render(48000.0,1.5,0.5f,0.f,0.f);
        auto bit12=render(48000.0,1.5,0.5f,0.f,0.5f);
        auto bit8=render(48000.0,1.5,0.5f,0.f,1.f);
        require(difference(clean.left,bit12.left) > 1e-7, "12-bit color changes tail", failures);
        require(difference(bit12.left,bit8.left) > 1e-7, "8-bit differs from 12-bit", failures);

        Processor p;
        p.initialize(nullptr);
        require(p.getLatencySamples()==0, "Reported latency is 0 samples", failures);
        p.terminate();

        // Bypass must be exact for a one-sample impulse.
        auto bypass=render(48000.0,0.1,0.5f,0.f,0.f,true,true);
        require(bypass.left[0]==1.f && bypass.right[0]==1.f, "Bypass passes input sample exactly", failures);
        require(energy(bypass.left,1,bypass.left.size())==0.0, "Bypass adds no tail", failures);
    }
    catch(const std::exception& e)
    {
        std::cout << "[FAIL] Exception: " << e.what() << "\n";
        ++failures;
    }

    if(failures)
    {
        std::cout << failures << " measurement test(s) failed.\n";
        return 1;
    }
    std::cout << "All Ugly Reverb DSP measurements passed.\n";
    return 0;
}

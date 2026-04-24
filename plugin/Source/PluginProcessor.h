#pragma once

#include <array>
#include <juce_audio_processors/juce_audio_processors.h>
#include "chaos/Lorenz.h"
#include "chaos/Thomas.h"
#include "chaos/Rossler.h"
#include "chaos/Chua.h"
#include "chaos/Aizawa.h"
#include "chaos/Henon.h"
#include "dsp/SVFMorph.h"
#include "dsp/MoogLadder.h"
#include "dsp/DiodeLadder.h"
#include "dsp/TunedComb.h"
#include "dsp/TiltEQ.h"

class ManifoldProcessor : public juce::AudioProcessor
{
public:
    struct PortraitPoint { float x, y, z; };

    ManifoldProcessor();
    ~ManifoldProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Manifold"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override    { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    // SPSC: audio thread writes via pushPortraitPoint(); GUI thread drains via popPortraitPoints().
    int popPortraitPoints (PortraitPoint* dst, int maxPoints) noexcept;

private:
    juce::AudioProcessorValueTreeState apvts;

    manifold::chaos::Lorenz lorenz;
    manifold::chaos::Thomas thomas;
    manifold::chaos::Rossler rossler;
    manifold::chaos::Chua    chua;
    manifold::chaos::Aizawa  aizawa;
    manifold::chaos::Henon   henon;
    manifold::dsp::SVFMorph    svfL,    svfR;
    manifold::dsp::MoogLadder  moogL,   moogR;
    manifold::dsp::DiodeLadder diodeL,  diodeR;
    manifold::dsp::TunedComb   combL,   combR;
    manifold::dsp::TiltEQ      tiltL,   tiltR;
    double currentSampleRate = 48000.0;

    // One-pole LP smoothers for each mod axis, driven by Warmth.
    float modSmoothedX = 0.0f;
    float modSmoothedY = 0.0f;
    float modSmoothedZ = 0.0f;

    // Portrait pipe — decimate to ~3 kHz so a 60 Hz GUI gets ~50 fresh points/frame.
    static constexpr int kPortraitFifoSize  = 4096;
    static constexpr int kPortraitDecimation = 16;
    juce::AbstractFifo portraitFifo { kPortraitFifoSize };
    std::array<PortraitPoint, kPortraitFifoSize> portraitBuf {};
    int portraitDecimCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ManifoldProcessor)
};

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Params.h"
#include "dsp/Shaper.h"

#include <cmath>
#include <cstring>

namespace
{
    // Mod matrix for the Lorenz → chain routing. Hardcoded in MVP; mod depths become
    // sound-design-time data (per-preset) rather than user automation.
    constexpr float kModDriveDepth     = 2.5f;   // lor1.x -> fold.drive      (linear, positive)
    constexpr float kModCutoffOctaves  = 4.0f;   // lor1.y -> filt.cutoff     (exp2,   positive)
    constexpr float kModResonanceDepth = 0.3f;   // lor1.z -> filt.resonance  (linear, positive)
    constexpr float kModMorphDepth     = 0.35f;  // lor1.z -> filt.morph      (linear, bipolar)

    constexpr float kTiltPivotHz  = 800.0f;      // warmth tilt pivot
    constexpr float kTiltMaxDb    = 5.0f;        // +-dB at warmth = 1
}

ManifoldProcessor::ManifoldProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Manifold", manifold::params::makeLayout())
{
}

void ManifoldProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    lorenz.reset();
    thomas.reset();
    rossler.reset();
    svfL  .reset(); svfR  .reset();
    moogL .reset(); moogR .reset();
    diodeL.reset(); diodeR.reset();
    combL .prepare (sampleRate); combR.prepare (sampleRate);
    tiltL.reset(); tiltR.reset();
    tiltL.prepare (static_cast<float> (sampleRate), kTiltPivotHz);
    tiltR.prepare (static_cast<float> (sampleRate), kTiltPivotHz);
    modSmoothedX = modSmoothedY = modSmoothedZ = 0.0f;
    portraitFifo.reset();
    portraitDecimCounter = 0;
}

bool ManifoldProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in.isDisabled() || out.isDisabled()) return false;
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void ManifoldProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();
    const auto sr         = static_cast<float> (currentSampleRate);

    // Snapshot params for this block — parameter changes are smooth enough at block
    // boundaries for this DSP; per-sample reads would be wasteful.
    using namespace manifold::params;
    const auto filterType = (FilterType) (int) apvts.getRawParameterValue (id::filterType)->load();
    const auto chaosType  = (ChaosType)  (int) apvts.getRawParameterValue (id::chaosType)->load();
    const auto routing    = (Routing)    (int) apvts.getRawParameterValue (id::routing)->load();
    const auto shaperType = (manifold::dsp::shaper::Type) (int) apvts.getRawParameterValue (id::shaperType)->load();
    const float intensity = apvts.getRawParameterValue (id::intensity)->load();
    const float speedKnob = apvts.getRawParameterValue (id::speed)->load();
    const float warmth    = apvts.getRawParameterValue (id::warmth)->load();
    const float baseDrive = apvts.getRawParameterValue (id::drive)->load();
    const float baseCut   = apvts.getRawParameterValue (id::cutoff)->load();
    const float baseRes   = apvts.getRawParameterValue (id::resonance)->load();
    const float baseMorph = apvts.getRawParameterValue (id::morph)->load();
    const float outGainDb = apvts.getRawParameterValue (id::output)->load();

    lorenz.setRho   (lorenzRhoFromIntensity (intensity));
    lorenz.setSpeed (lorenzSpeedFromMacro   (speedKnob));
    thomas.setB     (thomasBFromIntensity   (intensity));
    thomas.setSpeed (lorenzSpeedFromMacro   (speedKnob));
    rossler.setC    (rosslerCFromIntensity  (intensity));
    rossler.setSpeed(lorenzSpeedFromMacro   (speedKnob));

    const float smoothingHz = warmthToSmoothingHz (warmth);
    // One-pole coefficient. smoothingHz == 0 -> a = 1 (bypass, no smoothing).
    const float smoothA = smoothingHz > 0.0f
        ? 1.0f - std::exp (-juce::MathConstants<float>::twoPi * smoothingHz / sr)
        : 1.0f;

    const float outGain = std::pow (10.0f, outGainDb / 20.0f);

    // Warmth also drives a post-filter tilt EQ: rotate spectrum darker as warmth climbs.
    const float tiltDb   = juce::jlimit (0.0f, 1.0f, warmth) * kTiltMaxDb;
    const float tiltLow  = std::pow (10.0f,  tiltDb / 20.0f);
    const float tiltHigh = std::pow (10.0f, -tiltDb / 20.0f);

    auto* left  = buffer.getWritePointer (0);
    auto* right = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        // One chaos step per sample, normalized to [-1, 1], smoothed per Warmth.
        const auto n = [&]
        {
            if (chaosType == ChaosType::Thomas)
            {
                const auto raw = thomas.step (currentSampleRate);
                const auto nt  = manifold::chaos::Thomas::normalize (raw);
                return manifold::chaos::Lorenz::Sample { nt.x, nt.y, nt.z };
            }
            if (chaosType == ChaosType::Rossler)
            {
                const auto raw = rossler.step (currentSampleRate);
                const auto nr  = manifold::chaos::Rossler::normalize (raw);
                return manifold::chaos::Lorenz::Sample { nr.x, nr.y, nr.z };
            }
            const auto raw = lorenz.step (currentSampleRate);
            return manifold::chaos::Lorenz::normalize (raw);
        }();

        modSmoothedX += smoothA * (static_cast<float> (n.x) - modSmoothedX);
        modSmoothedY += smoothA * (static_cast<float> (n.y) - modSmoothedY);
        modSmoothedZ += smoothA * (static_cast<float> (n.z) - modSmoothedZ);

        // Decimated push to the GUI portrait pipe. Drop on overflow — visualizer
        // is allowed to lose points; correctness lives on the audio side.
        if (++portraitDecimCounter >= kPortraitDecimation)
        {
            portraitDecimCounter = 0;
            int s1, sz1, s2, sz2;
            portraitFifo.prepareToWrite (1, s1, sz1, s2, sz2);
            if (sz1 > 0)
                portraitBuf[(size_t) s1] = { modSmoothedX, modSmoothedY, modSmoothedZ };
            portraitFifo.finishedWrite (sz1);
        }

        const float mx    = 0.5f * (modSmoothedX + 1.0f);                     // positive [0,1]
        const float my    = 0.5f * (modSmoothedY + 1.0f);                     // positive [0,1]
        const float mzPos = 0.5f * (modSmoothedZ + 1.0f);                     // positive [0,1]
        const float mzBip = modSmoothedZ;                                     // bipolar [-1,1]

        const float drive     = baseDrive + mx * kModDriveDepth;
        const float cutoff    = baseCut * std::exp2 (my * kModCutoffOctaves);
        const float resonance = juce::jlimit (0.0f, 0.95f,
                                              baseRes + mzPos * kModResonanceDepth);
        const float morph     = juce::jlimit (0.0f, 1.0f,
                                              baseMorph + mzBip * kModMorphDepth);

        // Comb repurposes morph as feedback-LP "tone" (Hz) so chaos-on-morph
        // turns into a slowly-warping decay brightness.
        const float toneHz = 200.0f + morph * 7800.0f;

        auto runFilter = [&] (float in, int chan) -> float
        {
            switch (filterType)
            {
                case FilterType::MoogLadder:
                    return chan == 0 ? moogL .process (in, cutoff, resonance, morph, sr)
                                     : moogR .process (in, cutoff, resonance, morph, sr);
                case FilterType::DiodeLadder:
                    return chan == 0 ? diodeL.process (in, cutoff, resonance, morph, sr)
                                     : diodeR.process (in, cutoff, resonance, morph, sr);
                case FilterType::TunedComb:
                    return chan == 0 ? combL .process (in, cutoff, resonance, toneHz, sr)
                                     : combR .process (in, cutoff, resonance, toneHz, sr);
                case FilterType::SVF:
                default:
                    return chan == 0 ? svfL  .process (in, cutoff, resonance, morph, sr)
                                     : svfR  .process (in, cutoff, resonance, morph, sr);
            }
        };

        auto shape = [&] (float v) { return manifold::dsp::shaper::process (shaperType, v, drive); };
        auto runChain = [&] (float x, int chan)
        {
            if (routing == Routing::FilterThenFold)
                return shape (runFilter (x, chan));
            return runFilter (shape (x), chan);
        };

        float lOut = outGain * tiltL.process (runChain (left[i], 0), tiltLow, tiltHigh);
        if (! std::isfinite (lOut))
        {
            // Filter state has corrupted — zero the sample and reset filters so we recover
            // instead of outputting silent/NaN forever.
            lOut = 0.0f;
            svfL.reset();  moogL.reset();  diodeL.reset();  combL.prepare (currentSampleRate);
            tiltL.reset(); tiltL.prepare ((float) currentSampleRate, kTiltPivotHz);
        }
        left[i] = juce::jlimit (-4.0f, 4.0f, lOut);

        if (right != nullptr)
        {
            float rOut = outGain * tiltR.process (runChain (right[i], 1), tiltLow, tiltHigh);
            if (! std::isfinite (rOut))
            {
                rOut = 0.0f;
                svfR.reset();  moogR.reset();  diodeR.reset();  combR.prepare (currentSampleRate);
                tiltR.reset(); tiltR.prepare ((float) currentSampleRate, kTiltPivotHz);
            }
            right[i] = juce::jlimit (-4.0f, 4.0f, rOut);
        }
    }
}

juce::AudioProcessorEditor* ManifoldProcessor::createEditor()
{
    return new ManifoldEditor (*this);
}

void ManifoldProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void ManifoldProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

int ManifoldProcessor::popPortraitPoints (PortraitPoint* dst, int maxPoints) noexcept
{
    const int available = portraitFifo.getNumReady();
    const int toRead    = juce::jmin (available, maxPoints);
    if (toRead <= 0) return 0;

    int s1, sz1, s2, sz2;
    portraitFifo.prepareToRead (toRead, s1, sz1, s2, sz2);
    if (sz1 > 0)
        std::memcpy (dst,        portraitBuf.data() + s1, (size_t) sz1 * sizeof (PortraitPoint));
    if (sz2 > 0)
        std::memcpy (dst + sz1,  portraitBuf.data() + s2, (size_t) sz2 * sizeof (PortraitPoint));
    portraitFifo.finishedRead (sz1 + sz2);
    return sz1 + sz2;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ManifoldProcessor();
}

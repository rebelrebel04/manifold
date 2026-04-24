#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace manifold::params
{

// Parameter IDs. Keep stable across versions — hosts serialize by ID.
namespace id
{
    inline constexpr const char* intensity  = "intensity";
    inline constexpr const char* speed      = "speed";
    inline constexpr const char* warmth     = "warmth";
    inline constexpr const char* drive      = "drive";
    inline constexpr const char* cutoff     = "cutoff";
    inline constexpr const char* resonance  = "resonance";
    inline constexpr const char* morph      = "morph";
    inline constexpr const char* output     = "output";
    inline constexpr const char* filterType = "filterType";
    inline constexpr const char* chaosType  = "chaosType";
    inline constexpr const char* routing    = "routing";
    inline constexpr const char* shaperType = "shaperType";
}

enum class FilterType : int { SVF = 0, MoogLadder = 1, DiodeLadder = 2, TunedComb = 3 };
enum class ChaosType  : int { Lorenz = 0, Thomas = 1, Rossler = 2, Chua = 3, Aizawa = 4, Henon = 5 };
enum class Routing    : int { FoldThenFilter = 0, FilterThenFold = 1 };
// Mirrors manifold::dsp::shaper::Type.
enum class ShaperType : int { Fold = 0, SoftClip = 1, HardClip = 2, Rectify = 3, Sine = 4, TubeAsym = 5, ChebyT3 = 6, ChebyT5 = 7 };

inline juce::StringArray filterTypeChoices()
{
    return { "SVF", "Moog Ladder", "Diode Ladder", "Tuned Comb" };
}

inline juce::StringArray chaosTypeChoices()
{
    return { "Lorenz", "Thomas", "Rossler", "Chua", "Aizawa", "Henon" };
}

inline juce::StringArray routingChoices()
{
    return { "Shape -> Filter", "Filter -> Shape" };
}

inline juce::StringArray shaperTypeChoices()
{
    return { "Fold", "Soft Clip", "Hard Clip", "Rectify", "Sine", "Tube Asym", "Cheby T3", "Cheby T5" };
}

inline juce::AudioProcessorValueTreeState::ParameterLayout makeLayout()
{
    using APF = juce::AudioParameterFloat;
    using Range = juce::NormalisableRange<float>;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // Macros — [0, 1] each; resolver in processBlock maps these to core raw params.
    p.push_back (std::make_unique<APF> (juce::ParameterID { id::intensity, 1 },
                                        "Intensity", Range { 0.0f, 1.0f }, 0.5f));
    p.push_back (std::make_unique<APF> (juce::ParameterID { id::speed, 1 },
                                        "Speed",     Range { 0.0f, 1.0f }, 0.4f));
    p.push_back (std::make_unique<APF> (juce::ParameterID { id::warmth, 1 },
                                        "Warmth",    Range { 0.0f, 1.0f }, 0.0f));

    // Base effect params — mod matrix nudges these per sample.
    p.push_back (std::make_unique<APF> (juce::ParameterID { id::drive, 1 },
                                        "Drive", Range { 1.0f, 4.0f }, 1.5f));

    Range cutoffRange { 20.0f, 20000.0f, 0.0f, 0.25f };   // skewed for audio taper
    p.push_back (std::make_unique<APF> (juce::ParameterID { id::cutoff, 1 },
                                        "Cutoff", cutoffRange, 150.0f));

    p.push_back (std::make_unique<APF> (juce::ParameterID { id::resonance, 1 },
                                        "Resonance", Range { 0.0f, 0.95f }, 0.4f));
    p.push_back (std::make_unique<APF> (juce::ParameterID { id::morph, 1 },
                                        "Morph", Range { 0.0f, 1.0f }, 0.5f));
    p.push_back (std::make_unique<APF> (juce::ParameterID { id::output, 1 },
                                        "Output", Range { -24.0f, 12.0f }, 0.0f));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { id::filterType, 1 }, "Filter",
        filterTypeChoices(), (int) FilterType::SVF));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { id::chaosType, 1 }, "Chaos",
        chaosTypeChoices(), (int) ChaosType::Lorenz));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { id::routing, 1 }, "Routing",
        routingChoices(), (int) Routing::FoldThenFilter));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { id::shaperType, 1 }, "Shaper",
        shaperTypeChoices(), (int) ShaperType::Fold));

    return { p.begin(), p.end() };
}

// Macro resolution — mirrors proto/macros.py for the Lorenz core.
inline float lorenzRhoFromIntensity (float intensity01) noexcept
{
    return 22.0f + (42.0f - 22.0f) * intensity01;
}

inline float lorenzSpeedFromMacro (float speed01) noexcept
{
    return 0.3f + (2.8f - 0.3f) * speed01;
}

// Thomas intensity mapping. b=0.208 is weakly-damped edge of chaos; lower b is more ergodic.
// Stop well above 0 to stay in chaotic regime (near-zero b gets quasi-periodic).
inline float thomasBFromIntensity (float intensity01) noexcept
{
    return 0.208f + (0.10f - 0.208f) * juce::jlimit (0.0f, 1.0f, intensity01);
}

// Rössler intensity mapping. c ~= 5.7 is the classic chaotic value; higher c widens
// the attractor and produces band-merging/sprawl. Below ~5.0 slips into periodic orbits.
inline float rosslerCFromIntensity (float intensity01) noexcept
{
    return 5.0f + (10.0f - 5.0f) * juce::jlimit (0.0f, 1.0f, intensity01);
}

// Chua intensity mapping. alpha ~= 15.6 is the classic double-scroll value; lower alpha
// sits in periodic regimes, higher alpha gets noisier/more sprawling.
inline float chuaAlphaFromIntensity (float intensity01) noexcept
{
    return 12.0f + (22.0f - 12.0f) * juce::jlimit (0.0f, 1.0f, intensity01);
}

// Aizawa intensity mapping. a ~= 0.95 is the classic petal attractor. Lower a tightens
// the petals; higher a (toward 1.0) makes the geometry looser and more complex.
inline float aizawaAFromIntensity (float intensity01) noexcept
{
    return 0.85f + (1.0f - 0.85f) * juce::jlimit (0.0f, 1.0f, intensity01);
}

// Hénon intensity mapping. a=1.0 is near period-3; a=1.4 is the classic fully-chaotic setting.
inline float henonAFromIntensity (float intensity01) noexcept
{
    return 1.0f + (1.4f - 1.0f) * juce::jlimit (0.0f, 1.0f, intensity01);
}

// warmth == 0 -> no smoothing. warmth > 0 -> lerp 400 Hz -> 8 Hz (higher warmth = slower).
inline float warmthToSmoothingHz (float warmth01) noexcept
{
    if (warmth01 <= 0.0f) return 0.0f;
    return 400.0f + (8.0f - 400.0f) * juce::jlimit (0.0f, 1.0f, warmth01);
}

} // namespace manifold::params

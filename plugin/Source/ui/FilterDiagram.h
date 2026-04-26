#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <complex>
#include <array>
#include <cmath>
#include "../Params.h"

namespace manifold::ui
{

// Draws the magnitude-frequency response for one filter type.
// Uses the exact Z-domain transfer functions of the implemented filters
// (SVFMorph, MoogLadder, DiodeLadder, TunedComb) so the diagrams are
// accurate depictions, not placeholders.
//
// Display params (representative — shows character, not a specific preset):
//   SVF / Moog / Diode : cutoff=1200 Hz, resonance=0.62, morph=0 (LP mode)
//   TunedComb          : pitch=500 Hz, feedback=0.72, tone=3500 Hz
class FilterDiagram : public juce::Component
{
public:
    explicit FilterDiagram (manifold::params::FilterType t) : filterType (t)
    {
        setInterceptsMouseClicks (false, false);
    }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat().reduced (2.0f, 2.0f);
        g.setColour (juce::Colour (0xff0a0a12));
        g.fillRoundedRectangle (b, 4.0f);

        const auto path = buildResponsePath (b.reduced (4.0f, 3.0f));
        const auto col  = accentColour();

        // Filled area under the curve
        auto fill = path;
        fill.lineTo (b.getRight() - 4.0f, b.getBottom() - 3.0f);
        fill.lineTo (b.getX()    + 4.0f, b.getBottom() - 3.0f);
        fill.closeSubPath();
        g.setColour (col.withAlpha (0.08f));
        g.fillPath (fill);

        // Glow + core strokes
        g.setColour (col.withAlpha (0.28f));
        g.strokePath (path, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
        g.setColour (col.withAlpha (0.85f));
        g.strokePath (path, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

private:
    manifold::params::FilterType filterType;

    static constexpr int   kN    = 256;
    static constexpr float kFMin = 20.0f;
    static constexpr float kFMax = 20000.0f;
    static constexpr float kSR   = 44100.0f;
    // Shared display params for the three ladder-style filters
    static constexpr float kCutoff = 1200.0f;
    static constexpr float kRes    = 0.62f;
    static constexpr float kMorph  = 0.0f;   // LP mode

    juce::Colour accentColour() const noexcept
    {
        using FT = manifold::params::FilterType;
        switch (filterType)
        {
            case FT::MoogLadder:  return juce::Colour (0xff7ad6ff);
            case FT::DiodeLadder: return juce::Colour (0xffffb870);
            case FT::TunedComb:   return juce::Colour (0xff8effa0);
            default:              return juce::Colour (0xffb59cff);  // SVF
        }
    }

    // ── Z-domain transfer functions ──────────────────────────────────────────
    // All use zm1 = e^(-jω) at each sample frequency.

    // SVF (Chamberlin TPT).
    // H_LP = f^2 / D,  H_BP = f(1-z^-1) / D,  H_HP = (1-z^-1)^2 / D
    // D = 1 + (-2 + f^2 + qf) z^-1 + (1 - qf) z^-2
    static float svfMag (std::complex<float> zm1, float fc, float res, float morph) noexcept
    {
        using C = std::complex<float>;
        const float f = 2.0f * std::sin (juce::MathConstants<float>::pi * fc / kSR);
        const float q = std::clamp (1.0f - res, 0.02f, 2.0f);

        const C denom = 1.0f
            + (-2.0f + f*f + q*f) * zm1
            + (1.0f  - q*f)       * (zm1 * zm1);

        const C numLP = C (f * f);
        const C numBP = C (f) * (1.0f - zm1);
        const C numHP = (1.0f - zm1) * (1.0f - zm1);

        const float wLP = std::clamp (1.0f - 2.0f * morph, 0.0f, 1.0f);
        const float wHP = std::clamp (2.0f * morph - 1.0f, 0.0f, 1.0f);
        const float wBP = 1.0f - wLP - wHP;

        return std::abs ((wLP * numLP + wBP * numBP + wHP * numHP) / denom);
    }

    // Moog ladder (linearised — ignores tanh for frequency-response purposes).
    // Each stage: H1 = G / (1 - (1-G) z^-1)
    // Y4 = H1^4 / (1 + k H1^4),  Y2_out = H1^2 / (1 + k H1^4)
    // morph blends Y4 (4-pole) and Y2_out (2-pole)
    static float moogMag (std::complex<float> zm1, float fc, float res, float morph) noexcept
    {
        using C = std::complex<float>;
        const float T  = 1.0f / kSR;
        const float wa = (2.0f / T) * std::tan (juce::MathConstants<float>::pi * fc * T);
        const float g  = wa * T * 0.5f;
        const float G  = g / (1.0f + g);
        const float k  = 4.0f * std::clamp (res, 0.0f, 0.99f);

        const C H1  = G / (1.0f - (1.0f - G) * zm1);
        const C H2  = H1 * H1;
        const C H4  = H2 * H2;
        const C den = 1.0f + k * H4;

        const C Hm = (H4 * (1.0f - morph) + H2 * morph) / den;
        return std::abs (Hm) * (1.0f + 0.5f * res);   // makeup gain matches processor
    }

    // Diode ladder — same topology as Moog but k = 6.5 * res (higher self-osc threshold,
    // sharper resonance peak gives the edgier 303-like character).
    static float diodeMag (std::complex<float> zm1, float fc, float res, float morph) noexcept
    {
        using C = std::complex<float>;
        const float T  = 1.0f / kSR;
        const float wa = (2.0f / T) * std::tan (juce::MathConstants<float>::pi * fc * T);
        const float g  = wa * T * 0.5f;
        const float G  = g / (1.0f + g);
        const float k  = 6.5f * std::clamp (res, 0.0f, 0.99f);

        const C H1  = G / (1.0f - (1.0f - G) * zm1);
        const C H2  = H1 * H1;
        const C H4  = H2 * H2;
        const C den = 1.0f + k * H4;

        const C Hm = (H4 * (1.0f - morph) + H2 * morph) / den;
        return std::abs (Hm) * (1.0f + 1.2f * res);
    }

    // Tuned comb: delay line + 1-pole LP in feedback.
    // H(z) = z^(-D) / (1 - fb * H_lp(z) * z^(-D))
    // z^(-D) evaluated as exp(-j omega D) for fractional delay D = sr / pitchHz.
    static float combMag (float freqHz) noexcept
    {
        using C = std::complex<float>;
        constexpr float pitchHz = 500.0f;
        constexpr float fb      = 0.72f;
        constexpr float toneHz  = 3500.0f;

        const float omega = juce::MathConstants<float>::twoPi * freqHz / kSR;
        const float D     = kSR / pitchHz;

        // 1-pole LP in feedback path
        const float a    = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * toneHz / kSR);
        const C zm1_tone { std::cos (omega), -std::sin (omega) };
        const C H_lp = a / (1.0f - (1.0f - a) * zm1_tone);

        // Fractional delay
        const C zD { std::cos (omega * D), -std::sin (omega * D) };

        const C H = zD / (1.0f - fb * H_lp * zD);
        return std::abs (H);
    }

    // ── Path builder ─────────────────────────────────────────────────────────
    juce::Path buildResponsePath (juce::Rectangle<float> b) const
    {
        using FT = manifold::params::FilterType;

        std::array<float, kN> mag {};
        float maxMag = 1e-9f;

        for (int i = 0; i < kN; ++i)
        {
            const float t = (float) i / (float) (kN - 1);
            const float f = kFMin * std::pow (kFMax / kFMin, t);
            const float omega = juce::MathConstants<float>::twoPi * f / kSR;
            const std::complex<float> zm1 { std::cos (omega), -std::sin (omega) };

            float m = 0.0f;
            switch (filterType)
            {
                case FT::SVF:         m = svfMag  (zm1, kCutoff, kRes, kMorph); break;
                case FT::MoogLadder:  m = moogMag (zm1, kCutoff, kRes, kMorph); break;
                case FT::DiodeLadder: m = diodeMag(zm1, kCutoff, kRes, kMorph); break;
                case FT::TunedComb:   m = combMag (f);                          break;
                default: break;
            }
            mag[(size_t) i] = m;
            maxMag = std::max (maxMag, m);
        }

        // Display in dB, normalised so 0 dB = passband peak. Range: -48..+3 dB.
        constexpr float kDbMin  = -48.0f;
        constexpr float kDbMax  =   3.0f;
        constexpr float kDbSpan = kDbMax - kDbMin;

        juce::Path path;
        for (int i = 0; i < kN; ++i)
        {
            const float normMag = mag[(size_t) i] / maxMag;
            const float db = normMag > 1e-9f ? 20.0f * std::log10 (normMag) : kDbMin;
            const float dbC = std::clamp (db, kDbMin, kDbMax);

            const float px = b.getX() + (float) i / (float) (kN - 1) * b.getWidth();
            const float py = b.getBottom() - (dbC - kDbMin) / kDbSpan * b.getHeight();

            i == 0 ? path.startNewSubPath (px, py) : path.lineTo (px, py);
        }
        return path;
    }
};

} // namespace manifold::ui

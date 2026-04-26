#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ManifoldLookAndFeel.h"
#include "../Params.h"
#include "../dsp/Shaper.h"

namespace manifold::ui
{

// Header-only component that renders an accurate waveshaper transfer-function plot
// for a given ShaperType. x-axis = input [-1, +1], y-axis = output [-1, +1].
//
// The curves are sampled directly from manifold::dsp::shaper::process() at a
// representative drive value chosen to best reveal each shaper's character.
// This guarantees the diagram always matches what the processor actually does.
class ShapeDiagram : public juce::Component
{
public:
    using ST = manifold::params::ShaperType;

    explicit ShapeDiagram (ST type) : shaperType (type) {}

    void paint (juce::Graphics& g) override
    {
        using LF  = manifold::ui::ManifoldLookAndFeel;
        using DSP = manifold::dsp::shaper::Type;

        const auto  bounds = getLocalBounds().toFloat();
        const float w      = bounds.getWidth();
        const float h      = bounds.getHeight();
        const float pad    = 3.0f;

        // Background
        g.setColour (LF::plate1());
        g.fillRoundedRectangle (bounds, 3.0f);

        // Zero-crossing axes
        g.setColour (LF::plateLine().withAlpha (0.9f));
        g.drawHorizontalLine ((int) (h * 0.5f), pad, w - pad);
        g.drawVerticalLine   ((int) (w * 0.5f), pad, h - pad);

        // Unity-gain diagonal (y = x reference, very faint)
        g.setColour (LF::plateLine().withAlpha (0.35f));
        g.drawLine (pad, h - pad, w - pad, pad, 0.7f);

        // Sample the actual transfer function
        static constexpr int kN = 220;
        const float  drive   = driveFor (shaperType);
        const juce::Colour accent = accentFor (shaperType);
        const DSP    dspType = static_cast<DSP> ((int) shaperType);

        juce::Path curve;
        bool first = true;

        for (int i = 0; i < kN; ++i)
        {
            // Input -1 → +1
            const float x  = -1.0f + 2.0f * (float) i / (float) (kN - 1);
            const float y  = manifold::dsp::shaper::process (dspType, x, drive);

            // Map to pixel space: x -1→pad, +1→w-pad; y +1→pad, -1→h-pad (inverted)
            const float px = pad + (x + 1.0f) * 0.5f * (w - 2.0f * pad);
            const float py = pad + (1.0f - y)  * 0.5f * (h - 2.0f * pad);

            if (first) { curve.startNewSubPath (px, py); first = false; }
            else        { curve.lineTo (px, py); }
        }

        // Outer glow
        g.setColour (accent.withAlpha (0.12f));
        g.strokePath (curve, juce::PathStrokeType (6.0f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        // Mid halo
        g.setColour (accent.withAlpha (0.40f));
        g.strokePath (curve, juce::PathStrokeType (2.2f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        // Core line
        g.setColour (accent.withAlpha (0.90f));
        g.strokePath (curve, juce::PathStrokeType (1.1f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

private:
    ST shaperType;

    // Drive values chosen to reveal each shaper's character in a small diagram.
    static float driveFor (ST t) noexcept
    {
        switch (t)
        {
            case ST::Fold:       return 2.5f;   // clear fold + re-fold
            case ST::SoftClip:   return 3.0f;   // obvious tanh saturation
            case ST::HardClip:   return 2.0f;   // visible flat-top clipping
            case ST::Rectify:    return 1.0f;   // classic V-shape
            case ST::Sine:       return 2.0f;   // wraps back through zero at x=±1
            case ST::TubeAsym:   return 2.5f;   // clear pos/neg asymmetry
            case ST::ChebyT3:    return 1.0f;   // natural T3 polynomial curve
            case ST::ChebyT5:    return 1.0f;   // natural T5 polynomial curve
            default:             return 1.0f;
        }
    }

    // Per-shaper accent hues — warm spectrum to contrast with the filter drawer's cool tones.
    static juce::Colour accentFor (ST t) noexcept
    {
        switch (t)
        {
            case ST::Fold:       return juce::Colour (0xff7adeff);   // cyan  — complex, cool
            case ST::SoftClip:   return juce::Colour (0xffffc07a);   // amber — warm saturation
            case ST::HardClip:   return juce::Colour (0xffff7a7a);   // red   — aggressive
            case ST::Rectify:    return juce::Colour (0xffffd07a);   // gold  — energetic
            case ST::Sine:       return juce::Colour (0xff80d4ff);   // sky   — smooth, wavy
            case ST::TubeAsym:   return juce::Colour (0xffffaa64);   // orange — tube warmth
            case ST::ChebyT3:    return juce::Colour (0xffe080ff);   // violet-pink — synthetic
            case ST::ChebyT5:    return juce::Colour (0xffb59cff);   // violet — main accent
            default:             return juce::Colour (0xffb59cff);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShapeDiagram)
};

} // namespace manifold::ui

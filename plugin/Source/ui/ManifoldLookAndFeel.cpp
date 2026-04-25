#include "ManifoldLookAndFeel.h"

namespace manifold::ui
{

// sRGB approximations of the design's oklch tokens at hue ~275.
// (See manifold-ui.css :root in the design bundle for the source values.)
const juce::Colour ManifoldLookAndFeel::kPlate0          { 0xff10101a };
const juce::Colour ManifoldLookAndFeel::kPlate1          { 0xff15151f };
const juce::Colour ManifoldLookAndFeel::kPlate2          { 0xff1e1e2a };
const juce::Colour ManifoldLookAndFeel::kPlate3          { 0xff262633 };
const juce::Colour ManifoldLookAndFeel::kPlateLine       { 0x80303040 };
const juce::Colour ManifoldLookAndFeel::kPlateLineStrong { 0xcc3a3a4c };
const juce::Colour ManifoldLookAndFeel::kInk1            { 0xfff0eef8 };
const juce::Colour ManifoldLookAndFeel::kInk2            { 0xffbcbac8 };
const juce::Colour ManifoldLookAndFeel::kInk3            { 0xff7c7a8a };
const juce::Colour ManifoldLookAndFeel::kInk4            { 0xff52505c };

ManifoldLookAndFeel::ManifoldLookAndFeel()
{
    // Default slider colours so any rotary picks up the violet accent unless the
    // host component sets a per-instance override (engine-tinted knobs).
    setColour (juce::Slider::rotarySliderFillColourId,    juce::Colour { 0xffb59cff });
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour { 0xff2c2c35 });
    setColour (juce::Slider::thumbColourId,               kInk1);
    setColour (juce::Slider::textBoxTextColourId,         kInk1);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);

    setColour (juce::Label::textColourId, kInk2);
}

void ManifoldLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                            int x, int y, int width, int height,
                                            float sliderPos,
                                            float rotaryStartAngle, float rotaryEndAngle,
                                            juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
    const float r     = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();

    // Proportional geometry — all radii as fractions of r so the knob renders
    // correctly at any size (BLEND ~r=20, secondary ~r=35, macro ~r=50).
    // Factors calibrated to match the absolute-offset look at r=40.
    const float bodyR      = r * 0.775f;   // outer matte body   (was r-9  @ r=40 → 31/40)
    const float chamferOut = bodyR + r * 0.011f;
    const float chamferIn  = bodyR - r * 0.026f;
    const float topR       = r * 0.675f;   // flat-top inset     (was r-13 @ r=40 → 27/40)
    const float trackR     = r * 0.925f;   // arc track radius   (was r-3  @ r=40 → 37/40)
    const float arcR       = trackR;
    const float ptrR       = r * 0.725f;   // pointer dot ring   (was r-11 @ r=40 → 29/40)

    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto track  = slider.findColour (juce::Slider::rotarySliderOutlineColourId);

    // Stroke widths — proportional with a minimum so tiny knobs still read.
    const float trackStroke = juce::jmax (1.0f,  r * 0.050f);   // 2.0 @ r=40
    const float glowOuter   = juce::jmax (3.0f,  r * 0.150f);   // 6.0 @ r=40
    const float glowMid     = juce::jmax (1.5f,  r * 0.085f);   // 3.4 @ r=40
    const float glowCore    = juce::jmax (1.0f,  r * 0.055f);   // 2.2 @ r=40

    // Pointer dot half-widths — proportional with minimums.
    const float dotGlowHW = juce::jmax (1.5f, r * 0.110f);      // 4.4 @ r=40
    const float dotCoreHW = juce::jmax (1.0f, r * 0.055f);      // 2.2 @ r=40
    const float dotHlHW   = juce::jmax (0.4f, r * 0.020f);      // 0.8 @ r=40

    // Arc track (always full sweep, dim).
    {
        juce::Path p;
        p.addCentredArc (centre.x, centre.y, trackR, trackR, 0.0f,
                         rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (track);
        g.strokePath (p, juce::PathStrokeType (trackStroke, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    }

    // Active arc — drawn three times at decreasing opacity / increasing width to
    // fake a layered drop-shadow glow without an offscreen blur pass.
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    {
        juce::Path p;
        p.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f,
                         rotaryStartAngle, angle, true);
        // Outer glow
        g.setColour (accent.withAlpha (0.30f));
        g.strokePath (p, juce::PathStrokeType (glowOuter, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        // Mid glow
        g.setColour (accent.withAlpha (0.65f));
        g.strokePath (p, juce::PathStrokeType (glowMid, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
        // Bright core
        g.setColour (accent);
        g.strokePath (p, juce::PathStrokeType (glowCore, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    }

    // Outer body — radial gradient #5a5966 → #2d2d36 → #14141a, light source upper-left.
    {
        juce::ColourGradient grad (juce::Colour { 0xff5a5966 },
                                   centre.x - bodyR * 0.3f, centre.y - bodyR * 0.4f,
                                   juce::Colour { 0xff14141a },
                                   centre.x + bodyR * 0.6f, centre.y + bodyR * 0.7f,
                                   true);
        grad.addColour (0.5, juce::Colour { 0xff2d2d36 });
        g.setGradientFill (grad);
        g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    }

    // Chamfer rings — outer highlight, inner shadow.
    g.setColour (juce::Colour::fromRGBA (180, 180, 200, 115));
    g.drawEllipse (centre.x - chamferOut, centre.y - chamferOut,
                   chamferOut * 2.0f, chamferOut * 2.0f, 0.9f);
    g.setColour (juce::Colour::fromRGBA (0, 0, 0, 180));
    g.drawEllipse (centre.x - chamferIn,  centre.y - chamferIn,
                   chamferIn * 2.0f,  chamferIn * 2.0f,  0.7f);

    // Flat top — slightly lighter inset disc.
    {
        juce::ColourGradient grad (juce::Colour { 0xff3e3e48 },
                                   centre.x - topR * 0.3f, centre.y - topR * 0.4f,
                                   juce::Colour { 0xff1a1a20 },
                                   centre.x + topR * 0.6f, centre.y + topR * 0.7f,
                                   true);
        g.setGradientFill (grad);
        g.fillEllipse (centre.x - topR, centre.y - topR, topR * 2.0f, topR * 2.0f);
    }

    // Pointer dot — engine-tinted with a tiny white inner highlight.
    // JUCE rotary angle convention: 0 = 12 o'clock, positive = clockwise.
    {
        const float px = centre.x + std::sin (angle) * ptrR;
        const float py = centre.y - std::cos (angle) * ptrR;
        // glow
        g.setColour (accent.withAlpha (0.45f));
        g.fillEllipse (px - dotGlowHW, py - dotGlowHW, dotGlowHW * 2.0f, dotGlowHW * 2.0f);
        // core
        g.setColour (accent);
        g.fillEllipse (px - dotCoreHW, py - dotCoreHW, dotCoreHW * 2.0f, dotCoreHW * 2.0f);
        // inner highlight
        g.setColour (juce::Colour::fromRGBA (255, 255, 255, 240));
        g.fillEllipse (px - dotHlHW, py - dotHlHW, dotHlHW * 2.0f, dotHlHW * 2.0f);
    }

    // Centre detail dot — tiny sense of depth.
    g.setColour (juce::Colour { 0xff4a4a54 });
    g.fillEllipse (centre.x - 1.0f, centre.y - 1.0f, 2.0f, 2.0f);
}

} // namespace manifold::ui

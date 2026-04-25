#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace manifold::ui
{

// Dark-plate aesthetic for Manifold. Currently overrides rotary sliders to render
// a matte-black industrial knob with a glowing accent arc — see drawRotarySlider().
//
// Future overrides (combo box, scrollbar, button) will land here as later phases
// of the design implementation pull the rest of the chrome into the same vocabulary.
class ManifoldLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ManifoldLookAndFeel();
    ~ManifoldLookAndFeel() override = default;

    // Matte body, chamfered ring, glowing accent arc, pointer dot at the edge.
    void drawRotarySlider (juce::Graphics&,
                           int x, int y, int width, int height,
                           float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

private:
    // Palette tokens (sRGB approximations of the design's oklch values).
    // Kept here so the rest of the UI shares one source of truth.
    static const juce::Colour kPlate0;
    static const juce::Colour kPlate1;
    static const juce::Colour kPlate2;
    static const juce::Colour kPlate3;
    static const juce::Colour kPlateLine;
    static const juce::Colour kPlateLineStrong;
    static const juce::Colour kInk1;
    static const juce::Colour kInk2;
    static const juce::Colour kInk3;
    static const juce::Colour kInk4;

public:
    // Public accessors for callers that need the shared palette.
    static juce::Colour plate0()          noexcept { return kPlate0; }
    static juce::Colour plate1()          noexcept { return kPlate1; }
    static juce::Colour plate2()          noexcept { return kPlate2; }
    static juce::Colour plate3()          noexcept { return kPlate3; }
    static juce::Colour plateLine()       noexcept { return kPlateLine; }
    static juce::Colour plateLineStrong() noexcept { return kPlateLineStrong; }
    static juce::Colour ink1()            noexcept { return kInk1; }
    static juce::Colour ink2()            noexcept { return kInk2; }
    static juce::Colour ink3()            noexcept { return kInk3; }
    static juce::Colour ink4()            noexcept { return kInk4; }
};

} // namespace manifold::ui

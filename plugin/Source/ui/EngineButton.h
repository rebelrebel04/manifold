#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace manifold::ui
{

// One engine in the chaos selector strip. Behaves as a juce::Button toggle so we
// can attach it to a bool APVTS parameter via juce::AudioProcessorValueTreeState::ButtonAttachment.
//
// Renders:
//   - Card background with subtle gradient + 1px border
//   - Active state: hue-tinted border + outer glow + badge dot
//   - Engine glyph (from EngineGlyphs.h) centered in the upper portion
//   - Engine name in mono caps along the bottom
//
// Engine index maps 1:1 to manifold::params::kChaosEngineParamIds order.
class EngineButton : public juce::Button
{
public:
    EngineButton (int engineIndex, const juce::String& displayName, juce::Colour hue);
    ~EngineButton() override = default;

    void paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown) override;

private:
    int          engineIdx;
    juce::String name;
    juce::Colour hue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EngineButton)
};

} // namespace manifold::ui

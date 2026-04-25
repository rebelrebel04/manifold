#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace manifold::ui
{

// Manifold wordmark in the design's "C" direction:
// custom geometric letterforms (M-A-N-I-F-_-L-D in stroked ink) with a stylised
// violet "O" — concentric ring plus an inner s-curve, with a soft glow.
//
// Source paths trace `WordmarkC` in `manifold-viz.jsx` from the design bundle.
// The component scales via setSize(); the paint() preserves the design's 220×38 aspect.
class ManifoldWordmark : public juce::Component
{
public:
    explicit ManifoldWordmark (juce::Colour accent = juce::Colour { 0xffb59cff });
    ~ManifoldWordmark() override = default;

    void setAccent (juce::Colour c);
    void paint (juce::Graphics& g) override;

private:
    juce::Colour accentColour;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ManifoldWordmark)
};

} // namespace manifold::ui

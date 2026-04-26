#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

namespace manifold::ui
{

// Generic right-side drawer overlay.
// Caller supplies a list of options (name, description, diagram draw function)
// and a callback that fires when a selection is made. The drawer closes itself
// after selection; the caller can also call hide() directly.
//
// Features:
//   - Slide + fade animation on show() / hide()
//   - Hover highlight on mouse-over
//   - Selected state: accent gradient + left bar + outline + checkmark
//
// Usage:
//   drawer.configure (title, options, onSelected, currentIndex);
//   drawer.show();
class PickerDrawer : public juce::Component,
                     private juce::Timer
{
public:
    struct Option
    {
        juce::String name;
        juce::String description;
        // Draws the diagram into the supplied bounds (already clipped).
        std::function<void (juce::Graphics&, juce::Rectangle<float>)> drawDiagram;
    };

    PickerDrawer();

    // (Re)configure before showing. Safe to call every time the picker is opened.
    void configure (const juce::String&       title,
                    std::vector<Option>        options,
                    std::function<void(int)>   onSelected,
                    int                        selectedIndex);

    void show();
    void hide();

    // Optional callback fired right after the drawer fully hides.
    // Useful for restoring sibling component state (e.g. re-showing a hidden OpenGL component).
    std::function<void()> onHide;

    void paint     (juce::Graphics&) override;
    void resized   () override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

private:
    static constexpr int   kHeaderH  = 48;
    static constexpr int   kCardH    = 90;
    static constexpr int   kDiagramW = 100;
    static constexpr int   kCardPad  =   8;
    static constexpr float kAnimStep = 0.10f;   // ~10 frames -> ~160 ms at 60 fps

    juce::String             title_;
    std::vector<Option>      options_;
    std::function<void(int)> onSelected_;
    int                      selectedIdx_ = 0;
    int                      hoverIdx_    = -1;

    // Animation state
    float animProgress_ = 1.0f;   // 0 = fully closed, 1 = fully open
    bool  closing_      = false;

    void timerCallback() override;
    void applyAnimTransform();

    // Returns the local rect for option card i.
    juce::Rectangle<int> cardBounds (int i) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PickerDrawer)
};

} // namespace manifold::ui

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

// Forward-declare to avoid pulling the full preset header into every UI consumer.
namespace manifold::preset { class PresetManager; }

namespace manifold::ui
{

// Left-side preset browse panel.
//
// Shows all factory (and, in Phase 8b.2, user) presets with category tabs for
// filtering. Selecting a row loads the preset and closes the panel.
// Slides in from the left edge using the same smoothstep animation as PickerDrawer.
//
// Phase 8b.1 scope: browse + load only. Save button / user-preset write lands in 8b.2.
class PresetPanel : public juce::Component,
                    private juce::Timer
{
public:
    explicit PresetPanel (manifold::preset::PresetManager& pm);

    void show();
    void hide();

    // Fired after the panel fully hides (used to restore portrait bounds).
    std::function<void()> onHide;

    // Stub for Phase 8b.2 save flow — connected when the save button is added.
    std::function<void()> onSaveRequested;

    void paint          (juce::Graphics&) override;
    void resized        () override;
    void mouseDown      (const juce::MouseEvent&) override;
    void mouseDrag      (const juce::MouseEvent&) override;
    void mouseUp        (const juce::MouseEvent&) override;
    void mouseMove      (const juce::MouseEvent&) override;
    void mouseExit      (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    static constexpr int   kHeaderH    = 48;   // panel header height
    static constexpr int   kTabH       = 34;   // category-tab strip height
    static constexpr int   kRowH       = 58;   // preset list row height
    static constexpr int   kScrollW    =  5;   // painted scroll-thumb width
    static constexpr int   kScrollHitW = 16;   // scroll hit-zone (wider = easier to grab)
    static constexpr float kAnimStep   = 0.10f; // ~10 frames -> ~160 ms at 60 fps

    manifold::preset::PresetManager& pm_;

    int  selectedCat_  = 0;    // index into categories() — 0 = "All"
    int  hoverRowIdx_  = -1;   // index into current filteredIndices()
    int  hoverTabIdx_  = -1;

    // Scroll state
    int  scrollOffset_         = 0;
    bool isDraggingThumb_      = false;
    int  thumbDragStartY_      = 0;
    int  thumbDragStartOffset_ = 0;

    // Animation state
    float animProgress_ = 1.0f;
    bool  closing_      = false;

    // ── Data helpers ─────────────────────────────────────────────────────────
    static const juce::StringArray& categories();

    // Returns the subset of preset indices (into PresetManager) that match the
    // currently selected category tab. Recomputed fresh each call — cheap for
    // the current small factory library; will be cached in 8b.2 once user presets
    // can make the list longer.
    std::vector<int> filteredIndices() const;

    int listAreaTop()  const noexcept { return kHeaderH + kTabH; }
    int listAreaH()    const noexcept { return juce::jmax (0, getHeight() - listAreaTop()); }
    int contentH()     const          { return (int) filteredIndices().size() * kRowH; }
    int maxScroll()    const          { return juce::jmax (0, contentH() - listAreaH()); }

    juce::Rectangle<int> rowBounds (int filteredIdx) const noexcept;
    juce::Rectangle<int> tabBounds (int catIdx)      const noexcept;

    static juce::Colour engineColour (const juce::String& engine) noexcept;

    void timerCallback()    override;
    void applyAnimTransform();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetPanel)
};

} // namespace manifold::ui

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

    // startInSaveMode=true opens directly into the save form rather than browse.
    void show (bool startInSaveMode = false);
    void hide();

    // Transition to save mode while the panel is already visible (e.g. save button
    // clicked while browse is open).
    void enterSaveMode();

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
    static constexpr int   kHeaderH    = 48;
    static constexpr int   kTabH       = 34;
    static constexpr int   kRowH       = 58;
    static constexpr int   kScrollW    =  5;
    static constexpr int   kScrollHitW = 16;
    static constexpr float kAnimStep   = 0.10f;

    // ── Panel state ────────────────────────────────────────────────────────
    enum class State { Browse, SaveForm, Collision };
    State state_ = State::Browse;

    manifold::preset::PresetManager& pm_;

    // Browse state
    int  selectedCat_  = 0;
    int  hoverRowIdx_  = -1;
    int  hoverTabIdx_  = -1;
    int  scrollOffset_         = 0;
    bool isDraggingThumb_      = false;
    int  thumbDragStartY_      = 0;
    int  thumbDragStartOffset_ = 0;

    // Save form state
    int          saveCatIdx_     = 0;   // index into saveCategories()
    juce::String collisionName_;

    // ── Child components (save form) ───────────────────────────────────────
    juce::TextEditor nameInput_;
    juce::TextButton saveConfirmBtn { "SAVE" };
    juce::TextButton cancelBtn      { "CANCEL" };
    juce::TextButton overwriteBtn   { "OVERWRITE" };
    juce::TextButton renameBtn      { "RENAME" };

    // Animation state
    float animProgress_ = 1.0f;
    bool  closing_      = false;

    // ── Data helpers ─────────────────────────────────────────────────────────
    static const juce::StringArray& categories();
    // Categories available for saving (all except "All").
    static const juce::StringArray& saveCategories();

    std::vector<int> filteredIndices() const;

    int listAreaTop()  const noexcept { return kHeaderH + kTabH; }
    int listAreaH()    const noexcept { return juce::jmax (0, getHeight() - listAreaTop()); }
    int contentH()     const          { return (int) filteredIndices().size() * kRowH; }
    int maxScroll()    const          { return juce::jmax (0, contentH() - listAreaH()); }

    juce::Rectangle<int> rowBounds       (int filteredIdx) const noexcept;
    juce::Rectangle<int> tabBounds       (int catIdx)      const noexcept;
    juce::Rectangle<int> savePillBounds  (int catIdx)      const noexcept;

    static juce::Colour engineColour (const juce::String& engine) noexcept;

    // ── State transitions ─────────────────────────────────────────────────
    void enterBrowseMode();
    // enterSaveMode() is public (panel can be open when save btn is clicked).
    void enterCollisionMode (const juce::String& name);
    void attemptSave();
    void commitSave (bool overwrite);

    // ── Paint helpers ─────────────────────────────────────────────────────
    void paintBrowse    (juce::Graphics&) const;
    void paintSaveForm  (juce::Graphics&) const;
    void paintCollision (juce::Graphics&) const;
    void paintHeader    (juce::Graphics&, const juce::String& title) const;

    void timerCallback()    override;
    void applyAnimTransform();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetPanel)
};

} // namespace manifold::ui

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

    // Public lookup helpers — used by file-scope utilities in PresetPanel.cpp.
    // (Static state, no instance access; safe to expose.)
    static const juce::StringArray& categories();
    static const juce::StringArray& saveCategories();

private:
    static constexpr int   kHeaderH    = 48;
    static constexpr int   kTabH       = 34;
    static constexpr int   kRowH       = 58;
    static constexpr int   kDividerH   = 22;     // "★ FAVORITES" / "ALL PRESETS" section labels
    static constexpr int   kBannerH    = 38;     // Filter banner shown when ★ tab is active
    static constexpr int   kStarW      = 22;     // Star hit zone (left edge of row)
    static constexpr int   kTrashW     = 26;     // Trash hit zone (right side of row, before USER badge)
    static constexpr int   kScrollW    =  5;
    static constexpr int   kScrollHitW = 16;
    static constexpr float kAnimStep   = 0.10f;

    // ── Panel state ────────────────────────────────────────────────────────
    enum class State { Browse, SaveForm, Collision };
    State state_ = State::Browse;

    manifold::preset::PresetManager& pm_;

    // Browse state
    // Category indices: 0=All, 1=★ (favorites filter), 2..N=specific category.
    int  selectedCat_  = 0;
    int  hoverTabIdx_  = -1;
    // Preset-manager indices (-1 = none) for sub-row hover affordances.
    int  hoverRowPmIdx_  = -1;   // whole-row hover (controls trash visibility)
    int  hoverFavPmIdx_  = -1;   // star icon hovered (controls scale/glow)
    int  hoverDelPmIdx_  = -1;   // trash icon hovered (controls red tint)
    // pm index of the row currently in delete-confirm state (-1 = none).
    int  confirmingDelPmIdx_ = -1;
    int  hoverConfirmBtn_    = 0; // 0=none, 1=cancel, 2=delete (within confirming row)

    int  scrollOffset_         = 0;
    bool isDraggingThumb_      = false;
    int  thumbDragStartY_      = 0;
    int  thumbDragStartOffset_ = 0;

    // Save form state
    int          saveCatIdx_     = 0;   // index into saveCategories()
    juce::String collisionName_;

    // ── Child components (save form) ───────────────────────────────────────
    // Custom button class for the save form / collision actions. Matches the
    // design's two visual variants (filled accent vs. ghost outline) and adds
    // an optional checkmark prefix for primary commits.
    struct ActionButton : public juce::Button
    {
        enum class Style { Filled, Ghost };
        ActionButton (const juce::String& label, Style s, juce::Colour accent,
                      bool checkmarkPrefix = false);
        void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
        juce::String label_;
        Style        style_;
        juce::Colour accent_;
        bool         checkmark_;
    };

    juce::TextEditor nameInput_;
    ActionButton saveConfirmBtn;
    ActionButton cancelBtn;
    ActionButton overwriteBtn;
    ActionButton renameBtn;

    // Animation state
    float animProgress_ = 1.0f;
    bool  closing_      = false;

    // ── Data helpers ─────────────────────────────────────────────────────────

public:
    // Variable-height items in the scrollable list.
    // Public so file-scope hit-test helpers in PresetPanel.cpp can use it.
    struct LayoutItem
    {
        enum class Kind { FavoritesDivider, AllPresetsDivider, FilterBanner, Preset };
        Kind kind;
        int  presetIdx = -1;   // valid when kind == Preset
        int  yTop      = 0;    // y offset relative to listAreaTop() (pre-scroll)
        int  height    = 0;
    };

private:
    std::vector<LayoutItem> computeLayout() const;

    int listAreaTop()  const noexcept { return kHeaderH + kTabH; }
    int listAreaH()    const noexcept { return juce::jmax (0, getHeight() - listAreaTop()); }
    int contentH()     const;
    int maxScroll()    const          { return juce::jmax (0, contentH() - listAreaH()); }

    // Whole-row bounds in panel coords (post-scroll).
    juce::Rectangle<int> rowBoundsForLayoutItem (const LayoutItem& it) const noexcept;
    // Sub-row hit zones (panel coords); pass the whole-row bounds.
    juce::Rectangle<int> starBounds       (juce::Rectangle<int> row) const noexcept;
    juce::Rectangle<int> trashBounds      (juce::Rectangle<int> row) const noexcept;
    juce::Rectangle<int> confirmCancelBounds (juce::Rectangle<int> row) const noexcept;
    juce::Rectangle<int> confirmDeleteBounds (juce::Rectangle<int> row) const noexcept;

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
    // paintBrowse is non-const because it clamps scrollOffset_ if content shrank
    // (e.g. after a delete) — keeps the thumb from running off the end.
    void paintBrowse    (juce::Graphics&);
    void paintSaveForm  (juce::Graphics&) const;
    void paintCollision (juce::Graphics&) const;
    void paintHeader    (juce::Graphics&, const juce::String& title) const;

    // Browse-list painters
    void paintPresetRow     (juce::Graphics&, const LayoutItem&, int currentIdx) const;
    void paintConfirmStrip  (juce::Graphics&, juce::Rectangle<int> rowB,
                             const juce::String& presetName) const;
    void paintStar          (juce::Graphics&, juce::Rectangle<int> bounds,
                             bool isFavorite, bool hovered) const;
    void paintTrashIcon     (juce::Graphics&, juce::Rectangle<int> bounds,
                             bool isFactory, bool rowHovered, bool iconHovered) const;
    void paintSectionDivider(juce::Graphics&, juce::Rectangle<int> bounds,
                             const juce::String& label, bool isFavoritesStyle) const;
    void paintFilterBanner  (juce::Graphics&, juce::Rectangle<int> bounds,
                             int favCount) const;
    void paintTabStrip      (juce::Graphics&) const;

    void timerCallback()    override;
    void applyAnimTransform();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetPanel)
};

} // namespace manifold::ui

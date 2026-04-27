#include "PresetPanel.h"
#include "ManifoldLookAndFeel.h"
#include "../preset/PresetManager.h"

namespace manifold::ui
{

static constexpr juce::uint32 kAccentArgb  = 0xffb59cff;
static constexpr juce::uint32 kAmberArgb   = 0xffffb870;   // Chua/warning hue

// ─────────────────────────────────────────────────────────────────────────────
// ActionButton — primary/secondary save-form button (matches design CSS:
// .btn-save filled accent w/ glow + checkmark, .btn-cancel ghost outline).
// ─────────────────────────────────────────────────────────────────────────────
PresetPanel::ActionButton::ActionButton (const juce::String& label, Style s,
                                          juce::Colour accent, bool checkmark)
    : juce::Button (label),
      label_ (label), style_ (s), accent_ (accent), checkmark_ (checkmark)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void PresetPanel::ActionButton::paintButton (juce::Graphics& g,
                                              bool isMouseOver, bool isButtonDown)
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    juce::ignoreUnused (isButtonDown);

    const auto bounds = getLocalBounds().toFloat();

    if (style_ == Style::Filled)
    {
        // Soft glow (fake-blur via concentric expanded rounds at low alpha).
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (accent_.withAlpha (0.06f));
            g.fillRoundedRectangle (bounds.expanded ((float) i), 5.0f + (float) i);
        }
        // Solid fill + subtle border.
        const auto fill = isMouseOver ? accent_.brighter (0.12f) : accent_;
        g.setColour (fill);
        g.fillRoundedRectangle (bounds, 5.0f);
        g.setColour (accent_.brighter (0.20f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);
        // Near-black text against the saturated accent.
        g.setColour (juce::Colour (0xff0e0e18));
    }
    else   // Ghost
    {
        const float borderA = isMouseOver ? 0.85f : 0.45f;
        g.setColour (accent_.withAlpha (borderA));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);
        if (isMouseOver)
        {
            g.setColour (accent_.withAlpha (0.06f));
            g.fillRoundedRectangle (bounds, 5.0f);
        }
        g.setColour (isMouseOver ? accent_.brighter (0.20f) : accent_);
        juce::ignoreUnused (LF::ink2);
    }

    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));

    // Optional checkmark prefix (filled variants only).
    int textL = (int) bounds.getX();
    if (checkmark_)
    {
        const float cy = bounds.getCentreY();
        const float cx = bounds.getX() + 14.0f;
        juce::Path check;
        check.startNewSubPath (cx - 4.0f, cy);
        check.lineTo          (cx - 1.0f, cy + 3.0f);
        check.lineTo          (cx + 4.0f, cy - 3.0f);
        g.strokePath (check, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
        textL = (int) cx + 8;
    }

    g.drawFittedText (label_,
                      juce::Rectangle<int> (textL, 0,
                                            (int) bounds.getRight() - textL,
                                            getHeight()),
                      juce::Justification::centred, 1);
}

static inline float smoothStep (float t) noexcept
{
    return t * t * (3.0f - 2.0f * t);
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────
PresetPanel::PresetPanel (manifold::preset::PresetManager& pm)
    : pm_ (pm),
      // Save form: filled-violet primary + ghost cancel.
      saveConfirmBtn ("SAVE PRESET", ActionButton::Style::Filled,
                      juce::Colour (kAccentArgb), /*checkmark*/ true),
      cancelBtn      ("CANCEL",      ActionButton::Style::Ghost,
                      juce::Colour (0xff7a8088)),   // neutral dim slate
      // Collision: filled-amber overwrite + ghost rename.
      overwriteBtn   ("OVERWRITE",   ActionButton::Style::Filled,
                      juce::Colour (kAmberArgb)),
      renameBtn      ("RENAME",      ActionButton::Style::Ghost,
                      juce::Colour (kAmberArgb))
{
    using LF = manifold::ui::ManifoldLookAndFeel;

    setInterceptsMouseClicks (true, true);
    setVisible (false);

    // ── TextEditor (name input in save form) ─────────────────────────────────
    nameInput_.setMultiLine (false);
    nameInput_.setReturnKeyStartsNewLine (false);
    nameInput_.setScrollbarsShown (false);
    nameInput_.setPopupMenuEnabled (false);
    nameInput_.setFont (juce::FontOptions (14.0f));
    nameInput_.setColour (juce::TextEditor::backgroundColourId,     LF::plate0());
    nameInput_.setColour (juce::TextEditor::textColourId,           LF::ink1());
    nameInput_.setColour (juce::TextEditor::highlightColourId,      juce::Colour (kAccentArgb).withAlpha (0.30f));
    nameInput_.setColour (juce::TextEditor::highlightedTextColourId, LF::ink1());
    nameInput_.setColour (juce::TextEditor::outlineColourId,        LF::plateLine());
    nameInput_.setColour (juce::TextEditor::focusedOutlineColourId, juce::Colour (kAccentArgb));
    nameInput_.setVisible (false);
    addAndMakeVisible (nameInput_);

    // ── Action buttons (start hidden — visibility flipped by enter*Mode). ──
    for (auto* b : { &saveConfirmBtn, &cancelBtn, &overwriteBtn, &renameBtn })
    {
        b->setVisible (false);
        addAndMakeVisible (*b);
    }

    // Button callbacks.
    saveConfirmBtn.onClick = [this] { attemptSave(); };
    cancelBtn.onClick      = [this] { enterBrowseMode(); };
    overwriteBtn.onClick   = [this] { commitSave (true); };
    renameBtn.onClick      = [this] { enterSaveMode(); };
}

// ─────────────────────────────────────────────────────────────────────────────
// Categories
// ─────────────────────────────────────────────────────────────────────────────
// Category indices:
//   0 = All (everything; favorites sort to top with section dividers)
//   1 = ★  (favorites filter — across all categories, with a count banner)
//   2..7 = the six sound territories
const juce::StringArray& PresetPanel::categories()
{
    static const juce::StringArray kCats {
        "All", juce::CharPointer_UTF8 ("\xe2\x98\x85"),   // ★
        "Wobble", "Growl", "Drone", "Metal", "Glitch", "Alien"
    };
    return kCats;
}

const juce::StringArray& PresetPanel::saveCategories()
{
    static const juce::StringArray kSave {
        "Wobble", "Growl", "Drone", "Metal", "Glitch", "Alien"
    };
    return kSave;
}

// Predicates derived from selectedCat_.
static bool isAllCat (int catIdx)         { return catIdx == 0; }
static bool isFavCat (int catIdx)         { return catIdx == 1; }
static juce::String catName (int catIdx)
{
    return PresetPanel::saveCategories()[juce::jlimit (0, PresetPanel::saveCategories().size() - 1, catIdx - 2)];
}

// Build the variable-height item list for the current view + filter.
//
//   All view:    [★ FAVORITES divider, fav rows..., ALL PRESETS divider, non-fav rows...]
//                (dividers omitted when the corresponding section would be empty)
//   ★ view:      [filter banner, fav rows across all categories...]
//   Specific:    [rows in that category only — no fav-priority sort, no dividers]
std::vector<PresetPanel::LayoutItem> PresetPanel::computeLayout() const
{
    std::vector<LayoutItem> out;
    int y = 0;

    auto append = [&] (LayoutItem::Kind k, int presetIdx, int h)
    {
        LayoutItem it; it.kind = k; it.presetIdx = presetIdx; it.yTop = y; it.height = h;
        out.push_back (it);
        y += h;
    };

    if (isFavCat (selectedCat_))
    {
        // ★ — favorites only, with banner on top.
        const int favCount = pm_.getFavoriteCount();
        append (LayoutItem::Kind::FilterBanner, -1, kBannerH);
        for (int i = 0; i < pm_.getCount(); ++i)
            if (pm_.isFavorite (i))
                append (LayoutItem::Kind::Preset, i, kRowH);
        juce::ignoreUnused (favCount);
        return out;
    }

    if (isAllCat (selectedCat_))
    {
        // All — fav-first sort with section dividers.
        std::vector<int> favs, rest;
        for (int i = 0; i < pm_.getCount(); ++i)
            (pm_.isFavorite (i) ? favs : rest).push_back (i);

        if (! favs.empty())
        {
            append (LayoutItem::Kind::FavoritesDivider, -1, kDividerH);
            for (int pi : favs) append (LayoutItem::Kind::Preset, pi, kRowH);
        }
        if (! rest.empty())
        {
            // Only show "ALL PRESETS" divider when favorites preceded it (otherwise redundant).
            if (! favs.empty())
                append (LayoutItem::Kind::AllPresetsDivider, -1, kDividerH);
            for (int pi : rest) append (LayoutItem::Kind::Preset, pi, kRowH);
        }
        return out;
    }

    // Specific category — flat list, original order.
    const auto wantCat = catName (selectedCat_);
    for (int i = 0; i < pm_.getCount(); ++i)
        if (pm_.getCategory (i) == wantCat)
            append (LayoutItem::Kind::Preset, i, kRowH);

    return out;
}

int PresetPanel::contentH() const
{
    const auto items = computeLayout();
    if (items.empty()) return 0;
    return items.back().yTop + items.back().height;
}

juce::Colour PresetPanel::engineColour (const juce::String& engine) noexcept
{
    if (engine == "Lorenz")  return juce::Colour { 0xffb59cff };
    if (engine == "Thomas")  return juce::Colour { 0xff7ad6ff };
    if (engine == "Rossler") return juce::Colour { 0xff8effa0 };
    if (engine == "Chua")    return juce::Colour { 0xffffb870 };
    if (engine == "Aizawa")  return juce::Colour { 0xff8aa8ff };
    if (engine == "Henon")   return juce::Colour { 0xffff8eb6 };
    return juce::Colour { 0xffb59cff };
}

// ─────────────────────────────────────────────────────────────────────────────
// Geometry
// ─────────────────────────────────────────────────────────────────────────────
juce::Rectangle<int> PresetPanel::rowBoundsForLayoutItem (const LayoutItem& it) const noexcept
{
    return { 0, listAreaTop() + it.yTop - scrollOffset_, getWidth(), it.height };
}

juce::Rectangle<int> PresetPanel::starBounds (juce::Rectangle<int> row) const noexcept
{
    // Left edge: small button strip flanking the engine dot.
    return { row.getX() + 4, row.getCentreY() - kStarW / 2, kStarW, kStarW };
}

juce::Rectangle<int> PresetPanel::trashBounds (juce::Rectangle<int> row) const noexcept
{
    // Right side: just inside the scroll-thumb gutter, before the USER badge.
    // USER badge (when present) sits at row.getRight() - 44 ≈ 10px right margin + badge.
    // Place trash to the left of where the badge starts.
    const int rightGutter = kScrollHitW + 6;        // keep clear of the scroll grab zone
    const int badgeShift  = 50;                     // reserve room when USER badge is shown
    const int x           = row.getRight() - rightGutter - badgeShift - kTrashW;
    return { x, row.getCentreY() - kTrashW / 2, kTrashW, kTrashW };
}

juce::Rectangle<int> PresetPanel::confirmDeleteBounds (juce::Rectangle<int> row) const noexcept
{
    // [Delete] — red filled button on the right (mirrors collision overwrite styling).
    constexpr int w = 70, h = 26;
    return { row.getRight() - 14 - w,
             row.getCentreY() - h / 2, w, h };
}

juce::Rectangle<int> PresetPanel::confirmCancelBounds (juce::Rectangle<int> row) const noexcept
{
    // [Cancel] — ghost button to the left of [Delete].
    constexpr int w = 60, h = 26;
    const auto del = confirmDeleteBounds (row);
    return { del.getX() - 8 - w,
             row.getCentreY() - h / 2, w, h };
}

juce::Rectangle<int> PresetPanel::tabBounds (int catIdx) const noexcept
{
    const int n = categories().size();
    const int w = getWidth() / n;
    return { catIdx * w, kHeaderH, w, kTabH };
}

juce::Rectangle<int> PresetPanel::savePillBounds (int catIdx) const noexcept
{
    // Six save-category pills arranged in 2 rows of 3.
    const int col     = catIdx % 3;
    const int row     = catIdx / 3;
    const int margin  = 12;
    const int gap     = 6;
    const int pillW   = (getWidth() - 2 * margin - 2 * gap) / 3;
    const int pillH   = 28;
    const int rowGap  = 6;
    const int originY = kHeaderH + 120;   // below "Category" label in save form

    return { margin + col * (pillW + gap),
             originY + row * (pillH + rowGap),
             pillW, pillH };
}

// ─────────────────────────────────────────────────────────────────────────────
// Show / Hide
// ─────────────────────────────────────────────────────────────────────────────
void PresetPanel::show (bool startInSaveMode)
{
    closing_      = false;
    animProgress_ = 0.0f;

    if (startInSaveMode)
        enterSaveMode();
    else
        enterBrowseMode();

    setAlpha (0.0f);
    setTransform (juce::AffineTransform::translation (-(float) getWidth(), 0.0f));
    setVisible (true);
    toFront (false);
    startTimer (16);
}

void PresetPanel::hide()
{
    if (closing_) return;
    closing_ = true;
    if (! isTimerRunning())
        startTimer (16);
}

// ─────────────────────────────────────────────────────────────────────────────
// Animation
// ─────────────────────────────────────────────────────────────────────────────
void PresetPanel::timerCallback()
{
    animProgress_ += closing_ ? -kAnimStep : kAnimStep;
    animProgress_  = juce::jlimit (0.0f, 1.0f, animProgress_);
    applyAnimTransform();

    if (closing_ && animProgress_ <= 0.0f)
    {
        stopTimer();
        setVisible (false);
        setAlpha (1.0f);
        setTransform ({});
        if (onHide) onHide();
    }
    else if (! closing_ && animProgress_ >= 1.0f)
    {
        stopTimer();
    }
}

void PresetPanel::applyAnimTransform()
{
    const float t      = smoothStep (animProgress_);
    const float slideX = -(float) getWidth() * (1.0f - t);
    setAlpha (t);
    setTransform (juce::AffineTransform::translation (slideX, 0.0f));
}

// ─────────────────────────────────────────────────────────────────────────────
// State transitions
// ─────────────────────────────────────────────────────────────────────────────
void PresetPanel::enterBrowseMode()
{
    state_ = State::Browse;
    scrollOffset_ = 0;
    hoverTabIdx_       = -1;
    hoverRowPmIdx_     = -1;
    hoverFavPmIdx_     = -1;
    hoverDelPmIdx_     = -1;
    confirmingDelPmIdx_ = -1;
    hoverConfirmBtn_   = 0;
    nameInput_.setVisible (false);
    saveConfirmBtn.setVisible (false);
    cancelBtn.setVisible (false);
    overwriteBtn.setVisible (false);
    renameBtn.setVisible (false);
    repaint();
}

void PresetPanel::enterSaveMode()
{
    state_      = State::SaveForm;
    saveCatIdx_ = 0;   // default to "Wobble"

    // Pre-fill name only when re-editing a user preset; otherwise start blank.
    const bool isUser = pm_.isUserPreset (pm_.getCurrentIndex());
    nameInput_.setText (isUser ? pm_.getName (pm_.getCurrentIndex()) : juce::String{}, false);
    if (isUser) nameInput_.selectAll();

    nameInput_.setVisible (true);
    saveConfirmBtn.setVisible (true);
    cancelBtn.setVisible (true);
    overwriteBtn.setVisible (false);
    renameBtn.setVisible (false);

    // Defer focus so the component tree is fully visible first.
    juce::MessageManager::callAsync ([this] { nameInput_.grabKeyboardFocus(); });
    repaint();
}

void PresetPanel::enterCollisionMode (const juce::String& name)
{
    state_         = State::Collision;
    collisionName_ = name;
    nameInput_.setVisible (false);
    saveConfirmBtn.setVisible (false);
    cancelBtn.setVisible (false);
    overwriteBtn.setVisible (true);
    renameBtn.setVisible (true);
    repaint();
}

void PresetPanel::attemptSave()
{
    const auto name = nameInput_.getText().trim();
    if (name.isEmpty()) return;   // silently block empty names

    if (pm_.userPresetNameExists (name))
        enterCollisionMode (name);
    else
        commitSave (false);
}

void PresetPanel::commitSave (bool overwrite)
{
    const auto name     = collisionName_.isNotEmpty() ? collisionName_
                                                      : nameInput_.getText().trim();
    const auto category = saveCategories()[saveCatIdx_];
    collisionName_ = {};

    pm_.saveUserPreset (name, category, overwrite);

    // Switch back to browse (current preset is now the newly saved one).
    enterBrowseMode();
}

// ─────────────────────────────────────────────────────────────────────────────
// Layout
// ─────────────────────────────────────────────────────────────────────────────
void PresetPanel::resized()
{
    // ── Save form child components ────────────────────────────────────────────
    // TextEditor sits below the "Name" label in save form.
    const int inputY = kHeaderH + 52;
    nameInput_.setBounds (12, inputY, getWidth() - 24, 36);

    // Action button pair for save form (CANCEL | SAVE PRESET).
    // SAVE is wider to accommodate "SAVE PRESET" + checkmark prefix.
    const int sfBtnY    = kHeaderH + 240;
    const int sfBtnH    = 30;
    const int sfCancelW = 84;
    const int sfSaveW   = 124;
    const int sfGap     = 8;
    const int rMargin   = 14;
    saveConfirmBtn.setBounds (getWidth() - rMargin - sfSaveW,
                              sfBtnY, sfSaveW, sfBtnH);
    cancelBtn.setBounds      (getWidth() - rMargin - sfSaveW - sfGap - sfCancelW,
                              sfBtnY, sfCancelW, sfBtnH);

    // Action buttons for collision state (RENAME | OVERWRITE).
    const int colBtnY  = kHeaderH + 120;
    const int colBtnH  = 30;
    const int colOvrW  = 104;
    const int colRenW  = 92;
    overwriteBtn.setBounds (getWidth() - rMargin - colOvrW,
                            colBtnY, colOvrW, colBtnH);
    renameBtn.setBounds    (getWidth() - rMargin - colOvrW - sfGap - colRenW,
                            colBtnY, colRenW, colBtnH);
}

// ─────────────────────────────────────────────────────────────────────────────
// Paint — dispatches to per-state helpers
// ─────────────────────────────────────────────────────────────────────────────
void PresetPanel::paint (juce::Graphics& g)
{
    using LF = manifold::ui::ManifoldLookAndFeel;

    // Background.
    g.setColour (LF::plate2());
    g.fillRect (getLocalBounds().toFloat());

    switch (state_)
    {
        case State::Browse:
            paintHeader (g, "PRESETS");
            paintBrowse (g);
            break;
        case State::SaveForm:
            paintHeader (g, "SAVE PRESET");
            paintSaveForm (g);
            break;
        case State::Collision:
            paintHeader (g, "SAVE PRESET");
            paintCollision (g);
            break;
    }

    // Right-edge drop shadow (casts onto content to the right of the panel).
    {
        const auto b = getLocalBounds().toFloat();
        juce::ColourGradient sh (juce::Colours::transparentBlack, b.getRight() - 18.0f, 0.0f,
                                 juce::Colours::black.withAlpha (0.42f), b.getRight(), 0.0f,
                                 false);
        g.setGradientFill (sh);
        g.fillRect (b.withLeft (b.getRight() - 18.0f));
    }
}

// ─── Header ──────────────────────────────────────────────────────────────────
void PresetPanel::paintHeader (juce::Graphics& g, const juce::String& title) const
{
    using LF = manifold::ui::ManifoldLookAndFeel;

    const auto hdr = juce::Rectangle<float> (0.0f, 0.0f, (float) getWidth(), (float) kHeaderH);
    juce::ColourGradient hg (LF::plate3(), hdr.getX(), hdr.getY(),
                              LF::plate2(), hdr.getX(), hdr.getBottom(), false);
    g.setGradientFill (hg);
    g.fillRect (hdr);

    g.setColour (LF::plateLine());
    g.drawHorizontalLine (kHeaderH - 1, 0.0f, (float) getWidth());

    g.setColour (LF::ink2());
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawFittedText (title, 18, 0, getWidth() - 62, kHeaderH,
                      juce::Justification::centredLeft, 1);

    // Close X.
    const float bx = (float) getWidth() - 22.0f;
    const float by = (float) kHeaderH * 0.5f;
    g.setColour (LF::ink3());
    juce::Path x;
    x.startNewSubPath (bx - 6.0f, by - 6.0f); x.lineTo (bx + 6.0f, by + 6.0f);
    x.startNewSubPath (bx + 6.0f, by - 6.0f); x.lineTo (bx - 6.0f, by + 6.0f);
    g.strokePath (x, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));
}

// ─── Browse ──────────────────────────────────────────────────────────────────
void PresetPanel::paintBrowse (juce::Graphics& g)
{
    using LF = manifold::ui::ManifoldLookAndFeel;

    // Clamp scroll if content shrank (delete, category change, etc.).
    scrollOffset_ = juce::jlimit (0, maxScroll(), scrollOffset_);

    paintTabStrip (g);

    // Preset list (clipped to scrollable area).
    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (0, listAreaTop(), getWidth(), listAreaH());

        const auto items     = computeLayout();
        const int  currentIdx = pm_.getCurrentIndex();

        if (items.empty())
        {
            g.setColour (LF::ink4());
            g.setFont (juce::FontOptions (11.0f));
            g.drawFittedText ("No presets in this category.",
                              12, listAreaTop() + 20, getWidth() - 24, 24,
                              juce::Justification::centredLeft, 1);
        }

        for (const auto& it : items)
        {
            const auto rect = rowBoundsForLayoutItem (it);
            if (rect.getBottom() < listAreaTop() || rect.getY() > getHeight())
                continue;

            switch (it.kind)
            {
                case LayoutItem::Kind::FavoritesDivider:
                    paintSectionDivider (g, rect, "FAVORITES", true);
                    break;
                case LayoutItem::Kind::AllPresetsDivider:
                    paintSectionDivider (g, rect, "ALL PRESETS", false);
                    break;
                case LayoutItem::Kind::FilterBanner:
                    paintFilterBanner (g, rect, pm_.getFavoriteCount());
                    break;
                case LayoutItem::Kind::Preset:
                    paintPresetRow (g, it, currentIdx);
                    break;
            }
        }
    }

    // Scroll thumb.
    const int ms = maxScroll();
    if (ms > 0)
    {
        const int   availH = listAreaH();
        const float thumbH = juce::jmax (24.0f, (float) availH * (float) availH / (float) contentH());
        const float thumbY = (float) listAreaTop()
                             + (float) scrollOffset_ / (float) ms * ((float) availH - thumbH);
        const float thumbX = (float) getWidth() - (float) kScrollW - 2.0f;
        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillRoundedRectangle (thumbX, (float) listAreaTop(), (float) kScrollW, (float) availH, 2.0f);
        g.setColour (juce::Colour (kAccentArgb).withAlpha (0.35f));
        g.fillRoundedRectangle (thumbX, thumbY, (float) kScrollW, thumbH, 2.0f);
    }
}

// Tab strip: same visual vocab as before, with a custom-painted star glyph
// for the favorites filter tab (drawing the U+2605 Unicode codepoint via
// drawFittedText ran into font fallback issues, so we use a juce::Path).
void PresetPanel::paintTabStrip (juce::Graphics& g) const
{
    using LF = manifold::ui::ManifoldLookAndFeel;

    const auto tabArea = juce::Rectangle<int> (0, kHeaderH, getWidth(), kTabH);
    g.setColour (LF::plate0());
    g.fillRect (tabArea);
    g.setColour (LF::plateLine());
    g.drawHorizontalLine (kHeaderH + kTabH - 1, 0.0f, (float) getWidth());

    const auto& cats = categories();
    constexpr juce::uint32 kStarArgb = 0xfff5c95f;
    const auto starColour = juce::Colour (kStarArgb);

    for (int i = 0; i < cats.size(); ++i)
    {
        const auto tb      = tabBounds (i).toFloat();
        const bool active  = (i == selectedCat_);
        const bool hov     = (i == hoverTabIdx_) && ! active;
        const bool starTab = isFavCat (i);

        // Pill background.
        if (active)
        {
            const auto pill   = tb.reduced (3.0f, 7.0f);
            const auto fill   = starTab ? starColour : juce::Colour (kAccentArgb);
            g.setColour (fill.withAlpha (0.22f));
            g.fillRoundedRectangle (pill, 4.0f);
            g.setColour (fill.withAlpha (0.55f));
            g.drawRoundedRectangle (pill.reduced (0.5f), 4.0f, 1.0f);
        }
        else if (hov)
        {
            g.setColour (LF::plate3());
            g.fillRoundedRectangle (tb.reduced (3.0f, 7.0f), 4.0f);
        }

        if (starTab)
        {
            // Custom-painted star, centered in the tab bounds.
            const float cx = tb.getCentreX();
            const float cy = tb.getCentreY();
            const float s  = active ? 1.05f : 0.95f;   // a touch bigger when selected
            auto pt = [&] (float x, float y) {
                return juce::Point<float> (cx + (x - 6.5f) * s, cy + (y - 6.5f) * s);
            };
            juce::Path star;
            star.startNewSubPath (pt (6.5f, 1.2f));
            star.lineTo (pt (8.1f, 4.6f));   star.lineTo (pt (11.7f, 5.1f));
            star.lineTo (pt (9.1f, 7.7f));   star.lineTo (pt (9.7f, 11.3f));
            star.lineTo (pt (6.5f, 9.6f));   star.lineTo (pt (3.3f, 11.3f));
            star.lineTo (pt (3.9f, 7.7f));   star.lineTo (pt (1.3f, 5.1f));
            star.lineTo (pt (4.9f, 4.6f));   star.closeSubPath();

            const auto col = active ? juce::Colours::white
                                    : starColour.withAlpha (hov ? 1.0f : 0.85f);
            if (active)
            {
                // Soft glow behind the filled star.
                g.setColour (starColour.withAlpha (0.40f));
                g.fillEllipse (cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
            }
            g.setColour (col);
            g.fillPath (star);
        }
        else
        {
            g.setColour (active ? juce::Colour (kAccentArgb)
                                : (hov ? LF::ink2() : LF::ink3()));
            g.setFont (juce::FontOptions (9.0f, active ? juce::Font::bold : juce::Font::plain));
            g.drawFittedText (cats[i], tabBounds (i).reduced (2, 0),
                              juce::Justification::centred, 1);
        }
    }
}

// One preset row — handles both default state and the inline delete-confirm overlay.
void PresetPanel::paintPresetRow (juce::Graphics& g, const LayoutItem& it, int currentIdx) const
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    const int  pi      = it.presetIdx;
    const auto rowI    = rowBoundsForLayoutItem (it);
    const auto row     = rowI.toFloat();
    const bool sel     = (pi == currentIdx);
    const bool rowHov  = (pi == hoverRowPmIdx_) && ! sel;
    const bool factory = ! pm_.isUserPreset (pi);
    const bool confirm = (pi == confirmingDelPmIdx_);

    // Base background.
    if (confirm)
    {
        // Red-tinted gradient (matches the design's collision-style strip).
        constexpr juce::uint32 kDangerArgb = 0xffdc6347;
        juce::ColourGradient bg (juce::Colour (kDangerArgb).withAlpha (0.22f), row.getX(), row.getY(),
                                  juce::Colour (kDangerArgb).withAlpha (0.10f), row.getX(), row.getBottom(),
                                  false);
        g.setGradientFill (bg); g.fillRect (row);
        g.setColour (juce::Colour (kDangerArgb).withAlpha (0.55f));
        g.drawRect (row.reduced (0.5f), 1.0f);
    }
    else if (sel)
    {
        juce::ColourGradient bg (LF::plate3(), row.getX(), row.getY(),
                                  LF::plate2().brighter (0.04f), row.getX(), row.getBottom(), false);
        g.setGradientFill (bg); g.fillRect (row);
        g.setColour (juce::Colour (kAccentArgb).withAlpha (0.85f));
        g.fillRect (row.withWidth (3.0f));
        g.setColour (juce::Colour (kAccentArgb).withAlpha (0.20f));
        g.drawRect (row.reduced (0.5f), 1.0f);
    }
    else if (rowHov)
    {
        g.setColour (LF::plate2().brighter (0.10f)); g.fillRect (row);
    }
    else
    {
        g.setColour (LF::plate2()); g.fillRect (row);
    }

    g.setColour (LF::plateLine());
    g.drawHorizontalLine (rowI.getBottom() - 1,
                          row.getX() + 12.0f, row.getRight() - 12.0f);

    // Confirming state: skip the normal row content and paint just the red
    // overlay. (The design CSS dims the underlying content to 25%; in JUCE we
    // simplify by replacing it entirely — same UX effect, less paint cost.)
    if (confirm)
    {
        paintConfirmStrip (g, rowI, pm_.getName (pi));
        return;
    }

    // Star (left).
    paintStar (g, starBounds (rowI), pm_.isFavorite (pi),
               hoverFavPmIdx_ == pi);

    // Engine dot — shifted right to make room for the star.
    const float dotR  = 5.0f;
    const float dotCX = (float) starBounds (rowI).getRight() + 12.0f;
    const float dotCY = row.getCentreY();
    const auto  ecol  = engineColour (pm_.getPrimaryEngine (pi));
    g.setColour (ecol.withAlpha (0.28f));
    g.fillEllipse (dotCX - dotR * 1.7f, dotCY - dotR * 1.7f, dotR * 3.4f, dotR * 3.4f);
    g.setColour (ecol);
    g.fillEllipse (dotCX - dotR, dotCY - dotR, dotR * 2.0f, dotR * 2.0f);

    // Reserve right-side space for trash + USER badge.
    const float rightReserve = (float) (kScrollW + kTrashW + 8 + (factory ? 0 : 44));
    const float textX  = dotCX + dotR + 12.0f;
    const float textW  = (float) getWidth() - textX - rightReserve;

    g.setColour (sel ? LF::ink1() : LF::ink2());
    g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    g.drawFittedText (pm_.getName (pi), (int) textX, rowI.getY() + 10, (int) textW, 18,
                      juce::Justification::centredLeft, 1);

    g.setColour (LF::ink3());
    g.setFont (juce::FontOptions (10.0f));
    g.drawFittedText (pm_.getCategory (pi) + " - " + pm_.getPrimaryEngine (pi),
                      (int) textX, rowI.getY() + 31, (int) textW, 16,
                      juce::Justification::centredLeft, 1);

    // Trash (right of name) — visible only on row hover. Factory: 30% no pointer events.
    paintTrashIcon (g, trashBounds (rowI), factory, rowHov, hoverDelPmIdx_ == pi);

    // USER badge.
    if (! factory)
    {
        const float bw = 34.0f, bh = 16.0f;
        const float bx = row.getRight() - bw - 12.0f;
        const float by = row.getCentreY() - bh * 0.5f;
        g.setColour (juce::Colour (kAccentArgb).withAlpha (0.20f));
        g.fillRoundedRectangle (bx, by, bw, bh, 3.0f);
        g.setColour (juce::Colour (kAccentArgb).withAlpha (0.70f));
        g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
        g.drawFittedText ("USER", (int) bx, (int) by, (int) bw, (int) bh,
                          juce::Justification::centred, 1);
    }
}

// "Delete '<name>'?" red strip with [Cancel] [Delete] buttons.
void PresetPanel::paintConfirmStrip (juce::Graphics& g, juce::Rectangle<int> rowB,
                                     const juce::String& presetName) const
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    constexpr juce::uint32 kDangerArgb = 0xffdc6347;

    // Trash glyph (small) + uppercase prompt on the left.
    const int gx = rowB.getX() + 18;
    const int gy = rowB.getCentreY() - 5;
    juce::Path tp;
    tp.startNewSubPath ((float) gx,        (float) gy + 1);
    tp.lineTo          ((float) gx + 10,   (float) gy + 1);
    tp.startNewSubPath ((float) gx + 1,    (float) gy + 1);
    tp.lineTo          ((float) gx + 1.5f, (float) gy + 9);
    tp.lineTo          ((float) gx + 8.5f, (float) gy + 9);
    tp.lineTo          ((float) gx + 9,    (float) gy + 1);
    g.setColour (juce::Colour (kDangerArgb));
    g.strokePath (tp, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

    g.setColour (juce::Colour (kDangerArgb).brighter (0.30f));
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    g.drawFittedText ("DELETE",
                      gx + 18, rowB.getCentreY() - 8, 70, 16,
                      juce::Justification::centredLeft, 1);

    g.setColour (LF::ink1());
    g.setFont (juce::FontOptions (12.5f, juce::Font::plain));
    g.drawFittedText ("\"" + presetName + "\"?",
                      gx + 70, rowB.getCentreY() - 8,
                      confirmCancelBounds (rowB).getX() - (gx + 70) - 8, 16,
                      juce::Justification::centredLeft, 1);

    // [Cancel] — ghost.
    {
        const auto cb = confirmCancelBounds (rowB).toFloat();
        const bool h  = (hoverConfirmBtn_ == 1);
        g.setColour (h ? LF::ink2().withAlpha (0.10f) : juce::Colours::transparentBlack);
        g.fillRoundedRectangle (cb, 4.0f);
        g.setColour (h ? LF::ink2() : LF::ink3());
        g.drawRoundedRectangle (cb.reduced (0.5f), 4.0f, 1.0f);
        g.setColour (h ? LF::ink1() : LF::ink2());
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawFittedText ("CANCEL", confirmCancelBounds (rowB),
                          juce::Justification::centred, 1);
    }
    // [Delete] — red filled.
    {
        const auto db = confirmDeleteBounds (rowB).toFloat();
        const bool h  = (hoverConfirmBtn_ == 2);
        const auto base = juce::Colour (kDangerArgb);
        g.setColour (h ? base.brighter (0.15f) : base);
        g.fillRoundedRectangle (db, 4.0f);
        g.setColour (base.brighter (0.25f));
        g.drawRoundedRectangle (db.reduced (0.5f), 4.0f, 1.0f);
        g.setColour (juce::Colours::white);
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawFittedText ("DELETE", confirmDeleteBounds (rowB),
                          juce::Justification::centred, 1);
    }
}

// Filled gold star when favorited; outlined ghost when not. Subtle scale on hover.
void PresetPanel::paintStar (juce::Graphics& g, juce::Rectangle<int> bounds,
                             bool isFavorite, bool hovered) const
{
    constexpr juce::uint32 kStarArgb     = 0xfff5c95f;   // warm gold (filled)
    constexpr juce::uint32 kStarHoverArgb = 0xffffd47a;
    constexpr juce::uint32 kGhostArgb     = 0xff5a6068;  // dim slate

    const float scale = hovered ? 1.10f : 1.0f;
    const float cx = (float) bounds.getCentreX();
    const float cy = (float) bounds.getCentreY();
    const float s  = 6.5f * scale;   // half-size of the 13×13 design viewBox

    // 5-point star path (matches the SVG in the artboard, scaled).
    auto pt = [&] (float x, float y) {
        return juce::Point<float> (cx + (x - 6.5f) * scale, cy + (y - 6.5f) * scale);
    };
    juce::Path star;
    star.startNewSubPath (pt (6.5f, 1.2f));
    star.lineTo          (pt (8.1f, 4.6f));
    star.lineTo          (pt (11.7f, 5.1f));
    star.lineTo          (pt (9.1f, 7.7f));
    star.lineTo          (pt (9.7f, 11.3f));
    star.lineTo          (pt (6.5f, 9.6f));
    star.lineTo          (pt (3.3f, 11.3f));
    star.lineTo          (pt (3.9f, 7.7f));
    star.lineTo          (pt (1.3f, 5.1f));
    star.lineTo          (pt (4.9f, 4.6f));
    star.closeSubPath();

    if (isFavorite)
    {
        const auto col = juce::Colour (hovered ? kStarHoverArgb : kStarArgb);
        // Soft glow.
        g.setColour (col.withAlpha (0.35f));
        g.fillEllipse (cx - s, cy - s, s * 2.0f, s * 2.0f);
        g.setColour (col);
        g.fillPath (star);
    }
    else
    {
        g.setColour (juce::Colour (hovered ? kStarArgb : kGhostArgb));
        g.strokePath (star, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }
}

// Trash bin glyph drawn into bounds. Visible only when the row is hovered.
// Factory presets render at 30% opacity (visible affordance, but disabled).
void PresetPanel::paintTrashIcon (juce::Graphics& g, juce::Rectangle<int> bounds,
                                  bool isFactory, bool rowHovered, bool iconHovered) const
{
    if (! rowHovered) return;   // hidden by default

    constexpr juce::uint32 kDangerArgb = 0xffdc6347;
    using LF = manifold::ui::ManifoldLookAndFeel;

    const float a = isFactory ? 0.30f : 1.0f;
    const auto  base = isFactory ? LF::ink3()
                                 : (iconHovered ? juce::Colour (kDangerArgb) : LF::ink2());

    // Hover background (only for active user-preset trash).
    if (! isFactory && iconHovered)
    {
        g.setColour (juce::Colour (kDangerArgb).withAlpha (0.18f));
        g.fillRoundedRectangle (bounds.toFloat(), 4.0f);
        g.setColour (juce::Colour (kDangerArgb).withAlpha (0.40f));
        g.drawRoundedRectangle (bounds.toFloat().reduced (0.5f), 4.0f, 1.0f);
    }

    g.setColour (base.withAlpha (a));

    // 12×12 design viewBox, centered in bounds.
    const float cx = (float) bounds.getCentreX();
    const float cy = (float) bounds.getCentreY();
    auto p = [&] (float x, float y) {
        return juce::Point<float> (cx + (x - 6.0f), cy + (y - 6.0f));
    };
    juce::Path bin;
    // Top horizontal line
    bin.startNewSubPath (p (2.0f,  3.0f));
    bin.lineTo          (p (10.0f, 3.0f));
    // Bin body
    bin.startNewSubPath (p (3.0f,  3.0f));
    bin.lineTo          (p (3.5f,  10.0f));
    bin.lineTo          (p (8.5f,  10.0f));
    bin.lineTo          (p (9.0f,  3.0f));
    // Tick marks
    bin.startNewSubPath (p (5.0f,  5.0f));
    bin.lineTo          (p (5.0f,  8.0f));
    bin.startNewSubPath (p (7.0f,  5.0f));
    bin.lineTo          (p (7.0f,  8.0f));
    // Lid handle
    bin.startNewSubPath (p (4.5f,  3.0f));
    bin.lineTo          (p (4.5f,  1.5f));
    bin.lineTo          (p (7.5f,  1.5f));
    bin.lineTo          (p (7.5f,  3.0f));
    g.strokePath (bin, juce::PathStrokeType (1.3f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
}

// Small uppercase section label with a trailing hairline. When isFavoritesStyle
// is true, prefixes a custom-painted gold star glyph (avoids font-fallback
// issues with the U+2605 Unicode codepoint).
void PresetPanel::paintSectionDivider (juce::Graphics& g, juce::Rectangle<int> bounds,
                                       const juce::String& label, bool isFavoritesStyle) const
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    constexpr juce::uint32 kStarArgb = 0xfff5c95f;

    const auto txtRect = bounds.reduced (16, 0).withTrimmedTop (8);
    int labelStartX = txtRect.getX();

    // Optional star glyph at the start.
    if (isFavoritesStyle)
    {
        // Anchor on the text rect's centerY (not bounds.centerY) so the star
        // mid-aligns with the FAVORITES text, which sits below the trimmed-top.
        const float cx = (float) txtRect.getX() + 5.0f;
        const float cy = (float) txtRect.getCentreY();
        auto pt = [&] (float x, float y) {
            return juce::Point<float> (cx + (x - 6.5f) * 0.75f, cy + (y - 6.5f) * 0.75f);
        };
        juce::Path star;
        star.startNewSubPath (pt (6.5f, 1.2f));
        star.lineTo (pt (8.1f, 4.6f));   star.lineTo (pt (11.7f, 5.1f));
        star.lineTo (pt (9.1f, 7.7f));   star.lineTo (pt (9.7f, 11.3f));
        star.lineTo (pt (6.5f, 9.6f));   star.lineTo (pt (3.3f, 11.3f));
        star.lineTo (pt (3.9f, 7.7f));   star.lineTo (pt (1.3f, 5.1f));
        star.lineTo (pt (4.9f, 4.6f));   star.closeSubPath();
        g.setColour (juce::Colour (kStarArgb));
        g.fillPath (star);
        labelStartX += 14;   // leave room for the star
    }

    g.setColour (isFavoritesStyle ? juce::Colour (kStarArgb) : LF::ink4());
    g.setFont (juce::FontOptions (8.5f, juce::Font::bold));

    // Measure label width to anchor the trailing hairline.
    juce::GlyphArrangement ga;
    ga.addLineOfText (g.getCurrentFont(), label, 0.0f, 0.0f);
    const float labelW = ga.getBoundingBox (0, ga.getNumGlyphs(), true).getWidth() + 8.0f;

    g.drawFittedText (label,
                      { labelStartX, txtRect.getY(), txtRect.getRight() - labelStartX, txtRect.getHeight() },
                      juce::Justification::centredLeft, 1);

    // Trailing hairline that fades to transparent.
    const float lineY  = (float) bounds.getCentreY() + 4.0f;
    const float lineX0 = (float) labelStartX + labelW;
    juce::ColourGradient lg (LF::plateLine(),                  lineX0,                     lineY,
                              juce::Colours::transparentBlack, (float) bounds.getRight(),  lineY,
                              false);
    g.setGradientFill (lg);
    g.fillRect (juce::Rectangle<float> (lineX0, lineY,
                                         (float) bounds.getRight() - lineX0 - 12.0f,
                                         1.0f));
}

// Gold-tinted "Showing N favorites across all categories" pill.
void PresetPanel::paintFilterBanner (juce::Graphics& g, juce::Rectangle<int> bounds, int favCount) const
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    constexpr juce::uint32 kStarArgb = 0xfff5c95f;

    const auto pill = bounds.reduced (10, 6).toFloat();
    g.setColour (juce::Colour (kStarArgb).withAlpha (0.10f));
    g.fillRoundedRectangle (pill, 5.0f);
    g.setColour (juce::Colour (kStarArgb).withAlpha (0.30f));
    g.drawRoundedRectangle (pill.reduced (0.5f), 5.0f, 1.0f);

    // Star glyph at left.
    {
        const float cx = pill.getX() + 14.0f;
        const float cy = pill.getCentreY();
        auto pt = [&] (float x, float y) {
            return juce::Point<float> (cx + (x - 6.5f) * 0.8f, cy + (y - 6.5f) * 0.8f);
        };
        juce::Path star;
        star.startNewSubPath (pt (6.5f, 1.2f));
        star.lineTo (pt (8.1f, 4.6f));   star.lineTo (pt (11.7f, 5.1f));
        star.lineTo (pt (9.1f, 7.7f));   star.lineTo (pt (9.7f, 11.3f));
        star.lineTo (pt (6.5f, 9.6f));   star.lineTo (pt (3.3f, 11.3f));
        star.lineTo (pt (3.9f, 7.7f));   star.lineTo (pt (1.3f, 5.1f));
        star.lineTo (pt (4.9f, 4.6f));   star.closeSubPath();
        g.setColour (juce::Colour (kStarArgb));
        g.fillPath (star);
    }

    g.setColour (juce::Colour (kStarArgb).brighter (0.20f));
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    const juce::String msg = "SHOWING " + juce::String (favCount)
                              + " FAVORITE" + (favCount == 1 ? "" : "S");
    g.drawFittedText (msg, (int) pill.getX() + 26, (int) pill.getY(),
                      (int) pill.getWidth() - 26, (int) pill.getHeight(),
                      juce::Justification::centredLeft, 1);

    g.setColour (LF::ink4());
    g.setFont (juce::FontOptions (8.0f));
    g.drawFittedText ("ACROSS ALL CATEGORIES",
                      (int) pill.getX(), (int) pill.getY(),
                      (int) pill.getWidth() - 12, (int) pill.getHeight(),
                      juce::Justification::centredRight, 1);

    juce::ignoreUnused (favCount);
}

// ─── Save form ───────────────────────────────────────────────────────────────
void PresetPanel::paintSaveForm (juce::Graphics& g) const
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    const int pad = 12;

    // "Name" field label — TextEditor child is positioned by resized().
    g.setColour (LF::ink3());
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawFittedText ("NAME", pad, kHeaderH + 16, getWidth() - pad * 2, 16,
                      juce::Justification::centredLeft, 1);

    // "Category" label.
    g.drawFittedText ("CATEGORY", pad, kHeaderH + 100, getWidth() - pad * 2, 16,
                      juce::Justification::centredLeft, 1);

    // Category save-pills.
    const auto& saveCats = saveCategories();
    for (int i = 0; i < saveCats.size(); ++i)
    {
        const auto  pill   = savePillBounds (i).toFloat();
        const bool  active = (i == saveCatIdx_);

        if (active)
        {
            g.setColour (juce::Colour (kAccentArgb).withAlpha (0.22f));
            g.fillRoundedRectangle (pill, 5.0f);
            g.setColour (juce::Colour (kAccentArgb).withAlpha (0.60f));
            g.drawRoundedRectangle (pill.reduced (0.5f), 5.0f, 1.0f);
            g.setColour (juce::Colour (kAccentArgb));
        }
        else
        {
            g.setColour (LF::plate3());
            g.fillRoundedRectangle (pill, 5.0f);
            g.setColour (LF::plateLine());
            g.drawRoundedRectangle (pill.reduced (0.5f), 5.0f, 1.0f);
            g.setColour (LF::ink3());
        }

        g.setFont (juce::FontOptions (9.5f, active ? juce::Font::bold : juce::Font::plain));
        g.drawFittedText (saveCats[i], savePillBounds (i).reduced (2, 0),
                          juce::Justification::centred, 1);
    }

    // "Context" label + chips.
    const int chipY = kHeaderH + 202;
    g.setColour (LF::ink3());
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawFittedText ("CONTEXT", pad, chipY, getWidth() - pad * 2, 16,
                      juce::Justification::centredLeft, 1);

    const auto info = pm_.getCurrentStateInfo();
    const juce::String chips[] = { info.primaryEngine, info.shaperName, info.filterName };
    int chipX = pad;
    for (const auto& chip : chips)
    {
        const int chipW = 8 + (int) (chip.length() * 7.2f);  // rough text width estimate
        const auto chipRect = juce::Rectangle<float> ((float) chipX, (float) chipY + 20,
                                                      (float) chipW, 22.0f);
        g.setColour (LF::plate3());
        g.fillRoundedRectangle (chipRect, 4.0f);
        g.setColour (LF::plateLine());
        g.drawRoundedRectangle (chipRect.reduced (0.5f), 4.0f, 1.0f);
        g.setColour (LF::ink2());
        g.setFont (juce::FontOptions (9.5f));
        g.drawFittedText (chip, (int) chipRect.getX(), (int) chipRect.getY(),
                          (int) chipRect.getWidth(), (int) chipRect.getHeight(),
                          juce::Justification::centred, 1);
        chipX += chipW + 6;
    }
}

// ─── Collision ───────────────────────────────────────────────────────────────
void PresetPanel::paintCollision (juce::Graphics& g) const
{
    using LF = manifold::ui::ManifoldLookAndFeel;

    // Amber warning accent strip under the header.
    g.setColour (juce::Colour (kAmberArgb).withAlpha (0.12f));
    g.fillRect (0, kHeaderH, getWidth(), 4);
    g.setColour (juce::Colour (kAmberArgb).withAlpha (0.55f));
    g.fillRect (0, kHeaderH, getWidth(), 1);

    const int textX = 16, textW = getWidth() - 32;
    const int y0    = kHeaderH + 28;

    g.setColour (LF::ink2());
    g.setFont (juce::FontOptions (12.0f));
    g.drawFittedText ("A preset named", textX, y0, textW, 20,
                      juce::Justification::centredLeft, 1);

    g.setColour (juce::Colour (kAmberArgb));
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawFittedText ("\"" + collisionName_ + "\"", textX, y0 + 22, textW, 22,
                      juce::Justification::centredLeft, 1);

    g.setColour (LF::ink2());
    g.setFont (juce::FontOptions (12.0f));
    g.drawFittedText ("already exists.", textX, y0 + 46, textW, 20,
                      juce::Justification::centredLeft, 1);

    g.setColour (LF::ink3());
    g.setFont (juce::FontOptions (10.0f));
    g.drawFittedText ("Rename to keep both, or overwrite to replace.",
                      textX, y0 + 76, textW, 18,
                      juce::Justification::centredLeft, 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Interaction
// ─────────────────────────────────────────────────────────────────────────────
// Resolve the LayoutItem under (x, y) in panel coords, or nullptr-equivalent.
// Returns true if found; out_item / out_row populated.
static bool hitTestItem (const std::vector<PresetPanel::LayoutItem>& items,
                         int yTopOfList, int scrollY, int panelWidth,
                         int mouseX, int mouseY,
                         PresetPanel::LayoutItem& outItem,
                         juce::Rectangle<int>& outRow)
{
    juce::ignoreUnused (mouseX);
    for (const auto& it : items)
    {
        const int top = yTopOfList + it.yTop - scrollY;
        if (mouseY >= top && mouseY < top + it.height)
        {
            outItem = it;
            outRow  = { 0, top, panelWidth, it.height };
            return true;
        }
    }
    return false;
}

void PresetPanel::mouseDown (const juce::MouseEvent& e)
{
    // Close X (right 44 px of header) — all states.
    if (e.y < kHeaderH)
    {
        if (e.x >= getWidth() - 44) hide();
        return;
    }

    // ── Browse ──────────────────────────────────────────────────────────────
    if (state_ == State::Browse)
    {
        // Category tab strip.
        if (e.y < kHeaderH + kTabH)
        {
            const int n  = categories().size();
            const int w  = getWidth() / n;
            const int ti = juce::jlimit (0, n - 1, e.x / juce::jmax (1, w));
            if (ti != selectedCat_)
            {
                selectedCat_         = ti;
                scrollOffset_        = 0;
                hoverRowPmIdx_       = -1;
                confirmingDelPmIdx_  = -1;
                hoverConfirmBtn_     = 0;
                repaint();
            }
            return;
        }

        // Scroll thumb zone.
        if (maxScroll() > 0 && e.x >= getWidth() - kScrollHitW)
        {
            isDraggingThumb_ = true; thumbDragStartY_ = e.y; thumbDragStartOffset_ = scrollOffset_;
            return;
        }

        // Resolve which item is under the cursor.
        const auto items = computeLayout();
        LayoutItem item;
        juce::Rectangle<int> rowB;
        if (! hitTestItem (items, listAreaTop(), scrollOffset_, getWidth(),
                           e.x, e.y, item, rowB))
        {
            // Click in empty list area — cancel any pending confirm.
            if (confirmingDelPmIdx_ != -1)
            {
                confirmingDelPmIdx_ = -1; hoverConfirmBtn_ = 0; repaint();
            }
            return;
        }

        // Only preset rows respond to clicks; dividers and the banner are inert.
        if (item.kind != LayoutItem::Kind::Preset) return;
        const int pi = item.presetIdx;

        // ── If this row is in confirm state, handle Cancel / Delete buttons.
        if (pi == confirmingDelPmIdx_)
        {
            if (confirmCancelBounds (rowB).contains (e.x, e.y))
            {
                confirmingDelPmIdx_ = -1; hoverConfirmBtn_ = 0; repaint();
                return;
            }
            if (confirmDeleteBounds (rowB).contains (e.x, e.y))
            {
                const int target = confirmingDelPmIdx_;
                confirmingDelPmIdx_ = -1; hoverConfirmBtn_ = 0;
                pm_.deleteUserPreset (target);   // fires onPresetDeleted → repaint via editor
                return;
            }
            // Click elsewhere on the confirming row is ignored (don't accidentally load).
            return;
        }

        // ── Star → toggle favorite.
        if (starBounds (rowB).contains (e.x, e.y))
        {
            pm_.toggleFavorite (pi);
            // If we were in "All" view, sort may have changed — recompute layout next paint.
            return;
        }

        // ── Trash → enter confirm (user presets only).
        if (trashBounds (rowB).contains (e.x, e.y) && pm_.isUserPreset (pi))
        {
            // Cancel any other open confirm before opening this one.
            confirmingDelPmIdx_ = pi;
            hoverConfirmBtn_    = 0;
            repaint();
            return;
        }

        // Click on row body → load and close.
        // Cancel any unrelated open confirm before loading.
        if (confirmingDelPmIdx_ != -1) confirmingDelPmIdx_ = -1;
        pm_.load (pi);
        hide();
        return;
    }

    // ── Save form: category pills ────────────────────────────────────────────
    if (state_ == State::SaveForm)
    {
        const auto& saveCats = saveCategories();
        for (int i = 0; i < saveCats.size(); ++i)
        {
            if (savePillBounds (i).contains (e.x, e.y))
            {
                saveCatIdx_ = i;
                repaint();
                return;
            }
        }
    }
}

void PresetPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (state_ != State::Browse || ! isDraggingThumb_) return;
    const int ms = maxScroll();
    if (ms <= 0) return;
    const int   availH = listAreaH();
    const float thumbH = juce::jmax (24.0f, (float) availH * (float) availH / (float) contentH());
    const float travel = (float) availH - thumbH;
    if (travel <= 0.0f) return;
    scrollOffset_ = juce::jlimit (0, ms,
                                  thumbDragStartOffset_ + juce::roundToInt (
                                      (float) (e.y - thumbDragStartY_) * (float) ms / travel));
    repaint();
}

void PresetPanel::mouseUp (const juce::MouseEvent&)
{
    isDraggingThumb_ = false;
}

void PresetPanel::mouseMove (const juce::MouseEvent& e)
{
    if (state_ != State::Browse) return;

    int newRowPm  = -1;
    int newFavPm  = -1;
    int newDelPm  = -1;
    int newTab    = -1;
    int newCfBtn  = 0;

    if (e.y >= listAreaTop())
    {
        const auto items = computeLayout();
        LayoutItem item;
        juce::Rectangle<int> rowB;
        if (hitTestItem (items, listAreaTop(), scrollOffset_, getWidth(),
                         e.x, e.y, item, rowB)
            && item.kind == LayoutItem::Kind::Preset)
        {
            const int pi = item.presetIdx;
            newRowPm = pi;

            if (pi == confirmingDelPmIdx_)
            {
                if (confirmCancelBounds (rowB).contains (e.x, e.y)) newCfBtn = 1;
                else if (confirmDeleteBounds (rowB).contains (e.x, e.y)) newCfBtn = 2;
            }
            else
            {
                if (starBounds (rowB).contains (e.x, e.y))           newFavPm = pi;
                if (trashBounds (rowB).contains (e.x, e.y) && pm_.isUserPreset (pi))
                    newDelPm = pi;
            }
        }
    }
    else if (e.y >= kHeaderH)
    {
        const int n = categories().size();
        newTab = juce::jlimit (0, n - 1, e.x / juce::jmax (1, getWidth() / n));
    }

    if (newRowPm != hoverRowPmIdx_ || newTab != hoverTabIdx_
        || newFavPm != hoverFavPmIdx_ || newDelPm != hoverDelPmIdx_
        || newCfBtn != hoverConfirmBtn_)
    {
        hoverRowPmIdx_   = newRowPm;
        hoverTabIdx_     = newTab;
        hoverFavPmIdx_   = newFavPm;
        hoverDelPmIdx_   = newDelPm;
        hoverConfirmBtn_ = newCfBtn;
        repaint();
    }
}

void PresetPanel::mouseExit (const juce::MouseEvent&)
{
    if (hoverRowPmIdx_ != -1 || hoverTabIdx_ != -1
        || hoverFavPmIdx_ != -1 || hoverDelPmIdx_ != -1
        || hoverConfirmBtn_ != 0)
    {
        hoverRowPmIdx_ = hoverTabIdx_ = hoverFavPmIdx_ = hoverDelPmIdx_ = -1;
        hoverConfirmBtn_ = 0;
        repaint();
    }
}

void PresetPanel::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (state_ != State::Browse || e.y < listAreaTop()) return;
    const int ms = maxScroll();
    if (ms <= 0) return;
    scrollOffset_ = juce::jlimit (0, ms, scrollOffset_ + juce::roundToInt (-w.deltaY * 80.0f));
    repaint();
}

} // namespace manifold::ui

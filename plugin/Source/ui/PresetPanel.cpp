#include "PresetPanel.h"
#include "ManifoldLookAndFeel.h"
#include "../preset/PresetManager.h"

namespace manifold::ui
{

static constexpr juce::uint32 kAccentArgb  = 0xffb59cff;
static constexpr juce::uint32 kAmberArgb   = 0xffffb870;   // Chua/warning hue

static inline float smoothStep (float t) noexcept
{
    return t * t * (3.0f - 2.0f * t);
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────
PresetPanel::PresetPanel (manifold::preset::PresetManager& pm)
    : pm_ (pm)
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

    // ── Action buttons ───────────────────────────────────────────────────────
    // Neutral style (CANCEL, RENAME use this).
    for (auto* btn : { &cancelBtn, &renameBtn })
    {
        btn->setColour (juce::TextButton::buttonColourId,  LF::plate3());
        btn->setColour (juce::TextButton::textColourOffId, LF::ink2());
        btn->setVisible (false);
        addAndMakeVisible (*btn);
    }
    // Accent style (SAVE, OVERWRITE).
    for (auto* btn : { &saveConfirmBtn, &overwriteBtn })
    {
        btn->setColour (juce::TextButton::buttonColourId,  juce::Colour (kAccentArgb).withAlpha (0.18f));
        btn->setColour (juce::TextButton::textColourOffId, juce::Colour (kAccentArgb));
        btn->setVisible (false);
        addAndMakeVisible (*btn);
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
const juce::StringArray& PresetPanel::categories()
{
    static const juce::StringArray kCats {
        "All", "Wobble", "Growl", "Drone", "Metal", "Glitch", "Alien"
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

std::vector<int> PresetPanel::filteredIndices() const
{
    std::vector<int> result;
    const auto& cats    = categories();
    const bool  showAll = (selectedCat_ == 0);
    const auto  cat     = cats[selectedCat_];

    for (int i = 0; i < pm_.getCount(); ++i)
        if (showAll || pm_.getCategory (i) == cat)
            result.push_back (i);

    return result;
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
juce::Rectangle<int> PresetPanel::rowBounds (int fi) const noexcept
{
    return { 0, listAreaTop() + fi * kRowH - scrollOffset_, getWidth(), kRowH };
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
    hoverRowIdx_  = hoverTabIdx_ = -1;
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

    // Action button pair for save form (CANCEL | SAVE).
    const int sfBtnY  = kHeaderH + 240;
    const int sfBtnH  = 32;
    const int sfBtnW  = 86;
    const int sfGap   = 8;
    const int rMargin = 14;
    cancelBtn.setBounds       (getWidth() - rMargin - 2 * sfBtnW - sfGap, sfBtnY, sfBtnW, sfBtnH);
    saveConfirmBtn.setBounds  (getWidth() - rMargin - sfBtnW,             sfBtnY, sfBtnW, sfBtnH);

    // Action buttons for collision state (RENAME | OVERWRITE).
    const int colBtnY = kHeaderH + 120;
    const int colBtnH = 32;
    const int colBtnW = 106;
    renameBtn.setBounds    (getWidth() - rMargin - 2 * colBtnW - sfGap, colBtnY, colBtnW, colBtnH);
    overwriteBtn.setBounds (getWidth() - rMargin - colBtnW,             colBtnY, colBtnW, colBtnH);
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
void PresetPanel::paintBrowse (juce::Graphics& g) const
{
    using LF = manifold::ui::ManifoldLookAndFeel;

    // Category tab strip.
    {
        const auto tabArea = juce::Rectangle<int> (0, kHeaderH, getWidth(), kTabH);
        g.setColour (LF::plate0());
        g.fillRect (tabArea);
        g.setColour (LF::plateLine());
        g.drawHorizontalLine (kHeaderH + kTabH - 1, 0.0f, (float) getWidth());

        const auto& cats = categories();
        for (int i = 0; i < cats.size(); ++i)
        {
            const auto tb     = tabBounds (i).toFloat();
            const bool active = (i == selectedCat_);
            const bool hov    = (i == hoverTabIdx_) && ! active;

            if (active)
            {
                const auto pill = tb.reduced (4.0f, 7.0f);
                g.setColour (juce::Colour (kAccentArgb).withAlpha (0.22f));
                g.fillRoundedRectangle (pill, 4.0f);
                g.setColour (juce::Colour (kAccentArgb).withAlpha (0.50f));
                g.drawRoundedRectangle (pill.reduced (0.5f), 4.0f, 1.0f);
            }
            else if (hov)
            {
                g.setColour (LF::plate3());
                g.fillRoundedRectangle (tb.reduced (4.0f, 7.0f), 4.0f);
            }

            g.setColour (active ? juce::Colour (kAccentArgb)
                                : (hov ? LF::ink2() : LF::ink3()));
            g.setFont (juce::FontOptions (9.0f, active ? juce::Font::bold : juce::Font::plain));
            g.drawFittedText (cats[i], tabBounds (i).reduced (2, 0),
                              juce::Justification::centred, 1);
        }
    }

    // Preset list (clipped to scrollable area).
    {
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (0, listAreaTop(), getWidth(), listAreaH());

        const auto indices    = filteredIndices();
        const int  currentIdx = pm_.getCurrentIndex();

        if (indices.empty())
        {
            g.setColour (LF::ink4());
            g.setFont (juce::FontOptions (11.0f));
            g.drawFittedText ("No presets in this category.",
                              12, listAreaTop() + 20, getWidth() - 24, 24,
                              juce::Justification::centredLeft, 1);
        }

        for (int fi = 0; fi < (int) indices.size(); ++fi)
        {
            const int  pi   = indices[(size_t) fi];
            const auto row  = rowBounds (fi).toFloat();
            const bool sel  = (pi == currentIdx);
            const bool hov  = (fi == hoverRowIdx_) && ! sel;
            const bool last = (fi == (int) indices.size() - 1);

            if (row.getBottom() < (float) listAreaTop() || row.getY() > (float) getHeight())
                continue;

            if (sel)
            {
                juce::ColourGradient bg (LF::plate3(), row.getX(), row.getY(),
                                          LF::plate2().brighter (0.04f), row.getX(), row.getBottom(), false);
                g.setGradientFill (bg); g.fillRect (row);
                g.setColour (juce::Colour (kAccentArgb).withAlpha (0.85f));
                g.fillRect (row.withWidth (3.0f));
                g.setColour (juce::Colour (kAccentArgb).withAlpha (0.20f));
                g.drawRect (row.reduced (0.5f), 1.0f);
            }
            else if (hov)
            {
                g.setColour (LF::plate2().brighter (0.10f)); g.fillRect (row);
            }
            else
            {
                g.setColour (LF::plate2()); g.fillRect (row);
            }

            if (! last)
            {
                g.setColour (LF::plateLine());
                g.drawHorizontalLine ((int) row.getBottom() - 1,
                                      row.getX() + 12.0f, row.getRight() - 12.0f);
            }

            // Engine dot with glow.
            const float dotR  = 5.0f;
            const float dotCX = row.getX() + 20.0f;
            const float dotCY = row.getCentreY();
            const auto  ecol  = engineColour (pm_.getPrimaryEngine (pi));
            g.setColour (ecol.withAlpha (0.28f));
            g.fillEllipse (dotCX - dotR * 1.7f, dotCY - dotR * 1.7f, dotR * 3.4f, dotR * 3.4f);
            g.setColour (ecol);
            g.fillEllipse (dotCX - dotR, dotCY - dotR, dotR * 2.0f, dotR * 2.0f);

            // Name + meta.
            const float textX = dotCX + dotR + 12.0f;
            const float textW = (float) getWidth() - textX - (float) kScrollW - 6.0f
                                - (pm_.isUserPreset (pi) ? 44.0f : 0.0f);

            g.setColour (sel ? LF::ink1() : LF::ink2());
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawFittedText (pm_.getName (pi), (int) textX, (int) row.getY() + 10, (int) textW, 18,
                              juce::Justification::centredLeft, 1);

            g.setColour (LF::ink3());
            g.setFont (juce::FontOptions (10.0f));
            g.drawFittedText (pm_.getCategory (pi) + " - " + pm_.getPrimaryEngine (pi),
                              (int) textX, (int) row.getY() + 31, (int) textW, 16,
                              juce::Justification::centredLeft, 1);

            // USER badge.
            if (pm_.isUserPreset (pi))
            {
                const float bw = 34.0f, bh = 16.0f;
                const float bx = row.getRight() - bw - 10.0f;
                const float by = row.getCentreY() - bh * 0.5f;
                g.setColour (juce::Colour (kAccentArgb).withAlpha (0.20f));
                g.fillRoundedRectangle (bx, by, bw, bh, 3.0f);
                g.setColour (juce::Colour (kAccentArgb).withAlpha (0.70f));
                g.setFont (juce::FontOptions (8.5f, juce::Font::bold));
                g.drawFittedText ("USER", (int) bx, (int) by, (int) bw, (int) bh,
                                  juce::Justification::centred, 1);
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
            if (ti != selectedCat_) { selectedCat_ = ti; scrollOffset_ = 0; hoverRowIdx_ = -1; repaint(); }
            return;
        }

        // Scroll thumb zone.
        if (maxScroll() > 0 && e.x >= getWidth() - kScrollHitW)
        {
            isDraggingThumb_ = true; thumbDragStartY_ = e.y; thumbDragStartOffset_ = scrollOffset_;
            return;
        }

        // Preset row.
        const auto indices = filteredIndices();
        const int  fi      = (e.y - listAreaTop() + scrollOffset_) / kRowH;
        if (fi >= 0 && fi < (int) indices.size())
        {
            pm_.load (indices[(size_t) fi]);
            hide();
        }
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

    int newRow = -1, newTab = -1;
    if (e.y >= listAreaTop())
    {
        const auto indices = filteredIndices();
        const int  fi      = (e.y - listAreaTop() + scrollOffset_) / kRowH;
        if (fi >= 0 && fi < (int) indices.size()) newRow = fi;
    }
    else if (e.y >= kHeaderH)
    {
        const int n = categories().size();
        newTab = juce::jlimit (0, n - 1, e.x / juce::jmax (1, getWidth() / n));
    }

    if (newRow != hoverRowIdx_ || newTab != hoverTabIdx_)
    {
        hoverRowIdx_ = newRow; hoverTabIdx_ = newTab; repaint();
    }
}

void PresetPanel::mouseExit (const juce::MouseEvent&)
{
    if (hoverRowIdx_ != -1 || hoverTabIdx_ != -1)
    {
        hoverRowIdx_ = hoverTabIdx_ = -1; repaint();
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

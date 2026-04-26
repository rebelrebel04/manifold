#include "PresetPanel.h"
#include "ManifoldLookAndFeel.h"
#include "../preset/PresetManager.h"

namespace manifold::ui
{

static constexpr juce::uint32 kAccentArgb = 0xffb59cff;

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
    setInterceptsMouseClicks (true, true);
    setVisible (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

// ─────────────────────────────────────────────────────────────────────────────
// Categories
// ─────────────────────────────────────────────────────────────────────────────
const juce::StringArray& PresetPanel::categories()
{
    static const juce::StringArray kCats {
        "All", "Wobble", "Shattered", "Alien", "Metallic", "Vocal", "Organic"
    };
    return kCats;
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
juce::Rectangle<int> PresetPanel::rowBounds (int filteredIdx) const noexcept
{
    return { 0, listAreaTop() + filteredIdx * kRowH - scrollOffset_, getWidth(), kRowH };
}

juce::Rectangle<int> PresetPanel::tabBounds (int catIdx) const noexcept
{
    const int n = categories().size();
    const int w = getWidth() / n;
    return { catIdx * w, kHeaderH, w, kTabH };
}

// ─────────────────────────────────────────────────────────────────────────────
// Show / Hide
// ─────────────────────────────────────────────────────────────────────────────
void PresetPanel::show()
{
    closing_      = false;
    animProgress_ = 0.0f;
    scrollOffset_ = 0;
    hoverRowIdx_  = -1;
    hoverTabIdx_  = -1;

    setAlpha (0.0f);
    // Start slid fully off-screen to the left.
    setTransform (juce::AffineTransform::translation (-(float) getWidth(), 0.0f));
    setVisible (true);
    toFront (false);
    startTimer (16);  // ~60 fps
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
    const float slideX = -(float) getWidth() * (1.0f - t);   // 0 = fully open, -W = closed
    setAlpha (t);
    setTransform (juce::AffineTransform::translation (slideX, 0.0f));
}

// ─────────────────────────────────────────────────────────────────────────────
// Layout
// ─────────────────────────────────────────────────────────────────────────────
void PresetPanel::resized()
{
    // No child components; all layout is handled in paint() + mouse handlers.
}

// ─────────────────────────────────────────────────────────────────────────────
// Paint
// ─────────────────────────────────────────────────────────────────────────────
void PresetPanel::paint (juce::Graphics& g)
{
    using LF = manifold::ui::ManifoldLookAndFeel;

    // Panel background.
    g.setColour (LF::plate2());
    g.fillRect (getLocalBounds().toFloat());

    // ── Header ───────────────────────────────────────────────────────────────
    {
        const auto hdr = juce::Rectangle<float> (0.0f, 0.0f, (float) getWidth(), (float) kHeaderH);

        juce::ColourGradient hg (LF::plate3(), hdr.getX(), hdr.getY(),
                                 LF::plate2(), hdr.getX(), hdr.getBottom(), false);
        g.setGradientFill (hg);
        g.fillRect (hdr);

        g.setColour (LF::plateLine());
        g.drawHorizontalLine (kHeaderH - 1, 0.0f, (float) getWidth());

        g.setColour (LF::ink2());
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawFittedText ("PRESETS", 18, 0, getWidth() - 62, kHeaderH,
                          juce::Justification::centredLeft, 1);

        // Close X (right 44 px of header).
        const float bx = (float) getWidth() - 22.0f;
        const float by = (float) kHeaderH * 0.5f;
        g.setColour (LF::ink3());
        juce::Path x;
        x.startNewSubPath (bx - 6.0f, by - 6.0f); x.lineTo (bx + 6.0f, by + 6.0f);
        x.startNewSubPath (bx + 6.0f, by - 6.0f); x.lineTo (bx - 6.0f, by + 6.0f);
        g.strokePath (x, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    }

    // ── Category tab strip ───────────────────────────────────────────────────
    {
        const auto tabArea = juce::Rectangle<int> (0, kHeaderH, getWidth(), kTabH);
        g.setColour (LF::plate0());
        g.fillRect (tabArea);

        g.setColour (LF::plateLine());
        g.drawHorizontalLine (kHeaderH + kTabH - 1, 0.0f, (float) getWidth());

        const auto& cats = categories();
        for (int i = 0; i < cats.size(); ++i)
        {
            const auto  tb     = tabBounds (i).toFloat();
            const bool  active = (i == selectedCat_);
            const bool  hov    = (i == hoverTabIdx_) && ! active;

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
                const auto pill = tb.reduced (4.0f, 7.0f);
                g.setColour (LF::plate3());
                g.fillRoundedRectangle (pill, 4.0f);
            }

            g.setColour (active ? juce::Colour (kAccentArgb)
                                : (hov ? LF::ink2() : LF::ink3()));
            g.setFont (juce::FontOptions (9.0f, active ? juce::Font::bold : juce::Font::plain));
            g.drawFittedText (cats[i], tabBounds (i).reduced (2, 0),
                              juce::Justification::centred, 1);
        }
    }

    // ── Preset list (clipped to scrollable area) ─────────────────────────────
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

            // Cull fully off-screen rows.
            if (row.getBottom() < (float) listAreaTop() || row.getY() > (float) getHeight())
                continue;

            // Background.
            if (sel)
            {
                juce::ColourGradient bg (LF::plate3(), row.getX(), row.getY(),
                                         LF::plate2().brighter (0.04f), row.getX(), row.getBottom(), false);
                g.setGradientFill (bg);
                g.fillRect (row);
                g.setColour (juce::Colour (kAccentArgb).withAlpha (0.85f));
                g.fillRect (row.withWidth (3.0f));
                g.setColour (juce::Colour (kAccentArgb).withAlpha (0.20f));
                g.drawRect (row.reduced (0.5f), 1.0f);
            }
            else if (hov)
            {
                g.setColour (LF::plate2().brighter (0.10f));
                g.fillRect (row);
            }
            else
            {
                g.setColour (LF::plate2());
                g.fillRect (row);
            }

            // Separator.
            if (! last)
            {
                g.setColour (LF::plateLine());
                g.drawHorizontalLine ((int) row.getBottom() - 1,
                                      row.getX() + 12.0f, row.getRight() - 12.0f);
            }

            // Engine colour dot with soft glow.
            const float dotR   = 5.0f;
            const float dotCX  = row.getX() + 20.0f;
            const float dotCY  = row.getCentreY();
            const auto  ecol   = engineColour (pm_.getPrimaryEngine (pi));

            g.setColour (ecol.withAlpha (0.28f));
            g.fillEllipse (dotCX - dotR * 1.7f, dotCY - dotR * 1.7f,
                           dotR * 3.4f, dotR * 3.4f);
            g.setColour (ecol);
            g.fillEllipse (dotCX - dotR, dotCY - dotR, dotR * 2.0f, dotR * 2.0f);

            // Name + meta text.
            const float textX = dotCX + dotR + 12.0f;
            const float textW = (float) getWidth() - textX - (float) kScrollW - 6.0f;

            g.setColour (sel ? LF::ink1() : LF::ink2());
            g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
            g.drawFittedText (pm_.getName (pi),
                              (int) textX, (int) row.getY() + 10, (int) textW, 18,
                              juce::Justification::centredLeft, 1);

            g.setColour (LF::ink3());
            g.setFont (juce::FontOptions (10.0f));
            g.drawFittedText (pm_.getCategory (pi) + " - " + pm_.getPrimaryEngine (pi),
                              (int) textX, (int) row.getY() + 31, (int) textW, 16,
                              juce::Justification::centredLeft, 1);

            // USER badge (Phase 8b.2 placeholder — never shown while all presets are factory).
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
    }   // clip region released

    // ── Scroll thumb ─────────────────────────────────────────────────────────
    const int ms = maxScroll();
    if (ms > 0)
    {
        const int   availH = listAreaH();
        const float thumbH = juce::jmax (24.0f, (float) availH * (float) availH
                                                      / (float) contentH());
        const float thumbY = (float) listAreaTop()
                             + (float) scrollOffset_ / (float) ms
                               * ((float) availH - thumbH);
        const float thumbX = (float) getWidth() - (float) kScrollW - 2.0f;

        g.setColour (juce::Colours::white.withAlpha (0.04f));
        g.fillRoundedRectangle (thumbX, (float) listAreaTop(),
                                (float) kScrollW, (float) availH, 2.0f);
        g.setColour (juce::Colour (kAccentArgb).withAlpha (0.35f));
        g.fillRoundedRectangle (thumbX, thumbY, (float) kScrollW, thumbH, 2.0f);
    }

    // ── Right-edge drop shadow (casts onto content to the right of the panel) ─
    {
        const auto b = getLocalBounds().toFloat();
        juce::ColourGradient sh (juce::Colours::transparentBlack, b.getRight() - 18.0f, 0.0f,
                                 juce::Colours::black.withAlpha (0.42f), b.getRight(), 0.0f,
                                 false);
        g.setGradientFill (sh);
        g.fillRect (b.withLeft (b.getRight() - 18.0f));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Interaction
// ─────────────────────────────────────────────────────────────────────────────
void PresetPanel::mouseDown (const juce::MouseEvent& e)
{
    // ── Header close hit-area (right 44 px) ─────────────────────────────────
    if (e.y < kHeaderH)
    {
        if (e.x >= getWidth() - 44) hide();
        return;
    }

    // ── Category tab strip ───────────────────────────────────────────────────
    if (e.y < kHeaderH + kTabH)
    {
        const int n  = categories().size();
        const int w  = getWidth() / n;
        const int ti = juce::jlimit (0, n - 1, e.x / w);
        if (ti != selectedCat_)
        {
            selectedCat_  = ti;
            scrollOffset_ = 0;
            hoverRowIdx_  = -1;
            repaint();
        }
        return;
    }

    // ── Scroll thumb zone (right strip, below list top) ──────────────────────
    if (maxScroll() > 0 && e.x >= getWidth() - kScrollHitW)
    {
        isDraggingThumb_      = true;
        thumbDragStartY_      = e.y;
        thumbDragStartOffset_ = scrollOffset_;
        return;
    }

    // ── Preset row selection ─────────────────────────────────────────────────
    const auto indices = filteredIndices();
    const int  fi      = (e.y - listAreaTop() + scrollOffset_) / kRowH;
    if (fi >= 0 && fi < (int) indices.size())
    {
        pm_.load (indices[(size_t) fi]);
        hide();
    }
}

void PresetPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (! isDraggingThumb_) return;
    const int ms = maxScroll();
    if (ms <= 0) return;

    const int   availH = listAreaH();
    const float thumbH = juce::jmax (24.0f, (float) availH * (float) availH / (float) contentH());
    const float travel = (float) availH - thumbH;
    if (travel <= 0.0f) return;

    const int deltaY = e.y - thumbDragStartY_;
    scrollOffset_ = juce::jlimit (0, ms,
                                  thumbDragStartOffset_
                                  + juce::roundToInt ((float) deltaY * (float) ms / travel));
    repaint();
}

void PresetPanel::mouseUp (const juce::MouseEvent&)
{
    isDraggingThumb_ = false;
}

void PresetPanel::mouseMove (const juce::MouseEvent& e)
{
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

    const bool changed = (newRow != hoverRowIdx_ || newTab != hoverTabIdx_);
    hoverRowIdx_ = newRow;
    hoverTabIdx_ = newTab;
    if (changed) repaint();
}

void PresetPanel::mouseExit (const juce::MouseEvent&)
{
    if (hoverRowIdx_ != -1 || hoverTabIdx_ != -1)
    {
        hoverRowIdx_ = hoverTabIdx_ = -1;
        repaint();
    }
}

void PresetPanel::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    if (e.y < listAreaTop()) return;   // ignore wheel over header/tabs
    const int ms = maxScroll();
    if (ms <= 0) return;

    const int delta = juce::roundToInt (-w.deltaY * 80.0f);
    scrollOffset_   = juce::jlimit (0, ms, scrollOffset_ + delta);
    repaint();
}

} // namespace manifold::ui

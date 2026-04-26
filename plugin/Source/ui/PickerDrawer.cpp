#include "PickerDrawer.h"
#include "ManifoldLookAndFeel.h"

namespace manifold::ui
{

static constexpr juce::uint32 kAccentArgb = 0xffb59cff;

// Smooth-step ease: 3t^2 - 2t^3
static inline float smoothStep (float t) noexcept
{
    return t * t * (3.0f - 2.0f * t);
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────
PickerDrawer::PickerDrawer()
{
    setInterceptsMouseClicks (true, true);
    setVisible (false);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void PickerDrawer::configure (const juce::String&       title,
                               std::vector<Option>        options,
                               std::function<void(int)>   onSelected,
                               int                        selectedIndex)
{
    title_       = title;
    options_     = std::move (options);
    onSelected_  = std::move (onSelected);
    selectedIdx_ = selectedIndex;
    hoverIdx_    = -1;
    resized();
    repaint();
}

// ─────────────────────────────────────────────────────────────────────────────
// Show / Hide
// ─────────────────────────────────────────────────────────────────────────────
void PickerDrawer::show()
{
    closing_      = false;
    animProgress_ = 0.0f;
    hoverIdx_     = -1;

    // Start fully slid-off to the right, invisible.
    setAlpha (0.0f);
    setTransform (juce::AffineTransform::translation ((float) getWidth(), 0.0f));

    setVisible (true);
    toFront (false);
    startTimer (16);  // ~60 fps
}

void PickerDrawer::hide()
{
    if (closing_) return;
    closing_ = true;
    if (! isTimerRunning())
        startTimer (16);
}

// ─────────────────────────────────────────────────────────────────────────────
// Animation
// ─────────────────────────────────────────────────────────────────────────────
void PickerDrawer::timerCallback()
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

void PickerDrawer::applyAnimTransform()
{
    const float t      = smoothStep (animProgress_);
    const float slideX = (float) getWidth() * (1.0f - t);
    setAlpha (t);
    setTransform (juce::AffineTransform::translation (slideX, 0.0f));
}

// ─────────────────────────────────────────────────────────────────────────────
// Layout
// ─────────────────────────────────────────────────────────────────────────────
juce::Rectangle<int> PickerDrawer::cardBounds (int i) const noexcept
{
    return { 0, kHeaderH + i * kCardH, getWidth(), kCardH };
}

void PickerDrawer::resized()
{
    // No child components; close hit-area is handled in mouseDown.
}

// ─────────────────────────────────────────────────────────────────────────────
// Paint
// ─────────────────────────────────────────────────────────────────────────────
void PickerDrawer::paint (juce::Graphics& g)
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    auto bounds = getLocalBounds().toFloat();

    // Panel background.
    g.setColour (LF::plate2());
    g.fillRect (bounds);

    // Left-edge shadow strip.
    juce::ColourGradient shadow (juce::Colours::black.withAlpha (0.45f), bounds.getX(), 0,
                                 juce::Colours::transparentBlack,        bounds.getX() + 18.0f, 0,
                                 false);
    g.setGradientFill (shadow);
    g.fillRect (bounds.withWidth (18.0f));

    // Header bar.
    {
        auto hdr = bounds.removeFromTop ((float) kHeaderH);
        juce::ColourGradient hg (LF::plate3(), hdr.getX(), hdr.getY(),
                                 LF::plate2(), hdr.getX(), hdr.getBottom(), false);
        g.setGradientFill (hg);
        g.fillRect (hdr);

        g.setColour (LF::plateLine());
        g.drawHorizontalLine ((int) hdr.getBottom() - 1, hdr.getX(), hdr.getRight());

        g.setColour (LF::ink2());
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawFittedText (title_.toUpperCase(),
                          ((int) hdr.getX() + 18), 0, getWidth() - 62, kHeaderH,
                          juce::Justification::centredLeft, 1);
    }

    // Option cards.
    for (int i = 0; i < (int) options_.size(); ++i)
    {
        const auto& opt      = options_[(size_t) i];
        const auto  card     = cardBounds (i).toFloat();
        const bool  selected = (i == selectedIdx_);
        const bool  hovered  = (i == hoverIdx_) && ! selected;
        const bool  isLast   = (i == (int) options_.size() - 1);

        // ── Card background ──────────────────────────────────────────────
        if (selected)
        {
            // Richer gradient: plate3 top -> plate2 mid -> slight plate3 tint bottom
            juce::ColourGradient bg (LF::plate3(), card.getX(), card.getY(),
                                     LF::plate2().brighter (0.04f), card.getX(), card.getBottom(), false);
            g.setGradientFill (bg);
            g.fillRect (card);

            // Accent left-edge bar (3 px, full card height).
            g.setColour (juce::Colour (kAccentArgb).withAlpha (0.85f));
            g.fillRect (card.withWidth (3.0f));

            // Accent outline — subtle border around the whole card.
            g.setColour (juce::Colour (kAccentArgb).withAlpha (0.22f));
            g.drawRect (card.reduced (0.5f), 1.0f);
        }
        else if (hovered)
        {
            // Mild brightness lift for hover.
            g.setColour (LF::plate2().brighter (0.12f));
            g.fillRect (card);
        }
        else
        {
            g.setColour (LF::plate2());
            g.fillRect (card);
        }

        // ── Separator line ───────────────────────────────────────────────
        if (! isLast)
        {
            g.setColour (LF::plateLine());
            g.drawHorizontalLine ((int) card.getBottom() - 1,
                                  card.getX() + 12.0f, card.getRight() - 12.0f);
        }

        // ── Diagram area (left side) ─────────────────────────────────────
        const auto diagRect = card.withWidth  ((float) kDiagramW)
                                  .withHeight (card.getHeight() - 2.0f * (float) kCardPad)
                                  .translated ((float) kCardPad, (float) kCardPad);

        if (opt.drawDiagram)
            opt.drawDiagram (g, diagRect);

        // ── Name + description (right of diagram) ────────────────────────
        const float checkW  = selected ? 28.0f : 0.0f;
        const float textX   = card.getX() + (float) kDiagramW + 2.0f * (float) kCardPad;
        const float textW   = card.getRight() - textX - (float) kCardPad - checkW;

        g.setColour (selected ? LF::ink1() : LF::ink2());
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawFittedText (opt.name,
                          (int) textX, (int) card.getY() + kCardPad,
                          (int) textW, 18,
                          juce::Justification::centredLeft, 1);

        // Description: 11 pt, ink2 (selected) or ink3 (unselected)
        g.setColour (selected ? LF::ink2() : LF::ink3());
        g.setFont (juce::FontOptions (11.0f, juce::Font::plain));
        g.drawFittedText (opt.description,
                          (int) textX, (int) card.getY() + kCardPad + 20,
                          (int) textW, (int) card.getHeight() - kCardPad * 2 - 20,
                          juce::Justification::topLeft, 3);

        // ── Checkmark for selected option ────────────────────────────────
        if (selected)
        {
            const float cx = card.getRight() - 18.0f;
            const float cy = card.getCentreY();
            juce::Path check;
            check.startNewSubPath (cx - 6.0f, cy - 0.5f);
            check.lineTo          (cx - 2.0f, cy + 4.5f);
            check.lineTo          (cx + 6.0f, cy - 5.5f);
            g.setColour (juce::Colour (kAccentArgb));
            g.strokePath (check, juce::PathStrokeType (2.0f,
                                                        juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        }
    }

    // ── Close X in top-right 44px of header ─────────────────────────────
    {
        const float bx = (float) getWidth() - 22.0f;
        const float by = (float) kHeaderH * 0.5f;
        g.setColour (LF::ink3());
        juce::Path x;
        x.startNewSubPath (bx - 6.0f, by - 6.0f); x.lineTo (bx + 6.0f, by + 6.0f);
        x.startNewSubPath (bx + 6.0f, by - 6.0f); x.lineTo (bx - 6.0f, by + 6.0f);
        g.strokePath (x, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Interaction
// ─────────────────────────────────────────────────────────────────────────────
void PickerDrawer::mouseDown (const juce::MouseEvent& e)
{
    // Close hit-area: right 44px of header.
    if (e.y < kHeaderH)
    {
        if (e.x >= getWidth() - 44)
            hide();
        return;
    }

    // Option card selection.
    const int i = (e.y - kHeaderH) / kCardH;
    if (i >= 0 && i < (int) options_.size())
    {
        selectedIdx_ = i;
        hoverIdx_    = -1;
        repaint();
        if (onSelected_) onSelected_ (i);
        hide();
    }
}

void PickerDrawer::mouseMove (const juce::MouseEvent& e)
{
    const int raw     = (e.y < kHeaderH) ? -1 : (e.y - kHeaderH) / kCardH;
    const int clamped = (raw >= 0 && raw < (int) options_.size()) ? raw : -1;
    if (clamped != hoverIdx_)
    {
        hoverIdx_ = clamped;
        repaint();
    }
}

void PickerDrawer::mouseExit (const juce::MouseEvent&)
{
    if (hoverIdx_ != -1)
    {
        hoverIdx_ = -1;
        repaint();
    }
}

} // namespace manifold::ui

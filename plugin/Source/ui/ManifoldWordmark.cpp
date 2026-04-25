#include "ManifoldWordmark.h"
#include "ManifoldLookAndFeel.h"

namespace manifold::ui
{

namespace
{
    // Design viewBox is 220 × 38; we render into our local bounds with that aspect.
    constexpr float kVbW = 220.0f;
    constexpr float kVbH = 38.0f;

    // Letterform paths in design coordinates (220×38). Direct ports of WordmarkC.
    // These are stroked, not filled — see paint().
    juce::Path makeLetterforms()
    {
        juce::Path p;

        // M
        p.startNewSubPath (2.0f, 32.0f);
        p.lineTo (2.0f,  6.0f);
        p.lineTo (12.0f, 20.0f);
        p.lineTo (22.0f, 6.0f);
        p.lineTo (22.0f, 32.0f);

        // A — outer triangle + crossbar
        p.startNewSubPath (28.0f, 32.0f);
        p.lineTo (36.0f, 6.0f);
        p.lineTo (44.0f, 32.0f);
        p.startNewSubPath (31.0f, 24.0f);
        p.lineTo (41.0f, 24.0f);

        // N
        p.startNewSubPath (50.0f, 32.0f);
        p.lineTo (50.0f, 6.0f);
        p.lineTo (66.0f, 32.0f);
        p.lineTo (66.0f, 6.0f);

        // I
        p.startNewSubPath (74.0f, 6.0f);
        p.lineTo (74.0f, 32.0f);

        // F
        p.startNewSubPath (82.0f, 32.0f);
        p.lineTo (82.0f, 6.0f);
        p.lineTo (98.0f, 6.0f);
        p.startNewSubPath (82.0f, 18.0f);
        p.lineTo (94.0f, 18.0f);

        // L
        p.startNewSubPath (132.0f, 6.0f);
        p.lineTo (132.0f, 32.0f);
        p.lineTo (148.0f, 32.0f);

        // D
        p.startNewSubPath (154.0f, 32.0f);
        p.lineTo (154.0f, 6.0f);
        p.lineTo (168.0f, 6.0f);
        p.lineTo (176.0f, 14.0f);
        p.lineTo (176.0f, 24.0f);
        p.lineTo (168.0f, 32.0f);
        p.lineTo (154.0f, 32.0f);

        return p;
    }

    // Stylised "O" — outer ring at the design's centre (114, 19), radius 9.
    juce::Path makeOuterRing()
    {
        juce::Path p;
        p.addEllipse (114.0f - 9.0f, 19.0f - 9.0f, 18.0f, 18.0f);
        return p;
    }

    // Inner accent curve — a flowing horizontal S inside the O hint at the
    // double-scroll attractor topology without being a literal Lorenz glyph.
    juce::Path makeInnerCurve()
    {
        juce::Path p;
        p.startNewSubPath (114.0f - 9.0f, 19.0f);
        p.quadraticTo (114.0f - 4.0f, 19.0f - 4.0f, 114.0f, 19.0f);
        p.quadraticTo (114.0f + 4.0f, 19.0f + 4.0f, 114.0f + 9.0f, 19.0f);
        return p;
    }
}

ManifoldWordmark::ManifoldWordmark (juce::Colour accent) : accentColour (accent)
{
    setOpaque (false);
    setInterceptsMouseClicks (false, false);
}

void ManifoldWordmark::setAccent (juce::Colour c)
{
    accentColour = c;
    repaint();
}

void ManifoldWordmark::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    if (b.getWidth() <= 0.0f || b.getHeight() <= 0.0f) return;

    // Fit the design viewBox into our bounds preserving aspect, vertically centred.
    const float scale  = juce::jmin (b.getWidth() / kVbW, b.getHeight() / kVbH);
    const float drawnW = kVbW * scale;
    const float drawnH = kVbH * scale;
    const float dx = b.getX() + (b.getWidth()  - drawnW) * 0.5f;
    const float dy = b.getY() + (b.getHeight() - drawnH) * 0.5f;

    auto vbToScreen = juce::AffineTransform::scale (scale).translated (dx, dy);

    // ── Letterforms (ink) ──────────────────────────────────────
    auto letters = makeLetterforms();
    letters.applyTransform (vbToScreen);
    g.setColour (ManifoldLookAndFeel::ink1());
    g.strokePath (letters,
                  juce::PathStrokeType (1.8f * scale,
                                        juce::PathStrokeType::mitered,
                                        juce::PathStrokeType::butt));

    // ── O — violet ring with layered glow + inner curve ────────
    auto ring = makeOuterRing();
    ring.applyTransform (vbToScreen);
    auto inner = makeInnerCurve();
    inner.applyTransform (vbToScreen);

    // Outer glow
    g.setColour (accentColour.withAlpha (0.32f));
    g.strokePath (ring, juce::PathStrokeType (4.5f * scale,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    // Mid glow
    g.setColour (accentColour.withAlpha (0.65f));
    g.strokePath (ring, juce::PathStrokeType (2.6f * scale,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    // Bright ring core
    g.setColour (accentColour);
    g.strokePath (ring, juce::PathStrokeType (1.6f * scale,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    // Inner s-curve, slightly dimmer
    g.setColour (accentColour.withAlpha (0.85f));
    g.strokePath (inner, juce::PathStrokeType (1.2f * scale,
                                               juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

} // namespace manifold::ui

#include "EngineButton.h"
#include "EngineGlyphs.h"
#include "ManifoldLookAndFeel.h"

namespace manifold::ui
{

EngineButton::EngineButton (int engineIndex, const juce::String& displayName, juce::Colour h)
    : juce::Button (displayName), engineIdx (engineIndex), name (displayName), hue (h)
{
    setClickingTogglesState (true);
}

void EngineButton::paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    juce::ignoreUnused (isButtonDown);
    using LF = ManifoldLookAndFeel;

    const auto bounds = getLocalBounds().toFloat();
    const float radius = 10.0f;
    const bool  on     = getToggleState();

    // ── Card background — subtle gradient. Active: hue-tinted bottom glow.
    {
        juce::ColourGradient grad (LF::plate2(),
                                   bounds.getCentreX(), bounds.getY(),
                                   LF::plate1(),
                                   bounds.getCentreX(), bounds.getBottom(),
                                   false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bounds, radius);

        if (on)
        {
            // Hue-tinted bottom radial glow inside the card
            juce::ColourGradient bg (hue.withAlpha (0.20f),
                                     bounds.getCentreX(), bounds.getBottom(),
                                     hue.withAlpha (0.0f),
                                     bounds.getCentreX(), bounds.getY(),
                                     true);
            g.setGradientFill (bg);
            g.fillRoundedRectangle (bounds, radius);
        }
    }

    // Border
    g.setColour (on ? hue.withAlpha (0.85f)
                    : LF::plateLine().withAlpha (isMouseOver ? 0.9f : 0.55f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), radius, on ? 1.4f : 1.0f);

    // Outer glow when active — drawn outside the rounded rect.
    if (on)
    {
        for (int i = 1; i <= 3; ++i)
        {
            g.setColour (hue.withAlpha (0.10f / (float) i));
            g.drawRoundedRectangle (bounds.expanded ((float) i * 1.2f),
                                    radius + (float) i * 1.2f,
                                    1.5f);
        }
    }

    // Active badge dot, top-right
    {
        const float dotR = 3.0f;
        const float bx = bounds.getRight() - 8.0f;
        const float by = bounds.getY()     + 8.0f;
        g.setColour (on ? hue : LF::plate3());
        g.fillEllipse (bx - dotR, by - dotR, dotR * 2.0f, dotR * 2.0f);
        if (on)
        {
            g.setColour (hue.withAlpha (0.4f));
            g.fillEllipse (bx - dotR * 2.0f, by - dotR * 2.0f, dotR * 4.0f, dotR * 4.0f);
        }
    }

    // Glyph area — upper region. 48×48 viewBox scaled to fit.
    auto glyphArea = bounds.reduced (10.0f, 8.0f).removeFromTop (bounds.getHeight() * 0.62f);
    const float gScale = juce::jmin (glyphArea.getWidth(), glyphArea.getHeight()) / 48.0f;
    const float gx = glyphArea.getCentreX() - 24.0f * gScale;
    const float gy = glyphArea.getCentreY() - 24.0f * gScale;
    auto vb = juce::AffineTransform::scale (gScale).translated (gx, gy);

    auto strokeColour = on ? hue : LF::ink3();
    juce::PathStrokeType stroke (1.2f * juce::jmax (1.0f, gScale * 0.85f),
                                 juce::PathStrokeType::curved,
                                 juce::PathStrokeType::rounded);

    auto strokePath = [&] (juce::Path p)
    {
        p.applyTransform (vb);
        if (on)
        {
            // Layered glow when active
            g.setColour (hue.withAlpha (0.30f));
            g.strokePath (p, juce::PathStrokeType (stroke.getStrokeThickness() + 2.4f,
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        }
        g.setColour (strokeColour);
        g.strokePath (p, stroke);
    };

    auto fillPoint = [&] (juce::Point<float> pt, float r)
    {
        const float x = pt.x * gScale + gx;
        const float y = pt.y * gScale + gy;
        g.setColour (strokeColour);
        g.fillEllipse (x - r * gScale, y - r * gScale, r * 2.0f * gScale, r * 2.0f * gScale);
    };

    // Dispatch on engine index — must match kChaosEngineParamIds order in Params.h:
    // 0=Lorenz, 1=Thomas, 2=Rossler, 3=Chua, 4=Aizawa, 5=Henon
    switch (engineIdx)
    {
        case 0: strokePath (glyphs::lorenz());  break;
        case 1: strokePath (glyphs::thomas());  break;
        case 2: strokePath (glyphs::rossler()); break;
        case 3: strokePath (glyphs::chua());    break;
        case 4: strokePath (glyphs::aizawa());  break;
        case 5:
        {
            // Hénon: dashed arc + discrete points
            auto arc = glyphs::henonArc();
            arc.applyTransform (vb);
            // Dashed stroke approximation: draw with dashed PathStrokeType
            const float dashes[] = { 1.2f * gScale, 3.0f * gScale };
            juce::PathStrokeType dashStroke (1.0f * juce::jmax (1.0f, gScale * 0.85f),
                                             juce::PathStrokeType::curved,
                                             juce::PathStrokeType::butt);
            g.setColour (strokeColour.withAlpha (0.5f));
            juce::Path dashed;
            dashStroke.createDashedStroke (dashed, arc, dashes, 2);
            g.fillPath (dashed);

            for (auto pt : glyphs::henonPoints())
                fillPoint (pt, 1.6f);
            break;
        }
        default: break;
    }

    // Label below
    g.setColour (on ? LF::ink1() : LF::ink3());
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    auto labelArea = bounds.reduced (4.0f, 4.0f).removeFromBottom (16.0f);
    g.drawFittedText (name, labelArea.toNearestInt(), juce::Justification::centred, 1);
}

} // namespace manifold::ui

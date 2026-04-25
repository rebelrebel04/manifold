#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace manifold::ui::glyphs
{

// All glyphs live in a 48×48 viewBox. The host EngineButton is responsible for
// scaling them and applying stroke/colour. Paths are stored as functions so we
// don't depend on static-init ordering.

// ── Lorenz: classic two-lobed butterfly (ported from manifold-atoms.jsx) ──
inline juce::Path lorenz()
{
    juce::Path p;
    // Left wing
    p.startNewSubPath (8.0f, 30.0f);
    p.cubicTo ( 6.0f, 22.0f, 14.0f, 12.0f, 22.0f, 18.0f);
    p.cubicTo (28.0f, 22.0f, 28.0f, 28.0f, 22.0f, 32.0f);
    p.cubicTo (16.0f, 36.0f,  6.0f, 36.0f,  8.0f, 30.0f);
    p.closeSubPath();
    // Right wing
    p.startNewSubPath (40.0f, 30.0f);
    p.cubicTo (42.0f, 22.0f, 34.0f, 12.0f, 26.0f, 18.0f);
    p.cubicTo (20.0f, 22.0f, 20.0f, 28.0f, 26.0f, 32.0f);
    p.cubicTo (32.0f, 36.0f, 42.0f, 36.0f, 40.0f, 30.0f);
    p.closeSubPath();
    // Inner crossover (suggesting orbital flow)
    p.startNewSubPath (14.0f, 22.0f);
    p.cubicTo (18.0f, 24.0f, 22.0f, 24.0f, 24.0f, 26.0f);
    p.startNewSubPath (24.0f, 26.0f);
    p.cubicTo (26.0f, 24.0f, 30.0f, 24.0f, 34.0f, 22.0f);
    return p;
}

// ── Rössler: concentric circles + tangent escape (ported) ──
inline juce::Path rossler()
{
    juce::Path p;
    p.addEllipse (14.0f, 14.0f, 20.0f, 20.0f);   // outer r=10
    p.addEllipse (18.0f, 18.0f, 12.0f, 12.0f);   // mid r=6
    p.addEllipse (22.0f, 22.0f,  4.0f,  4.0f);   // inner r=2
    p.startNewSubPath (34.0f, 24.0f);
    p.lineTo (42.0f, 30.0f);
    return p;
}

// ── Hénon: dashed arc with discrete points (ported) ──
inline juce::Path henonArc()
{
    juce::Path p;
    p.startNewSubPath (6.0f, 36.0f);
    p.cubicTo (14.0f, 18.0f, 28.0f, 14.0f, 42.0f, 24.0f);
    return p;
}
inline std::array<juce::Point<float>, 6> henonPoints()
{
    return { juce::Point<float> { 8.0f,  34.0f },
             juce::Point<float> {14.0f,  28.0f },
             juce::Point<float> {22.0f,  20.0f },
             juce::Point<float> {30.0f,  18.0f },
             juce::Point<float> {38.0f,  22.0f },
             juce::Point<float> {42.0f,  28.0f } };
}

// ── Thomas: three overlapping orbital ellipses, rotated 0/60/120°.
// Reads as "cyclically symmetric, no dominant plane" — Thomas's key character.
inline juce::Path thomas()
{
    juce::Path p;
    // Each orbit is a wide ellipse 28×10, rotated about (24,24).
    constexpr int kOrbits = 3;
    const float cx = 24.0f, cy = 24.0f;
    const float w = 28.0f, h = 10.0f;
    for (int i = 0; i < kOrbits; ++i)
    {
        juce::Path orbit;
        orbit.addEllipse (cx - w * 0.5f, cy - h * 0.5f, w, h);
        const float deg = 60.0f * (float) i;
        const float rad = juce::degreesToRadians (deg);
        orbit.applyTransform (juce::AffineTransform::rotation (rad, cx, cy));
        p.addPath (orbit);
    }
    return p;
}

// ── Chua: stacked double-scroll spirals (interlocked).
// Two side-by-side spiralling ellipses with a connecting curve at the centre.
inline juce::Path chua()
{
    juce::Path p;
    // Outer left spiral lobes
    p.addEllipse (8.0f,  17.0f, 18.0f, 14.0f);
    p.addEllipse (12.0f, 19.0f, 12.0f, 10.0f);
    p.addEllipse (15.5f, 21.0f,  6.0f,  6.0f);
    // Outer right spiral lobes
    p.addEllipse (22.0f, 17.0f, 18.0f, 14.0f);
    p.addEllipse (24.0f, 19.0f, 12.0f, 10.0f);
    p.addEllipse (26.5f, 21.0f,  6.0f,  6.0f);
    // Connector through the saddle
    p.startNewSubPath (16.0f, 24.0f);
    p.quadraticTo (24.0f, 16.0f, 32.0f, 24.0f);
    return p;
}

// ── Aizawa: stacked-petal flower (multi-petal vertical orbit).
// Five petals radiating from a central core, rendered as ellipses rotated around
// (24,24). Suggests the petal-like Aizawa attractor structure.
inline juce::Path aizawa()
{
    juce::Path p;
    constexpr int kPetals = 5;
    const float cx = 24.0f, cy = 24.0f;
    const float petalW = 6.0f, petalH = 18.0f;
    for (int i = 0; i < kPetals; ++i)
    {
        juce::Path petal;
        petal.addEllipse (cx - petalW * 0.5f, cy - petalH * 0.5f, petalW, petalH);
        const float deg = (360.0f / (float) kPetals) * (float) i;
        const float rad = juce::degreesToRadians (deg);
        petal.applyTransform (juce::AffineTransform::rotation (rad, cx, cy));
        p.addPath (petal);
    }
    // Central pip
    p.addEllipse (22.5f, 22.5f, 3.0f, 3.0f);
    return p;
}

} // namespace manifold::ui::glyphs

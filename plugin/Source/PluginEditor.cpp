#include "PluginEditor.h"
#include "Params.h"
#include "ui/FilterDiagram.h"
#include "ui/ShapeDiagram.h"

namespace
{
    constexpr int kEditorWidth        = 920;
    constexpr int kEditorHeight       = 760;

    constexpr int kHeaderH       = 48;
    constexpr int kEngineRowH    = 64;
    constexpr int kMacroRowH     = 138;
    constexpr int kRoutingRowH   = 64;
    constexpr int kSecondaryRowH = 110;
    constexpr int kFooterH       = 28;

    constexpr const char* kBuildTag = "v0.16.0-dev";
}

// ─────────────────────────────────────────────────────────────────
// LabeledKnob — uses ManifoldLookAndFeel's matte rotary; per-knob accent set later.
// ─────────────────────────────────────────────────────────────────
ManifoldEditor::LabeledKnob::LabeledKnob (const juce::String& l, juce::Slider::SliderStyle style)
    : label (l)
{
    slider.setSliderStyle (style);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    addAndMakeVisible (slider);
}

void ManifoldEditor::LabeledKnob::resized()
{
    auto b = getLocalBounds();
    b.removeFromTop (16);  // label header
    slider.setBounds (b.reduced (4));
}

void ManifoldEditor::LabeledKnob::paint (juce::Graphics& g)
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    g.setColour (LF::ink3());
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawFittedText (label, getLocalBounds().removeFromTop (16),
                      juce::Justification::centred, 1);
}

// ─────────────────────────────────────────────────────────────────
// PickerCard — clickable card showing the current Shape/Filter selection.
// Phase 4 makes them visually live; Phase 6/7 will wire click → drawer open.
// ─────────────────────────────────────────────────────────────────
ManifoldEditor::PickerCard::PickerCard (const juce::String& kind)
    : juce::TextButton (kind), kindLabel (kind), optionName (juce::String())
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ManifoldEditor::PickerCard::setOptionName (const juce::String& n)
{
    optionName = n;
    repaint();
}

void ManifoldEditor::PickerCard::paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    juce::ignoreUnused (isButtonDown);
    using LF = manifold::ui::ManifoldLookAndFeel;
    const auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient bg (LF::plate2(), bounds.getCentreX(), bounds.getY(),
                             LF::plate1(), bounds.getCentreX(), bounds.getBottom(), false);
    g.setGradientFill (bg);
    g.fillRoundedRectangle (bounds, 8.0f);

    g.setColour (isMouseOver ? LF::plateLineStrong() : LF::plateLine());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, 1.0f);

    auto inner = bounds.reduced (12.0f, 8.0f);

    // Label (kind) — small uppercase mono on the left.
    g.setColour (LF::ink4());
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    auto kindBox = inner.removeFromLeft (54.0f);
    g.drawFittedText (kindLabel, kindBox.toNearestInt(),
                      juce::Justification::centredLeft, 1);

    // Arrow chevron on the right.
    auto arrowBox = inner.removeFromRight (18.0f);
    g.setColour (LF::ink4());
    {
        juce::Path p;
        p.startNewSubPath (arrowBox.getCentreX() - 4.0f, arrowBox.getCentreY() - 2.0f);
        p.lineTo          (arrowBox.getCentreX(),         arrowBox.getCentreY() + 2.0f);
        p.lineTo          (arrowBox.getCentreX() + 4.0f, arrowBox.getCentreY() - 2.0f);
        g.strokePath (p, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
    }

    // Current selection name — large UI font, centred between label and arrow.
    g.setColour (LF::ink1());
    g.setFont (juce::FontOptions (14.0f, juce::Font::plain));
    g.drawFittedText (optionName, inner.toNearestInt(),
                      juce::Justification::centredLeft, 1);
}

// ─────────────────────────────────────────────────────────────────
// BypassOverlay — transparent component layered above everything else.
// Non-interactive; paints a translucent veil + status badge when bypassed.
// ─────────────────────────────────────────────────────────────────
ManifoldEditor::BypassOverlay::BypassOverlay()
{
    setInterceptsMouseClicks (false, false);   // clicks pass through to controls below
    setVisible (false);
}

void ManifoldEditor::BypassOverlay::paint (juce::Graphics& g)
{
    // Translucent dark veil — no banner, the bypass button LED communicates state.
    g.setColour (juce::Colour (0xff080810).withAlpha (0.68f));
    g.fillRect (getLocalBounds().toFloat());
}

// ─────────────────────────────────────────────────────────────────
// BypassButton — LED-style pill, on = active (DSP running), off = bypassed.
// ─────────────────────────────────────────────────────────────────
ManifoldEditor::BypassButton::BypassButton()
    : juce::Button ("Bypass")
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

void ManifoldEditor::BypassButton::paintButton (juce::Graphics& g, bool isMouseOver, bool /*isButtonDown*/)
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    const bool active = ! getToggleState();   // toggle = bypassed; visual semantic is reversed
    const auto bounds = getLocalBounds().toFloat();

    g.setColour (LF::plate0());
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (isMouseOver ? LF::plateLineStrong() : LF::plateLine());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    // LED dot
    auto inner = bounds.reduced (8.0f, 5.0f);
    auto ledArea = inner.removeFromLeft (14.0f);
    const float ledR = 4.5f;
    const auto ledCentre = ledArea.getCentre();
    if (active)
    {
        const auto led = juce::Colour { 0xffb59cff };
        // glow
        g.setColour (led.withAlpha (0.5f));
        g.fillEllipse (ledCentre.x - ledR * 1.6f, ledCentre.y - ledR * 1.6f,
                       ledR * 3.2f, ledR * 3.2f);
        g.setColour (led);
        g.fillEllipse (ledCentre.x - ledR, ledCentre.y - ledR, ledR * 2.0f, ledR * 2.0f);
    }
    else
    {
        g.setColour (LF::plate3());
        g.fillEllipse (ledCentre.x - ledR, ledCentre.y - ledR, ledR * 2.0f, ledR * 2.0f);
    }

    g.setColour (active ? LF::ink1() : LF::ink3());
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawFittedText (active ? "ACTIVE" : "BYPASS",
                      inner.toNearestInt(),
                      juce::Justification::centredLeft, 1);
}

// ─────────────────────────────────────────────────────────────────
// SignalPathToggle — flips id::routing between Shape→Filter and Filter→Shape.
// ─────────────────────────────────────────────────────────────────
ManifoldEditor::SignalPathToggle::SignalPathToggle (juce::AudioProcessorValueTreeState& a)
    : juce::Button ("SignalPath"),
      apvts (a),
      param (a.getParameter (manifold::params::id::routing)),
      attach (*param, [this] (float newValue)
      {
          shapeFirst = newValue < 0.5f;
          repaint();
      })
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    attach.sendInitialUpdate();
}

void ManifoldEditor::SignalPathToggle::clicked()
{
    // Flip via the parameter attachment so host automation/preset listeners stay coherent.
    attach.setValueAsCompleteGesture (shapeFirst ? 1.0f : 0.0f);
}

void ManifoldEditor::SignalPathToggle::paintButton (juce::Graphics& g, bool isMouseOver, bool /*down*/)
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    const auto bounds = getLocalBounds().toFloat();

    g.setColour (LF::plate0());
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour (isMouseOver ? LF::plateLineStrong() : LF::plateLine());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    g.setColour (LF::ink3());
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));

    // Labels are fixed anchors — SHAPE always left, FILTER always right.
    // Only the arrow direction changes to show signal flow.
    const auto leftLabel  = juce::String ("SHAPE");
    const auto rightLabel = juce::String ("FILTER");

    auto inner = bounds.reduced (10.0f, 4.0f);
    const auto leftBox  = inner.removeFromLeft  (44.0f);
    const auto rightBox = inner.removeFromRight (44.0f);

    g.drawFittedText (leftLabel,  leftBox.toNearestInt(),
                      juce::Justification::centred, 1);
    g.drawFittedText (rightLabel, rightBox.toNearestInt(),
                      juce::Justification::centred, 1);

    // Arrow in the middle — direction tracks shapeFirst so it always points toward the filter.
    auto arrowBox = inner;
    const float ay = arrowBox.getCentreY();
    juce::Path arrow;
    if (shapeFirst)
    {
        // Points right: Shape → Filter
        arrow.startNewSubPath (arrowBox.getX() + 4.0f,        ay);
        arrow.lineTo          (arrowBox.getRight() - 8.0f,     ay);
        arrow.startNewSubPath (arrowBox.getRight() - 12.0f,    ay - 4.0f);
        arrow.lineTo          (arrowBox.getRight() - 8.0f,     ay);
        arrow.lineTo          (arrowBox.getRight() - 12.0f,    ay + 4.0f);
    }
    else
    {
        // Points left: Filter → Shape
        arrow.startNewSubPath (arrowBox.getRight() - 4.0f,     ay);
        arrow.lineTo          (arrowBox.getX() + 8.0f,         ay);
        arrow.startNewSubPath (arrowBox.getX() + 12.0f,        ay - 4.0f);
        arrow.lineTo          (arrowBox.getX() + 8.0f,         ay);
        arrow.lineTo          (arrowBox.getX() + 12.0f,        ay + 4.0f);
    }

    const auto arrowColour = juce::Colour { 0xffb59cff };
    g.setColour (arrowColour);
    g.strokePath (arrow, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

// ─────────────────────────────────────────────────────────────────
// PresetRow — Phase 9 unified pill with icon buttons (matches design).
// ─────────────────────────────────────────────────────────────────

// Icon button: paints a juce::Path glyph centred in its bounds, scaled from
// the design's source viewBox to the actual button size.
ManifoldEditor::PresetRow::IconButton::IconButton (const juce::String& tooltip,
                                                   juce::Path g, float sw)
    : juce::Button (tooltip), glyph (std::move (g)), stroke (sw)
{
    setTooltip (tooltip);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    // Size deduced from the path's bounding box, since each glyph in the
    // design uses its own viewBox (14×14 burger, 10×10 chevrons, 12×12 save).
    glyphSize = juce::jmax (glyph.getBounds().getRight(), glyph.getBounds().getBottom());
}

void ManifoldEditor::PresetRow::IconButton::paintButton (juce::Graphics& g,
                                                          bool isMouseOver, bool isButtonDown)
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    const auto bounds = getLocalBounds().toFloat();
    juce::ignoreUnused (isButtonDown);

    // Hairline divider (1px wide on the LEFT of every button except the first
    // and last — those get their dividers at the start/end of the pill instead).
    // We paint the dividers from PresetRow::paint() so the math sees the full
    // row; here we just render the icon.

    const auto col = isMouseOver ? LF::ink1() : LF::ink2();
    g.setColour (col);

    // Centre the glyph by translating to the button centre and offsetting by
    // half the source viewBox.
    juce::Graphics::ScopedSaveState ss (g);
    g.addTransform (juce::AffineTransform::translation (bounds.getCentreX() - glyphSize * 0.5f,
                                                         bounds.getCentreY() - glyphSize * 0.5f));
    g.strokePath (glyph, juce::PathStrokeType (stroke,
                                                juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));
}

namespace
{
    // ── Icon paths from manifold-frame.jsx ────────────────────────────────
    juce::Path makeBurgerIcon()
    {
        juce::Path p;
        p.startNewSubPath ( 2.0f,  4.0f); p.lineTo (12.0f,  4.0f);
        p.startNewSubPath ( 2.0f,  7.0f); p.lineTo (12.0f,  7.0f);
        p.startNewSubPath ( 2.0f, 10.0f); p.lineTo (12.0f, 10.0f);
        return p;
    }
    juce::Path makePrevIcon()
    {
        juce::Path p;
        p.startNewSubPath (6.5f, 2.0f);
        p.lineTo          (3.0f, 5.0f);
        p.lineTo          (6.5f, 8.0f);
        return p;
    }
    juce::Path makeNextIcon()
    {
        juce::Path p;
        p.startNewSubPath (3.5f, 2.0f);
        p.lineTo          (7.0f, 5.0f);
        p.lineTo          (3.5f, 8.0f);
        return p;
    }
    juce::Path makeSaveIcon()
    {
        // Floppy disk: outer notched rectangle + inner label tab.
        juce::Path p;
        p.startNewSubPath ( 2.0f,  2.0f);
        p.lineTo          ( 2.0f, 10.0f);
        p.lineTo          (10.0f, 10.0f);
        p.lineTo          (10.0f,  4.0f);
        p.lineTo          ( 8.0f,  2.0f);
        p.lineTo          ( 2.0f,  2.0f);
        p.closeSubPath();
        p.startNewSubPath ( 4.0f,  2.0f);
        p.lineTo          ( 4.0f,  5.0f);
        p.lineTo          ( 8.0f,  5.0f);
        p.lineTo          ( 8.0f,  2.0f);
        return p;
    }
}

ManifoldEditor::PresetRow::PresetRow (manifold::preset::PresetManager& pm)
    : presetManager (pm),
      browseBtn ("Browse presets",         makeBurgerIcon(), 1.5f),
      prevBtn   ("Previous preset",        makePrevIcon(),   1.5f),
      nextBtn   ("Next preset",            makeNextIcon(),   1.5f),
      saveBtn   ("Save current as preset", makeSaveIcon(),   1.3f)
{
    addAndMakeVisible (browseBtn);
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);
    addAndMakeVisible (saveBtn);

    setMouseCursor (juce::MouseCursor::PointingHandCursor);

    refreshLabel();

    // Browse / Save are wired from ManifoldEditor (they need the preset panel
    // reference). Prev/Next are self-contained.
    prevBtn.onClick = [this] { presetManager.prev(); };
    nextBtn.onClick = [this] { presetManager.next(); };
}

void ManifoldEditor::PresetRow::mouseUp (const juce::MouseEvent& e)
{
    // Click on the pill body (not on a child IconButton) acts like clicking
    // the browse icon. We fire on mouseUp — and only if the release is still
    // within bounds — so the user can drag-out to cancel, mirroring how a
    // standard juce::Button behaves. Firing on mouseDown would race the
    // editor's global click-outside watcher and flicker the panel.
    if (! getLocalBounds().contains (e.getPosition())) return;
    if (browseBtn.onClick) browseBtn.onClick();
}

void ManifoldEditor::PresetRow::refreshLabel()
{
    isModified_ = presetManager.isModified();
    if (isModified_)
    {
        categoryUpper_ = {};
        presetName_    = "- modified -";
    }
    else
    {
        const int idx = presetManager.getCurrentIndex();
        categoryUpper_ = presetManager.getCategory (idx).toUpperCase();
        presetName_    = presetManager.getName     (idx);
    }
    repaint();
}

void ManifoldEditor::PresetRow::paint (juce::Graphics& g)
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    const auto bounds = getLocalBounds().toFloat();

    // Pill container: dark plate0 fill, rounded, plateLineStrong border.
    g.setColour (LF::plate0());
    g.fillRoundedRectangle (bounds, 6.0f);

    // Subtle inset highlight along the top edge.
    g.setColour (LF::plateLine().withAlpha (0.30f));
    g.drawHorizontalLine (1, bounds.getX() + 4.0f, bounds.getRight() - 4.0f);

    g.setColour (LF::plateLineStrong());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.0f);

    // Inner hairlines between sections (4 dividers — flanking each button group).
    auto vline = [&] (float x)
    {
        g.setColour (LF::plateLine());
        g.fillRect (juce::Rectangle<float> (x, bounds.getY() + 3.0f, 1.0f, bounds.getHeight() - 6.0f));
    };
    vline ((float) browseBtn.getRight());
    vline ((float) prevBtn  .getRight());
    vline ((float) nextBtn  .getX() - 1.0f);
    vline ((float) saveBtn  .getX() - 1.0f);

    // Centre name region — drawn between prevBtn.right and nextBtn.x.
    const int nameLeft  = prevBtn.getRight() + 8;
    const int nameRight = nextBtn.getX()    - 8;
    if (nameRight <= nameLeft) return;

    const auto nameRect = juce::Rectangle<int> (nameLeft, 0, nameRight - nameLeft, getHeight());

    if (isModified_)
    {
        g.setColour (LF::ink2());
        g.setFont (juce::FontOptions (11.0f, juce::Font::italic));
        g.drawFittedText (presetName_, nameRect, juce::Justification::centred, 1);
        return;
    }

    // "WOBBLE  ·  Resin Drop" — measure each part to centre the whole composite.
    constexpr juce::uint32 kAccentArgb = 0xffb59cff;

    const auto tagFont  = juce::FontOptions (9.0f,  juce::Font::bold);
    const auto nameFont = juce::FontOptions (11.0f, juce::Font::plain);

    juce::GlyphArrangement gaTag, gaName;
    gaTag .addLineOfText (juce::Font (tagFont),  categoryUpper_, 0.0f, 0.0f);
    gaName.addLineOfText (juce::Font (nameFont), presetName_,    0.0f, 0.0f);

    // Approximate letter-spacing on the tag (CSS letter-spacing 0.15em ≈ 1.4px per letter).
    const float tagW   = gaTag .getBoundingBox (0, gaTag .getNumGlyphs(), true).getWidth()
                          + 1.4f * (float) (categoryUpper_.length() - 1);
    const float nameW  = gaName.getBoundingBox (0, gaName.getNumGlyphs(), true).getWidth();
    const float dotPad = 14.0f;     // total spacing for " · " incl. dot
    const float total  = tagW + dotPad + nameW;

    float x = (float) nameRect.getX() + (nameRect.getWidth() - total) * 0.5f;
    if (x < (float) nameRect.getX()) x = (float) nameRect.getX();   // never overflow left

    // Tag (uppercase mono accent).
    g.setColour (juce::Colour (kAccentArgb));
    g.setFont   (tagFont);
    {
        // Manual letter-spacing render.
        float tx = x;
        for (int i = 0; i < categoryUpper_.length(); ++i)
        {
            const auto ch = categoryUpper_.substring (i, i + 1);
            g.drawText (ch, juce::Rectangle<float> (tx, 0.0f, 20.0f, (float) getHeight()),
                        juce::Justification::centredLeft, false);
            juce::GlyphArrangement gc;
            gc.addLineOfText (juce::Font (tagFont), ch, 0.0f, 0.0f);
            tx += gc.getBoundingBox (0, gc.getNumGlyphs(), true).getWidth() + 1.4f;
        }
    }

    // Painted dot (avoid Unicode font-fallback risk).
    const float dotCX = x + tagW + dotPad * 0.5f;
    const float dotCY = (float) getHeight() * 0.5f;
    g.setColour (LF::ink3());
    g.fillEllipse (dotCX - 1.5f, dotCY - 1.5f, 3.0f, 3.0f);

    // Name (mono ink1).
    g.setColour (LF::ink1());
    g.setFont   (nameFont);
    g.drawFittedText (presetName_,
                      juce::Rectangle<int> ((int) (x + tagW + dotPad), 0,
                                            (int) std::ceil (nameW) + 4, getHeight()),
                      juce::Justification::centredLeft, 1);
}

void ManifoldEditor::PresetRow::resized()
{
    auto b = getLocalBounds();
    const int btnW = 22;   // matches design — 22px wide button slots

    browseBtn.setBounds (b.removeFromLeft  (btnW));
    prevBtn  .setBounds (b.removeFromLeft  (btnW));
    saveBtn  .setBounds (b.removeFromRight (btnW));
    nextBtn  .setBounds (b.removeFromRight (btnW));
    // Centre name region is the remaining `b` — drawn in paint().
}

// ─────────────────────────────────────────────────────────────────
// ManifoldEditor
// ─────────────────────────────────────────────────────────────────
ManifoldEditor::ManifoldEditor (ManifoldProcessor& p)
    : AudioProcessorEditor (&p), processor (p), portrait (p)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (wordmark);
    addAndMakeVisible (portrait);
    addAndMakeVisible (intensityKnob);
    addAndMakeVisible (speedKnob);
    addAndMakeVisible (warmthKnob);

    addAndMakeVisible (driveKnob);
    addAndMakeVisible (cutoffKnob);
    addAndMakeVisible (resonanceKnob);
    addAndMakeVisible (morphKnob);
    addAndMakeVisible (outputKnob);

    addAndMakeVisible (shapePicker);
    addAndMakeVisible (filterPicker);

    // Header preset row + preset panel — both need the PresetManager.
    presetRow   = std::make_unique<PresetRow> (processor.getPresetManager());
    presetPanel = std::make_unique<manifold::ui::PresetPanel> (processor.getPresetManager());
    addAndMakeVisible (*presetRow);

    // Wire browseBtn and saveBtn here (not in PresetRow ctor) so they can reference the panel.
    presetRow->browseBtn.onClick = [this] { openPresetPanel(); };
    presetRow->saveBtn.onClick   = [this] { openPresetPanel (true); };

    // Header label + panel repaint on any preset event:
    //   onPresetLoaded   → after load() (incl. construction, save, prev/next)
    //   onPresetDeleted  → after deleteUserPreset (may put us in "modified" state)
    //   onFavoritesChanged → toggle favorite (panel only — header unchanged)
    processor.getPresetManager().onPresetLoaded = [this]
    {
        presetRow->refreshLabel();
        if (presetPanel && presetPanel->isVisible())
            presetPanel->repaint();
    };
    processor.getPresetManager().onPresetDeleted = [this]
    {
        presetRow->refreshLabel();
        if (presetPanel && presetPanel->isVisible())
            presetPanel->repaint();
    };
    processor.getPresetManager().onFavoritesChanged = [this]
    {
        if (presetPanel && presetPanel->isVisible())
            presetPanel->repaint();
    };

    addAndMakeVisible (bypassButton);
    bypassButton.setTooltip ("Master bypass - when active, the input is passed through dry (no DSP).");

    intensityKnob .slider.setTooltip ("Depth of chaos motion (Lorenz rho). Low = gentle wobble; high = aggressive regime-switching.");
    speedKnob     .slider.setTooltip ("Orbit rate of the chaos core. Low = slow drift; high = frantic motion.");
    warmthKnob    .slider.setTooltip ("Smooths the chaos mod signals AND tilts the output spectrum warmer. 0 = raw motion, flat tone; higher = slower motion, darker tone.");
    driveKnob     .slider.setTooltip ("Gain into the shaper. Each SHAPE curve responds differently; chaos modulates drive on top.");
    cutoffKnob    .slider.setTooltip ("Filter cutoff (Hz). Repurposed as pitch for Tuned Comb.");
    resonanceKnob .slider.setTooltip ("Filter resonance / feedback. High values self-oscillate - chaos can push it there.");
    morphKnob     .slider.setTooltip ("SVF: LP <-> BP <-> HP blend. Moog/Diode: 4-pole <-> 2-pole tonal slope. Comb: feedback-path brightness.");
    outputKnob    .slider.setTooltip ("Post-filter output gain (dB).");
    shapePicker   .setTooltip ("Transfer curve (waveshaper). Click to choose.");
    filterPicker  .setTooltip ("Resonant filter model. Click to choose.");

    // Engine toggle row — six EngineButtons with per-engine hue and glyph.
    using namespace manifold::params;
    static constexpr juce::uint32 kEngineHues[kNumChaosEngines] = {
        0xffb59cff,  // Lorenz  — violet
        0xff7ad6ff,  // Thomas  — cyan
        0xff8effa0,  // Rossler — green
        0xffffb870,  // Chua    — orange
        0xff8aa8ff,  // Aizawa  — blue
        0xffff8eb6,  // Henon   — magenta/pink
    };
    engineButtons.reserve (kNumChaosEngines);
    for (int i = 0; i < kNumChaosEngines; ++i)
    {
        auto btn = std::make_unique<manifold::ui::EngineButton> (
            i, juce::String (kChaosEngineNames[i]), juce::Colour (kEngineHues[i]));
        btn->onStateChange = [this] { updateBlendEnabled(); };
        addAndMakeVisible (*btn);
        engineButtons.push_back (std::move (btn));
    }

    blendKnob.slider.setTooltip ("Hybridisation depth across active chaos engines. 0 = primary engine only; 1 = full equal-weight mix. Disabled when fewer than 2 engines are active.");
    addAndMakeVisible (blendKnob);

    auto& apvts = processor.getAPVTS();

    // Slider attachments.
    intensityAttach = std::make_unique<SliderAttachment> (apvts, id::intensity, intensityKnob.slider);
    speedAttach     = std::make_unique<SliderAttachment> (apvts, id::speed,     speedKnob.slider);
    warmthAttach    = std::make_unique<SliderAttachment> (apvts, id::warmth,    warmthKnob.slider);
    driveAttach     = std::make_unique<SliderAttachment> (apvts, id::drive,     driveKnob.slider);
    cutoffAttach    = std::make_unique<SliderAttachment> (apvts, id::cutoff,    cutoffKnob.slider);
    resonanceAttach = std::make_unique<SliderAttachment> (apvts, id::resonance, resonanceKnob.slider);
    morphAttach     = std::make_unique<SliderAttachment> (apvts, id::morph,     morphKnob.slider);
    outputAttach    = std::make_unique<SliderAttachment> (apvts, id::output,    outputKnob.slider);
    blendAttach     = std::make_unique<SliderAttachment> (apvts, id::blend,     blendKnob.slider);
    bypassAttach    = std::make_unique<ButtonAttachment> (apvts, id::bypass,    bypassButton);

    // Watch bypass param — show/hide the overlay component (which sits above all children).
    if (auto* bp = apvts.getParameter (id::bypass))
    {
        bypassParamWatch = std::make_unique<juce::ParameterAttachment> (*bp,
            [this] (float v)
            {
                isBypassed = v > 0.5f;
                bypassOverlay.setVisible (isBypassed);
            });
        bypassParamWatch->sendInitialUpdate();
    }

    // Overlay added last (topmost z-order) but NOT made visible — visibility is driven
    // entirely by the param watch above so the default non-bypassed state is correct.
    addChildComponent (bypassOverlay);

    for (int i = 0; i < kNumChaosEngines; ++i)
        engineAttachments[(size_t) i] = std::make_unique<ButtonAttachment> (
            apvts, kChaosEngineParamIds[i], *engineButtons[(size_t) i]);
    updateBlendEnabled();

    // Picker cards: subscribe to the underlying choice params and refresh display.
    auto* filterParam = apvts.getParameter (id::filterType);
    auto* shaperParam = apvts.getParameter (id::shaperType);
    filterPickerAttach = std::make_unique<juce::ParameterAttachment> (
        *filterParam,
        [this, filterParam] (float)
        {
            const int idx = (int) filterParam->convertFrom0to1 (filterParam->getValue());
            const auto choices = manifold::params::filterTypeChoices();
            filterPicker.setOptionName (choices[juce::jlimit (0, choices.size() - 1, idx)]);
        });
    shaperPickerAttach = std::make_unique<juce::ParameterAttachment> (
        *shaperParam,
        [this, shaperParam] (float)
        {
            const int idx = (int) shaperParam->convertFrom0to1 (shaperParam->getValue());
            const auto choices = manifold::params::shaperTypeChoices();
            shapePicker.setOptionName (choices[juce::jlimit (0, choices.size() - 1, idx)]);
        });
    filterPickerAttach->sendInitialUpdate();
    shaperPickerAttach->sendInitialUpdate();

    // Signal-path toggle — built once apvts is reachable.
    sigPath = std::make_unique<SignalPathToggle> (apvts);
    addAndMakeVisible (*sigPath);

    // Preset panel (left-side) + shape/filter drawers (right-side). All three are
    // mutually exclusive — openPresetPanel / openShapeDrawer / openFilterDrawer
    // each close the other two before opening.
    addChildComponent (*presetPanel);
    addChildComponent (shapeDrawer);
    addChildComponent (filterDrawer);
    shapePicker.onClick  = [this] { openShapeDrawer(); };
    filterPicker.onClick = [this] { openFilterDrawer(); };

    // Global mouse watcher — closes any open drawer on click-outside.
    // wantsEventsForAllNestedComponents=true means every child click also notifies us;
    // the original target still receives its own event normally.
    drawerMouseWatcher = std::make_unique<DrawerMouseWatcher> (*this);
    addMouseListener (drawerMouseWatcher.get(), true);

    setSize (kEditorWidth, kEditorHeight);
}

ManifoldEditor::~ManifoldEditor()
{
    removeMouseListener (drawerMouseWatcher.get());
    setLookAndFeel (nullptr);
}

void ManifoldEditor::DrawerMouseWatcher::mouseDown (const juce::MouseEvent& e)
{
    // Convert the click position into the editor's local coordinate space.
    const auto pos = e.getEventRelativeTo (&editor).getPosition();

    if (editor.shapeDrawer.isVisible() && ! editor.shapeDrawer.getBounds().contains (pos))
        editor.shapeDrawer.hide();

    if (editor.filterDrawer.isVisible() && ! editor.filterDrawer.getBounds().contains (pos))
        editor.filterDrawer.hide();

    if (editor.presetPanel && editor.presetPanel->isVisible()
        && ! editor.presetPanel->getBounds().contains (pos))
        editor.presetPanel->hide();
}

void ManifoldEditor::updateBlendEnabled()
{
    int active = 0;
    for (auto& b : engineButtons)
        if (b->getToggleState()) ++active;

    const bool enabled = active >= 2;
    blendKnob.setEnabled (enabled);
    blendKnob.setAlpha   (enabled ? 1.0f : 0.4f);
}

void ManifoldEditor::refreshPickerNames()
{
    if (filterPickerAttach) filterPickerAttach->sendInitialUpdate();
    if (shaperPickerAttach) shaperPickerAttach->sendInitialUpdate();
}

void ManifoldEditor::openShapeDrawer()
{
    // Toggle: clicking the same button while open closes the drawer.
    if (shapeDrawer.isVisible()) { shapeDrawer.hide(); return; }

    // Mutually exclusive with the left panel and filter drawer.
    if (presetPanel && presetPanel->isVisible()) presetPanel->hide();
    if (filterDrawer.isVisible())                filterDrawer.hide();

    using ST  = manifold::params::ShaperType;
    using Opt = manifold::ui::PickerDrawer::Option;

    static const struct { ST type; const char* name; const char* desc; } kDefs[] =
    {
        { ST::Fold,      "Wavefolder",
          "Reflects signal peaks back into the waveform. Bright, inharmonic character with dense upper partials. Best at high drive on bass." },
        { ST::SoftClip,  "Soft Clip",
          "Tanh saturation. Gentle odd harmonics that round transients warmly. The safe choice for bus processing at any drive level." },
        { ST::HardClip,  "Hard Clip",
          "Brick-wall limiting. Generates a full harmonic series instantly. Harsh and aggressive - pairs well with low resonance settings." },
        { ST::Rectify,   "Full-Wave Rectify",
          "Inverts the negative half and centers the output. Strong even harmonics and a subtle octave-up character at moderate drive." },
        { ST::Sine,      "Sine Shaper",
          "Maps signal through a sine function. Smooth and musical at low drive; wraps into complex folds as peaks exceed the curve." },
        { ST::TubeAsym,  "Tube Asymmetric",
          "Positive half saturates harder than negative. Adds even harmonics, DC motion, and a classic tube-circuit warmth." },
        { ST::ChebyT3,   "Chebyshev T3",
          "Third-degree polynomial. Adds a pure third harmonic for synthetic, brassy timbres with minimal aliasing artifacts." },
        { ST::ChebyT5,   "Chebyshev T5",
          "Fifth-degree polynomial. Dense harmonic content up to the 5th partial. Complex and metallic - chaotic modulation thrives here." },
    };

    std::vector<Opt> options;
    for (auto& d : kDefs)
    {
        Opt opt;
        opt.name        = d.name;
        opt.description = d.desc;
        const ST type   = d.type;
        opt.drawDiagram = [type] (juce::Graphics& g, juce::Rectangle<float> r)
        {
            juce::Graphics::ScopedSaveState save (g);
            g.setOrigin (r.getTopLeft().toInt());
            manifold::ui::ShapeDiagram diag (type);
            diag.setBounds ({ 0, 0, (int) r.getWidth(), (int) r.getHeight() });
            diag.paint (g);
        };
        options.push_back (std::move (opt));
    }

    auto& apvts    = processor.getAPVTS();
    const int currentIdx = (int) apvts.getRawParameterValue (manifold::params::id::shaperType)->load();

    shapeDrawer.configure (
        "Shape",
        std::move (options),
        [this] (int idx)
        {
            if (auto* p = processor.getAPVTS().getParameter (manifold::params::id::shaperType))
            {
                const float norm = p->convertTo0to1 ((float) idx);
                p->beginChangeGesture();
                p->setValueNotifyingHost (norm);
                p->endChangeGesture();
            }
        },
        currentIdx);

    // Restore full portrait bounds before re-narrowing (handles switch from panel).
    resized();
    portrait.setBounds (portrait.getBounds().withRight (shapeDrawer.getX()));
    shapeDrawer.onHide = [this]
    {
        if (! filterDrawer.isVisible() && (! presetPanel || ! presetPanel->isVisible()))
            resized();
    };
    shapeDrawer.show();
}

void ManifoldEditor::openFilterDrawer()
{
    // Toggle: clicking the same button while open closes the drawer.
    if (filterDrawer.isVisible()) { filterDrawer.hide(); return; }

    // Mutually exclusive with the left panel and shape drawer.
    if (presetPanel && presetPanel->isVisible()) presetPanel->hide();
    if (shapeDrawer.isVisible())                 shapeDrawer.hide();

    using FT  = manifold::params::FilterType;
    using Opt = manifold::ui::PickerDrawer::Option;

    // Build one Option per filter type. The drawDiagram lambda captures a FilterDiagram
    // by value (small object) and paints it into the supplied rect each frame.
    static const struct { FT type; const char* name; const char* desc; } kDefs[] =
    {
        { FT::SVF,         "SVF",          "State-variable filter. LP/BP/HP blend via MORPH. Smooth self-oscillation, musical and versatile." },
        { FT::MoogLadder,  "Moog Ladder",  "4-pole TPT ladder. MORPH blends 4-pole to 2-pole slope. Warm low-end resonance, classic decay." },
        { FT::DiodeLadder, "Diode Ladder", "TB-303-flavored ladder. Asymmetric feedback for a sharper, edgier resonance peak and 2nd-harmonic bite." },
        { FT::TunedComb,   "Tuned Comb",   "Karplus-Strong resonator. CUTOFF sets pitch; MORPH controls feedback brightness. Metallic sustain." },
    };

    std::vector<Opt> options;
    for (auto& d : kDefs)
    {
        Opt opt;
        opt.name        = d.name;
        opt.description = d.desc;
        const FT type   = d.type;
        opt.drawDiagram = [type] (juce::Graphics& g, juce::Rectangle<float> r)
        {
            // Translate g so the diagram paints into r (which is in drawer-space).
            juce::Graphics::ScopedSaveState save (g);
            g.setOrigin (r.getTopLeft().toInt());
            manifold::ui::FilterDiagram diag (type);
            diag.setBounds ({ 0, 0, (int) r.getWidth(), (int) r.getHeight() });
            diag.paint (g);
        };
        options.push_back (std::move (opt));
    }

    // Current selection from param.
    auto& apvts = processor.getAPVTS();
    const int currentIdx = (int) apvts.getRawParameterValue (manifold::params::id::filterType)->load();

    filterDrawer.configure (
        "Filter",
        std::move (options),
        [this] (int idx)
        {
            if (auto* p = processor.getAPVTS().getParameter (manifold::params::id::filterType))
            {
                const float norm = p->convertTo0to1 ((float) idx);
                p->beginChangeGesture();
                p->setValueNotifyingHost (norm);
                p->endChangeGesture();
            }
        },
        currentIdx);

    // JUCE OpenGL composites the GL surface on top of any CPU sibling within the
    // portrait's bounds, regardless of z-order. Rather than hiding the portrait
    // (which tears down the GL context and causes a flicker on restore), we shrink
    // the portrait's right edge to exactly where the drawer begins. The GL surface
    // then only covers the left portion, the drawer occupies the right — no overlap,
    // no compositing conflict, no context teardown. resized() restores full bounds
    // once the drawer has fully animated out and hidden itself.
    resized();
    portrait.setBounds (portrait.getBounds().withRight (filterDrawer.getX()));
    filterDrawer.onHide = [this]
    {
        if (! shapeDrawer.isVisible() && (! presetPanel || ! presetPanel->isVisible()))
            resized();
    };
    filterDrawer.show();
}

void ManifoldEditor::openPresetPanel (bool saveMode)
{
    if (! presetPanel) return;

    // Panel already open: switch mode rather than toggling visibility.
    // SAVE click while panel is visible → enter save form; BROWSE click → close.
    if (presetPanel->isVisible())
    {
        if (saveMode)
            presetPanel->enterSaveMode();
        else
            presetPanel->hide();
        return;
    }

    // Mutually exclusive with the right drawers.
    if (shapeDrawer.isVisible())  shapeDrawer.hide();
    if (filterDrawer.isVisible()) filterDrawer.hide();

    // Restore layout to full bounds before narrowing from the left.
    resized();
    portrait.setBounds (portrait.getBounds().withLeft (presetPanel->getRight()));

    presetPanel->onHide = [this]
    {
        if (! shapeDrawer.isVisible() && ! filterDrawer.isVisible())
            resized();
    };
    presetPanel->show (saveMode);
}

void ManifoldEditor::paint (juce::Graphics& g)
{
    using LF = manifold::ui::ManifoldLookAndFeel;
    g.fillAll (LF::plate1());

    // Header strip — gradient + bottom hairline.
    auto header = getLocalBounds().removeFromTop (kHeaderH);
    {
        juce::ColourGradient hg (LF::plate2(),
                                 header.getX(), (float) header.getY(),
                                 LF::plate1(),
                                 header.getX(), (float) header.getBottom(),
                                 false);
        g.setGradientFill (hg);
        g.fillRect (header);
    }
    g.setColour (LF::plateLine());
    g.drawHorizontalLine (header.getBottom() - 1, 0.0f, (float) getWidth());

    g.setColour (LF::ink4());
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    auto vbox = header.withTrimmedLeft (220).withTrimmedRight (240);
    g.drawFittedText (kBuildTag, vbox, juce::Justification::centredLeft, 1);

    // Footer strip — just a divider + placeholder labels for now (no live data).
    auto footer = getLocalBounds().removeFromBottom (kFooterH);
    g.setColour (LF::plate0());
    g.fillRect (footer);
    g.setColour (LF::plateLine());
    g.drawHorizontalLine (footer.getY(), 0.0f, (float) getWidth());

    g.setColour (LF::ink4());
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawFittedText ("MANIFOLD", footer.reduced (16, 0).withWidth (120),
                      juce::Justification::centredLeft, 1);
    g.drawFittedText (kBuildTag, footer.reduced (16, 0),
                      juce::Justification::centredRight, 1);

    // Bypass visual state is handled by BypassOverlay (a child component on top of
    // everything, including the OpenGL portrait). Nothing to draw here for bypass.
}

void ManifoldEditor::resized()
{
    using namespace manifold::params;
    auto b = getLocalBounds();

    // Header
    auto header = b.removeFromTop (kHeaderH);
    wordmark.setBounds (header.removeFromLeft (210).reduced (16, 8));
    bypassButton.setBounds (header.removeFromRight (104).reduced (12, 10));

    // Preset row fills the middle of the header — between wordmark and bypass.
    if (presetRow != nullptr)
        presetRow->setBounds (header.reduced (12, 10));

    // Footer
    b.removeFromBottom (kFooterH);

    // Secondary knob row (5 across) at the bottom.
    auto secondary = b.removeFromBottom (kSecondaryRowH);
    {
        const int n = 5;
        const int w = secondary.getWidth() / n;
        driveKnob    .setBounds (secondary.removeFromLeft (w).reduced (10, 4));
        cutoffKnob   .setBounds (secondary.removeFromLeft (w).reduced (10, 4));
        resonanceKnob.setBounds (secondary.removeFromLeft (w).reduced (10, 4));
        morphKnob    .setBounds (secondary.removeFromLeft (w).reduced (10, 4));
        outputKnob   .setBounds (secondary.reduced (10, 4));
    }

    // Routing strip (Shape | sigpath | Filter).
    auto routing = b.removeFromBottom (kRoutingRowH).reduced (22, 10);
    const int sigPathW = 150;
    const int gap = 14;
    const int pickerW = (routing.getWidth() - sigPathW - 2 * gap) / 2;
    shapePicker.setBounds  (routing.removeFromLeft (pickerW));
    routing.removeFromLeft (gap);
    if (sigPath != nullptr)
        sigPath->setBounds (routing.removeFromLeft (sigPathW).reduced (0, 8));
    routing.removeFromLeft (gap);
    filterPicker.setBounds (routing);

    // Macros (3 big knobs).
    auto macros = b.removeFromBottom (kMacroRowH);
    {
        const int w = macros.getWidth() / 3;
        intensityKnob.setBounds (macros.removeFromLeft (w).reduced (16, 6));
        speedKnob    .setBounds (macros.removeFromLeft (w).reduced (16, 6));
        warmthKnob   .setBounds (macros.reduced (16, 6));
    }

    // Engine row + BLEND knob (between header and portrait).
    auto engRow = b.removeFromTop (kEngineRowH);
    const int blendW = 76;
    auto blendArea = engRow.removeFromRight (blendW);
    blendKnob.setBounds (blendArea.reduced (4, 4));

    const int btnW = engRow.getWidth() / kNumChaosEngines;
    for (int i = 0; i < (int) engineButtons.size(); ++i)
        engineButtons[(size_t) i]->setBounds (engRow.removeFromLeft (btnW).reduced (4, 6));

    // Portrait fills whatever's left between the engine row and the macro row.
    portrait.setBounds (b);

    // Overlay covers everything below the header (bypass button stays accessible).
    bypassOverlay.setBounds (getLocalBounds().withTrimmedTop (kHeaderH).withTrimmedBottom (kFooterH));

    // Shape + Filter drawers share the same right-panel slot.
    const int drawerW = (getWidth() * 46) / 100;
    const juce::Rectangle<int> drawerBounds (getWidth() - drawerW, kHeaderH,
                                             drawerW, getHeight() - kHeaderH - kFooterH);
    shapeDrawer .setBounds (drawerBounds);
    filterDrawer.setBounds (drawerBounds);

    // Preset panel — left-side overlay below the header.
    const int panelW = (getWidth() * 42) / 100;
    if (presetPanel)
        presetPanel->setBounds (0, kHeaderH, panelW, getHeight() - kHeaderH - kFooterH);
}

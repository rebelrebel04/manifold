#include "PluginEditor.h"
#include "Params.h"

namespace
{
    constexpr int kEditorWidth          = 720;
    constexpr int kEditorHeightCollapsed = 480;
    constexpr int kAdvancedPanelHeight  = 170;
    constexpr int kEditorHeightExpanded = kEditorHeightCollapsed + kAdvancedPanelHeight;

    constexpr const char* kBuildTag = "v0.6.0";

    const juce::Colour kBg          (0xff05050a);
    const juce::Colour kBgPanel     (0xff0d0d18);
    const juce::Colour kViolet      (0xff8a4dff);
    const juce::Colour kVioletDim   (0xff5a2fbf);
    const juce::Colour kTextBright  (0xffe8e6ff);
    const juce::Colour kTextDim     (0xff7a7a8c);
}

ManifoldEditor::LabeledKnob::LabeledKnob (const juce::String& l, juce::Slider::SliderStyle style)
    : label (l)
{
    slider.setSliderStyle (style);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    slider.setColour (juce::Slider::rotarySliderFillColourId,    kViolet);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, kVioletDim.withAlpha (0.4f));
    slider.setColour (juce::Slider::thumbColourId,               kTextBright);
    slider.setColour (juce::Slider::textBoxTextColourId,         kTextBright);
    slider.setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
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
    g.setColour (kTextDim);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.drawFittedText (label, getLocalBounds().removeFromTop (16),
                      juce::Justification::centred, 1);
}

ManifoldEditor::AdvancedPanel::AdvancedPanel()
{
    filterLabel.setText ("FILTER", juce::dontSendNotification);
    filterLabel.setColour (juce::Label::textColourId, kTextDim);
    filterLabel.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    filterLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (filterLabel);

    filterCombo.addItemList (manifold::params::filterTypeChoices(), 1);
    filterCombo.setColour (juce::ComboBox::backgroundColourId, kBg);
    filterCombo.setColour (juce::ComboBox::outlineColourId,    kVioletDim.withAlpha (0.5f));
    filterCombo.setColour (juce::ComboBox::textColourId,       kTextBright);
    filterCombo.setColour (juce::ComboBox::arrowColourId,      kViolet);
    addAndMakeVisible (filterCombo);

    addAndMakeVisible (drive);
    addAndMakeVisible (cutoff);
    addAndMakeVisible (resonance);
    addAndMakeVisible (morph);
    addAndMakeVisible (output);
}

void ManifoldEditor::AdvancedPanel::resized()
{
    auto b = getLocalBounds().reduced (8, 6);

    auto top = b.removeFromTop (28);
    filterLabel.setBounds (top.removeFromLeft (60));
    top.removeFromLeft (6);
    filterCombo.setBounds (top.removeFromLeft (200));

    b.removeFromTop (6);
    const int n = 5;
    const int w = b.getWidth() / n;
    drive    .setBounds (b.removeFromLeft (w));
    cutoff   .setBounds (b.removeFromLeft (w));
    resonance.setBounds (b.removeFromLeft (w));
    morph    .setBounds (b.removeFromLeft (w));
    output   .setBounds (b);
}

void ManifoldEditor::AdvancedPanel::paint (juce::Graphics& g)
{
    g.fillAll (kBgPanel);
    g.setColour (kVioletDim.withAlpha (0.25f));
    g.drawHorizontalLine (0, 0.0f, (float) getWidth());
}

ManifoldEditor::ManifoldEditor (ManifoldProcessor& p)
    : AudioProcessorEditor (&p), processor (p), portrait (p)
{
    addAndMakeVisible (portrait);
    addAndMakeVisible (intensityKnob);
    addAndMakeVisible (speedKnob);
    addAndMakeVisible (warmthKnob);

    advancedToggle.setClickingTogglesState (true);
    advancedToggle.setColour (juce::TextButton::buttonColourId,   kBgPanel);
    advancedToggle.setColour (juce::TextButton::buttonOnColourId, kVioletDim);
    advancedToggle.setColour (juce::TextButton::textColourOffId,  kTextDim);
    advancedToggle.setColour (juce::TextButton::textColourOnId,   kTextBright);
    advancedToggle.onClick = [this]
    {
        advancedOpen = advancedToggle.getToggleState();
        advancedPanel.setVisible (advancedOpen);
        setSize (kEditorWidth, advancedOpen ? kEditorHeightExpanded : kEditorHeightCollapsed);
    };
    addAndMakeVisible (advancedToggle);

    advancedPanel.setVisible (false);
    addChildComponent (advancedPanel);

    beaconsToggle.setClickingTogglesState (true);
    beaconsToggle.setColour (juce::TextButton::buttonColourId,   kBgPanel);
    beaconsToggle.setColour (juce::TextButton::buttonOnColourId, kVioletDim);
    beaconsToggle.setColour (juce::TextButton::textColourOffId,  kTextDim);
    beaconsToggle.setColour (juce::TextButton::textColourOnId,   kTextBright);
    beaconsToggle.onClick = [this] { portrait.setBeaconsVisible (beaconsToggle.getToggleState()); };
    addAndMakeVisible (beaconsToggle);

    intensityKnob       .slider.setTooltip ("Depth of chaos motion (Lorenz rho). Low = gentle wobble; high = aggressive regime-switching.");
    speedKnob           .slider.setTooltip ("Orbit rate of the chaos core. Low = slow drift; high = frantic motion.");
    warmthKnob          .slider.setTooltip ("Smooths the chaos mod signals AND tilts the output spectrum warmer. 0 = raw motion, flat tone; higher = slower motion, darker tone.");
    advancedPanel.drive .slider.setTooltip ("Wavefolder gain before the filter. Adds harmonic content; chaos adds more on top.");
    advancedPanel.cutoff.slider.setTooltip ("Filter cutoff (Hz). Repurposed as pitch for Tuned Comb.");
    advancedPanel.resonance.slider.setTooltip ("Filter resonance / feedback. High values self-oscillate — chaos can push it there.");
    advancedPanel.morph .slider.setTooltip ("SVF: LP <-> BP <-> HP blend. Moog/Diode: 4-pole <-> 2-pole tonal slope. Comb: feedback-path brightness.");
    advancedPanel.output.slider.setTooltip ("Post-filter output gain (dB).");
    advancedPanel.filterCombo.setTooltip ("Resonant filter model. Each type has its own character — Morph behaves differently per filter.");

    auto& apvts = processor.getAPVTS();
    using namespace manifold::params;
    intensityAttach = std::make_unique<SliderAttachment> (apvts, id::intensity, intensityKnob.slider);
    speedAttach     = std::make_unique<SliderAttachment> (apvts, id::speed,     speedKnob.slider);
    warmthAttach    = std::make_unique<SliderAttachment> (apvts, id::warmth,    warmthKnob.slider);
    driveAttach     = std::make_unique<SliderAttachment> (apvts, id::drive,     advancedPanel.drive.slider);
    cutoffAttach    = std::make_unique<SliderAttachment> (apvts, id::cutoff,    advancedPanel.cutoff.slider);
    resonanceAttach = std::make_unique<SliderAttachment> (apvts, id::resonance, advancedPanel.resonance.slider);
    morphAttach     = std::make_unique<SliderAttachment> (apvts, id::morph,     advancedPanel.morph.slider);
    outputAttach    = std::make_unique<SliderAttachment> (apvts, id::output,    advancedPanel.output.slider);
    filterAttach    = std::make_unique<ComboBoxAttachment> (apvts, id::filterType, advancedPanel.filterCombo);

    setSize (kEditorWidth, kEditorHeightCollapsed);
}

void ManifoldEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    auto headerArea = getLocalBounds().removeFromTop (44);
    g.setColour (kViolet);
    g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    g.drawFittedText ("MANIFOLD", headerArea.reduced (16, 0), juce::Justification::left, 1);

    g.setColour (kTextDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawFittedText (kBuildTag, headerArea.reduced (130, 0),
                      juce::Justification::centredLeft, 1);
}

void ManifoldEditor::resized()
{
    auto b = getLocalBounds();

    // Header strip + advanced toggle live at the top.
    auto header = b.removeFromTop (44);
    advancedToggle.setBounds (header.removeFromRight (110).reduced (8, 10));
    beaconsToggle .setBounds (header.removeFromRight (100).reduced (8, 10));

    if (advancedOpen)
        advancedPanel.setBounds (b.removeFromBottom (kAdvancedPanelHeight));

    auto knobRow = b.removeFromBottom (140);
    portrait.setBounds (b);

    const int knobW = knobRow.getWidth() / 3;
    intensityKnob.setBounds (knobRow.removeFromLeft (knobW).reduced (8));
    speedKnob    .setBounds (knobRow.removeFromLeft (knobW).reduced (8));
    warmthKnob   .setBounds (knobRow.reduced (8));
}

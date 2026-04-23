#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "ui/PhasePortrait.h"

class ManifoldEditor : public juce::AudioProcessorEditor
{
public:
    explicit ManifoldEditor (ManifoldProcessor&);
    ~ManifoldEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct LabeledKnob : public juce::Component
    {
        LabeledKnob (const juce::String& label, juce::Slider::SliderStyle style);
        void resized() override;
        void paint (juce::Graphics&) override;

        juce::Slider slider;
        juce::String label;
    };

    struct AdvancedPanel : public juce::Component
    {
        AdvancedPanel();
        void resized() override;
        void paint (juce::Graphics&) override;

        juce::Label    filterLabel;
        juce::ComboBox filterCombo;

        LabeledKnob drive    { "DRIVE",     juce::Slider::RotaryHorizontalVerticalDrag };
        LabeledKnob cutoff   { "CUTOFF",    juce::Slider::RotaryHorizontalVerticalDrag };
        LabeledKnob resonance{ "RESONANCE", juce::Slider::RotaryHorizontalVerticalDrag };
        LabeledKnob morph    { "MORPH",     juce::Slider::RotaryHorizontalVerticalDrag };
        LabeledKnob output   { "OUTPUT",    juce::Slider::RotaryHorizontalVerticalDrag };
    };

    ManifoldProcessor& processor;

    manifold::ui::PhasePortrait portrait;

    LabeledKnob intensityKnob { "INTENSITY", juce::Slider::RotaryHorizontalVerticalDrag };
    LabeledKnob speedKnob     { "SPEED",     juce::Slider::RotaryHorizontalVerticalDrag };
    LabeledKnob warmthKnob    { "WARMTH",    juce::Slider::RotaryHorizontalVerticalDrag };

    juce::TextButton advancedToggle { "ADVANCED" };
    juce::TextButton beaconsToggle  { "BEACONS" };
    AdvancedPanel advancedPanel;
    bool advancedOpen = false;

    juce::TooltipWindow tooltipWindow { this, 550 };

    std::unique_ptr<SliderAttachment> intensityAttach, speedAttach, warmthAttach;
    std::unique_ptr<SliderAttachment>   driveAttach, cutoffAttach, resonanceAttach, morphAttach, outputAttach;
    std::unique_ptr<ComboBoxAttachment> filterAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ManifoldEditor)
};

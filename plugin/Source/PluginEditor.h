#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "Params.h"
#include "ui/PhasePortrait.h"
#include "ui/ManifoldLookAndFeel.h"
#include "ui/ManifoldWordmark.h"
#include "ui/EngineButton.h"

class ManifoldEditor : public juce::AudioProcessorEditor
{
public:
    explicit ManifoldEditor (ManifoldProcessor&);
    ~ManifoldEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void updateBlendEnabled();
    void refreshPickerNames();

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct LabeledKnob : public juce::Component
    {
        LabeledKnob (const juce::String& label, juce::Slider::SliderStyle style);
        void resized() override;
        void paint (juce::Graphics&) override;

        juce::Slider slider;
        juce::String label;
    };

    // Picker card — a click-to-open card showing the current Shape or Filter selection.
    // In Phase 4 it's purely visual + clickable; Phase 6/7 wires the drawers.
    struct PickerCard : public juce::TextButton
    {
        PickerCard (const juce::String& kind);
        void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
        void setOptionName (const juce::String& n);

        juce::String kindLabel;       // "SHAPE" / "FILTER"
        juce::String optionName;      // current selection display
    };

    // Master bypass switch — LED-style pill with "Active" / "Bypass" label.
    struct BypassButton : public juce::Button
    {
        BypassButton();
        void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
    };

    // Fullscreen dim overlay — sits on top of all children (added last) and paints a
    // translucent veil + badge when bypassed. Non-interactive so controls remain reachable.
    struct BypassOverlay : public juce::Component
    {
        BypassOverlay();
        void paint (juce::Graphics&) override;
    };

    // Signal-path toggle — flips id::routing between Shape→Filter and Filter→Shape.
    // Owns a juce::ParameterAttachment so external param changes (host automation,
    // preset load) refresh the visual.
    struct SignalPathToggle : public juce::Button
    {
        SignalPathToggle (juce::AudioProcessorValueTreeState& apvts);
        void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
        void clicked() override;

        juce::AudioProcessorValueTreeState& apvts;
        juce::RangedAudioParameter*         param;
        juce::ParameterAttachment           attach;
        bool   shapeFirst = true;
    };

    ManifoldProcessor& processor;

    manifold::ui::PhasePortrait portrait;

    // Macro tier
    LabeledKnob intensityKnob { "INTENSITY", juce::Slider::RotaryHorizontalVerticalDrag };
    LabeledKnob speedKnob     { "SPEED",     juce::Slider::RotaryHorizontalVerticalDrag };
    LabeledKnob warmthKnob    { "WARMTH",    juce::Slider::RotaryHorizontalVerticalDrag };

    // Routing strip
    PickerCard       shapePicker  { "SHAPE" };
    PickerCard       filterPicker { "FILTER" };
    std::unique_ptr<SignalPathToggle> sigPath;  // built once apvts is reachable

    // Header bypass switch
    BypassButton bypassButton;
    std::unique_ptr<ButtonAttachment> bypassAttach;
    std::unique_ptr<juce::ParameterAttachment> bypassParamWatch;
    bool isBypassed = false;
    BypassOverlay bypassOverlay;

    // Secondary knob tier
    LabeledKnob driveKnob     { "DRIVE",     juce::Slider::RotaryHorizontalVerticalDrag };
    LabeledKnob cutoffKnob    { "CUTOFF",    juce::Slider::RotaryHorizontalVerticalDrag };
    LabeledKnob resonanceKnob { "RESONANCE", juce::Slider::RotaryHorizontalVerticalDrag };
    LabeledKnob morphKnob     { "MORPH",     juce::Slider::RotaryHorizontalVerticalDrag };
    LabeledKnob outputKnob    { "OUTPUT",    juce::Slider::RotaryHorizontalVerticalDrag };

    juce::TooltipWindow tooltipWindow { this, 550 };
    manifold::ui::ManifoldLookAndFeel lookAndFeel;
    manifold::ui::ManifoldWordmark    wordmark;

    std::unique_ptr<SliderAttachment> intensityAttach, speedAttach, warmthAttach;
    std::unique_ptr<SliderAttachment> driveAttach, cutoffAttach, resonanceAttach, morphAttach, outputAttach;

    // Listeners that keep picker card display in sync with the underlying choice params.
    std::unique_ptr<juce::ParameterAttachment> filterPickerAttach, shaperPickerAttach;

    // Engine toggle row (above portrait).
    std::vector<std::unique_ptr<manifold::ui::EngineButton>> engineButtons;
    std::array<std::unique_ptr<ButtonAttachment>, manifold::params::kNumChaosEngines> engineAttachments;

    // BLEND knob — only meaningful when 2+ engines are active.
    LabeledKnob blendKnob { "BLEND", juce::Slider::RotaryHorizontalVerticalDrag };
    std::unique_ptr<SliderAttachment> blendAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ManifoldEditor)
};

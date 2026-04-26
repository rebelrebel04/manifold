#include "PresetManager.h"

namespace manifold::preset
{

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvts)
    : apvts_ (apvts)
{
    const auto& presets = getFactoryPresets();
    presetTrees_.reserve (presets.size());
    for (const auto& p : presets)
        presetTrees_.push_back (buildTree (p));

    // Load preset 0 so the plugin opens in a defined state. The host's
    // setStateInformation() (called later, if a saved DAW state exists) may
    // replace this with project-specific values.
    if (! presetTrees_.empty())
        load (0);
}

int PresetManager::getCount() const noexcept
{
    return (int) presetTrees_.size();
}

juce::String PresetManager::getName (int idx) const
{
    if (idx < 0 || idx >= getCount()) return {};
    return getFactoryPresets()[(size_t) idx].name;
}

juce::String PresetManager::getCategory (int idx) const
{
    if (idx < 0 || idx >= getCount()) return {};
    return getFactoryPresets()[(size_t) idx].category;
}

juce::String PresetManager::getPrimaryEngine (int idx) const
{
    if (idx < 0 || idx >= getCount()) return {};
    return getFactoryPresets()[(size_t) idx].primaryEngine;
}

bool PresetManager::isUserPreset (int) const noexcept
{
    // Phase 8b.1: all presets are factory. User-preset filesystem lands in 8b.2.
    return false;
}

juce::String PresetManager::getDisplayName (int idx) const
{
    if (idx < 0 || idx >= getCount()) return {};
    return getCategory (idx) + " - " + getName (idx);
}

void PresetManager::load (int idx)
{
    if (idx < 0 || idx >= getCount()) return;
    currentIndex_ = idx;
    // replaceState() propagates to all attached SliderAttachments / ButtonAttachments
    // / ParameterAttachments automatically — knobs, engine LEDs, picker cards,
    // bypass overlay, blend-knob enable state all auto-update.
    apvts_.replaceState (presetTrees_[(size_t) idx]);
    if (onPresetLoaded) onPresetLoaded();
}

void PresetManager::next()
{
    if (getCount() == 0) return;
    load ((currentIndex_ + 1) % getCount());
}

void PresetManager::prev()
{
    if (getCount() == 0) return;
    load ((currentIndex_ - 1 + getCount()) % getCount());
}

juce::ValueTree PresetManager::buildTree (const FactoryPreset& p) const
{
    // Root tag must match apvts.state.getType() ("Manifold") for replaceState()
    // to accept the tree. Each child is <PARAM id="..." value="..."/> matching
    // APVTS's native serialisation shape.
    juce::ValueTree tree (apvts_.state.getType());
    for (const auto& [id, value] : p.params)
    {
        juce::ValueTree paramNode ("PARAM");
        paramNode.setProperty ("id",    id,    nullptr);
        paramNode.setProperty ("value", value, nullptr);
        tree.appendChild (paramNode, nullptr);
    }
    return tree;
}

} // namespace manifold::preset

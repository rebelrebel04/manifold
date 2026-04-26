#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <vector>
#include "FactoryPresets.h"

namespace manifold::preset
{

// Phase 8a — minimal preset manager.
// Owns the factory preset list (compiled in via FactoryPresets.h), tracks the
// current selection, and applies presets to the host APVTS via replaceState().
//
// Phase 8a scope: factory presets only. No save flow, no user-preset filesystem.
// Those land in Phase 8b alongside the left-side preset panel UI.
//
// Notes on state lifecycle:
//   - On construction, loads preset 0 so the plugin opens in a defined state.
//   - The host's setStateInformation() (called *after* construction) may then
//     overwrite that with a DAW-saved state. In that case the manager's
//     currentIndex stays at 0 even though the actual params no longer match.
//     This is a known 8a limitation; 8b will track preset identity through
//     host save/restore by storing the current preset index in the APVTS state.
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& apvts);

    int          getCount()           const noexcept;
    juce::String getName          (int idx) const;
    juce::String getCategory      (int idx) const;
    juce::String getPrimaryEngine (int idx) const;
    // Returns "Category - Name" for the header label.
    juce::String getDisplayName   (int idx) const;
    // True for user-saved presets (idx >= factoryCount_).
    bool         isUserPreset     (int idx) const noexcept;

    // Stable identifier for this preset across renames / restarts.
    //   "factory:<slug>"  for compiled-in presets (slug is FactoryPreset::id)
    //   "user:<name>"     for user-saved presets
    // Used as the canonical key for favorites and any other ref-by-identity
    // feature. Empty string for invalid indices.
    juce::String getStableId      (int idx) const;

    int  getCurrentIndex() const noexcept { return currentIndex_; }
    void load (int idx);
    void next();
    void prev();

    // ── User-preset save API ────────────────────────────────────────────────
    // Check whether a user preset with this name (case-insensitive) already exists.
    bool userPresetNameExists (const juce::String& name) const;

    // Write current APVTS state to disk under ~/Library/Application Support/Manifold/presets/.
    // If overwrite=false and the file exists the call is a no-op (caller should check first).
    void saveUserPreset (const juce::String& name,
                         const juce::String& category,
                         bool                overwrite);

    // Snapshot of the current APVTS state for display in the save form.
    struct StateInfo
    {
        juce::String primaryEngine;
        juce::String shaperName;
        juce::String filterName;
    };
    StateInfo getCurrentStateInfo() const;

    // Platform preset directory: ~/Library/Application Support/Manifold/presets/
    static juce::File getUserPresetDir();

    // Fired (on the message thread) after a successful load.
    std::function<void()> onPresetLoaded;

private:
    juce::ValueTree buildTree (const FactoryPreset& p) const;
    void            scanUserPresets();

    juce::AudioProcessorValueTreeState& apvts_;
    // Combined vector: [factory presets 0..factoryCount_-1 | user presets factoryCount_..N]
    std::vector<juce::ValueTree> presetTrees_;
    int                          factoryCount_  = 0;
    int                          currentIndex_  = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};

} // namespace manifold::preset

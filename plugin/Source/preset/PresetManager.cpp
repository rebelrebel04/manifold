#include "PresetManager.h"
#include "../Params.h"

namespace manifold::preset
{

// ─── Internal helpers ─────────────────────────────────────────────────────────

// Strip characters that are unsafe in filenames and append the extension.
static juce::String sanitizeFilename (const juce::String& name)
{
    return name.retainCharacters (
               "abcdefghijklmnopqrstuvwxyz"
               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
               "0123456789 -_")
               .trim() + ".mfld";
}

// ─── Construction ─────────────────────────────────────────────────────────────

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvts)
    : apvts_ (apvts)
{
    // Build factory preset trees.
    const auto& fps = getFactoryPresets();
    presetTrees_.reserve (fps.size());
    for (const auto& p : fps)
        presetTrees_.push_back (buildTree (p));

    factoryCount_ = (int) presetTrees_.size();

    // Load user presets from disk (no-op if directory doesn't exist yet).
    scanUserPresets();

    // Load favorites from disk (no-op if file doesn't exist).
    loadFavoritesFromDisk();

    // Start in a defined state — the host's setStateInformation() may overwrite this.
    if (! presetTrees_.empty())
        load (0);
}

// ─── Queries ──────────────────────────────────────────────────────────────────

int PresetManager::getCount() const noexcept
{
    return (int) presetTrees_.size();
}

juce::String PresetManager::getName (int idx) const
{
    if (idx < 0 || idx >= getCount()) return {};
    if (idx < factoryCount_)
        return getFactoryPresets()[(size_t) idx].name;
    return presetTrees_[(size_t) idx].getProperty ("name").toString();
}

juce::String PresetManager::getCategory (int idx) const
{
    if (idx < 0 || idx >= getCount()) return {};
    if (idx < factoryCount_)
        return getFactoryPresets()[(size_t) idx].category;
    return presetTrees_[(size_t) idx].getProperty ("category").toString();
}

juce::String PresetManager::getPrimaryEngine (int idx) const
{
    if (idx < 0 || idx >= getCount()) return {};
    if (idx < factoryCount_)
        return getFactoryPresets()[(size_t) idx].primaryEngine;
    return presetTrees_[(size_t) idx].getProperty ("primaryEngine").toString();
}

juce::String PresetManager::getDisplayName (int idx) const
{
    if (idx < 0 || idx >= getCount()) return {};
    return getCategory (idx) + " - " + getName (idx);
}

bool PresetManager::isUserPreset (int idx) const noexcept
{
    return idx >= factoryCount_ && idx < getCount();
}

juce::String PresetManager::getStableId (int idx) const
{
    if (idx < 0 || idx >= getCount()) return {};
    if (idx < factoryCount_)
        return "factory:" + getFactoryPresets()[(size_t) idx].id;
    return "user:" + getName (idx);
}

// ─── Navigation ───────────────────────────────────────────────────────────────

void PresetManager::load (int idx)
{
    if (idx < 0 || idx >= getCount()) return;
    currentIndex_ = idx;
    apvts_.replaceState (presetTrees_[(size_t) idx]);
    if (onPresetLoaded) onPresetLoaded();
}

void PresetManager::next()
{
    if (getCount() == 0) return;
    if (currentIndex_ < 0) { load (0); return; }
    load ((currentIndex_ + 1) % getCount());
}

void PresetManager::prev()
{
    if (getCount() == 0) return;
    if (currentIndex_ < 0) { load (getCount() - 1); return; }
    load ((currentIndex_ - 1 + getCount()) % getCount());
}

// ─── User preset save/load ────────────────────────────────────────────────────

juce::File PresetManager::getUserPresetDir()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Manifold/presets");
}

void PresetManager::scanUserPresets()
{
    // Drop any previously loaded user presets while keeping factory ones intact.
    presetTrees_.resize ((size_t) factoryCount_);

    const auto dir = getUserPresetDir();
    if (! dir.isDirectory()) return;

    auto files = dir.findChildFiles (juce::File::findFiles, false, "*.mfld");
    files.sort();   // alphabetical by path

    for (const auto& f : files)
    {
        if (auto xml = juce::XmlDocument::parse (f))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            if (tree.isValid() && tree.hasType (apvts_.state.getType()))
                presetTrees_.push_back (tree);
        }
    }
}

bool PresetManager::userPresetNameExists (const juce::String& name) const
{
    for (int i = factoryCount_; i < getCount(); ++i)
        if (getName (i).equalsIgnoreCase (name))
            return true;
    return false;
}

void PresetManager::saveUserPreset (const juce::String& name,
                                    const juce::String& category,
                                    bool                overwrite)
{
    const auto dir  = getUserPresetDir();
    const auto file = dir.getChildFile (sanitizeFilename (name));

    if (file.existsAsFile() && ! overwrite) return;  // caller should guard, but be safe

    if (dir.createDirectory().failed()) return;       // can't create the directory

    // Copy current APVTS state and annotate with preset metadata.
    auto tree = apvts_.copyState();
    tree.setProperty ("name",          name,                            nullptr);
    tree.setProperty ("category",      category,                        nullptr);
    tree.setProperty ("primaryEngine", getCurrentStateInfo().primaryEngine, nullptr);

    if (auto xml = tree.createXml())
        xml->writeToFile (file, {});

    // Rescan so the new file appears in the list.
    scanUserPresets();

    // Select the newly saved preset.
    for (int i = factoryCount_; i < getCount(); ++i)
    {
        if (getName (i).equalsIgnoreCase (name))
        {
            load (i);
            break;
        }
    }
}

// ─── Favorites ────────────────────────────────────────────────────────────────

juce::File PresetManager::getFavoritesFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Manifold/favorites.json");
}

bool PresetManager::isFavorite (int idx) const
{
    if (idx < 0 || idx >= getCount()) return false;
    // Re-derive the stable ID and check membership.
    if (idx < factoryCount_)
        return favorites_.count ("factory:" + getFactoryPresets()[(size_t) idx].id) > 0;
    return favorites_.count ("user:" + getName (idx)) > 0;
}

void PresetManager::toggleFavorite (int idx)
{
    if (idx < 0 || idx >= getCount()) return;
    const auto id = getStableId (idx);
    if (id.isEmpty()) return;

    if (favorites_.count (id) > 0)
        favorites_.erase (id);
    else
        favorites_.insert (id);

    saveFavoritesToDisk();
    if (onFavoritesChanged) onFavoritesChanged();
}

int PresetManager::getFavoriteCount() const noexcept
{
    return (int) favorites_.size();
}

void PresetManager::loadFavoritesFromDisk()
{
    favorites_.clear();
    const auto file = getFavoritesFile();
    if (! file.existsAsFile()) return;

    auto json = juce::JSON::parse (file);
    if (auto* arr = json.getArray())
        for (const auto& v : *arr)
            if (v.isString())
                favorites_.insert (v.toString());
}

void PresetManager::saveFavoritesToDisk()
{
    const auto dir  = getFavoritesFile().getParentDirectory();
    if (dir.createDirectory().failed()) return;

    juce::Array<juce::var> arr;
    for (const auto& id : favorites_)
        arr.add (id);

    const auto file = getFavoritesFile();
    file.replaceWithText (juce::JSON::toString (juce::var (arr), true));
}

// ─── Delete ───────────────────────────────────────────────────────────────────

void PresetManager::deleteUserPreset (int idx)
{
    if (! isUserPreset (idx)) return;   // factory or invalid → no-op

    // Resolve the file from the preset's name (mirrors saveUserPreset path).
    const auto file = getUserPresetDir().getChildFile (sanitizeFilename (getName (idx)));

    // Capture the stable ID before the rescan invalidates indices.
    const auto deletedStableId = getStableId (idx);
    const bool deletedWasLoaded = (idx == currentIndex_);

    if (file.existsAsFile()) file.deleteFile();

    // Clean up favorites entry if present.
    if (! deletedStableId.isEmpty() && favorites_.erase (deletedStableId) > 0)
        saveFavoritesToDisk();

    // Rescan rebuilds presetTrees_ (factoryCount_ stays the same; user presets
    // collapse to fill the gap).
    scanUserPresets();

    // Adjust currentIndex_:
    if (deletedWasLoaded)
        currentIndex_ = -1;            // modified state — params preserved
    else if (currentIndex_ > idx)
        --currentIndex_;               // shift down to track the same preset

    if (onPresetDeleted) onPresetDeleted();
}

PresetManager::StateInfo PresetManager::getCurrentStateInfo() const
{
    using namespace manifold::params;
    StateInfo info;

    // First active chaos engine name.
    for (int i = 0; i < kNumChaosEngines; ++i)
    {
        if (apvts_.getRawParameterValue (kChaosEngineParamIds[i])->load() > 0.5f)
        {
            // "LORENZ" → "Lorenz"
            juce::String n = juce::String (kChaosEngineNames[i]).toLowerCase();
            info.primaryEngine = n.substring (0, 1).toUpperCase() + n.substring (1);
            break;
        }
    }
    if (info.primaryEngine.isEmpty()) info.primaryEngine = "Lorenz";

    // Shaper and filter display names.
    const auto shaperChoices = shaperTypeChoices();
    const auto filterChoices = filterTypeChoices();

    const int si = (int) apvts_.getRawParameterValue (id::shaperType)->load();
    const int fi = (int) apvts_.getRawParameterValue (id::filterType)->load();

    info.shaperName = shaperChoices[juce::jlimit (0, shaperChoices.size() - 1, si)];
    info.filterName = filterChoices[juce::jlimit (0, filterChoices.size() - 1, fi)];

    return info;
}

// ─── Private ──────────────────────────────────────────────────────────────────

juce::ValueTree PresetManager::buildTree (const FactoryPreset& p) const
{
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

#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <utility>

namespace manifold::preset
{

// One factory preset is a complete parameter snapshot — every APVTS param ID
// gets an explicit value so loading a preset is deterministic regardless of
// what the user had set previously. The value is in the parameter's natural
// range (Hz for cutoff, dB for output, 0..1 for floats, integer index for
// AudioParameterChoice, 0/1 for AudioParameterBool).
//
// Param ranges (see Params.h::makeLayout for source of truth):
//   intensity   [0, 1]
//   speed       [0, 1]
//   warmth      [0, 1]
//   drive       [1.0, 4.0]
//   cutoff      [20, 20000] Hz, log-tapered
//   resonance   [0, 0.95]
//   morph       [0, 1]
//   output      [-24, 12] dB
//   filterType  0=SVF · 1=MoogLadder · 2=DiodeLadder · 3=TunedComb
//   shaperType  0=Fold · 1=SoftClip · 2=HardClip · 3=Rectify · 4=Sine · 5=TubeAsym · 6=ChebyT3 · 7=ChebyT5
//   routing     0=Shape→Filter · 1=Filter→Shape
//   blend       [0, 1] — only meaningful when 2+ engines are active
//   chaos*      0/1 per engine
struct FactoryPreset
{
    // Stable kebab-case slug — the canonical identity of this preset.
    // Used by PresetManager::getStableId for favorites and any future
    // ref-by-identity needs. Renaming `name` is safe; renaming `id`
    // breaks favorites and is a one-way migration.
    juce::String id;
    juce::String name;
    juce::String category;
    juce::String primaryEngine;   // descriptive — first active engine, displayed in metadata
    std::vector<std::pair<juce::String, float>> params;
};

// Phase 8c — 30 hand-tuned factory presets across 6 categories (5 each):
//   WOBBLE · GROWL · DRONE · METAL · GLITCH · ALIEN
//
// Engine coverage: every engine stars in 4+ presets.
// Multi-core ratio: 11 of 30 use 2+ engines (BLEND meaningful).
// Routing variety: 5 of 30 use Filter→Shape.
inline const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> kPresets = {

        // ═══════════════════════════════════════════════════════════════════
        // WOBBLE — classic bass movement, gentle to mid-aggression
        // ═══════════════════════════════════════════════════════════════════

        // 1. Resin Drop — baseline Lorenz wobble, the friendly default.
        {
            "resin-drop", "Resin Drop", "Wobble", "Lorenz",
            {
                { "intensity",     0.55f  }, { "speed",         0.35f  }, { "warmth",        0.20f  },
                { "drive",         2.0f   }, { "cutoff",        400.0f }, { "resonance",     0.45f  },
                { "morph",         0.55f  }, { "output",        0.0f   },
                { "filterType",    0.0f   },   // SVF
                { "shaperType",    0.0f   },   // Fold
                { "routing",       0.0f   },   // Shape→Filter
                { "blend",         1.0f   },
                { "chaosLorenz",   1.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 2. Slow Tumble — lazy Rossler roll, warm and bus-friendly.
        {
            "slow-tumble", "Slow Tumble", "Wobble", "Rossler",
            {
                { "intensity",     0.45f  }, { "speed",         0.30f  }, { "warmth",        0.45f  },
                { "drive",         1.6f   }, { "cutoff",        350.0f }, { "resonance",     0.50f  },
                { "morph",         0.30f  }, { "output",        0.0f   },
                { "filterType",    1.0f   },   // MoogLadder
                { "shaperType",    1.0f   },   // SoftClip
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  1.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 3. Period-4 — Chua near-periodic jitter, low-drive subtle wobble.
        {
            "period-4", "Period-4", "Wobble", "Chua",
            {
                { "intensity",     0.30f  }, { "speed",         0.25f  }, { "warmth",        0.45f  },
                { "drive",         1.3f   }, { "cutoff",        200.0f }, { "resonance",     0.30f  },
                { "morph",         0.65f  }, { "output",        0.0f   },
                { "filterType",    1.0f   },   // MoogLadder
                { "shaperType",    1.0f   },   // SoftClip
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     1.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 4. Tide Cycle — Lorenz+Aizawa drift, two-engine gentle motion.
        {
            "tide-cycle", "Tide Cycle", "Wobble", "Lorenz",
            {
                { "intensity",     0.50f  }, { "speed",         0.40f  }, { "warmth",        0.30f  },
                { "drive",         1.8f   }, { "cutoff",        480.0f }, { "resonance",     0.55f  },
                { "morph",         0.45f  }, { "output",        0.0f   },
                { "filterType",    2.0f   },   // DiodeLadder
                { "shaperType",    1.0f   },   // SoftClip
                { "routing",       0.0f   },
                { "blend",         0.35f  },
                { "chaosLorenz",   1.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   1.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 5. Fathom Roll — Aizawa deep, slow, dubstep half-step movement.
        {
            "fathom-roll", "Fathom Roll", "Wobble", "Aizawa",
            {
                { "intensity",     0.50f  }, { "speed",         0.28f  }, { "warmth",        0.35f  },
                { "drive",         2.2f   }, { "cutoff",        280.0f }, { "resonance",     0.55f  },
                { "morph",         0.40f  }, { "output",        -1.0f  },
                { "filterType",    1.0f   },   // MoogLadder
                { "shaperType",    0.0f   },   // Fold
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   1.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // ═══════════════════════════════════════════════════════════════════
        // GROWL — aggressive, distorted, vocal-formant character
        // ═══════════════════════════════════════════════════════════════════

        // 6. Throat Lock — Henon vocal aggression, 303 bite, filter-first.
        {
            "throat-lock", "Throat Lock", "Growl", "Henon",
            {
                { "intensity",     0.65f  }, { "speed",         0.50f  }, { "warmth",        0.10f  },
                { "drive",         2.6f   }, { "cutoff",        700.0f }, { "resonance",     0.75f  },
                { "morph",         0.50f  }, { "output",        -3.0f  },
                { "filterType",    2.0f   },   // DiodeLadder
                { "shaperType",    6.0f   },   // ChebyT3
                { "routing",       1.0f   },   // Filter→Shape
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    1.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 7. Tar Pit — Lorenz+Henon hot, sticky, slow-attack growl.
        {
            "tar-pit", "Tar Pit", "Growl", "Lorenz",
            {
                { "intensity",     0.65f  }, { "speed",         0.42f  }, { "warmth",        0.20f  },
                { "drive",         2.8f   }, { "cutoff",        380.0f }, { "resonance",     0.65f  },
                { "morph",         0.35f  }, { "output",        -2.0f  },
                { "filterType",    1.0f   },   // MoogLadder
                { "shaperType",    5.0f   },   // TubeAsym
                { "routing",       0.0f   },
                { "blend",         0.55f  },
                { "chaosLorenz",   1.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    1.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 8. Hornet's Nest — Thomas fast buzz, hard-clip aggression, F→S.
        {
            "hornets-nest", "Hornet's Nest", "Growl", "Thomas",
            {
                { "intensity",     0.60f  }, { "speed",         0.65f  }, { "warmth",        0.05f  },
                { "drive",         3.0f   }, { "cutoff",        900.0f }, { "resonance",     0.70f  },
                { "morph",         0.55f  }, { "output",        -4.0f  },
                { "filterType",    2.0f   },   // DiodeLadder
                { "shaperType",    2.0f   },   // HardClip
                { "routing",       1.0f   },   // Filter→Shape
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   1.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 9. Diesel Cough — Chua choppy octave-up, broken engine vibe.
        {
            "diesel-cough", "Diesel Cough", "Growl", "Chua",
            {
                { "intensity",     0.55f  }, { "speed",         0.55f  }, { "warmth",        0.15f  },
                { "drive",         2.4f   }, { "cutoff",        320.0f }, { "resonance",     0.60f  },
                { "morph",         0.40f  }, { "output",        -3.0f  },
                { "filterType",    1.0f   },   // MoogLadder
                { "shaperType",    3.0f   },   // Rectify
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     1.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 10. Bonemeal — Henon+Rossler gritty, ChebyT5 dense harmonics.
        {
            "bonemeal", "Bonemeal", "Growl", "Rossler",
            {
                { "intensity",     0.70f  }, { "speed",         0.45f  }, { "warmth",        0.10f  },
                { "drive",         3.2f   }, { "cutoff",        550.0f }, { "resonance",     0.65f  },
                { "morph",         0.45f  }, { "output",        -4.0f  },
                { "filterType",    1.0f   },   // MoogLadder
                { "shaperType",    7.0f   },   // ChebyT5
                { "routing",       0.0f   },
                { "blend",         0.50f  },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  1.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    1.0f   },
                { "bypass",        0.0f   },
            }
        },

        // ═══════════════════════════════════════════════════════════════════
        // DRONE — sustained, evolving, sub-focused
        // ═══════════════════════════════════════════════════════════════════

        // 11. Cathedral Hum — Aizawa sine drone with band-pass focus.
        {
            "cathedral-hum", "Cathedral Hum", "Drone", "Aizawa",
            {
                { "intensity",     0.25f  }, { "speed",         0.18f  }, { "warmth",        0.65f  },
                { "drive",         1.4f   }, { "cutoff",        600.0f }, { "resonance",     0.55f  },
                { "morph",         0.50f  }, { "output",        -2.0f  },
                { "filterType",    0.0f   },   // SVF (morph 0.5 = BP)
                { "shaperType",    4.0f   },   // Sine
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   1.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 12. Mantle Drift — Lorenz+Aizawa ultra-slow, sub focus.
        {
            "mantle-drift", "Mantle Drift", "Drone", "Lorenz",
            {
                { "intensity",     0.30f  }, { "speed",         0.15f  }, { "warmth",        0.55f  },
                { "drive",         1.5f   }, { "cutoff",        240.0f }, { "resonance",     0.40f  },
                { "morph",         0.30f  }, { "output",        -1.0f  },
                { "filterType",    1.0f   },   // MoogLadder
                { "shaperType",    1.0f   },   // SoftClip
                { "routing",       0.0f   },
                { "blend",         0.30f  },
                { "chaosLorenz",   1.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   1.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 13. Subterrain — Lorenz pure sub-bass, near-static motion.
        {
            "subterrain", "Subterrain", "Drone", "Lorenz",
            {
                { "intensity",     0.20f  }, { "speed",         0.12f  }, { "warmth",        0.70f  },
                { "drive",         1.4f   }, { "cutoff",        180.0f }, { "resonance",     0.30f  },
                { "morph",         0.25f  }, { "output",        0.0f   },
                { "filterType",    1.0f   },   // MoogLadder
                { "shaperType",    1.0f   },   // SoftClip
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   1.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 14. Vapor Spire — Rossler+Thomas higher-register breathing drone.
        {
            "vapor-spire", "Vapor Spire", "Drone", "Thomas",
            {
                { "intensity",     0.40f  }, { "speed",         0.30f  }, { "warmth",        0.40f  },
                { "drive",         1.7f   }, { "cutoff",        850.0f }, { "resonance",     0.55f  },
                { "morph",         0.50f  }, { "output",        -2.0f  },
                { "filterType",    2.0f   },   // DiodeLadder
                { "shaperType",    4.0f   },   // Sine
                { "routing",       0.0f   },
                { "blend",         0.40f  },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   1.0f   }, { "chaosRossler",  1.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 15. Deep Carrier — Aizawa tube-warm sustained bed.
        {
            "deep-carrier", "Deep Carrier", "Drone", "Aizawa",
            {
                { "intensity",     0.35f  }, { "speed",         0.20f  }, { "warmth",        0.60f  },
                { "drive",         1.8f   }, { "cutoff",        220.0f }, { "resonance",     0.40f  },
                { "morph",         0.35f  }, { "output",        0.0f   },
                { "filterType",    1.0f   },   // MoogLadder
                { "shaperType",    5.0f   },   // TubeAsym
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   1.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // ═══════════════════════════════════════════════════════════════════
        // METAL — Karplus-Strong tuned-comb territory, plucked partials
        // ═══════════════════════════════════════════════════════════════════

        // 16. Bell Harvest — Henon plucked metal partials, complex.
        {
            "bell-harvest", "Bell Harvest", "Metal", "Henon",
            {
                { "intensity",     0.65f  }, { "speed",         0.55f  }, { "warmth",        0.05f  },
                { "drive",         2.8f   }, { "cutoff",        1200.0f}, { "resonance",     0.70f  },
                { "morph",         0.40f  }, { "output",        -3.0f  },
                { "filterType",    3.0f   },   // TunedComb
                { "shaperType",    7.0f   },   // ChebyT5
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    1.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 17. Anvil Stutter — Henon hammered tonal hits with chaos jitter.
        {
            "anvil-stutter", "Anvil Stutter", "Metal", "Henon",
            {
                { "intensity",     0.60f  }, { "speed",         0.50f  }, { "warmth",        0.10f  },
                { "drive",         2.5f   }, { "cutoff",        800.0f }, { "resonance",     0.75f  },
                { "morph",         0.50f  }, { "output",        -3.0f  },
                { "filterType",    3.0f   },   // TunedComb
                { "shaperType",    6.0f   },   // ChebyT3
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    1.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 18. Iron Filings — Henon+Chua complex inharmonic resonance cluster.
        {
            "iron-filings", "Iron Filings", "Metal", "Chua",
            {
                { "intensity",     0.55f  }, { "speed",         0.55f  }, { "warmth",        0.10f  },
                { "drive",         2.6f   }, { "cutoff",        1000.0f}, { "resonance",     0.80f  },
                { "morph",         0.45f  }, { "output",        -4.0f  },
                { "filterType",    3.0f   },   // TunedComb
                { "shaperType",    0.0f   },   // Fold
                { "routing",       0.0f   },
                { "blend",         0.50f  },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     1.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    1.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 19. Steel String — Lorenz plucked-string mid-pitch, musical.
        {
            "steel-string", "Steel String", "Metal", "Lorenz",
            {
                { "intensity",     0.40f  }, { "speed",         0.40f  }, { "warmth",        0.25f  },
                { "drive",         1.8f   }, { "cutoff",        500.0f }, { "resonance",     0.65f  },
                { "morph",         0.55f  }, { "output",        -2.0f  },
                { "filterType",    3.0f   },   // TunedComb
                { "shaperType",    1.0f   },   // SoftClip
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   1.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 20. Brass Knuckles — Thomas brassy comb tone with even harmonics.
        {
            "brass-knuckles", "Brass Knuckles", "Metal", "Thomas",
            {
                { "intensity",     0.55f  }, { "speed",         0.45f  }, { "warmth",        0.15f  },
                { "drive",         2.4f   }, { "cutoff",        700.0f }, { "resonance",     0.65f  },
                { "morph",         0.45f  }, { "output",        -3.0f  },
                { "filterType",    3.0f   },   // TunedComb
                { "shaperType",    5.0f   },   // TubeAsym
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   1.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // ═══════════════════════════════════════════════════════════════════
        // GLITCH — broken, edgy, stuttering
        // ═══════════════════════════════════════════════════════════════════

        // 21. Glass Break — Henon brittle, sharp, percussive shatter.
        {
            "glass-break", "Glass Break", "Glitch", "Henon",
            {
                { "intensity",     0.75f  }, { "speed",         0.60f  }, { "warmth",        0.05f  },
                { "drive",         3.2f   }, { "cutoff",        1500.0f}, { "resonance",     0.55f  },
                { "morph",         0.50f  }, { "output",        -5.0f  },
                { "filterType",    2.0f   },   // DiodeLadder
                { "shaperType",    2.0f   },   // HardClip
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    1.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 22. Datamosh — Chua+Henon dense corrupted-data buzz, F→S.
        {
            "datamosh", "Datamosh", "Glitch", "Chua",
            {
                { "intensity",     0.65f  }, { "speed",         0.65f  }, { "warmth",        0.05f  },
                { "drive",         2.8f   }, { "cutoff",        1100.0f}, { "resonance",     0.65f  },
                { "morph",         0.55f  }, { "output",        -4.0f  },
                { "filterType",    2.0f   },   // DiodeLadder
                { "shaperType",    7.0f   },   // ChebyT5
                { "routing",       1.0f   },   // Filter→Shape
                { "blend",         0.65f  },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     1.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    1.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 23. Crystalline — Henon high-shelf shimmer with chaos sparkle.
        {
            "crystalline", "Crystalline", "Glitch", "Henon",
            {
                { "intensity",     0.50f  }, { "speed",         0.55f  }, { "warmth",        0.10f  },
                { "drive",         2.0f   }, { "cutoff",        1800.0f}, { "resonance",     0.60f  },
                { "morph",         0.85f  }, { "output",        -3.0f  },
                { "filterType",    0.0f   },   // SVF (morph 0.85 = HP-leaning)
                { "shaperType",    6.0f   },   // ChebyT3
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    1.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 24. Skip Frame — Henon stuttering octave-up artifact.
        {
            "skip-frame", "Skip Frame", "Glitch", "Henon",
            {
                { "intensity",     0.70f  }, { "speed",         0.70f  }, { "warmth",        0.05f  },
                { "drive",         2.6f   }, { "cutoff",        600.0f }, { "resonance",     0.65f  },
                { "morph",         0.40f  }, { "output",        -4.0f  },
                { "filterType",    2.0f   },   // DiodeLadder
                { "shaperType",    3.0f   },   // Rectify
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    1.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 25. Pixel Storm — Thomas+Henon fast-moving wide harmonic chaos, F→S.
        {
            "pixel-storm", "Pixel Storm", "Glitch", "Thomas",
            {
                { "intensity",     0.60f  }, { "speed",         0.70f  }, { "warmth",        0.05f  },
                { "drive",         2.8f   }, { "cutoff",        1200.0f}, { "resonance",     0.70f  },
                { "morph",         0.55f  }, { "output",        -5.0f  },
                { "filterType",    2.0f   },   // DiodeLadder
                { "shaperType",    7.0f   },   // ChebyT5
                { "routing",       1.0f   },   // Filter→Shape
                { "blend",         0.60f  },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   1.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    1.0f   },
                { "bypass",        0.0f   },
            }
        },

        // ═══════════════════════════════════════════════════════════════════
        // ALIEN — uncanny, otherworldly, modulated
        // ═══════════════════════════════════════════════════════════════════

        // 26. Siren Field — Lorenz sweeping formant, eerie wide.
        {
            "siren-field", "Siren Field", "Alien", "Lorenz",
            {
                { "intensity",     0.50f  }, { "speed",         0.35f  }, { "warmth",        0.30f  },
                { "drive",         1.8f   }, { "cutoff",        800.0f }, { "resonance",     0.60f  },
                { "morph",         0.55f  }, { "output",        -2.0f  },
                { "filterType",    0.0f   },   // SVF
                { "shaperType",    4.0f   },   // Sine
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   1.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 27. Xenotongue — Chua+Aizawa vowel-shifting otherworldly speech.
        {
            "xenotongue", "Xenotongue", "Alien", "Chua",
            {
                { "intensity",     0.50f  }, { "speed",         0.40f  }, { "warmth",        0.30f  },
                { "drive",         2.2f   }, { "cutoff",        650.0f }, { "resonance",     0.70f  },
                { "morph",         0.50f  }, { "output",        -3.0f  },
                { "filterType",    2.0f   },   // DiodeLadder
                { "shaperType",    5.0f   },   // TubeAsym
                { "routing",       0.0f   },
                { "blend",         0.50f  },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     1.0f   }, { "chaosAizawa",   1.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 28. Nebula Shift — Lorenz+Thomas+Rossler 3-engine evolving cosmos pad.
        {
            "nebula-shift", "Nebula Shift", "Alien", "Lorenz",
            {
                { "intensity",     0.45f  }, { "speed",         0.30f  }, { "warmth",        0.45f  },
                { "drive",         1.7f   }, { "cutoff",        500.0f }, { "resonance",     0.50f  },
                { "morph",         0.45f  }, { "output",        -2.0f  },
                { "filterType",    1.0f   },   // MoogLadder
                { "shaperType",    4.0f   },   // Sine
                { "routing",       0.0f   },
                { "blend",         0.55f  },
                { "chaosLorenz",   1.0f   }, { "chaosThomas",   1.0f   }, { "chaosRossler",  1.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 29. Probe Echo — Thomas pulsing comb ping, spacecraft beacon.
        {
            "probe-echo", "Probe Echo", "Alien", "Thomas",
            {
                { "intensity",     0.45f  }, { "speed",         0.35f  }, { "warmth",        0.25f  },
                { "drive",         1.6f   }, { "cutoff",        720.0f }, { "resonance",     0.70f  },
                { "morph",         0.55f  }, { "output",        -3.0f  },
                { "filterType",    3.0f   },   // TunedComb
                { "shaperType",    4.0f   },   // Sine
                { "routing",       0.0f   },
                { "blend",         1.0f   },
                { "chaosLorenz",   0.0f   }, { "chaosThomas",   1.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     0.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },

        // 30. Inversion Layer — Lorenz+Chua filter-first reversal, saturating sweeps.
        {
            "inversion-layer", "Inversion Layer", "Alien", "Lorenz",
            {
                { "intensity",     0.45f  }, { "speed",         0.40f  }, { "warmth",        0.25f  },
                { "drive",         2.0f   }, { "cutoff",        580.0f }, { "resonance",     0.65f  },
                { "morph",         0.45f  }, { "output",        -2.0f  },
                { "filterType",    2.0f   },   // DiodeLadder
                { "shaperType",    5.0f   },   // TubeAsym
                { "routing",       1.0f   },   // Filter→Shape
                { "blend",         0.45f  },
                { "chaosLorenz",   1.0f   }, { "chaosThomas",   0.0f   }, { "chaosRossler",  0.0f   },
                { "chaosChua",     1.0f   }, { "chaosAizawa",   0.0f   }, { "chaosHenon",    0.0f   },
                { "bypass",        0.0f   },
            }
        },
    };
    return kPresets;
}

} // namespace manifold::preset

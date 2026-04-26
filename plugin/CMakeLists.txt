cmake_minimum_required(VERSION 3.22)

# Must be set BEFORE project() — CMake evaluates these at project init.
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Min macOS")
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Mac arch")

project(Manifold VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Manifold
    COMPANY_NAME "Quine"
    BUNDLE_ID com.quine.manifold
    # NOTE: PLUGIN_MANUFACTURER_CODE is a stable internal 4-char ID (never shown to users).
    # Changing it changes the VST3 UID, which invalidates host plugin databases and breaks
    # any existing user sessions that loaded the plugin. Keep this fixed for the life of the product.
    PLUGIN_MANUFACTURER_CODE Aprd
    PLUGIN_CODE Mfld
    FORMATS VST3 Standalone
    PRODUCT_NAME "Manifold"
    DESCRIPTION "Chaos-driven character transformer"
    VST3_CATEGORIES Fx Distortion Modulation
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

juce_generate_juce_header(Manifold)

target_sources(Manifold PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/ui/PhasePortrait.cpp
    Source/ui/ManifoldLookAndFeel.cpp
    Source/ui/ManifoldWordmark.cpp
    Source/ui/EngineButton.cpp
    Source/ui/PickerDrawer.cpp
)

target_compile_definitions(Manifold PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(Manifold PRIVATE
    juce::juce_audio_utils
    juce::juce_opengl
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

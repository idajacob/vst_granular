cmake_minimum_required(VERSION 3.22)
project(GranularSynth VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Download JUCE automatically ──────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(GranularSynth
    COMPANY_NAME "IndieGrain"
    PLUGIN_MANUFACTURER_CODE "IGrn"
    PLUGIN_CODE "GrSy"
    FORMATS VST3
    PRODUCT_NAME "GranularSynth"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES "Instrument" "Synth"
)

target_sources(GranularSynth
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
)

target_include_directories(GranularSynth PRIVATE src)

target_compile_definitions(GranularSynth
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_ALSA=0
        JUCE_JACK=0
        # Use JUCE's built-in codec implementations (no system libs needed)
        JUCE_USE_FLAC=1
        JUCE_USE_OGGVORBIS=1
        JUCE_USE_MP3AUDIOFORMAT=1
)

target_link_libraries(GranularSynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

cmake_minimum_required(VERSION 3.22)
project(GranularSynth VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find JUCE
find_package(JUCE CONFIG REQUIRED)

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
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FLAC    REQUIRED flac)
pkg_check_modules(VORBIS  REQUIRED vorbis)
pkg_check_modules(OGG     REQUIRED ogg)

target_link_libraries(GranularSynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
        ${FLAC_LIBRARIES}
        ${VORBIS_LIBRARIES}
        ${OGG_LIBRARIES}
        vorbisfile
        vorbisenc
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

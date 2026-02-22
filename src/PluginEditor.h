#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

//==============================================================================
// Indie-style knob look and feel
class IndieKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    IndieKnobLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider&) override;

    void drawLabel(juce::Graphics&, juce::Label&) override;
    juce::Font getLabelFont(juce::Label&) override;

private:
    juce::Colour bg        { 0xFF1C1C1E };
    juce::Colour knobBody  { 0xFF2E2A26 };
    juce::Colour knobRim   { 0xFF5C4A32 };
    juce::Colour accent    { 0xFFD4956A };
    juce::Colour textCol   { 0xFFE8D5B0 };
};

//==============================================================================
// Waveform display with playhead
class WaveformDisplay : public juce::Component,
                        public juce::Timer
{
public:
    WaveformDisplay(GranularSynthProcessor& p);
    void paint(juce::Graphics&) override;
    void timerCallback() override { repaint(); }
    void updateThumbnail(const juce::AudioBuffer<float>& buf, double sr);

private:
    GranularSynthProcessor& proc;
    juce::AudioThumbnailCache thumbnailCache { 1 };
    juce::AudioThumbnail       thumbnail;
    juce::AudioFormatManager   formatManager;
};

//==============================================================================
// A single knob with label
class LabelledKnob : public juce::Component
{
public:
    LabelledKnob(const juce::String& labelText,
                 juce::AudioProcessorValueTreeState& apvts,
                 const juce::String& paramID,
                 IndieKnobLookAndFeel& laf);

    void resized() override;

    juce::Slider slider;
    juce::Label  label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

//==============================================================================
// Section panel with a hand-drawn border look
class SectionPanel : public juce::Component
{
public:
    SectionPanel(const juce::String& title);
    void paint(juce::Graphics&) override;
    void resized() override;

    juce::Component contentArea;

private:
    juce::String title;
    juce::Colour borderCol { 0xFF5C4A32 };
    juce::Colour bgCol     { 0xFF211E1A };
    juce::Colour titleCol  { 0xFFD4956A };
};

//==============================================================================
class GranularSynthEditor : public juce::AudioProcessorEditor,
                            public juce::Timer,
                            public juce::DragAndDropTarget,
                            public juce::FileDragAndDropTarget
{
public:
    explicit GranularSynthEditor(GranularSynthProcessor&);
    ~GranularSynthEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // Drag and drop
    bool isInterestedInDragSource(const SourceDetails&) override { return false; }
    void itemDropped(const SourceDetails&) override {}
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int, int) override;

private:
    GranularSynthProcessor& proc;
    IndieKnobLookAndFeel    lookAndFeel;

    //==========================================================================
    // Waveform
    WaveformDisplay waveDisplay;

    // Load button
    juce::TextButton loadButton;

    // Sample name label
    juce::Label sampleLabel;

    //==========================================================================
    // Sections
    SectionPanel grainSection  { "GRAINS" };
    SectionPanel effectsSection{ "EFFECTS" };
    SectionPanel ampSection    { "ENVELOPE" };

    //==========================================================================
    // Grain knobs
    LabelledKnob knobGrainSize     { "SIZE",     proc.apvts, "grainSize",     lookAndFeel };
    LabelledKnob knobDensity       { "DENSITY",  proc.apvts, "grainDensity",  lookAndFeel };
    LabelledKnob knobPosition      { "POSITION", proc.apvts, "position",      lookAndFeel };
    LabelledKnob knobPosRand       { "SCATTER",  proc.apvts, "positionRandom",lookAndFeel };
    LabelledKnob knobPitch         { "PITCH",    proc.apvts, "pitch",         lookAndFeel };
    LabelledKnob knobPitchRand     { "P.SCATTER",proc.apvts, "pitchRandom",   lookAndFeel };
    LabelledKnob knobPan           { "PAN",      proc.apvts, "pan",           lookAndFeel };
    LabelledKnob knobVolume        { "VOLUME",   proc.apvts, "volume",        lookAndFeel };

    // Amp envelope knobs
    LabelledKnob knobAttack        { "ATTACK",   proc.apvts, "attack",        lookAndFeel };
    LabelledKnob knobDecay         { "DECAY",    proc.apvts, "decay",         lookAndFeel };
    LabelledKnob knobSustain       { "SUSTAIN",  proc.apvts, "sustain",       lookAndFeel };
    LabelledKnob knobRelease       { "RELEASE",  proc.apvts, "release",       lookAndFeel };

    // Reverb knobs
    LabelledKnob knobRevSize       { "SIZE",     proc.apvts, "reverbSize",    lookAndFeel };
    LabelledKnob knobRevDamp       { "DAMP",     proc.apvts, "reverbDamp",    lookAndFeel };
    LabelledKnob knobRevWet        { "MIX",      proc.apvts, "reverbWet",     lookAndFeel };
    LabelledKnob knobRevWidth      { "WIDTH",    proc.apvts, "reverbWidth",   lookAndFeel };

    // Delay knobs
    LabelledKnob knobDelayTime     { "TIME",     proc.apvts, "delayTime",     lookAndFeel };
    LabelledKnob knobDelayFB       { "FEEDBACK", proc.apvts, "delayFeedback", lookAndFeel };
    LabelledKnob knobDelayWet      { "MIX",      proc.apvts, "delayWet",      lookAndFeel };

    //==========================================================================
    // Sub-section labels for Reverb / Delay inside effects
    juce::Label reverbLabel, delayLabel;

    // File chooser (must outlive the async callback)
    std::unique_ptr<juce::FileChooser> fileChooser;

    void layoutKnobRow(juce::Rectangle<int> area,
                       std::initializer_list<LabelledKnob*> knobs);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularSynthEditor)
};

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <random>
#include <vector>

//==============================================================================
struct Grain
{
    bool active = false;
    float position = 0.0f;       // current read position in sample buffer
    float positionInc = 1.0f;    // playback speed
    float age = 0.0f;            // samples played so far
    float lifetime = 0.0f;       // total grain length in samples
    float pan = 0.0f;            // -1..1
    float amplitude = 1.0f;
    float startPosition = 0.0f;  // where in the buffer this grain started
};

//==============================================================================
class GranularSynthProcessor : public juce::AudioProcessor,
                                public juce::AudioProcessorValueTreeState::Listener
{
public:
    GranularSynthProcessor();
    ~GranularSynthProcessor() override;

    //==========================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GranularSynth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void parameterChanged(const juce::String& paramID, float newValue) override;

    //==========================================================================
    // Sample loading
    bool loadSample(const juce::File& file);
    bool hasSample() const { return sampleBuffer.getNumSamples() > 0; }
    const juce::String& getSampleName() const { return sampleName; }
    float getSamplePlayhead() const { return samplePlayhead.load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    //==========================================================================
    // Granular engine
    void spawnGrain(float midiNote, float velocity);
    void processGrains(juce::AudioBuffer<float>& buffer, int numSamples);
    float getWindowValue(float phase); // 0..1 -> amplitude

    //==========================================================================
    // Sample data
    juce::AudioBuffer<float> sampleBuffer;
    juce::String sampleName;
    double sampleSampleRate = 44100.0;
    std::atomic<float> samplePlayhead { 0.0f };

    //==========================================================================
    // Grains
    static constexpr int MAX_GRAINS = 64;
    std::array<Grain, MAX_GRAINS> grains;
    float grainSpawnTimer = 0.0f;

    //==========================================================================
    // MIDI state
    struct NoteInfo { float midiNote; float velocity; };
    std::vector<NoteInfo> activeNotes;
    juce::CriticalSection noteLock;

    //==========================================================================
    // DSP
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;

    // Delay
    static constexpr int DELAY_MAX_SAMPLES = 192000; // 4s @ 48k
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;

    //==========================================================================
    double currentSampleRate = 44100.0;
    std::mt19937 rng { std::random_device{}() };
    std::uniform_real_distribution<float> randDist { -1.0f, 1.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GranularSynthProcessor)
};

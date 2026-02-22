#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout GranularSynthProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Granular parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grainSize", "Grain Size", juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.5f), 80.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "grainDensity", "Density", juce::NormalisableRange<float>(1.0f, 64.0f, 0.1f, 0.5f), 8.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "position", "Position", juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "positionRandom", "Position Scatter", juce::NormalisableRange<float>(0.0f, 1.0f), 0.05f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "pitch", "Pitch", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "pitchRandom", "Pitch Scatter", juce::NormalisableRange<float>(0.0f, 24.0f, 0.01f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "pan", "Pan Spread", juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));

    // Amplitude / ADSR
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "attack",  "Attack",  juce::NormalisableRange<float>(0.001f, 4.0f, 0.001f, 0.4f), 0.01f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "decay",   "Decay",   juce::NormalisableRange<float>(0.001f, 4.0f, 0.001f, 0.4f), 0.1f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "sustain", "Sustain", juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release", juce::NormalisableRange<float>(0.001f, 8.0f, 0.001f, 0.4f), 0.5f));

    // Output
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "volume", "Volume", juce::NormalisableRange<float>(0.0f, 1.0f), 0.75f));

    // Reverb
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "reverbSize",   "Reverb Size",   juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "reverbDamp",   "Reverb Damp",   juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "reverbWet",    "Reverb Mix",    juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "reverbWidth",  "Reverb Width",  juce::NormalisableRange<float>(0.0f, 1.0f), 0.8f));

    // Delay
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delayTime",    "Delay Time",    juce::NormalisableRange<float>(0.01f, 2.0f, 0.001f, 0.5f), 0.375f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delayFeedback","Delay Feedback",juce::NormalisableRange<float>(0.0f, 0.95f), 0.35f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "delayWet",     "Delay Mix",     juce::NormalisableRange<float>(0.0f, 1.0f), 0.2f));

    return { params.begin(), params.end() };
}

//==============================================================================
GranularSynthProcessor::GranularSynthProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "GranularSynth", createParameters())
{
    apvts.addParameterListener("reverbSize",  this);
    apvts.addParameterListener("reverbDamp",  this);
    apvts.addParameterListener("reverbWet",   this);
    apvts.addParameterListener("reverbWidth", this);

    for (auto& g : grains)
        g.active = false;
}

GranularSynthProcessor::~GranularSynthProcessor()
{
    apvts.removeParameterListener("reverbSize",  this);
    apvts.removeParameterListener("reverbDamp",  this);
    apvts.removeParameterListener("reverbWet",   this);
    apvts.removeParameterListener("reverbWidth", this);
}

//==============================================================================
void GranularSynthProcessor::parameterChanged(const juce::String& paramID, float newValue)
{
    if (paramID == "reverbSize")   reverbParams.roomSize   = newValue;
    if (paramID == "reverbDamp")   reverbParams.damping    = newValue;
    if (paramID == "reverbWet")  { reverbParams.wetLevel   = newValue; reverbParams.dryLevel = 1.0f - newValue; }
    if (paramID == "reverbWidth")  reverbParams.width      = newValue;
    reverb.setParameters(reverbParams);
}

//==============================================================================
void GranularSynthProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    grainSpawnTimer = 0.0f;

    // Prepare reverb
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 2;
    reverb.prepare(spec);

    reverbParams.roomSize   = *apvts.getRawParameterValue("reverbSize");
    reverbParams.damping    = *apvts.getRawParameterValue("reverbDamp");
    float rWet              = *apvts.getRawParameterValue("reverbWet");
    reverbParams.wetLevel   = rWet;
    reverbParams.dryLevel   = 1.0f - rWet;
    reverbParams.width      = *apvts.getRawParameterValue("reverbWidth");
    reverb.setParameters(reverbParams);

    // Prepare delay buffer (stereo)
    delayBuffer.setSize(2, DELAY_MAX_SAMPLES);
    delayBuffer.clear();
    delayWritePos = 0;
}

void GranularSynthProcessor::releaseResources() {}

//==============================================================================
float GranularSynthProcessor::getWindowValue(float phase)
{
    // Hann window
    return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
}

//==============================================================================
void GranularSynthProcessor::spawnGrain(float midiNote, float velocity)
{
    if (!hasSample()) return;

    // Find free grain slot
    Grain* slot = nullptr;
    for (auto& g : grains)
    {
        if (!g.active) { slot = &g; break; }
    }
    if (!slot) return;

    float grainSizeMs  = *apvts.getRawParameterValue("grainSize");
    float pitchSemi    = *apvts.getRawParameterValue("pitch");
    float pitchRand    = *apvts.getRawParameterValue("pitchRandom");
    float posCenter    = *apvts.getRawParameterValue("position");
    float posRand      = *apvts.getRawParameterValue("positionRandom");
    float panSpread    = *apvts.getRawParameterValue("pan");

    // Randomise pitch scatter
    float pitchOffset  = pitchSemi + randDist(rng) * pitchRand;
    // MIDI note relative to middle C (60) adjusts pitch
    float noteOffset   = midiNote - 60.0f;
    float totalSemi    = pitchOffset + noteOffset;
    float speedRatio   = std::pow(2.0f, totalSemi / 12.0f);

    // Grain lifetime in samples
    float lifetimeSamples = (grainSizeMs / 1000.0f) * (float)currentSampleRate;

    // Position in sample buffer
    int   bufLen        = sampleBuffer.getNumSamples();
    float posScatter    = randDist(rng) * posRand * (float)bufLen;
    float startSample   = juce::jlimit(0.0f, (float)(bufLen - 1),
                                       posCenter * (float)bufLen + posScatter);

    slot->active        = true;
    slot->startPosition = startSample;
    slot->position      = startSample;
    slot->positionInc   = speedRatio;
    slot->age           = 0.0f;
    slot->lifetime      = lifetimeSamples;
    slot->pan           = randDist(rng) * panSpread;
    slot->amplitude     = velocity;
}

//==============================================================================
void GranularSynthProcessor::processGrains(juce::AudioBuffer<float>& buffer, int numSamples)
{
    if (!hasSample()) return;

    const float* srcL = sampleBuffer.getReadPointer(0);
    const float* srcR = sampleBuffer.getNumChannels() > 1
                        ? sampleBuffer.getReadPointer(1)
                        : sampleBuffer.getReadPointer(0);
    int bufLen = sampleBuffer.getNumSamples();

    float density     = *apvts.getRawParameterValue("grainDensity");
    float volume      = *apvts.getRawParameterValue("volume");

    // Samples per grain spawn (density = grains/sec)
    float samplesPerSpawn = (float)currentSampleRate / density;

    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getWritePointer(1);

    for (int s = 0; s < numSamples; ++s)
    {
        // Spawn new grains
        grainSpawnTimer += 1.0f;
        if (grainSpawnTimer >= samplesPerSpawn)
        {
            grainSpawnTimer -= samplesPerSpawn;
            juce::ScopedLock lock(noteLock);
            if (!activeNotes.empty())
            {
                // Round-robin note selection for chords
                static int noteIdx = 0;
                noteIdx = (noteIdx + 1) % (int)activeNotes.size();
                auto& n = activeNotes[noteIdx];
                spawnGrain(n.midiNote, n.velocity);
            }
        }

        // Render active grains
        float sumL = 0.0f, sumR = 0.0f;
        for (auto& g : grains)
        {
            if (!g.active) continue;

            float phase = g.age / g.lifetime;
            float win   = getWindowValue(phase);

            // Linear interpolation
            int   ipos  = (int)g.position;
            float frac  = g.position - (float)ipos;
            int   ipos2 = (ipos + 1) % bufLen;

            float sampleL = srcL[ipos] + frac * (srcL[ipos2] - srcL[ipos]);
            float sampleR = srcR[ipos] + frac * (srcR[ipos2] - srcR[ipos]);

            float env = win * g.amplitude * volume;

            // Pan law: equal power
            float panAngle = (g.pan + 1.0f) * 0.5f * juce::MathConstants<float>::halfPi;
            float panL = std::cos(panAngle);
            float panR = std::sin(panAngle);

            sumL += sampleL * env * panL;
            sumR += sampleR * env * panR;

            // Advance
            g.position += g.positionInc;
            g.age      += 1.0f;

            // Wrap position within buffer
            if ((int)g.position >= bufLen) g.position -= (float)bufLen;
            if (g.position < 0.0f)         g.position += (float)bufLen;

            if (g.age >= g.lifetime) g.active = false;
        }

        outL[s] += sumL;
        outR[s] += sumR;
    }

    // Update playhead display (using average grain position)
    if (bufLen > 0)
    {
        int activeCount = 0;
        float posSum = 0.0f;
        for (auto& g : grains)
            if (g.active) { posSum += g.startPosition; ++activeCount; }
        if (activeCount > 0)
            samplePlayhead.store(posSum / ((float)activeCount * (float)bufLen));
    }
}

//==============================================================================
void GranularSynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    // Handle MIDI
    for (const auto meta : midiMessages)
    {
        auto msg = meta.getMessage();
        if (msg.isNoteOn())
        {
            juce::ScopedLock lock(noteLock);
            activeNotes.push_back({ (float)msg.getNoteNumber(), msg.getVelocity() / 127.0f });
        }
        else if (msg.isNoteOff())
        {
            juce::ScopedLock lock(noteLock);
            float note = (float)msg.getNoteNumber();
            activeNotes.erase(std::remove_if(activeNotes.begin(), activeNotes.end(),
                [note](const NoteInfo& n) { return n.midiNote == note; }),
                activeNotes.end());
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            juce::ScopedLock lock(noteLock);
            activeNotes.clear();
        }
    }

    int numSamples = buffer.getNumSamples();

    // Granular processing
    processGrains(buffer, numSamples);

    // ---- Delay ----
    float delayTimeSec  = *apvts.getRawParameterValue("delayTime");
    float delayFeedback = *apvts.getRawParameterValue("delayFeedback");
    float delayWet      = *apvts.getRawParameterValue("delayWet");

    int delayInSamples = juce::jlimit(1, DELAY_MAX_SAMPLES - 1,
                                      (int)(delayTimeSec * (float)currentSampleRate));

    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getWritePointer(1);
    float* dlyL = delayBuffer.getWritePointer(0);
    float* dlyR = delayBuffer.getWritePointer(1);

    for (int s = 0; s < numSamples; ++s)
    {
        int readPos = (delayWritePos - delayInSamples + DELAY_MAX_SAMPLES) % DELAY_MAX_SAMPLES;

        float dL = dlyL[readPos];
        float dR = dlyR[readPos];

        dlyL[delayWritePos] = outL[s] + dL * delayFeedback;
        dlyR[delayWritePos] = outR[s] + dR * delayFeedback;

        outL[s] += dL * delayWet;
        outR[s] += dR * delayWet;

        delayWritePos = (delayWritePos + 1) % DELAY_MAX_SAMPLES;
    }

    // ---- Reverb ----
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    reverb.process(ctx);
}

//==============================================================================
bool GranularSynthProcessor::loadSample(const juce::File& file)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader) return false;

    juce::AudioBuffer<float> tempBuf((int)reader->numChannels, (int)reader->lengthInSamples);
    reader->read(&tempBuf, 0, (int)reader->lengthInSamples, 0, true, true);

    // Resample to current sample rate if needed
    if (reader->sampleRate != currentSampleRate && currentSampleRate > 0.0)
    {
        double ratio = currentSampleRate / reader->sampleRate;
        int newLen   = (int)(reader->lengthInSamples * ratio);
        juce::AudioBuffer<float> resampled(tempBuf.getNumChannels(), newLen);

        for (int ch = 0; ch < tempBuf.getNumChannels(); ++ch)
        {
            for (int i = 0; i < newLen; ++i)
            {
                float srcPos = (float)i / (float)ratio;
                int   iSrc   = (int)srcPos;
                float frac   = srcPos - iSrc;
                int   iSrc2  = juce::jmin(iSrc + 1, tempBuf.getNumSamples() - 1);
                float v      = tempBuf.getSample(ch, iSrc) * (1.0f - frac)
                             + tempBuf.getSample(ch, iSrc2) * frac;
                resampled.setSample(ch, i, v);
            }
        }
        sampleBuffer = std::move(resampled);
    }
    else
    {
        sampleBuffer = std::move(tempBuf);
    }

    // Make stereo if mono
    if (sampleBuffer.getNumChannels() == 1)
    {
        juce::AudioBuffer<float> stereo(2, sampleBuffer.getNumSamples());
        stereo.copyFrom(0, 0, sampleBuffer, 0, 0, sampleBuffer.getNumSamples());
        stereo.copyFrom(1, 0, sampleBuffer, 0, 0, sampleBuffer.getNumSamples());
        sampleBuffer = std::move(stereo);
    }

    sampleSampleRate = currentSampleRate;
    sampleName = file.getFileNameWithoutExtension();
    return true;
}

//==============================================================================
void GranularSynthProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    // Save sample path if loaded
    if (hasSample())
        state.setProperty("sampleName", sampleName, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void GranularSynthProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
juce::AudioProcessorEditor* GranularSynthProcessor::createEditor()
{
    return new GranularSynthEditor(*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GranularSynthProcessor();
}

#include "PluginEditor.h"
#include "PluginProcessor.h"

// Helper: make a juce::Font (JUCE 7 compatible)
static juce::Font makeFont(float height, int style = juce::Font::plain)
{
    return juce::Font(height, style);
}

//==============================================================================
// IndieKnobLookAndFeel
//==============================================================================
IndieKnobLookAndFeel::IndieKnobLookAndFeel()
{
    setColour(juce::Slider::thumbColourId,               accent);
    setColour(juce::Slider::rotarySliderFillColourId,    accent);
    setColour(juce::Slider::rotarySliderOutlineColourId, knobRim);
    setColour(juce::Label::textColourId,                 textCol);
    setColour(juce::TextButton::buttonColourId,   juce::Colour(0xFF2E2A26));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xFF5C4A32));
    setColour(juce::TextButton::textColourOffId,  textCol);
}

void IndieKnobLookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x, int y, int w, int h,
    float sliderPos, float startAngle, float endAngle,
    juce::Slider& /*slider*/)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(6.0f);
    auto centre = bounds.getCentre();
    float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

    // Shadow
    g.setColour(juce::Colour(0x44000000));
    g.fillEllipse(bounds.translated(2.0f, 2.0f));

    // Body gradient
    juce::ColourGradient grad(knobBody.brighter(0.15f), centre.x, centre.y - radius * 0.5f,
                              knobBody.darker(0.3f),    centre.x, centre.y + radius,
                              true);
    g.setGradientFill(grad);
    g.fillEllipse(bounds);

    // Rim
    g.setColour(knobRim);
    g.drawEllipse(bounds, 1.5f);
    g.setColour(knobRim.brighter(0.3f));
    g.drawEllipse(bounds.reduced(1.5f), 0.6f);

    // Track arc (background)
    juce::Path trackArc;
    float trackR = radius + 3.0f;
    trackArc.addCentredArc(centre.x, centre.y, trackR, trackR,
                           0.0f, startAngle, endAngle, true);
    g.setColour(knobRim.darker(0.5f));
    g.strokePath(trackArc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    // Value arc
    float valAngle = startAngle + sliderPos * (endAngle - startAngle);
    juce::Path valArc;
    valArc.addCentredArc(centre.x, centre.y, trackR, trackR,
                         0.0f, startAngle, valAngle, true);
    g.setColour(accent);
    g.strokePath(valArc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    // Indicator dot
    float dotAngle = valAngle - juce::MathConstants<float>::halfPi;
    float dotX = centre.x + std::cos(dotAngle) * (radius * 0.62f);
    float dotY = centre.y + std::sin(dotAngle) * (radius * 0.62f);
    g.setColour(accent);
    g.fillEllipse(dotX - 3.0f, dotY - 3.0f, 6.0f, 6.0f);
    g.setColour(accent.brighter(0.5f));
    g.fillEllipse(dotX - 1.5f, dotY - 1.5f, 3.0f, 3.0f);
}

void IndieKnobLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.setColour(juce::Colour(0xFFE8D5B0));
    g.setFont(getLabelFont(label));
    g.drawText(label.getText(), label.getLocalBounds(),
               label.getJustificationType(), true);
}

juce::Font IndieKnobLookAndFeel::getLabelFont(juce::Label&)
{
    return makeFont(10.5f, juce::Font::bold);
}

//==============================================================================
// WaveformDisplay
//==============================================================================
WaveformDisplay::WaveformDisplay(GranularSynthProcessor& p)
    : proc(p),
      thumbnail(512, formatManager, thumbnailCache)
{
    formatManager.registerBasicFormats();
    startTimerHz(30);
}

void WaveformDisplay::updateThumbnail(const juce::AudioBuffer<float>& buf, double sr)
{
    juce::MemoryOutputStream stream;
    juce::WavAudioFormat wav;
    auto writer = std::unique_ptr<juce::AudioFormatWriter>(
        wav.createWriterFor(&stream, sr, (unsigned)buf.getNumChannels(), 16, {}, 0));
    if (writer)
    {
        writer->writeFromAudioSampleBuffer(buf, 0, buf.getNumSamples());
        writer->flush();
    }
    juce::MemoryInputStream mis(stream.getData(), stream.getDataSize(), false);
    thumbnail.setReader(wav.createReaderFor(&mis, false), 0);
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    g.setColour(juce::Colour(0xFF141210));
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(juce::Colour(0xFF5C4A32));
    g.drawRoundedRectangle(bounds, 4.0f, 1.2f);

    if (proc.hasSample())
    {
        g.setColour(juce::Colour(0xFF5C4A32).brighter(0.3f));
        thumbnail.drawChannels(g, bounds.toNearestInt(), 0.0,
                               thumbnail.getTotalLength(), 1.0f);

        // Playhead
        float ph = proc.getSamplePlayhead();
        float px = bounds.getX() + ph * bounds.getWidth();
        g.setColour(juce::Colour(0xFFD4956A));
        g.drawLine(px, bounds.getY(), px, bounds.getBottom(), 1.5f);

        // Position marker
        float posKnobX = bounds.getX()
            + (*proc.apvts.getRawParameterValue("position")) * bounds.getWidth();
        g.setColour(juce::Colour(0x55D4956A));
        float sw = 8.0f;
        g.fillRect(juce::Rectangle<float>(posKnobX - sw * 0.5f, bounds.getY(),
                                          sw, bounds.getHeight()));
    }
    else
    {
        g.setColour(juce::Colour(0xFF5C4A32));
        g.setFont(makeFont(13.0f));
        g.drawText("drop a sample here  //  or click LOAD",
                   bounds.toNearestInt(), juce::Justification::centred);
    }
}

//==============================================================================
// LabelledKnob
//==============================================================================
LabelledKnob::LabelledKnob(const juce::String& labelText,
                            juce::AudioProcessorValueTreeState& apvts,
                            const juce::String& paramID,
                            IndieKnobLookAndFeel& laf)
{
    slider.setLookAndFeel(&laf);
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled(true, true, nullptr);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setFont(makeFont(10.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, juce::Colour(0xFFD4956A));
    label.setJustificationType(juce::Justification::centred);
    label.setLookAndFeel(&laf);
    addAndMakeVisible(label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, paramID, slider);
}

void LabelledKnob::resized()
{
    auto area = getLocalBounds();
    int  labelH = 14;
    label.setBounds(area.removeFromBottom(labelH));
    slider.setBounds(area);
}

//==============================================================================
// SectionPanel
//==============================================================================
SectionPanel::SectionPanel(const juce::String& t) : title(t)
{
    addAndMakeVisible(contentArea);
}

void SectionPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour(bgCol);
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.2f);
    g.setColour(borderCol.darker(0.4f));
    g.drawRoundedRectangle(bounds.reduced(1.5f), 5.5f, 0.5f);

    // Title pill
    juce::Font titleFont = makeFont(11.0f, juce::Font::bold);
    int tw = (int)titleFont.getStringWidth(title) + 14;
    auto pill = juce::Rectangle<float>(12.0f, -7.0f, (float)tw, 14.0f);
    g.setColour(bgCol);
    g.fillRoundedRectangle(pill, 3.0f);
    g.setColour(borderCol);
    g.drawRoundedRectangle(pill, 3.0f, 1.0f);
    g.setColour(titleCol);
    g.setFont(titleFont);
    g.drawText(title, pill.toNearestInt(), juce::Justification::centred);
}

void SectionPanel::resized()
{
    contentArea.setBounds(getLocalBounds().reduced(8, 12));
}

//==============================================================================
// GranularSynthEditor
//==============================================================================
GranularSynthEditor::GranularSynthEditor(GranularSynthProcessor& p)
    : AudioProcessorEditor(&p), proc(p), waveDisplay(p)
{
    setLookAndFeel(&lookAndFeel);
    setSize(780, 540);

    addAndMakeVisible(waveDisplay);

    // Load button
    loadButton.setButtonText("LOAD");
    loadButton.setLookAndFeel(&lookAndFeel);
    loadButton.onClick = [this]
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Load Sample",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");

        fileChooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc)
            {
                auto results = fc.getResults();
                if (!results.isEmpty())
                {
                    auto file = results[0];
                    if (proc.loadSample(file))
                    {
                        sampleLabel.setText(proc.getSampleName(), juce::dontSendNotification);
                        waveDisplay.repaint();
                    }
                }
            });
    };
    addAndMakeVisible(loadButton);

    // Sample name label
    sampleLabel.setText("no sample loaded", juce::dontSendNotification);
    sampleLabel.setFont(makeFont(11.5f, juce::Font::italic));
    sampleLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF9A8060));
    sampleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(sampleLabel);

    addAndMakeVisible(grainSection);
    addAndMakeVisible(effectsSection);
    addAndMakeVisible(ampSection);

    for (auto* k : { &knobGrainSize, &knobDensity, &knobPosition, &knobPosRand,
                     &knobPitch, &knobPitchRand, &knobPan, &knobVolume })
        grainSection.contentArea.addAndMakeVisible(k);

    for (auto* k : { &knobAttack, &knobDecay, &knobSustain, &knobRelease })
        ampSection.contentArea.addAndMakeVisible(k);

    reverbLabel.setText("REVERB", juce::dontSendNotification);
    reverbLabel.setFont(makeFont(10.0f, juce::Font::bold));
    reverbLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF9A8060));
    effectsSection.contentArea.addAndMakeVisible(reverbLabel);

    delayLabel.setText("DELAY", juce::dontSendNotification);
    delayLabel.setFont(makeFont(10.0f, juce::Font::bold));
    delayLabel.setColour(juce::Label::textColourId, juce::Colour(0xFF9A8060));
    effectsSection.contentArea.addAndMakeVisible(delayLabel);

    for (auto* k : { &knobRevSize, &knobRevDamp, &knobRevWet, &knobRevWidth })
        effectsSection.contentArea.addAndMakeVisible(k);
    for (auto* k : { &knobDelayTime, &knobDelayFB, &knobDelayWet })
        effectsSection.contentArea.addAndMakeVisible(k);

    waveDisplay.setInterceptsMouseClicks(false, false);

    startTimerHz(30);
}

GranularSynthEditor::~GranularSynthEditor()
{
    setLookAndFeel(nullptr);
    stopTimer();
}

//==============================================================================
void GranularSynthEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1A1714));

    // Subtle horizontal lines texture
    g.setColour(juce::Colour(0x08FFFFFF));
    for (int y = 0; y < getHeight(); y += 3)
        g.drawHorizontalLine(y, 0.0f, (float)getWidth());

    // Logo
    g.setColour(juce::Colour(0xFFD4956A));
    g.setFont(makeFont(22.0f, juce::Font::bold | juce::Font::italic));
    g.drawText("granular.", juce::Rectangle<int>(12, 8, 140, 28),
               juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xFF5C4A32));
    g.setFont(makeFont(10.5f));
    g.drawText("by indiegrains", juce::Rectangle<int>(12, 30, 140, 14),
               juce::Justification::centredLeft);
}

//==============================================================================
void GranularSynthEditor::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Header
    auto header = area.removeFromTop(44);
    header.removeFromLeft(160);
    loadButton .setBounds(header.removeFromLeft(52).reduced(0, 8));
    sampleLabel.setBounds(header.reduced(6, 10));

    // Waveform
    waveDisplay.setBounds(area.removeFromTop(80).reduced(0, 4));
    area.removeFromTop(6);

    // Three sections
    int grainW = 340;
    int ampW   = 200;

    auto grainArea = area.removeFromLeft(grainW);
    area.removeFromLeft(8);
    auto ampArea   = area.removeFromLeft(ampW);
    area.removeFromLeft(8);
    auto fxArea    = area;

    grainSection  .setBounds(grainArea);
    ampSection    .setBounds(ampArea);
    effectsSection.setBounds(fxArea);

    // Grain knobs — 2 rows of 4
    {
        auto ca   = grainSection.contentArea.getLocalBounds().reduced(4, 8);
        auto row1 = ca.removeFromTop(ca.getHeight() / 2);
        auto row2 = ca;

        auto placeRow = [](juce::Rectangle<int> row,
                           std::initializer_list<LabelledKnob*> knobs)
        {
            int kw = row.getWidth() / (int)knobs.size();
            for (auto* k : knobs)
                k->setBounds(row.removeFromLeft(kw).reduced(2, 2));
        };
        placeRow(row1, { &knobGrainSize, &knobDensity, &knobPosition, &knobPosRand });
        placeRow(row2, { &knobPitch,     &knobPitchRand, &knobPan,    &knobVolume  });
    }

    // Amp knobs — 1 row of 4
    {
        auto ca = ampSection.contentArea.getLocalBounds().reduced(4, 8);
        int  kw = ca.getWidth() / 4;
        for (auto* k : { &knobAttack, &knobDecay, &knobSustain, &knobRelease })
            k->setBounds(ca.removeFromLeft(kw).reduced(2, 4));
    }

    // Effects knobs — reverb row + delay row
    {
        auto ca    = effectsSection.contentArea.getLocalBounds().reduced(4, 8);
        int  halfH = ca.getHeight() / 2;

        auto revArea = ca.removeFromTop(halfH);
        auto dlyArea = ca;

        reverbLabel.setBounds(revArea.removeFromTop(14));
        delayLabel .setBounds(dlyArea.removeFromTop(14));

        int rkw = revArea.getWidth() / 4;
        for (auto* k : { &knobRevSize, &knobRevDamp, &knobRevWet, &knobRevWidth })
            k->setBounds(revArea.removeFromLeft(rkw).reduced(2, 2));

        int dkw = dlyArea.getWidth() / 3;
        for (auto* k : { &knobDelayTime, &knobDelayFB, &knobDelayWet })
            k->setBounds(dlyArea.removeFromLeft(dkw).reduced(2, 2));
    }
}

//==============================================================================
void GranularSynthEditor::timerCallback()
{
    if (proc.hasSample() && sampleLabel.getText() != proc.getSampleName())
        sampleLabel.setText(proc.getSampleName(), juce::dontSendNotification);
}

//==============================================================================
bool GranularSynthEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
    {
        auto ext = juce::File(f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".aif" || ext == ".aiff" ||
            ext == ".flac" || ext == ".mp3" || ext == ".ogg")
            return true;
    }
    return false;
}

void GranularSynthEditor::filesDropped(const juce::StringArray& files, int, int)
{
    if (files.isEmpty()) return;
    if (proc.loadSample(juce::File(files[0])))
    {
        sampleLabel.setText(proc.getSampleName(), juce::dontSendNotification);
        waveDisplay.repaint();
    }
}

void GranularSynthEditor::layoutKnobRow(juce::Rectangle<int> area,
                                        std::initializer_list<LabelledKnob*> knobs)
{
    int kw = area.getWidth() / (int)knobs.size();
    for (auto* k : knobs)
        k->setBounds(area.removeFromLeft(kw).reduced(2, 2));
}

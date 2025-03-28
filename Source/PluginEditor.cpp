#include "PluginEditor.h"

//==============================================================================
SpectralEQAudioProcessorEditor::SpectralEQAudioProcessorEditor (SpectralEQAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p)
{
    // Set the plugin window size
    setSize (800, 500);

    // Helper lambda for repeated slider setup
    auto setupSlider = [this](juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible (s);
    };

    // Prepare all 9 sliders
    setupSlider (band1FreqSlider);
    setupSlider (band1GainSlider);
    setupSlider (band1QSlider);

    setupSlider (band2FreqSlider);
    setupSlider (band2GainSlider);
    setupSlider (band2QSlider);

    setupSlider (band3FreqSlider);
    setupSlider (band3GainSlider);
    setupSlider (band3QSlider);

    // Attach each slider to a parameter
    band1FreqAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band1Freq", band1FreqSlider);
    band1GainAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band1Gain", band1GainSlider);
    band1QAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band1Q",    band1QSlider);

    band2FreqAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band2Freq", band2FreqSlider);
    band2GainAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band2Gain", band2GainSlider);
    band2QAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band2Q",    band2QSlider);

    band3FreqAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band3Freq", band3FreqSlider);
    band3GainAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band3Gain", band3GainSlider);
    band3QAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band3Q",    band3QSlider);

    // Start a timer to repaint the spectrogram ~30 fps
    startTimerHz (30);
}

SpectralEQAudioProcessorEditor::~SpectralEQAudioProcessorEditor()
{
    // Attachments clean themselves up
}

//==============================================================================
void SpectralEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (juce::Colours::black);

    // Title
    g.setColour (juce::Colours::white);
    // Use the new Font constructor style
    g.setFont (juce::Font().withHeight (18.0f).withTypefaceStyle ("Bold"));

    g.drawFittedText ("Spectral EQ (3-Band) + Spectrogram", 10, 10, 300, 30,
                      juce::Justification::left, 1);

    // We'll draw the FFT-based path below the sliders
    auto scopeRect = getLocalBounds().withTop (150).reduced (10);

    juce::Path freqPath;
    freqPath.startNewSubPath ((float) scopeRect.getX(),
                              (float) scopeRect.getBottom());

    // We'll plot magnitude data from indices [1..(fftSize/2 - 1)]
    const auto halfSize = SpectralEQAudioProcessor::fftSize / 2;

    for (size_t i = 1; i < halfSize; ++i)
    {
        float dBValue = audioProcessor.scopeData[i];

        // Map from -100 dB .. 0 dB -> vertical range
        float yNorm = juce::jmap (dBValue,
                                  -100.0f,
                                  0.0f,
                                  (float) scopeRect.getHeight(),
                                  0.0f);

        // Map from i -> horizontal range
        float xNorm = juce::jmap ((float) i,
                                  0.0f,
                                  (float) halfSize,
                                  0.0f,
                                  (float) scopeRect.getWidth());

        float x = (float) scopeRect.getX() + xNorm;
        float y = (float) scopeRect.getY() + yNorm;
        freqPath.lineTo (x, y);
    }

    g.setColour (juce::Colours::green);
    g.strokePath (freqPath, juce::PathStrokeType (1.5f));
}

void SpectralEQAudioProcessorEditor::resized()
{
    // Lay out the 9 sliders across the top
    auto area = getLocalBounds().reduced (10);
    auto sliderArea = area.removeFromTop (140);

    const int columnWidth = sliderArea.getWidth() / 3;

    // Band 1
    {
        auto bandArea = sliderArea.removeFromLeft (columnWidth);
        band1FreqSlider.setBounds (bandArea.removeFromTop (bandArea.getHeight() / 3));
        band1GainSlider.setBounds (bandArea.removeFromTop (bandArea.getHeight() / 2));
        band1QSlider.setBounds    (bandArea);
    }
    // Band 2
    {
        auto bandArea = sliderArea.removeFromLeft (columnWidth);
        band2FreqSlider.setBounds (bandArea.removeFromTop (bandArea.getHeight() / 3));
        band2GainSlider.setBounds (bandArea.removeFromTop (bandArea.getHeight() / 2));
        band2QSlider.setBounds    (bandArea);
    }
    // Band 3
    {
        auto bandArea = sliderArea.removeFromLeft (columnWidth);
        band3FreqSlider.setBounds (bandArea.removeFromTop (bandArea.getHeight() / 3));
        band3GainSlider.setBounds (bandArea.removeFromTop (bandArea.getHeight() / 2));
        band3QSlider.setBounds    (bandArea);
    }
}

//==============================================================================
void SpectralEQAudioProcessorEditor::timerCallback()
{
    // If new FFT data is ready, repaint the spectrogram
    if (audioProcessor.newDataReady.exchange (false))
        repaint();
}

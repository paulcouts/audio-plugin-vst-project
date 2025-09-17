/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
SpectralEQAudioProcessorEditor::SpectralEQAudioProcessorEditor (SpectralEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (800, 500);

    // Helper lambda to configure a slider
    auto setupSlider = [this](juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible (s);
    };

    // Set up the 9 sliders
    setupSlider (band1FreqSlider);
    setupSlider (band1GainSlider);
    setupSlider (band1QSlider);

    setupSlider (band2FreqSlider);
    setupSlider (band2GainSlider);
    setupSlider (band2QSlider);

    setupSlider (band3FreqSlider);
    setupSlider (band3GainSlider);
    setupSlider (band3QSlider);

    // Create attachments to our AudioProcessorValueTreeState
    // -- Band 1
    band1FreqAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band1Freq", band1FreqSlider);
    band1GainAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band1Gain", band1GainSlider);
    band1QAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band1Q",    band1QSlider);

    // -- Band 2
    band2FreqAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band2Freq", band2FreqSlider);
    band2GainAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band2Gain", band2GainSlider);
    band2QAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band2Q",    band2QSlider);

    // -- Band 3
    band3FreqAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band3Freq", band3FreqSlider);
    band3GainAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band3Gain", band3GainSlider);
    band3QAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                                audioProcessor.apvts, "Band3Q",    band3QSlider);

    // Start a timer to update the FFT visualization about 30 times per second
    startTimerHz (30);
}

SpectralEQAudioProcessorEditor::~SpectralEQAudioProcessorEditor()
{
    // Attachments automatically clean up
}

//==============================================================================
void SpectralEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);

    // Title
    g.setColour (juce::Colours::white);
    g.setFont   (juce::Font (18.0f, juce::Font::bold));
    g.drawFittedText ("Spectral EQ (3-Band) + Spectrogram", 10, 10, 300, 30, juce::Justification::left, 1);

    // Draw the FFT spectrogram in the region below the sliders
    auto scopeRect = getLocalBounds().withTop (150).reduced (10);

    juce::Path freqPath;
    freqPath.startNewSubPath (scopeRect.getX(), scopeRect.getBottom());

    // plot the magnitude data from 0..fftSize/2
    const auto halfSize = SpectralEQAudioProcessor::fftSize / 2;

    for (size_t i = 1; i < halfSize; ++i)
    {
        auto dBValue = audioProcessor.scopeData[i];
        // Map from -100 dB .. 0 dB to the height of the rect
        auto yNorm = juce::jmap (dBValue, -100.0f, 0.0f, (float) scopeRect.getHeight(), 0.0f);
        auto xNorm = juce::jmap ((float) i, 0.0f, (float) halfSize, 0.0f, (float) scopeRect.getWidth());

        float x = (float) scopeRect.getX() + xNorm;
        float y = (float) scopeRect.getY() + yNorm;
        freqPath.lineTo (x, y);
    }

    g.setColour (juce::Colours::green);
    g.strokePath (freqPath, juce::PathStrokeType (1.5f));
}

void SpectralEQAudioProcessorEditor::resized()
{
    // Layout the 9 sliders in a grid at the top of the plugin
    auto area = getLocalBounds().reduced (10);
    auto sliderArea = area.removeFromTop (140);

    //n3 columns, each column for one band
    // Each column has 3 sliders: freq, gain, Q
    const int columnWidth = sliderArea.getWidth() / 3;

    auto band1Area = sliderArea.removeFromLeft (columnWidth);
    band1FreqSlider.setBounds (band1Area.removeFromTop (band1Area.getHeight() / 3));
    band1GainSlider.setBounds (band1Area.removeFromTop (band1Area.getHeight() / 2));
    band1QSlider.setBounds    (band1Area);

    auto band2Area = sliderArea.removeFromLeft (columnWidth);
    band2FreqSlider.setBounds (band2Area.removeFromTop (band2Area.getHeight() / 3));
    band2GainSlider.setBounds (band2Area.removeFromTop (band2Area.getHeight() / 2));
    band2QSlider.setBounds    (band2Area);

    auto band3Area = sliderArea.removeFromLeft (columnWidth);
    band3FreqSlider.setBounds (band3Area.removeFromTop (band3Area.getHeight() / 3));
    band3GainSlider.setBounds (band3Area.removeFromTop (band3Area.getHeight() / 2));
    band3QSlider.setBounds    (band3Area);
}

//==============================================================================
void SpectralEQAudioProcessorEditor::timerCallback()
{
    // If the processor says new FFT data is ready, repaint
    if (audioProcessor.newDataReady.exchange (false))
        repaint();
}

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
    Our Editor shows:
    - 3 sets of sliders (Freq, Gain, Q) for each band
    - A real-time spectrogram of the output signal
*/
class SpectralEQAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    SpectralEQAudioProcessorEditor (SpectralEQAudioProcessor&);
    ~SpectralEQAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    SpectralEQAudioProcessor& audioProcessor;

    // We'll have 9 sliders total (3 freq, 3 gain, 3 Q).
    juce::Slider band1FreqSlider, band1GainSlider, band1QSlider;
    juce::Slider band2FreqSlider, band2GainSlider, band2QSlider;
    juce::Slider band3FreqSlider, band3GainSlider, band3QSlider;

    // And each slider has an attachment to the APVTS parameters
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> band1FreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> band1GainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> band1QAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> band2FreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> band2GainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> band2QAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> band3FreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> band3GainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> band3QAttachment;

    // Called ~30 times/sec to refresh the spectrogram
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralEQAudioProcessorEditor)
};

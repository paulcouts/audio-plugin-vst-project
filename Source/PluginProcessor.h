#pragma once

#include <JuceHeader.h>

/**
    A simple struct to hold references to the three parameters
    for each EQ band: Frequency, Gain (in dB), and Q (resonance).
*/
struct BandParameters
{
    juce::AudioParameterFloat* freqParam = nullptr;
    juce::AudioParameterFloat* gainParam = nullptr;
    juce::AudioParameterFloat* qParam    = nullptr;
};

//==============================================================================
/**
    A processor that:
    1) Applies a 3-band parametric EQ using JUCE’s dsp module.
    2) Displays a real-time FFT-based spectrogram in the Editor.
*/
class SpectralEQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SpectralEQAudioProcessor();
    ~SpectralEQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override         { return JucePlugin_Name; }
    bool acceptsMidi() const override                   { return false; }
    bool producesMidi() const override                  { return false; }
    bool isMidiEffect() const override                  { return false; }
    double getTailLengthSeconds() const override        { return 0.0; }

    //==============================================================================
    // We only provide 1 dummy program, so these methods ignore their parameters.
    int getNumPrograms() override                       { return 1; }
    int getCurrentProgram() override                    { return 0; }
    void setCurrentProgram (int index) override         { (void)index; }
    const juce::String getProgramName (int index) override
    {
        (void)index;
        return {};
    }
    void changeProgramName (int index, const juce::String& newName) override
    {
        (void)index;
        (void)newName;
    }

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    /** Holds all plugin parameters (EQ bands, etc.). */
    juce::AudioProcessorValueTreeState apvts;

    //==============================================================================
    /** 
        FFT-related constants and buffers.
        We do a 1024-point FFT for visualization only.
    */
    static constexpr size_t fftOrder = 10;  // 2^10 = 1024
    static constexpr size_t fftSize  = 1 << fftOrder;

    std::array<float, fftSize * 2> fftData;  // Real + Imag
    std::array<float, fftSize>     scopeData; // Decibel magnitudes for the UI
    std::atomic<bool> newDataReady { false };

private:
    //==============================================================================
    /** The 3 parametric EQ bands (using ProcessorDuplicator for stereo). */
    using Filter    = juce::dsp::IIR::Filter<float>;
    using Coeffs    = juce::dsp::IIR::Coefficients<float>;
    using PeakFilter= juce::dsp::ProcessorDuplicator<Filter, Coeffs>;

    // Our chain of 3 parametric peak filters
    juce::dsp::ProcessorChain<PeakFilter, PeakFilter, PeakFilter> filterChain;

    // The per-band parameter references
    BandParameters band1, band2, band3;

    // Updates all filter coefficients from the parameter values
    void updateFilterChain();

    //==============================================================================
    /** FIFO for gathering samples for the FFT. */
    std::array<float, fftSize> fifo;
    int  fifoIndex         = 0;
    bool nextFFTBlockReady = false;

    juce::dsp::FFT                 forwardFFT { fftOrder };
    juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };

    //==============================================================================
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralEQAudioProcessor)
};


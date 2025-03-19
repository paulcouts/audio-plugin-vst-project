#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SpectralEQAudioProcessor::SpectralEQAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // Zero out FFT buffers
    fftData.fill (0.0f);
    scopeData.fill (0.0f);
    fifo.fill (0.0f);

    // Fetch parameter references for band 1
    band1.freqParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band1Freq"));
    band1.gainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band1Gain"));
    band1.qParam    = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band1Q"));

    // Band 2
    band2.freqParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band2Freq"));
    band2.gainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band2Gain"));
    band2.qParam    = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band2Q"));

    // Band 3
    band3.freqParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band3Freq"));
    band3.gainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band3Gain"));
    band3.qParam    = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band3Q"));
}

SpectralEQAudioProcessor::~SpectralEQAudioProcessor() {}

//==============================================================================
void SpectralEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 2;

    // Prepare our filter chain
    filterChain.prepare (spec);
    updateFilterChain();

    // Reset the FIFO and flags
    fifoIndex = 0;
    nextFFTBlockReady = false;
    fifo.fill(0.0f);
}

void SpectralEQAudioProcessor::releaseResources()
{
    // Not used here
}

bool SpectralEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Only allow stereo in/out
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

//==============================================================================
void SpectralEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Update the filter coefficients if any parameter changed
    updateFilterChain();

    // Create an AudioBlock and process through our filter chain
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    filterChain.process (context);

    // --- FFT for visualization (using left channel) ---
    auto* leftChannelData = buffer.getReadPointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        fifo[(size_t) fifoIndex] = leftChannelData[i];
        fifoIndex++;

        if (fifoIndex == fftSize)
        {
            fifoIndex = 0;
            nextFFTBlockReady = true;
        }
    }

    // If the FIFO is full, do the forward FFT
    if (nextFFTBlockReady)
    {
        window.multiplyWithWindowingTable (fifo.data(), fftSize);

        // Copy real samples into fftData and clear imaginary part
        std::copy (fifo.begin(), fifo.end(), fftData.begin());
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

        // Perform the FFT
        forwardFFT.performRealOnlyForwardTransform (fftData.data());

        // Convert to decibels for visualization
        for (size_t i = 0; i < fftSize / 2; ++i)
        {
            auto real = fftData[i * 2];
            auto imag = fftData[i * 2 + 1];
            auto magnitude = std::sqrt (real * real + imag * imag);
            scopeData[i] = juce::Decibels::gainToDecibels (magnitude);
        }

        newDataReady.store (true);
        nextFFTBlockReady = false;
    }
}

//==============================================================================
juce::AudioProcessorEditor* SpectralEQAudioProcessor::createEditor()
{
    return new SpectralEQAudioProcessorEditor (*this);
}

//==============================================================================
void SpectralEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Store parameters in the MemoryBlock
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SpectralEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore parameters from the MemoryBlock
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr)
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
/**
    This is where we define all the plugin parameters.
    - 3 bands, each with Frequency, Gain (dB), and Q.
*/
juce::AudioProcessorValueTreeState::ParameterLayout SpectralEQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Band 1
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "Band1Freq", "Band1 Freq", 
         juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.5f), 200.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "Band1Gain", "Band1 Gain (dB)",
         juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "Band1Q", "Band1 Q",
         juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f), 1.0f));

    // Band 2
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "Band2Freq", "Band2 Freq",
         juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.5f), 1000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "Band2Gain", "Band2 Gain (dB)",
         juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "Band2Q", "Band2 Q",
         juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f), 1.0f));

    // Band 3
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "Band3Freq", "Band3 Freq",
         juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.5f), 5000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "Band3Gain", "Band3 Gain (dB)",
         juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "Band3Q", "Band3 Q",
         juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f), 1.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
/**
    Updates the 3 peak filters in our chain based on the current
    parameter values (frequency, gain, Q).
*/
void SpectralEQAudioProcessor::updateFilterChain()
{
    auto updatePeakFilter = [] (auto& peakFilter, float freq, float gainDecibels, float Q, double sampleRate)
    {
        auto gainLinear = juce::Decibels::decibelsToGain (gainDecibels, -60.0f);
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, freq, Q, gainLinear);
        *peakFilter.state = *coeffs;
    };

    double sampleRate = getSampleRate();

    // Band 1
    {
        auto& peak = filterChain.get<0>();
        updatePeakFilter (peak, band1.freqParam->get(), band1.gainParam->get(), band1.qParam->get(), sampleRate);
    }

    // Band 2
    {
        auto& peak = filterChain.get<1>();
        updatePeakFilter (peak, band2.freqParam->get(), band2.gainParam->get(), band2.qParam->get(), sampleRate);
    }

    // Band 3
    {
        auto& peak = filterChain.get<2>();
        updatePeakFilter (peak, band3.freqParam->get(), band3.gainParam->get(), band3.qParam->get(), sampleRate);
    }
}

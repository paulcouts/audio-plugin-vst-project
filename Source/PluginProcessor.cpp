#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SpectralEQAudioProcessor::SpectralEQAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
    // Clear FFT buffers
    fftData.fill (0.0f);
    scopeData.fill (0.0f);
    fifo.fill (0.0f);

    // Link parameter references for each band
    band1.freqParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band1Freq"));
    band1.gainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band1Gain"));
    band1.qParam    = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band1Q"));

    band2.freqParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band2Freq"));
    band2.gainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band2Gain"));
    band2.qParam    = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band2Q"));

    band3.freqParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band3Freq"));
    band3.gainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band3Gain"));
    band3.qParam    = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter ("Band3Q"));
}

SpectralEQAudioProcessor::~SpectralEQAudioProcessor()
{
}

//==============================================================================
void SpectralEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 2;

    filterChain.prepare (spec);
    updateFilterChain();

    // Reset the FIFO & flags
    fifoIndex         = 0;
    nextFFTBlockReady = false;
    fifo.fill (0.0f);
}

void SpectralEQAudioProcessor::releaseResources()
{
    // Nothing special here
}

bool SpectralEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // We only allow stereo in/out
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

//==============================================================================
void SpectralEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& midiMessages)
{
    // We don't use midiMessages in this plugin
    (void) midiMessages;

    // Update EQ filters in case parameters changed
    updateFilterChain();

    // Run the filter chain
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    filterChain.process (context);

    // --- FFT for real-time spectrogram (left channel) ---
    auto* leftChannelData = buffer.getReadPointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        fifo[static_cast<size_t>(fifoIndex)] = leftChannelData[i];
        fifoIndex++;

        if (fifoIndex == fftSize)
        {
            fifoIndex         = 0;
            nextFFTBlockReady = true;
        }
    }

    if (nextFFTBlockReady)
    {
        // Window the data
        window.multiplyWithWindowingTable (fifo.data(), fftSize);

        // Copy real samples -> fftData; zero out imaginary part
        std::copy (fifo.begin(), fifo.end(), fftData.begin());
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);

        // Forward FFT
        forwardFFT.performRealOnlyForwardTransform (fftData.data());

        // Convert to decibels
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
    // Our custom editor class
    return new SpectralEQAudioProcessorEditor (*this);
}

//==============================================================================
void SpectralEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Copy state
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void SpectralEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore state
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout SpectralEQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ======================
    // Band 1
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "Band1Freq", "Band1 Freq",
         juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.5f), 200.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "Band1Gain", "Band1 Gain (dB)",
         juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "Band1Q", "Band1 Q",
         juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f), 1.0f));

    // ======================
    // Band 2
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "Band2Freq", "Band2 Freq",
         juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.5f), 1000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "Band2Gain", "Band2 Gain (dB)",
         juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "Band2Q", "Band2 Q",
         juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f), 1.0f));

    // ======================
    // Band 3
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "Band3Freq", "Band3 Freq",
         juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.5f), 5000.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "Band3Gain", "Band3 Gain (dB)",
         juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f));
    params.push_back (std::make_unique<juce::AudioParameterFloat>(
        "Band3Q", "Band3 Q",
         juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f), 1.0f));

    return { params.begin(), params.end() };
}

//==============================================================================
void SpectralEQAudioProcessor::updateFilterChain()
{
    auto updatePeakFilter = [] (auto& peakFilter, float freq, float gainDb, float Q, double sampleRate)
    {
        auto gainLinear = juce::Decibels::decibelsToGain (gainDb, -60.0f);
        auto coeffs     = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate,
                                                                               freq,
                                                                               Q,
                                                                               gainLinear);
        *peakFilter.state = *coeffs;
    };

    double sampleRate = getSampleRate();

    // Band 1
    {
        auto& peak = filterChain.get<0>();
        updatePeakFilter (peak, band1.freqParam->get(),
                                band1.gainParam->get(),
                                band1.qParam->get(),
                                sampleRate);
    }
    // Band 2
    {
        auto& peak = filterChain.get<1>();
        updatePeakFilter (peak, band2.freqParam->get(),
                                band2.gainParam->get(),
                                band2.qParam->get(),
                                sampleRate);
    }
    // Band 3
    {
        auto& peak = filterChain.get<2>();
        updatePeakFilter (peak, band3.freqParam->get(),
                                band3.gainParam->get(),
                                band3.qParam->get(),
                                sampleRate);
    }
}

//==============================================================================
/** Required for JUCE to instantiate the plugin. */
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectralEQAudioProcessor();
}

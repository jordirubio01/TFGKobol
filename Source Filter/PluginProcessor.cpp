/*
  ==============================================================================

    PLUGINPROCESSOR.CPP

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "Utils.h"

namespace IDs { // IDs utilitzades per la GUI
    static juce::Identifier oscilloscope { "oscilloscope" }; // Oscil·loscopi
    static juce::Identifier fft { "FFT" }; // Analitzador FFT
}

//==============================================================================
KobolVCFAudioProcessor::KobolVCFAudioProcessor() // Constructor
    : foleys::MagicProcessor(
          juce::AudioProcessor::BusesProperties() // Inicialització (bus de sortida estèreo, àudio)
              .withInput ("Input",  juce::AudioChannelSet::stereo(), true)   // Entrada externa
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    FOLEYS_SET_SOURCE_PATH(__FILE__);

    castParameter(apvts, ParameterID::outputLevel, outputLevelParam); // Enllaça outputLevel amb outputLevelParam

    castParameter(apvts, ParameterID::vcfInputLevel, vcfInputLevelParam);
    castParameter(apvts, ParameterID::cutoffParam, cutoffParam);
    castParameter(apvts, ParameterID::resonanceParam, resonanceParam);
    castParameter(apvts, ParameterID::filterBypass, filterBypassParam);

    apvts.state.addListener(this);

    // MAGIC GUI: register an oscilloscope to display in the GUI. keep a pointer to push samples into in processBlock(). Only interested in channel 0
    oscilloscope = magicState.createAndAddObject<foleys::MagicOscilloscope>(IDs::oscilloscope, 0); // Oscil·loscopi (canal esquerre)
    analyser     = magicState.createAndAddObject<foleys::MagicAnalyser>(IDs::fft, 1); // Analitzador FFT

    magicState.setGuiValueTree(BinaryData::magicFilter_xml, BinaryData::magicFilter_xmlSize); //Loads magic_XML config
}


//==============================================================================

KobolVCFAudioProcessor::~KobolVCFAudioProcessor(){ // Destructor

    apvts.state.removeListener(this);
}


//==============================================================================
//PrePareToPlay and releasesResources:
//==============================================================================

void KobolVCFAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    filterL.prepare(sampleRate);
    filterR.prepare(sampleRate);

    outputLevelSmoother.reset(sampleRate, 0.05);
    inputLevelSmoother.reset(sampleRate, 0.05);

    parametersChanged.store(true);
    analyser->prepareToPlay(sampleRate, samplesPerBlock);
    magicState.prepareToPlay(sampleRate, samplesPerBlock);
    reset();
}

//==============================================================================

void KobolVCFAudioProcessor::releaseResources() {}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool KobolVCFAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Check if the layout is supported. Thisc code only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}
#endif

//==============================================================================
void KobolVCFAudioProcessor::update()
{
    outputLevelSmoother.setTargetValue(
        juce::Decibels::decibelsToGain(outputLevelParam->get()));
    inputLevelSmoother.setTargetValue(
        vcfInputLevelParam->get() / 5.0f);

    filterCutoff    = cutoffCurve(cutoffParam->get());
    filterResonance = resonanceParam->get();
    filterBypass    = filterBypassParam->get() > 0.5f;
}


//==============================================================================
//Reset
//==============================================================================

void KobolVCFAudioProcessor::reset()
{
    filterL.reset();
    filterR.reset();
    outputLevelSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputLevelParam->get()));
    inputLevelSmoother.setCurrentAndTargetValue(vcfInputLevelParam->get() / 5.0f);
}

//==============================================================================
void KobolVCFAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    magicState.processMidiBuffer(midiMessages, buffer.getNumSamples()); //Allows PGM keyboard to be used

    bool expected = true;
    if (isNonRealtime() || parametersChanged.compare_exchange_strong(expected, false))
        update();

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    float* left  = buffer.getWritePointer(0);
    float* right = (numChannels > 1) ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        const float inLevel  = inputLevelSmoother.getNextValue();
        const float outLevel = outputLevelSmoother.getNextValue();

        // Canal esquerre (sempre present)
        float sampleL = left[i] * inLevel;
        if (!filterBypass)
            sampleL = filterL.processSample(sampleL, filterCutoff, filterResonance);
        left[i] = sampleL * outLevel;

        // Canal dret (si existeix; si no, es reutilitza el filtre esquerre)
        if (right != nullptr)
        {
            float sampleR = right[i] * inLevel;
            if (!filterBypass)
                sampleR = filterR.processSample(sampleR, filterCutoff, filterResonance);
            right[i] = sampleR * outLevel;
        }
    }

    protectYourEars(left,  numSamples);
    if (right) protectYourEars(right, numSamples);

    oscilloscope->pushSamples(buffer);
    analyser->pushSamples(buffer);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
KobolVCFAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::outputLevel, "Output Level",
        juce::NormalisableRange<float>(-24.0f, 6.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::vcfInputLevel, "VCF Input Level",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 5.0f,
        juce::AudioParameterFloatAttributes().withLabel("V")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::cutoffParam, "VCF Cutoff",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 7.0f,
        juce::AudioParameterFloatAttributes().withLabel("V")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::resonanceParam, "VCF Resonance",
        juce::NormalisableRange<float>(0.0f, 1.225f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel("")));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        ParameterID::filterBypass, "Filter Bypass",
        juce::NormalisableRange<float>(0.0f, 1.0f, 1.0f), 0.0f));

    return layout;
}

//==============================================================================
//PARAMETERS: Saving and loading parameters state
//==============================================================================
void KobolVCFAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Method to store your parameters in the memory block.

    magicState.getStateInformation(destData);
}

void KobolVCFAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    magicState.setStateInformation(data, sizeInBytes, getActiveEditor());

    if (resonanceParam->get() >= 1.0f)
        resonanceParam->setValueNotifyingHost(resonanceParam->convertTo0to1(0.99f));
}

//==============================================================================
// This creates new instances of the plugin..

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KobolVCFAudioProcessor();
}
//==============================================================================
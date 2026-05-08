/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.
    Moog Ladder Filter effect over external signal.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Filter.h"


//==============================================================================
//ParameterID:
//==============================================================================

namespace ParameterID // IDs dels paràmetres del plugin
{
    #define PARAMETER_ID(str) const juce::ParameterID str(#str, 1);

    PARAMETER_ID(outputLevel) // ID del nivell de sortida

    PARAMETER_ID(vcfInputLevel)
    PARAMETER_ID(cutoffParam)
    PARAMETER_ID(resonanceParam)
    PARAMETER_ID(filterBypass)

    #undef PARAMETER_ID
}


//==============================================================================
//CLASS DEFINITION:
//==============================================================================

class KobolVCFAudioProcessor : public foleys::MagicProcessor,
                                private juce::ValueTree::Listener
{
public:
    //==============================================================================
    KobolVCFAudioProcessor(); // Constructor
    ~KobolVCFAudioProcessor() override; // Destructor amb override
    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override; // Abans de començar l'àudio
    void releaseResources() override; // Allibera recursos quan s'atura l'àudio
    void reset() override; // Reinicia l'estat intern del processador

    #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    #endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override; // Funció principal processament àudio i MIDI

    //APVTS
    juce::AudioProcessorValueTreeState apvts;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;


private:
    // Instància d'un filtre per canal (estèreo)
    Filter filterL, filterR;

    juce::LinearSmoothedValue<float> outputLevelSmoother;
    juce::LinearSmoothedValue<float> inputLevelSmoother;

    void update();

    // Corbes de conversió (igual que a l'original)
    static float cutoffCurve(float v)
    {
        return juce::jlimit(16.0f, 16000.0f, 16.0f * std::exp2(v));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<bool> parametersChanged { false };
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override
    {
        parametersChanged.store(true);
    }

    // Paràmetres (actualitzats per update())
    float filterCutoff    { 16000.0f };
    float filterResonance { 0.0f };
    bool  filterBypass    { false };

    // Paràmetres APVTS
    juce::AudioParameterFloat* outputLevelParam;    // Punter al paràmetre nivell de sortida
    juce::AudioParameterFloat* vcfInputLevelParam;  // Punter al paràmetre nivell abans del filtre
    juce::AudioParameterFloat* cutoffParam;         // Punter al paràmetre cutoff
    juce::AudioParameterFloat* resonanceParam;      // Punter al paràmetre resonance
    juce::AudioParameterFloat* filterBypassParam;   // Punter al bypass del filtre

    // Foleys GUI
    foleys::MagicPlotSource* oscilloscope = nullptr; // Oscil·loscopi (GUI)
    foleys::MagicPlotSource* analyser = nullptr; // Espectre

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KobolVCFAudioProcessor)
};
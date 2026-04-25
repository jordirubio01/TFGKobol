/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Synth.h"


//#define USE_PGM 0
#define USE_PGM 1

//==============================================================================
//ParameterID:
//==============================================================================

namespace ParameterID // IDs dels paràmetres del plugin
{
    #define PARAMETER_ID(str) const juce::ParameterID str(#str, 1);

   

    PARAMETER_ID(outputLevel) // ID del nivell de sortida

    PARAMETER_ID(waveF) // ID de la waveform

    PARAMETER_ID(attackParam)
    PARAMETER_ID(decayParam)
    PARAMETER_ID(sustainParam)
    PARAMETER_ID(decayOff)

    PARAMETER_ID(vcfInputLevel)
    PARAMETER_ID(cutoffParam)
    PARAMETER_ID(resonanceParam)

    #undef PARAMETER_ID
}


//==============================================================================
//CLASS DEFINITION:
//==============================================================================
/**
*/
//class KOSCAudioProcessor  : public juce::AudioProcessor, private juce::ValueTree::Listener
class KobolVCOAudioProcessor  :
#if USE_PGM==0
public  juce::AudioProcessor, private juce::ValueTree::Listener
#else
public  foleys::MagicProcessor, private juce::ValueTree::Listener
#endif
{
public:
    
    //==============================================================================
    KobolVCOAudioProcessor(); // Constructor
    ~KobolVCOAudioProcessor() override; // Destructor amb override
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override; // Abans de començar l'àudio
    void releaseResources() override; // Allibera recursos quan s'atura l'àudio
    void reset() override; // Reinicia l'estat intern del processador

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override; // Funció principal processament àudio i MIDI
    
    //APVTS
    juce::AudioProcessorValueTreeState apvts;

    
    //==============================================================================
#if USE_PGM==0
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
#endif
    
    //==============================================================================


    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    
private:
    
    Synth synth; // Instància del motor de síntesi
 
    void splitBufferByEvents(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages); // Divideix segons esdeveniments MIDI
    void handleMIDI (uint8_t data0, uint8_t data1, uint8_t data2); // Processa missatges MIDI individuals (inclou midiMessage de synth)
    void render(juce::AudioBuffer<float>& buffer, int sampleCount, int bufferOffset); // Genera l'àudio (inclou render de synth)
    
    void update();
    
  
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    std::atomic<bool> parametersChanged{false};
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override {
        
        parametersChanged.store(true);
    }
    
    

    juce::AudioParameterFloat* outputLevelParam;   // Punter al paràmetre nivell de sortida
    juce::AudioParameterFloat* waveFParam;         // Punter al paràmetre waveform
    juce::AudioParameterFloat* attackParam;        // Punter al paràmetre d'attack
    juce::AudioParameterFloat* decayParam;         // Punter al paràmetre de decay
    juce::AudioParameterFloat* sustainParam;       // Punter al paràmetre de sustain
    juce::AudioParameterFloat* decayOffParam;      // Punter al paràmetre decay off

    juce::AudioParameterFloat* vcfInputLevelParam; // Punter al paràmetre nivell abans del filtre
    juce::AudioParameterFloat* cutoffParam;        // Punter al paràmetre cutoff
    juce::AudioParameterFloat* resonanceParam;     // Punter al paràmetre resonance

    // FOLEYS MAGIC GUI:
    foleys::MagicPlotSource* oscilloscope = nullptr; // Oscil·loscopi (GUI)
    foleys::MagicPlotSource* analyser = nullptr; // Espectre
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KobolVCOAudioProcessor)
};

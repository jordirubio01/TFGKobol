/*
  ==============================================================================

    Synth.h
    Created: 4 Jun 2024 1:55:22pm
    Author:  valen

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Voice.h"
#include "Utils.h"

class Synth{
    
public:
    

    juce::LinearSmoothedValue<float> outputLevelSmoother; // Suavitza canvis de nivell (evita clics)
    float waveForm; // Tipus de forma d'ona
    
    Synth(); // Constructor
    
    void allocateResources(double sampleRate, int samplesPerBlock);
    void deallocateResources();
    void reset();
    void render (float** outputBuffers, int sampleCount);
    void midiMessage (uint8_t data0, uint8_t data1, uint8_t data2);
    


private:
    float sampleRate; // Freqüència de mostreig actual
    Voice voice; // Instància d'una veu (caldrà poder decidir entre monofònic o polifònic) !
    void noteOn(int note, int velocity); // Esdeveniment nota activada
    void noteOff(int note); // Esdeveniment nota desactivada
    
    void controlChange(uint8_t data1, uint8_t data2); // Missatges MIDI de tipus control (slider, knobs)
    

    
};

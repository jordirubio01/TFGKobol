/*
  ==============================================================================

    Voice.h

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Oscillator.h"



struct Voice
{
    int note;       // Número de nota (si és 0, no n'hi ha cap activa)
    Oscillator osc; // Oscil·lador encarregat de generar el senyal
    float waveForm; // Forma d'ona

    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;

    
    void reset(){//OSC reset
        note=0;
        osc.reset();
        adsr.reset();
    }

    void prepare(double sampleRate) { // Cridat des de Synth::allocateResources
        adsr.setSampleRate(sampleRate);
    }
    
    
    float render(){ //Render Nextsample
        float sample = osc.nextSample(waveForm); // Crida l'oscil·lador per generar la mostra següent
        return sample * adsr.getNextSample(); // Aplica l'envolvent
    }
    
};


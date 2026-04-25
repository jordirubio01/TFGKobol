/*
  ==============================================================================

    Voice.h

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Oscillator.h"
#include "Filter.h"



struct Voice
{
    int note;       // Número de nota (si és 0, no n'hi ha cap activa)
    Oscillator osc; // Oscil·lador encarregat de generar el senyal
    float waveForm; // Forma d'ona

    // VCF
    Filter filter;
    float filterCutoff {22000.0f}; // Freqüència de tall
    float filterResonance {0.0f};  // Resonància [0, 1]

    juce::ADSR adsr;
    juce::ADSR::Parameters adsrParams;

    
    void reset(){//OSC reset
        note=0;
        osc.reset();
        filter.reset();
        adsr.reset();
    }

    void prepare(double sampleRate) { // Cridat des de Synth::allocateResources
        adsr.setSampleRate(sampleRate);
        filter.prepare(sampleRate);
    }
    
    
    float render(){ //Render Nextsample
        // 1. VCO
        float sample = osc.nextSample(waveForm); // Crida l'oscil·lador per generar la mostra següent
        // 2. VCF
        sample = filter.processSample(sample, filterCutoff, filterResonance);
        // 3. VCA
        return sample * adsr.getNextSample(); // Aplica l'envolvent
    }
    
};


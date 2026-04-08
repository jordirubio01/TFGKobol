/*
  ==============================================================================

    Voice.h

  ==============================================================================
*/

#pragma once
#include "Oscillator.h"



struct Voice
{
    int note;       // Número de nota (si és 0, no n'hi ha cap activa)
    Oscillator osc; // Oscil·lador encarregat de generar el senyal
    float waveForm; // Forma d'ona

    
    void reset(){//OSC reset
        note=0;
        osc.reset();
    }
    
    
    float render(){ //Render Nextsample
        return osc.nextSample(waveForm); // Crida l'oscil·lador per generar la mostra següent
    }
    
};


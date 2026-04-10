/*
  ==============================================================================

    Synth.cpp

  ==============================================================================
*/

#include "Synth.h"

Synth::Synth(){ // Constructor
    sampleRate=44100.0f; // Freqüència de mostreig per defecte 44100 Hz
}

void Synth::allocateResources(double sampleRate_, int /*samplesPerBlock*/){
    sampleRate=static_cast<float>(sampleRate_);
    voice.prepare(sampleRate_);
}

void Synth::deallocateResources(){
    // do nothing
}

void Synth::reset(){
    
    voice.reset();
    outputLevelSmoother.reset(sampleRate, 0.05);
}
//--------------------------------------------------------------------------------------
//NOTE ON/NOTE OFF:
//--------------------------------------------------------------------------------------
//NoteOn
void Synth::noteOn(int note, int velocity){ // Quan arriba un missatge MIDI NOTE ON
    
    voice.note=note; // Guarda el número de nota activa
    
    float freq=440.0f * std::exp2(float(note-69)/12.0f); // Converteix la nota a freqüència

    voice.osc.amplitude=(velocity/127.0f)*0.5f; // Amplitud segons la velocitat MIDI (0-127), màxim 50%
    voice.osc.inc=freq/sampleRate; // Increment de fase segons la freqüència
    voice.osc.reset(); // Reinicia la fase per començar la nota de zero

    // Actualitza paràmetres i aplica l'envolvent
    juce::ADSR::Parameters p;
    p.attack  = attackTime;
    p.decay   = decayTime;
    p.sustain = sustainLevel;
    p.release = 0.05f; // release curt per defecte
    voice.adsrParams = p;
    voice.adsr.setParameters(p);
    voice.adsr.noteOn();
}



//NoteOFF:
void Synth::noteOff(int note){ // Quan arriba un missatge MIDI NOTE OFF
    if(voice.note==note){ // Si la nota que s'apaga és la mateixa que està sonant...
        //voice.note=0; // Desactiva la veu
        voice.adsr.noteOff(); // Aplica release en comptes de tall abrupte
    }
}

//--------------------------------------------------------------------------------------
//MidiMessage:
//--------------------------------------------------------------------------------------
void Synth::midiMessage(uint8_t data0, uint8_t data1, uint8_t data2){ // Rep un missatge MIDI de 3 bytes
    
    switch (data0 & 0xF0){ // Extreu el tipus de missatge (NOTE ON, NOTE OFF)
            
        //NoteOFF: if command is 0x80
        case 0x80:
            noteOff(data1 & 0x7F); // Apaga la nota indicada al byte data1
            break;
            
        //NoteON: if command is 0x80
        case 0x90:{
            uint8_t note = data1 & 0x7F; // Extreu el número de nota MIDI
            uint8_t velo = data2 & 0x7F; // Extreu la velocitat
            if(velo>0){ // Si la velocitat és major que zero...
                noteOn(note, velo); // Activa la nota
            } else{ // En cas contrari...
                noteOff(note); // Apaga la nota
            }
            break;
        }
    }
}


//-------------------------------------------------------------------------------------
//RenderAudio:
//-------------------------------------------------------------------------------------
void Synth::render(float** outputBuffers, int sampleCount){ // Genera sampleCount mostres d'àudio
    
    float* outputBufferLeft=outputBuffers[0]; // Punter al buffer del canal esquerre
    float* outputBufferRight=outputBuffers[1]; // Punter al buffer del canal dret
    
    
    //1-Loop through the sample sin the buffer one-by-one. sampleCount is number of samples to render.
    //If there are MIDI, sampleCount will be less than num of samples in block
    for (int sample=0; sample<sampleCount; ++sample){
        
        float output=0.0f; // Inicialització de la sortida a zero
        
        voice.waveForm=waveForm; //WF
        
        if(voice.note>0 || voice.adsr.isActive()){ // Si hi ha una nota activa...
            output=voice.render(); // Genera la mostra d'àudio
        }
        // Si l'envolvent ha acabat, silenciem la veu
        if (!voice.adsr.isActive()) {
            voice.note = 0;
        }
        
        float outputLevel = outputLevelSmoother.getNextValue(); // Nivell de sortida suavitzat
        output *= outputLevel; // Aplica el volum suavitzat a la mostra
        
        outputBufferLeft[sample]=output; // Escriu la mostra al canal esquerre
        if(outputBufferRight!=nullptr){ // Si existeix canal dret...
            outputBufferRight[sample]=output; // Hi escriu la mostra
        }
    }
    protectYourEars(outputBufferLeft, sampleCount);
    protectYourEars(outputBufferRight, sampleCount);
}



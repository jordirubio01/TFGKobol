/*
  ==============================================================================

    Filter.h
    Created: 25 Apr  2026 11:01:00pm
    Author:  Jordi

    Implementació no lineal del filtre Moog Ladder.
    Basada en: Huovilainen, A. (2004). Non-Linear Digital Implementation
               of the Moog Ladder Filter. DAFx-04, Naples.

    Característiques:
      - 4 etapes amb tanh no lineal
      - Sobremostreig 2x per evitar aliasing
      - Compensació de mitja unitat de retard en el feedback
      - Cutoff i resonance modulables per mostra

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class Filter
{
public:

    Filter();

    //Inicialitza el filtre amb la freqüència de mostreig del host.
    void prepare(double sampleRate);

    // Reinicia els estats interns (canvi de nota, silenci, etc.).
    void reset();

    /**
     *  Processa una mostra a la freqüència de mostreig del host.
     *  Internament fa 2x oversampling i retorna la mostra decimada.
     *
     *  @param input    Mostra d'entrada en rang [-1, 1]
     *  @param cutoffHz Freqüència de tall en Hz  (actualitzada cada mostra)
     *  @param resonance Ressonància en [0, 1]    (1.0 = auto-oscil·lació)
     */
    float processSample(float input, float cutoffHz, float resonance);

private:

    // Nucli de filtratge (opera al sample rate intern, 2x oversampled)
    float processOversampled(float x);

    // Paràmetres interns
    double sampleRate    { 44100.0 };
    double sampleRateOS  { 88200.0 };   // sampleRate * oversampleFactor

    static constexpr int oversampleFactor { 2 };

    // Coeficient de freqüència (impulse invariant transform, eq. 21 del paper)
    float g          { 0.0f };
    float resonance  { 0.0f };

    // Voltatge tèrmic (convenció per a senyals normalitzades en [-1,1])
    static constexpr float Vt { 0.5f };

    // Estat intern de les 4 etapes
    float y[4]       { 0.0f, 0.0f, 0.0f, 0.0f };  // sortides de cada etapa
    float w[4]       { 0.0f, 0.0f, 0.0f, 0.0f };  // tanh(y / 2Vt) en cache
    float lastOut    { 0.0f };                    // per a la compensació de mitja unitat

    // Helpers
    void updateCoefficients(float cutoffHz);

    static inline float fasttanh(float x) noexcept
    {
        return std::tanh(x);
    }
};
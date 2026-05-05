/*
  ==============================================================================

    Filter.cpp
    Created: 25 Apr  2026 11:01:27pm
    Author:  Jordi

    Implementació no lineal del filtre Moog Ladder (Huovilainen 2004).

  ==============================================================================
*/

#include "Filter.h"

Filter::Filter()
{
    reset();
}

void Filter::prepare(double sr)
{
    sampleRate   = sr;
    sampleRateOS = sr * oversampleFactor;
    reset();
}

void Filter::reset()
{
    for (int i = 0; i < 4; ++i) { y[i] = 0.0f; w[i] = 0.0f; }
    lastOut = 0.0f;
}

// --------------------------------------------------------------------------------------
// UpdateCoeficients:
// --------------------------------------------------------------------------------------
// Actualitza el coeficient g a partir de la freqüència de tall (eq. 21 del paper)
// S'utilitza el sample rate intern (oversampled)
void Filter::updateCoefficients(float cutoffHz)
{
    // Limitem la freqüència de tall a Nyquist del rate intern
    const float maxCutoff = static_cast<float>(sampleRateOS) * 0.49f;
    const float fc = std::max(1.0f, std::min(cutoffHz, maxCutoff));

    g = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * fc / static_cast<float>(sampleRateOS));
}

// --------------------------------------------------------------------------------------
// ProcessOversampled:
// --------------------------------------------------------------------------------------
float Filter::processOversampled(float x)
{
    const float out = y[3];

    // Compensació de mitja unitat de retard en el feedback (eq. 23)
    const float fb = 0.5f * (out + lastOut);
    lastOut = out;

    // Etapa 1: entrada amb feedback (eq. 22)
    const float inp_tanh = fasttanh((x - 4.0f * resonance * fb) / (2.0f * Vt));
    y[0] += 2.0f * Vt * g * (inp_tanh - w[0]);
    w[0] = fasttanh(y[0] / (2.0f * Vt));

    // Etapes 2, 3, 4 (eq. 14-16)
    for (int k = 1; k < 4; ++k)
    {
        y[k] += g * (w[k - 1] - w[k]);
        w[k] = fasttanh(y[k]);
    }

    return y[3];
}

// --------------------------------------------------------------------------------------
// ProcessSample:
// --------------------------------------------------------------------------------------
// Processa una mostra al sample rate original
float Filter::processSample(float input, float cutoffHz, float res)
{
    resonance = std::max(0.0f, std::min(res, 1.225f));
    updateCoefficients(cutoffHz);

    // Sobremostreig 2x: repetir la mostra d'entrada i processar dues vegades
    processOversampled(input);                   // passa 1 (descartada)
    float output = processOversampled(input);    // passa 2 (decimada)

    //float compensation = 1.0f + resonance * 2.0f;
    //return output * compensation;
    return output;
}
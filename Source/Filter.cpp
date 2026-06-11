/*
  ==============================================================================

    Filter.cpp
    Created: 25 Apr  2026 11:01:27pm
    Author:  Jordi

    Non-linear Moog Ladder Filter implementation (Huovilainen 2004).

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
// UpdateCoefficients:
// --------------------------------------------------------------------------------------
// Update the coefficient g based on the cutoff frequency (eq. 21 of the paper)
// Uses the internal sample rate (oversampled)
void Filter::updateCoefficients(float cutoffHz)
{
    // Limit the cutoff frequency to Nyquist of the internal rate
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

    // Compensation for half-unit delay in the feedback (eq. 23)
    const float fb = 0.5f * (out + lastOut);
    lastOut = out;

    // Stage 1: input with feedback (eq. 22)
    const float inp_tanh = fasttanh((x - 4.0f * resonance * fb) / (2.0f * Vt));
    y[0] += 2.0f * Vt * g * (inp_tanh - w[0]);
    w[0] = fasttanh(y[0] / (2.0f * Vt));

    // Stages 2, 3, 4 (eq. 14-16)
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
// Process a sample at the original sample rate
float Filter::processSample(float input, float cutoffHz, float res)
{
    resonance = std::max(0.0f, std::min(res, 1.225f));
    updateCoefficients(cutoffHz);

    // Oversampling 2x: repeat the input sample and process twice
    processOversampled(input);                   // pass 1 (discarded)
    float output = processOversampled(input);    // pass 2 (decimated)

    //float compensation = 1.0f + resonance * 2.0f;
    //return output * compensation;
    return output;
}
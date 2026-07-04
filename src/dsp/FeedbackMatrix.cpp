#include "FeedbackMatrix.h"

void FeedbackMatrix::prepare(double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    reset();
}

void FeedbackMatrix::reset()
{
    for (auto& s : state) { s.s1 = 0; s.s2 = 0; }
}

// Topology-preserving TPT state-variable filter, single sample.
// Returns band-pass output (richest harmonic content for self-oscillation).
static float svfSample(FeedbackMatrix::SVFState& s, float x,
                       float g, float k, float fbAmt)
{
    // Inject feedback from band-pass back into input — above k~2 it self-oscillates
    x += fbAmt * s.s1;

    float hp = (x - k * s.s1 - s.s2) / (1.0f + g * (g + k));
    float bp = g * hp + s.s1;
    float lp = g * bp + s.s2;

    s.s1 = 2.0f * bp - s.s1;
    s.s2 = 2.0f * lp - s.s2;

    return bp;
}

void FeedbackMatrix::process(juce::AudioBuffer<float>& buffer, float chaos, float freq)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // freq: 0..1 → 80..6000 Hz
    float hz  = 80.0f * std::pow(75.0f, freq);
    float g   = std::tan(juce::MathConstants<float>::pi * hz / static_cast<float>(sampleRate));
    // Q: as chaos rises, Q rises sharply (resonance)
    float q   = 0.5f + chaos * 8.0f;
    float k   = 1.0f / q;

    // fbAmt > 0 injects band-pass output back into input.
    // Past ~unity this creates a sustained tone; chaos controls how far over.
    float fbAmt = chaos * 1.15f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float out = svfSample(state[ch], d[i], g, k, fbAmt);
            // Soft-clip to keep things from exploding into silence
            d[i] = std::tanh(out);
        }
    }
}

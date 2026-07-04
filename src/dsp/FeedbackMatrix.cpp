#include "FeedbackMatrix.h"

void FeedbackMatrix::prepare(double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    reset();
}

void FeedbackMatrix::reset()
{
    s1[0] = s1[1] = s2[0] = s2[1] = 0.0f;
}

// TPT state-variable filter, single sample. Returns band-pass output.
// fbAmt injects BP back into input — past ~unity gain it self-oscillates.
static float svfSample(float& st1, float& st2, float x,
                       float g, float k, float fbAmt)
{
    x += fbAmt * st1;
    float hp = (x - k * st1 - st2) / (1.0f + g * (g + k));
    float bp = g * hp + st1;
    float lp = g * bp + st2;
    st1 = 2.0f * bp - st1;
    st2 = 2.0f * lp - st2;
    return bp;
}

void FeedbackMatrix::process(juce::AudioBuffer<float>& buffer, float chaos, float freq)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    float hz    = 80.0f * std::pow(75.0f, freq); // 0..1 → 80..6000 Hz
    float g     = std::tan(juce::MathConstants<float>::pi * hz / static_cast<float>(sampleRate));
    float k     = 1.0f / (0.5f + chaos * 8.0f);
    float fbAmt = chaos * 1.15f;

    for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            d[i] = std::tanh(svfSample(s1[ch], s2[ch], d[i], g, k, fbAmt));
    }
}

#include "BitCrusher.h"

void BitCrusher::process(juce::AudioBuffer<float>& buffer, float bits, float rateReduction)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // bits: 1..16 (floored for quantisation step)
    float step = (bits < 15.9f) ? 2.0f / std::pow(2.0f, bits) : 0.0f;

    // rateReduction: 0..1 → hold 1..32 samples
    float holdLen = 1.0f + rateReduction * 31.0f;

    for (int ch = 0; ch < juce::jmin(numChannels, 2); ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            holdCounter[ch] += 1.0f;
            if (holdCounter[ch] >= holdLen)
            {
                holdCounter[ch] -= holdLen;
                float x = d[i];
                if (step > 0.0f)
                    x = std::round(x / step) * step;
                held[ch] = x;
            }
            d[i] = held[ch];
        }
    }
}

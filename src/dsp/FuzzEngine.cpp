#include "FuzzEngine.h"

void FuzzEngine::prepare(double sampleRate, int maxBlockSize)
{
    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32>(maxBlockSize), 2 };
    toneFilter.prepare(spec);
    toneFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
}

static float asymClip(float x, float drive)
{
    x *= (1.0f + drive * 48.0f);
    // Positive half: soft tanh; negative half: harder clip gives more even-order harmonics
    return (x >= 0.0f) ? std::tanh(x) : std::tanh(x * 1.8f) / 1.8f;
}

void FuzzEngine::process(juce::AudioBuffer<float>& buffer, float drive, float tone, float octave)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // Tone: 0..1 maps to 250..8000 Hz (log)
    float cutoff = 250.0f * std::pow(32.0f, tone);
    toneFilter.setCutoffFrequency(juce::jlimit(250.0f, 8000.0f, cutoff));

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            float dry    = d[i];
            float fuzzed = asymClip(dry, drive);

            // Octave-up: full-wave rectify and high-pass the DC offset
            float rectified = std::abs(fuzzed) * 2.0f - 1.0f;
            d[i] = fuzzed + octave * (rectified - fuzzed);
        }
    }

    juce::dsp::AudioBlock<float>             block(buffer);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    toneFilter.process(ctx);
}

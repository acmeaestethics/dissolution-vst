#pragma once
#include <JuceHeader.h>

// Self-oscillating resonant filter. chaos > ~0.85 enters self-oscillation.
class FeedbackMatrix
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, float chaos, float freq);

private:
    double sampleRate = 44100.0;

    // Per-channel SVF state
    struct SVFState { float s1 = 0, s2 = 0; };
    SVFState state[2];

    juce::Random rng;
};

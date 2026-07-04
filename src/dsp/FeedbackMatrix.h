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
    float s1[2] = { 0, 0 };
    float s2[2] = { 0, 0 };
    juce::Random rng;
};

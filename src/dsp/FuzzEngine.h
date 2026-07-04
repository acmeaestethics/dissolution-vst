#pragma once
#include <JuceHeader.h>

class FuzzEngine
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& buffer, float drive, float tone, float octave);

private:
    juce::dsp::StateVariableTPTFilter<float> toneFilter;
};

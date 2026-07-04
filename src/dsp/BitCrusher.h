#pragma once
#include <JuceHeader.h>

class BitCrusher
{
public:
    void process(juce::AudioBuffer<float>& buffer, float bits, float rateReduction);

private:
    float held[2] = { 0.0f, 0.0f };
    float holdCounter[2] = { 0.0f, 0.0f };
};

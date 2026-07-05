#pragma once
#include <JuceHeader.h>
#include "dsp/FuzzEngine.h"
#include "dsp/FeedbackMatrix.h"
#include "dsp/BitCrusher.h"
#include "dsp/ShimmerReverb.h"
#include "dsp/SpectralFreeze.h"

class DissolutionProcessor : public juce::AudioProcessor
{
public:
    DissolutionProcessor();
    ~DissolutionProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int  getNumPrograms() override                            { return 1; }
    int  getCurrentProgram() override                         { return 0; }
    void setCurrentProgram(int) override                      {}
    const juce::String getProgramName(int) override           { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& dest) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    FuzzEngine      fuzz;
    FeedbackMatrix  feedbackMatrix;
    BitCrusher      crusher;
    ShimmerReverb   shimmer;
    SpectralFreeze  freeze;

    juce::AudioBuffer<float> dryBuffer;
    juce::Random             noiseRng;

    // One-pole IIR smoothers for every continuous parameter (eliminates zipper noise).
    // Updated once per block; coefficient set in prepareToPlay (~20 ms time constant).
    float smoothCoef = 0.0f;
    float smDrive = 0.0f,  smTone = 0.7f,    smOct = 0.0f;
    float smChaos = 0.0f,  smFreq = 0.4f;
    float smBits  = 16.0f, smRate = 0.0f;
    float smSize  = 0.6f,  smDamp = 0.5f,    smShimmer = 0.0f, smRevMix = 0.4f;
    float smNoise = 0.0f,  smWetDry = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DissolutionProcessor)
};

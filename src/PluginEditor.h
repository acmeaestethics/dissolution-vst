#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class DissolutionEditor : public juce::AudioProcessorEditor
{
public:
    explicit DissolutionEditor(DissolutionProcessor&);
    ~DissolutionEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    DissolutionProcessor& proc;

    // Sliders
    juce::Slider fuzzDrive, fuzzTone, fuzzOctave;
    juce::Slider fbChaos, fbFreq;
    juce::Slider crushBits, crushRate;
    juce::Slider revSize, revDamp, revShimmer, revMix;
    juce::Slider noise, wetDry;

    // Toggle
    juce::TextButton freezeBtn;

    // Labels
    juce::Label fuzzDriveLbl, fuzzToneLbl, fuzzOctaveLbl;
    juce::Label fbChaosLbl, fbFreqLbl;
    juce::Label crushBitsLbl, crushRateLbl;
    juce::Label revSizeLbl, revDampLbl, revShimmerLbl, revMixLbl;
    juce::Label noiseLbl, wetDryLbl;

    // Attachments
    using Attach  = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<Attach> fuzzDriveA, fuzzToneA, fuzzOctaveA;
    std::unique_ptr<Attach> fbChaosA, fbFreqA;
    std::unique_ptr<Attach> crushBitsA, crushRateA;
    std::unique_ptr<Attach> revSizeA, revDampA, revShimmerA, revMixA;
    std::unique_ptr<Attach> noiseA, wetDryA;
    std::unique_ptr<BAttach> freezeA;

    void setupSlider(juce::Slider& s, juce::Label& l, const juce::String& name);
    void attachAll();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DissolutionEditor)
};

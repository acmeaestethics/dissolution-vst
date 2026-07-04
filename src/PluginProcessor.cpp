#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout DissolutionProcessor::createLayout()
{
    using Param  = juce::AudioParameterFloat;
    using ParamB = juce::AudioParameterBool;
    using NR     = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Fuzz
    layout.add(std::make_unique<Param>("fuzz_drive",   "Fuzz Drive",   NR(0.0f, 1.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<Param>("fuzz_tone",    "Fuzz Tone",    NR(0.0f, 1.0f, 0.01f), 0.7f));
    layout.add(std::make_unique<Param>("fuzz_octave",  "Octave Up",    NR(0.0f, 1.0f, 0.01f), 0.0f));

    // Feedback chaos filter
    layout.add(std::make_unique<Param>("fb_chaos",     "Chaos",        NR(0.0f, 1.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<Param>("fb_freq",      "Chaos Freq",   NR(0.0f, 1.0f, 0.01f), 0.4f));

    // Crusher
    layout.add(std::make_unique<Param>("crush_bits",   "Bit Depth",    NR(1.0f, 16.0f, 0.1f), 16.0f));
    layout.add(std::make_unique<Param>("crush_rate",   "Rate Crush",   NR(0.0f, 1.0f,  0.01f), 0.0f));

    // Shimmer reverb
    layout.add(std::make_unique<Param>("rev_size",     "Room Size",    NR(0.0f, 1.0f, 0.01f), 0.6f));
    layout.add(std::make_unique<Param>("rev_damp",     "Damp",         NR(0.0f, 1.0f, 0.01f), 0.5f));
    layout.add(std::make_unique<Param>("rev_shimmer",  "Shimmer",      NR(0.0f, 1.0f, 0.01f), 0.0f));
    layout.add(std::make_unique<Param>("rev_mix",      "Reverb Mix",   NR(0.0f, 1.0f, 0.01f), 0.4f));

    // Spectral freeze
    layout.add(std::make_unique<ParamB>("freeze",      "Freeze",       false));

    // Noise floor
    layout.add(std::make_unique<Param>("noise",        "Noise Floor",  NR(0.0f, 1.0f, 0.01f), 0.0f));

    // Global
    layout.add(std::make_unique<Param>("wet_dry",      "Wet/Dry",      NR(0.0f, 1.0f, 0.01f), 1.0f));

    return layout;
}

DissolutionProcessor::DissolutionProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "DISSOLUTION", createLayout())
{
}

void DissolutionProcessor::prepareToPlay(double sr, int blockSize)
{
    fuzz.prepare(sr, blockSize);
    feedbackMatrix.prepare(sr, blockSize);
    shimmer.prepare(sr, blockSize);
    freeze.prepare(sr, blockSize);
    dryBuffer.setSize(2, blockSize);
}

void DissolutionProcessor::releaseResources()
{
    feedbackMatrix.reset();
    shimmer.reset();
    freeze.reset();
}

void DissolutionProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh      = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    float fuzzDrive  = *apvts.getRawParameterValue("fuzz_drive");
    float fuzzTone   = *apvts.getRawParameterValue("fuzz_tone");
    float fuzzOct    = *apvts.getRawParameterValue("fuzz_octave");
    float fbChaos    = *apvts.getRawParameterValue("fb_chaos");
    float fbFreq     = *apvts.getRawParameterValue("fb_freq");
    float crushBits  = *apvts.getRawParameterValue("crush_bits");
    float crushRate  = *apvts.getRawParameterValue("crush_rate");
    float revSize    = *apvts.getRawParameterValue("rev_size");
    float revDamp    = *apvts.getRawParameterValue("rev_damp");
    float revShimmer = *apvts.getRawParameterValue("rev_shimmer");
    float revMix     = *apvts.getRawParameterValue("rev_mix");
    bool  isFrozen   = *apvts.getRawParameterValue("freeze") > 0.5f;
    float noise      = *apvts.getRawParameterValue("noise");
    float wetDry     = *apvts.getRawParameterValue("wet_dry");

    // Stash dry signal for global wet/dry blend
    dryBuffer.setSize(numCh, numSamples, false, false, true);
    for (int ch = 0; ch < numCh; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Inject noise floor (before fuzz — it becomes part of the distortion texture)
    if (noise > 0.001f)
    {
        float noiseAmt = noise * noise * 0.08f; // squared for perceptual scaling
        juce::Random rng;
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* d = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
                d[i] += (rng.nextFloat() * 2.0f - 1.0f) * noiseAmt;
        }
    }

    // Chain: fuzz → chaos filter → crusher → shimmer reverb → spectral freeze
    if (fuzzDrive > 0.001f || fuzzOct > 0.001f)
        fuzz.process(buffer, fuzzDrive, fuzzTone, fuzzOct);

    if (fbChaos > 0.001f)
        feedbackMatrix.process(buffer, fbChaos, fbFreq);

    if (crushBits < 15.9f || crushRate > 0.001f)
        crusher.process(buffer, crushBits, crushRate);

    shimmer.process(buffer, revSize, revDamp, revShimmer, revMix);

    freeze.process(buffer, isFrozen);

    // Global wet/dry
    if (wetDry < 0.999f)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                wet[i] = dry[i] * (1.0f - wetDry) + wet[i] * wetDry;
        }
    }
}

void DissolutionProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void DissolutionProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DissolutionProcessor();
}

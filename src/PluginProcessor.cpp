#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout DissolutionProcessor::createLayout()
{
    using Param  = juce::AudioParameterFloat;
    using ParamB = juce::AudioParameterBool;
    using NR     = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<Param>("fuzz_drive",  "Fuzz Drive",  NR(0.0f, 1.0f,  0.01f), 0.0f));
    layout.add(std::make_unique<Param>("fuzz_tone",   "Fuzz Tone",   NR(0.0f, 1.0f,  0.01f), 0.7f));
    layout.add(std::make_unique<Param>("fuzz_octave", "Octave Up",   NR(0.0f, 1.0f,  0.01f), 0.0f));
    layout.add(std::make_unique<Param>("fb_chaos",    "Chaos",       NR(0.0f, 1.0f,  0.01f), 0.0f));
    layout.add(std::make_unique<Param>("fb_freq",     "Chaos Freq",  NR(0.0f, 1.0f,  0.01f), 0.4f));
    layout.add(std::make_unique<Param>("crush_bits",  "Bit Depth",   NR(1.0f, 16.0f, 0.1f),  16.0f));
    layout.add(std::make_unique<Param>("crush_rate",  "Rate Crush",  NR(0.0f, 1.0f,  0.01f), 0.0f));
    layout.add(std::make_unique<Param>("rev_size",    "Room Size",   NR(0.0f, 1.0f,  0.01f), 0.6f));
    layout.add(std::make_unique<Param>("rev_damp",    "Damp",        NR(0.0f, 1.0f,  0.01f), 0.5f));
    layout.add(std::make_unique<Param>("rev_shimmer", "Shimmer",     NR(0.0f, 1.0f,  0.01f), 0.0f));
    layout.add(std::make_unique<Param>("rev_mix",     "Reverb Mix",  NR(0.0f, 1.0f,  0.01f), 0.4f));
    layout.add(std::make_unique<ParamB>("freeze",     "Freeze",      false));
    layout.add(std::make_unique<Param>("noise",       "Noise Floor", NR(0.0f, 1.0f,  0.01f), 0.0f));
    layout.add(std::make_unique<Param>("wet_dry",     "Wet/Dry",     NR(0.0f, 1.0f,  0.01f), 1.0f));

    return layout;
}

DissolutionProcessor::DissolutionProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "DISSOLUTION", createLayout())
{
}

void DissolutionProcessor::prepareToPlay(double sr, int maxBlock)
{
    fuzz.prepare(sr, maxBlock);
    feedbackMatrix.prepare(sr, maxBlock);
    shimmer.prepare(sr, maxBlock);
    freeze.prepare(sr, maxBlock);
    dryBuffer.setSize(2, maxBlock);

    // ~20 ms one-pole smoothing coefficient
    smoothCoef = static_cast<float>(std::exp(-1.0 / (sr * 0.02)));

    // Initialise smoothers to current param values so there's no ramp at startup
    smDrive   = *apvts.getRawParameterValue("fuzz_drive");
    smTone    = *apvts.getRawParameterValue("fuzz_tone");
    smOct     = *apvts.getRawParameterValue("fuzz_octave");
    smChaos   = *apvts.getRawParameterValue("fb_chaos");
    smFreq    = *apvts.getRawParameterValue("fb_freq");
    smBits    = *apvts.getRawParameterValue("crush_bits");
    smRate    = *apvts.getRawParameterValue("crush_rate");
    smSize    = *apvts.getRawParameterValue("rev_size");
    smDamp    = *apvts.getRawParameterValue("rev_damp");
    smShimmer = *apvts.getRawParameterValue("rev_shimmer");
    smRevMix  = *apvts.getRawParameterValue("rev_mix");
    smNoise   = *apvts.getRawParameterValue("noise");
    smWetDry  = *apvts.getRawParameterValue("wet_dry");

    // Report FFT latency so DAWs can compensate on parallel tracks
    setLatencySamples(SpectralFreeze::kLatencySamples);
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

    // Advance each one-pole smoother one step toward the current APVTS value.
    // Applied once per block — eliminates zipper noise without per-sample overhead.
    auto smooth = [&](float& s, const char* id) {
        s = s * smoothCoef + *apvts.getRawParameterValue(id) * (1.0f - smoothCoef);
    };
    smooth(smDrive,   "fuzz_drive");
    smooth(smTone,    "fuzz_tone");
    smooth(smOct,     "fuzz_octave");
    smooth(smChaos,   "fb_chaos");
    smooth(smFreq,    "fb_freq");
    smooth(smBits,    "crush_bits");
    smooth(smRate,    "crush_rate");
    smooth(smSize,    "rev_size");
    smooth(smDamp,    "rev_damp");
    smooth(smShimmer, "rev_shimmer");
    smooth(smRevMix,  "rev_mix");
    smooth(smNoise,   "noise");
    smooth(smWetDry,  "wet_dry");

    const bool isFrozen = *apvts.getRawParameterValue("freeze") > 0.5f;

    // Stash dry signal for global wet/dry blend
    dryBuffer.setSize(numCh, numSamples, false, false, true);
    for (int ch = 0; ch < numCh; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    // Noise floor injected pre-fuzz so it becomes part of the distortion texture
    if (smNoise > 0.001f)
    {
        float noiseAmt = smNoise * smNoise * 0.08f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* d = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
                d[i] += (noiseRng.nextFloat() * 2.0f - 1.0f) * noiseAmt;
        }
    }

    if (smDrive > 0.001f || smOct > 0.001f)
        fuzz.process(buffer, smDrive, smTone, smOct);

    if (smChaos > 0.001f)
        feedbackMatrix.process(buffer, smChaos, smFreq);

    if (smBits < 15.9f || smRate > 0.001f)
        crusher.process(buffer, smBits, smRate);

    shimmer.process(buffer, smSize, smDamp, smShimmer, smRevMix);

    freeze.process(buffer, isFrozen);

    if (smWetDry < 0.999f)
    {
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto*       wet = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                wet[i] = dry[i] * (1.0f - smWetDry) + wet[i] * smWetDry;
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

juce::AudioProcessorEditor* DissolutionProcessor::createEditor()
{
    return new DissolutionEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DissolutionProcessor();
}

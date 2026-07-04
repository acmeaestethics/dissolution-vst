#include "ShimmerReverb.h"

// Comb filter delay times in samples at 44100 (scaled for other rates).
// Slightly detuned between L and R for width.
static const int kCombDelaysL[4] = { 1557, 1617, 1491, 1422 };
static const int kCombDelaysR[4] = { 1617, 1557, 1422, 1491 };
static const int kApDelays[2]    = { 225,  341  };

void ShimmerReverb::prepare(double sr, int /*maxBlockSize*/)
{
    sampleRate = sr;
    float scale = static_cast<float>(sr) / 44100.0f;

    for (int ch = 0; ch < 2; ++ch)
    {
        const int* combD = (ch == 0) ? kCombDelaysL : kCombDelaysR;
        for (int c = 0; c < kNumCombs; ++c)
        {
            int sz = static_cast<int>(combD[c] * scale) + 2;
            comb[ch][c].resize(sz);
        }
        for (int a = 0; a < kNumAllpass; ++a)
        {
            int sz = static_cast<int>(kApDelays[a] * scale) + 2;
            allpass[ch][a].resize(sz);
        }

        shimBuf[ch].assign(kShimBufSize, 0.0f);
        shimWritePos[ch] = 0;
        shimReadPos[ch]  = kShimBufSize / 2.0f; // start half a buffer behind
        shimFeedback[ch] = 0.0f;
    }

    reset();
}

void ShimmerReverb::reset()
{
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int c = 0; c < kNumCombs;   ++c) std::fill(comb[ch][c].buf.begin(),    comb[ch][c].buf.end(),    0.0f);
        for (int a = 0; a < kNumAllpass; ++a) std::fill(allpass[ch][a].buf.begin(), allpass[ch][a].buf.end(), 0.0f);
        std::fill(shimBuf[ch].begin(), shimBuf[ch].end(), 0.0f);
    }
}

float ShimmerReverb::readLinear(const std::vector<float>& buf, float pos) const
{
    int   n    = static_cast<int>(buf.size());
    int   i0   = static_cast<int>(pos) % n;
    int   i1   = (i0 + 1) % n;
    float frac = pos - std::floor(pos);
    if (i0 < 0) i0 += n;
    return buf[i0] * (1.0f - frac) + buf[i1] * frac;
}

void ShimmerReverb::process(juce::AudioBuffer<float>& buffer,
                            float size, float damp, float shimmer, float mix)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();
    float scale = static_cast<float>(sampleRate) / 44100.0f;

    // Feedback coefficient for comb filters derived from size (0.5..0.95)
    float fb = 0.5f + size * 0.45f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* d = buffer.getWritePointer(ch);
        const int* combD = (ch == 0) ? kCombDelaysL : kCombDelaysR;

        for (int i = 0; i < numSamples; ++i)
        {
            float dry = d[i];
            float wet = 0.0f;

            // --- Parallel comb filters ---
            for (int c = 0; c < kNumCombs; ++c)
            {
                int   delaySamp = static_cast<int>(combD[c] * scale);
                float delayed   = comb[ch][c].read(delaySamp);
                float filtered  = delayed * (1.0f - damp * 0.5f) + combFb[ch][c] * damp * 0.5f;
                combFb[ch][c]   = filtered;
                comb[ch][c].write(dry + filtered * fb);
                wet += filtered;
            }
            wet *= 0.25f; // average the 4 combs

            // --- Series allpass diffusers ---
            for (int a = 0; a < kNumAllpass; ++a)
            {
                int   ds = static_cast<int>(kApDelays[a] * scale);
                float v  = allpass[ch][a].read(ds);
                float w  = wet + v * 0.5f;
                allpass[ch][a].write(w);
                wet = v - w * 0.5f;
            }

            // --- Shimmer: pitch shift wet signal up one octave ---
            // Write current wet into shimmer circular buffer
            shimBuf[ch][shimWritePos[ch]] = wet;

            // Read at 2x speed (octave up) — as we write 1 sample, read advances 2
            float shimOut   = readLinear(shimBuf[ch], shimReadPos[ch]);
            shimReadPos[ch] += 2.0f; // double speed = +1 octave
            if (shimReadPos[ch] >= kShimBufSize) shimReadPos[ch] -= kShimBufSize;
            shimWritePos[ch] = (shimWritePos[ch] + 1) % kShimBufSize;

            // Blend shimmer back into wet reverb
            wet += shimmer * shimOut;

            d[i] = dry * (1.0f - mix) + wet * mix;
        }
    }
}

#pragma once
#include <JuceHeader.h>

// Schroeder reverb (4 comb + 2 allpass) with pitch-shimmer feedback loop.
class ShimmerReverb
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, float size, float damp, float shimmer, float mix);

private:
    static constexpr int kNumCombs   = 4;
    static constexpr int kNumAllpass = 2;

    struct DelayLine
    {
        std::vector<float> buf;
        int writePos = 0;

        void resize(int n) { buf.assign(n, 0.0f); writePos = 0; }

        float read(int delaySamples) const
        {
            int idx = writePos - delaySamples;
            if (idx < 0) idx += static_cast<int>(buf.size());
            return buf[idx];
        }

        void write(float x)
        {
            buf[writePos] = x;
            if (++writePos >= static_cast<int>(buf.size())) writePos = 0;
        }
    };

    // Two sets (L/R) of combs and allpasses
    DelayLine comb[2][kNumCombs];
    float     combFb[2][kNumCombs] = {};

    DelayLine allpass[2][kNumAllpass];

    // Two-grain pitch shifter for shimmer (+1 octave).
    // Grains are offset by half the buffer and crossfaded with Hann windows
    // so neither grain ever clicks on wrap-around.
    static constexpr int kShimBufSize = 4096;
    std::vector<float> shimBuf[2];
    int   shimWritePos[2]      = { 0, 0 };
    float shimGrainPos[2][2]   = {};   // [channel][grain 0 or 1]

    double sampleRate = 44100.0;

    float readLinear(const std::vector<float>& buf, float pos) const;
};

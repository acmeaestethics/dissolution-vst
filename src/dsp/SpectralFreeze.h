#pragma once
#include <JuceHeader.h>

// FFT-based spectral freeze (Hann OLA, 75% overlap).
// frozen=false: transparent overlap-add passthrough (latency = kFFTSize samples).
// frozen=true:  locks the live magnitude spectrum on first frozen frame, then
//               re-synthesises each hop with randomised phases — the metallic,
//               shimmering wash characteristic of Paulstretch / Ben Frost.
class SpectralFreeze
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, bool frozen);
    bool isActive() const; // true while frozen or draining

private:
    static constexpr int kOrder   = 11;          // FFT size = 2048
    static constexpr int kFFTSize = 1 << kOrder;
    static constexpr int kHop     = kFFTSize / 4; // 75% overlap

    juce::dsp::FFT fft { kOrder };
    juce::Random   rng;

    struct ChannelState
    {
        std::vector<float> inFifo;      // kFFTSize — circular input
        std::vector<float> outFifo;     // kFFTSize — overlap-add output
        std::vector<float> fftBuffer;   // 2*kFFTSize — analysis (complex)
        std::vector<float> synthBuffer; // 2*kFFTSize — synthesis (complex → real)
        std::vector<float> frozenMag;   // kFFTSize/2+1 — locked magnitudes
        std::vector<float> frozenPhase; // kFFTSize/2+1 — phases at freeze point
        std::vector<float> window;      // kFFTSize — Hann
        int  writePos   = 0;
        int  readPos    = 0;
        int  hopCounter = 0;
        bool hasFrozen  = false;
    };

    ChannelState ch[2];

    void processChannel(ChannelState& c, float* data, int numSamples, bool frozen);
    void buildHannWindow(std::vector<float>& w);
};

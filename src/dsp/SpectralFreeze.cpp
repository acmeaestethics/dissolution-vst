#include "SpectralFreeze.h"

void SpectralFreeze::buildHannWindow(std::vector<float>& w)
{
    int n = static_cast<int>(w.size());
    for (int i = 0; i < n; ++i)
        w[i] = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * i / n));
}

void SpectralFreeze::prepare(double /*sampleRate*/, int /*maxBlockSize*/)
{
    for (auto& c : ch)
    {
        c.inFifo.assign(kFFTSize, 0.0f);
        c.outFifo.assign(kFFTSize, 0.0f);
        c.fftBuffer.assign(2 * kFFTSize, 0.0f);
        c.synthBuffer.assign(2 * kFFTSize, 0.0f);
        c.frozenMag.assign(kFFTSize / 2 + 1, 0.0f);
        c.frozenPhase.assign(kFFTSize / 2 + 1, 0.0f);
        c.window.resize(kFFTSize);
        buildHannWindow(c.window);
        c.writePos   = 0;
        c.readPos    = 0;
        c.hopCounter = 0;
        c.hasFrozen  = false;
    }
}

void SpectralFreeze::reset()
{
    for (auto& c : ch)
    {
        std::fill(c.inFifo.begin(),    c.inFifo.end(),    0.0f);
        std::fill(c.outFifo.begin(),   c.outFifo.end(),   0.0f);
        std::fill(c.frozenMag.begin(), c.frozenMag.end(), 0.0f);
        c.writePos   = 0;
        c.readPos    = 0;
        c.hopCounter = 0;
        c.hasFrozen  = false;
    }
}

bool SpectralFreeze::isActive() const
{
    return ch[0].hasFrozen || ch[1].hasFrozen;
}

void SpectralFreeze::processChannel(ChannelState& c, float* data, int numSamples, bool frozen)
{
    const float normScale = 1.0f / (kFFTSize * 0.5f); // OLA normalization for Hann 75% overlap

    for (int i = 0; i < numSamples; ++i)
    {
        c.inFifo[c.writePos] = data[i];

        // Output from overlap-add buffer (latency = kFFTSize samples)
        data[i] = c.outFifo[c.readPos];
        c.outFifo[c.readPos] = 0.0f;

        c.writePos = (c.writePos + 1) % kFFTSize;
        c.readPos  = (c.readPos  + 1) % kFFTSize;
        ++c.hopCounter;

        if (c.hopCounter < kHop)
            continue;
        c.hopCounter = 0;

        // ---- Gather windowed analysis frame ----
        for (int k = 0; k < kFFTSize; ++k)
        {
            int src = (c.writePos + k) % kFFTSize; // oldest sample first
            c.fftBuffer[k]            = c.inFifo[src] * c.window[k];
            c.fftBuffer[k + kFFTSize] = 0.0f;
        }

        // ---- Forward FFT (complex output in c.fftBuffer) ----
        fft.performRealOnlyForwardTransform(c.fftBuffer.data());
        // c.fftBuffer[2k], c.fftBuffer[2k+1] = real, imag of bin k

        // ---- Extract magnitudes and phases from live spectrum ----
        if (!frozen)
        {
            for (int k = 0; k <= kFFTSize / 2; ++k)
            {
                float re = c.fftBuffer[2 * k];
                float im = c.fftBuffer[2 * k + 1];
                c.frozenMag[k]   = std::sqrt(re * re + im * im);
                c.frozenPhase[k] = std::atan2(im, re);
            }
            c.hasFrozen = false;
            // Live passthrough: inverse-transform the live spectrum
            std::copy(c.fftBuffer.begin(), c.fftBuffer.end(), c.synthBuffer.begin());
        }
        else
        {
            if (!c.hasFrozen)
            {
                // Latch magnitudes on first frozen frame
                for (int k = 0; k <= kFFTSize / 2; ++k)
                {
                    float re = c.fftBuffer[2 * k];
                    float im = c.fftBuffer[2 * k + 1];
                    c.frozenMag[k] = std::sqrt(re * re + im * im);
                }
                c.hasFrozen = true;
            }

            // Synthesise: frozen magnitude + randomised phase (metallic shimmer)
            std::fill(c.synthBuffer.begin(), c.synthBuffer.end(), 0.0f);
            for (int k = 1; k < kFFTSize / 2; ++k)
            {
                float ph = rng.nextFloat() * juce::MathConstants<float>::twoPi;
                float mg = c.frozenMag[k];
                c.synthBuffer[2 * k]     = mg * std::cos(ph);
                c.synthBuffer[2 * k + 1] = mg * std::sin(ph);
                // Conjugate mirror for real IFFT
                c.synthBuffer[2 * (kFFTSize - k)]     =  c.synthBuffer[2 * k];
                c.synthBuffer[2 * (kFFTSize - k) + 1] = -c.synthBuffer[2 * k + 1];
            }
            c.synthBuffer[0] = c.frozenMag[0]; // DC
            c.synthBuffer[1] = 0.0f;
        }

        // ---- Inverse FFT ----
        fft.performRealOnlyInverseTransform(c.synthBuffer.data());

        // ---- Windowed overlap-add into output FIFO ----
        for (int k = 0; k < kFFTSize; ++k)
        {
            int dst = (c.readPos + k) % kFFTSize;
            c.outFifo[dst] += c.synthBuffer[k] * c.window[k] * normScale;
        }
    }
}

void SpectralFreeze::process(juce::AudioBuffer<float>& buffer, bool frozen)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    for (int c = 0; c < juce::jmin(numChannels, 2); ++c)
        processChannel(ch[c], buffer.getWritePointer(c), numSamples, frozen);
}

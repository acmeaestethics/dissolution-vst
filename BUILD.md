# Building DISSOLUTION

## Prerequisites

| Tool | Version |
|------|---------|
| CMake | ≥ 3.22 |
| Visual Studio 2022 | "Desktop development with C++" workload |
| Git | any |

JUCE 7.0.12 is fetched automatically on first configure.

## Build (Windows)

```powershell
cd G:\Scripts\Dissolution

# Configure (fetches JUCE on first run — ~3-5 min)
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build
cmake --build build --config Release

# VST3 lands here:
# build\Dissolution_artefacts\Release\VST3\Dissolution.vst3
#
# Standalone:
# build\Dissolution_artefacts\Release\Standalone\Dissolution.exe
```

Copy the `.vst3` folder into `C:\Program Files\Common Files\VST3\` and rescan in your DAW.

---

## Signal chain

```
Input
  └─ Noise Floor        — injects vinyl/tape noise BEFORE fuzz (becomes part of distortion texture)
  └─ Fuzz Engine        — asymmetric tanh clipping, tone filter, optional octave-up
  └─ Chaos Filter       — self-oscillating SVF; past ~85% chaos it sustains/screams
  └─ Bit Crusher        — bit-depth quantisation + sample-rate hold
  └─ Shimmer Reverb     — Schroeder comb+allpass + pitch-shifted (octave up) feedback
  └─ Spectral Freeze    — FFT frame capture; frozen frames resynth with random phases
  └─ Global Wet/Dry
Output
```

---

## Parameter reference

| Param | Range | Notes |
|-------|-------|-------|
| DRIVE | 0–1 | 0 = clean, 1 = full obliteration |
| TONE | 0–1 | Low-pass post-fuzz; 0 = bassy smear, 1 = open fizz |
| OCT+ | 0–1 | Full-wave rectification blend; adds octave-up content |
| CHAOS | 0–1 | SVF feedback; past ~0.85 it self-oscillates |
| FREQ | 0–1 | Chaos filter centre frequency (80–6000 Hz log) |
| BITS | 1–16 | Bit depth; 16 = transparent, 1 = 1-bit hell |
| RATE | 0–1 | Sample-rate crush; 0 = off, 1 = heavy aliasing |
| SIZE | 0–1 | Reverb room size / comb feedback |
| DAMP | 0–1 | High-frequency absorption in reverb tail |
| SHIMMER | 0–1 | Pitch-shifted (+1 octave) feedback blend into reverb |
| MIX | 0–1 | Reverb wet/dry (inside the reverb module) |
| FREEZE | toggle | Locks spectral frame; resynths metallic shimmer wash |
| NOISE | 0–1 | White noise injected pre-fuzz; quiet = warmth, loud = static |
| WET/DRY | 0–1 | Global blend of processed vs clean signal |

---

## Intended use / sound

Stack DRIVE + CHAOS to get MBV-style wall-of-fuzz that breathes. Push CHAOS past 0.85 for
APTBS/DBA feedback screams. Add SHIMMER for the +1 octave reverb wash (Loveless territory).
Hit FREEZE mid-sustain for the metallic Ben Frost held-chord smear. BITS+RATE at low values
gives Aphex-IDM aliasing textures. NOISE at 5-10% adds a vinyl floor that interacts with
the fuzz in ways that feel physical.

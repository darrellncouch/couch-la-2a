#pragma once
#include <cmath>
#include <atomic>
#include <algorithm>

// ─────────────────────────────────────────────────────────────────────────────
// OptoCompressor  –  Teletronix LA-2A style optical leveling amplifier model
//
// Topology:
//   • Feed-forward sidechain (sidechain reads input, not output)
//   • Electro-optical T4B cell modeled as a two-time-constant envelope follower:
//       - Fast path:  attack ~2 ms, release ~60 ms  (lamp response / initial)
//       - Slow path:  attack ~10 ms, release ~1.5 s (thermal "memory" / tail)
//       - Cell state = weighted blend: 70 % fast + 30 % slow
//   • Gain reduction applied via soft-knee in COMPRESS mode (~4:1 effective ratio)
//   • Hard-knee in LIMIT mode (ratio → very high, acts as peak limiter)
//   • Stereo linked sidechain: max(|L|, |R|) drives the opto cell
//   • GAIN knob = make-up output gain (0–100 → 0–40 dB)
//   • PEAK REDUCTION knob = compression depth (0–100 → threshold 0 to −40 dBFS)
// ─────────────────────────────────────────────────────────────────────────────

class OptoCompressor
{
public:
    // Call before processing starts / when sample rate changes
    void prepare (double sampleRate);

    // Parameter setters (called from audio thread via atomic reads)
    void setGain          (float g);    // 0–100
    void setPeakReduction (float pr);   // 0–100
    void setMode          (int mode);   // 0 = Compress, 1 = Limit

    // Process a stereo block in-place
    void processBlock (float* L, float* R, int numSamples);

    // Meter outputs (thread-safe reads from UI thread)
    float getGainReductionDb()  const noexcept { return grDbMeter.load();  }
    float getOutputLevelDb()    const noexcept { return outLevelMeter.load(); }

private:
    // ── Parameters ────────────────────────────────────────────────────────
    float gainParam      = 50.f;   // 0–100
    float peakReduction  = 0.f;    // 0–100
    int   mode           = 0;      // 0 = compress, 1 = limit

    // ── Derived from params ───────────────────────────────────────────────
    float thresholdLin   = 1.f;    // linear amplitude threshold
    float outputGainLin  = 1.f;    // linear output makeup gain
    float ratio          = 4.f;    // compression ratio

    // ── Opto cell state ───────────────────────────────────────────────────
    float fastEnv        = 0.f;    // fast-path envelope
    float slowEnv        = 0.f;    // slow-path envelope ("thermal memory")

    // ── Output level tracker (for VU meter) ──────────────────────────────
    float outEnv         = 0.f;

    // ── Filter coefficients ───────────────────────────────────────────────
    float attackFastCoeff  = 0.f;
    float relFastCoeff     = 0.f;
    float attackSlowCoeff  = 0.f;
    float relSlowCoeff     = 0.f;
    float outMeterCoeff    = 0.f;   // VU ballistic (~300 ms integration)

    double sr = 44100.0;

    // ── Meter outputs (written by audio thread, read by UI thread) ────────
    std::atomic<float> grDbMeter    { 0.f  };
    std::atomic<float> outLevelMeter{ -60.f };

    // ── Helpers ───────────────────────────────────────────────────────────
    void updateCoefficients();
    void updateDerivedParams();

    static float makeCoeff (double timeSeconds, double sampleRate)
    {
        return static_cast<float> (std::exp (-1.0 / (timeSeconds * sampleRate)));
    }
};

#include "OptoCompressor.h"

// ─────────────────────────────────────────────────────────────────────────────
// prepare
// ─────────────────────────────────────────────────────────────────────────────

void OptoCompressor::prepare (double sampleRate)
{
    sr = sampleRate;
    updateCoefficients();
    updateDerivedParams();

    // Reset state
    fastEnv = 0.f;
    slowEnv = 0.f;
    outEnv  = 0.f;

    grDbMeter.store (0.f);
    outLevelMeter.store (-60.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Parameter setters
// ─────────────────────────────────────────────────────────────────────────────

void OptoCompressor::setGain (float g)
{
    gainParam = g;
    updateDerivedParams();
}

void OptoCompressor::setPeakReduction (float pr)
{
    peakReduction = pr;
    updateDerivedParams();
}

void OptoCompressor::setMode (int m)
{
    mode = m;
    updateDerivedParams();
}

// ─────────────────────────────────────────────────────────────────────────────
// updateCoefficients  –  time constants → per-sample decay coefficients
// ─────────────────────────────────────────────────────────────────────────────

void OptoCompressor::updateCoefficients()
{
    attackFastCoeff = makeCoeff (0.002,  sr);   // 2 ms  fast attack
    relFastCoeff    = makeCoeff (0.060,  sr);   // 60 ms fast release
    attackSlowCoeff = makeCoeff (0.010,  sr);   // 10 ms slow attack
    relSlowCoeff    = makeCoeff (1.5,    sr);   // 1.5 s  slow release (opto memory)
    outMeterCoeff   = makeCoeff (0.300,  sr);   // 300 ms VU ballistic
}

// ─────────────────────────────────────────────────────────────────────────────
// updateDerivedParams  –  knob values → DSP values
// ─────────────────────────────────────────────────────────────────────────────

void OptoCompressor::updateDerivedParams()
{
    // GAIN 0–100 → makeup gain 0–40 dB
    const float gainDb   = gainParam * 0.40f;
    outputGainLin  = std::pow (10.f, gainDb / 20.f);

    // PEAK REDUCTION 0–100 → threshold 0 to −40 dBFS
    const float thrDb    = -(peakReduction * 0.40f);
    thresholdLin   = std::pow (10.f, thrDb / 20.f);

    // Ratio: COMPRESS ≈ 4:1,  LIMIT ≈ very high
    ratio = (mode == 1) ? 100.f : 4.f;
}

// ─────────────────────────────────────────────────────────────────────────────
// processBlock  –  stereo, in-place
// ─────────────────────────────────────────────────────────────────────────────

void OptoCompressor::processBlock (float* L, float* R, int numSamples)
{
    // Snapshot params for this block
    const float thrLin    = thresholdLin;
    const float outGain   = outputGainLin;
    const float rat       = ratio;

    float grDbAccum  = 0.f;
    float outAccum   = 0.f;

    for (int i = 0; i < numSamples; ++i)
    {
        // ── Stereo-linked sidechain  (peak of both channels) ────────────
        const float inAbs = std::max (std::abs (L[i]), std::abs (R[i]));

        // ── Opto cell: fast envelope ──────────────────────────────────
        if (inAbs > fastEnv)
            fastEnv = attackFastCoeff * fastEnv + (1.f - attackFastCoeff) * inAbs;
        else
            fastEnv = relFastCoeff * fastEnv + (1.f - relFastCoeff) * inAbs;

        // ── Opto cell: slow envelope (thermal memory) ─────────────────
        if (inAbs > slowEnv)
            slowEnv = attackSlowCoeff * slowEnv + (1.f - attackSlowCoeff) * inAbs;
        else
            slowEnv = relSlowCoeff * slowEnv + (1.f - relSlowCoeff) * inAbs;

        // ── Combine: 70 % fast + 30 % slow ────────────────────────────
        const float optoCell = 0.70f * fastEnv + 0.30f * slowEnv;

        // ── Gain reduction in dB ───────────────────────────────────────
        float grDb = 0.f;
        if (optoCell > thrLin)
        {
            const float optoCellDb  = 20.f * std::log10 (optoCell + 1e-9f);
            const float thrDb       = 20.f * std::log10 (thrLin   + 1e-9f);
            const float overDb      = optoCellDb - thrDb;

            if (mode == 1)
            {
                // LIMIT: hard knee – clamp everything above threshold
                grDb = overDb;
            }
            else
            {
                // COMPRESS: soft knee ~4:1
                const float knee  = 6.f;   // ±3 dB knee around threshold
                if (overDb < knee)
                    grDb = (overDb * overDb) / (2.f * knee);   // gentle curve in knee
                else
                    grDb = overDb * (1.f - 1.f / rat);
            }
        }

        // ── Apply gain reduction ───────────────────────────────────────
        const float grLin = std::pow (10.f, -grDb / 20.f);
        const float gainLin = grLin * outGain;

        L[i] *= gainLin;
        R[i] *= gainLin;

        grDbAccum  += grDb;

        // ── Output level tracker ───────────────────────────────────────
        const float outAbs = std::max (std::abs (L[i]), std::abs (R[i]));
        outEnv = outMeterCoeff * outEnv + (1.f - outMeterCoeff) * outAbs;
        outAccum += outEnv;
    }

    // ── Update meter atomics (averaged over block) ──────────────────────
    const float n = static_cast<float> (numSamples);
    grDbMeter.store (grDbAccum / n);
    const float outAvg = outAccum / n;
    const float outDb  = outAvg > 1e-9f ? 20.f * std::log10 (outAvg) : -60.f;
    outLevelMeter.store (outDb);
}

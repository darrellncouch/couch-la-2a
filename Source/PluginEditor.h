#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ── Custom LookAndFeel ─────────────────────────────────────────────────────────
// LA-2A knobs: large black knob, white pointer, chrome ring
class CouchLA2ALookAndFeel : public juce::LookAndFeel_V4
{
public:
    CouchLA2ALookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;
};

// ── Main editor ────────────────────────────────────────────────────────────────
class CouchLA2AEditor final : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit CouchLA2AEditor (CouchLA2AProcessor&);
    ~CouchLA2AEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    void timerCallback() override;

    // ── Draw helpers ──────────────────────────────────────────────────────
    void drawPanel         (juce::Graphics&) const;
    void drawKnobScale     (juce::Graphics&, float cx, float cy,
                            float innerR, float outerR, float labelR) const;
    void drawKnobLabels    (juce::Graphics&) const;
    void drawModeSwitch    (juce::Graphics&) const;
    void drawVUMeterFace   (juce::Graphics&) const;
    void drawVUNeedle      (juce::Graphics&) const;
    void drawBranding      (juce::Graphics&) const;
    void drawPowerLED      (juce::Graphics&) const;

    // ── VU needle angle (clock-face convention, ±60° from 12 o'clock) ────
    // VU scale: −20 VU (full left −π/3) … +3 VU (full right +π/3)
    // 0 VU = −18 dBFS reference
    float needleAngleForVU (float outDbFS) const noexcept;

    // ── Helpers ───────────────────────────────────────────────────────────
    juce::Rectangle<float> getLimitBtnBounds()    const noexcept;
    juce::Rectangle<float> getCompressBtnBounds() const noexcept;
    int getCurrentMode() const noexcept;  // 0=compress, 1=limit

    // ── Members ───────────────────────────────────────────────────────────
    CouchLA2AProcessor& processor;
    CouchLA2ALookAndFeel lnf;

    juce::Slider gainSlider, peakReductionSlider;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttach> gainAttach, peakReductionAttach;

    // Needle physics
    float needleAngle    = 0.f;
    float needleVelocity = 0.f;

    // ── Layout constants ──────────────────────────────────────────────────
    static constexpr int PLUGIN_W = 700;
    static constexpr int PLUGIN_H = 210;

    // LIMIT / COMPRESS switch buttons (left side)
    static constexpr int SW_X = 14,  SW_W = 62, SW_H = 24;
    static constexpr int SW_LIMIT_Y   = 58;
    static constexpr int SW_COMPRESS_Y = 86;

    // GAIN knob (large, left-center)
    static constexpr int GAIN_X = 88,  GAIN_Y = 38, GAIN_W = 130, GAIN_H = 130;

    // VU meter face (center)
    static constexpr int   VM_X = 238, VM_Y = 15, VM_W = 224, VM_H = 152;
    static constexpr float VM_PX = VM_X + VM_W * 0.5f;  // = 350
    static constexpr float VM_PY = VM_Y + VM_H - 4.f;   // = 163

    // VU needle radii
    static constexpr float NEEDLE_R    = 68.f;
    static constexpr float SCALE_R_OUT = 62.f;
    static constexpr float SCALE_R_IN  = 52.f;
    static constexpr float LABEL_R     = 42.f;

    // PEAK REDUCTION knob (large, right-center)
    static constexpr int PR_X = 482,  PR_Y = 38, PR_W = 130, PR_H = 130;

    // Bottom area
    static constexpr int BOT_Y = 175;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CouchLA2AEditor)
};

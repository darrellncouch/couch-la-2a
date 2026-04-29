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
    void drawRackScrews    (juce::Graphics&) const;
    void drawKnobScale     (juce::Graphics&, float cx, float cy,
                            float innerR, float outerR, float labelR) const;
    void drawKnobLabels    (juce::Graphics&) const;
    void drawModeSwitch    (juce::Graphics&) const;
    void drawVUMeterFace   (juce::Graphics&) const;
    void drawVUNeedle      (juce::Graphics&) const;
    void drawBranding      (juce::Graphics&) const;
    void drawPowerSwitch   (juce::Graphics&) const;

    // ── VU needle angle (clock-face convention, ±60° from 12 o'clock) ────
    // VU scale: −20 VU (full left −π/3) … +3 VU (full right +π/3)
    // 0 VU = −18 dBFS reference
    float needleAngleForVU (float outDbFS) const noexcept;

    // ── Helpers ───────────────────────────────────────────────────────────
    juce::Rectangle<float> getLimitBtnBounds()    const noexcept;
    juce::Rectangle<float> getCompressBtnBounds() const noexcept;
    juce::Rectangle<float> getPowerBtnBounds()    const noexcept;
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

    // Power toggle state
    mutable bool powerOn = true;

    // ── Layout constants ──────────────────────────────────────────────────
    static constexpr int PLUGIN_W = 880;
    static constexpr int PLUGIN_H = 240;
    static constexpr int EAR_W    = 28;

    // LIMIT/COMPRESS toggle – positioned near the bottom-left
    static constexpr int SW_X = 38,  SW_W = 78, SW_H = 24;
    static constexpr int SW_LIMIT_Y    = 148;
    static constexpr int SW_COMPRESS_Y = 188;

    static constexpr int GAIN_X = 190, GAIN_Y = 65, GAIN_W = 105, GAIN_H = 105;

    // VU meter face – centred in the wider plugin
    static constexpr int   VM_X = 330, VM_Y = 18, VM_W = 220, VM_H = 165;
    // Inner opening of the window bezel (inset ~18px each side at plugin scale)
    static constexpr int   VM_INSET = 18;
    static constexpr float VM_PX = VM_X + VM_W * 0.5f;
    // Pivot sits 16px higher than the inner bezel bottom so needle emerges from within the face
    static constexpr float VM_PY = VM_Y + VM_H - VM_INSET - 16.f;

    // Radii sized to fit within the inner opening (inner half-width ~92px)
    // Arc baseline at SCALE_R_IN; ticks extend outward to SCALE_R_OUT; labels beyond that.
    static constexpr float NEEDLE_R    = 100.f;
    static constexpr float SCALE_R_IN  =  68.f;   // arc baseline
    static constexpr float SCALE_R_OUT =  80.f;   // outer tick tip (ticks go outward = up)
    static constexpr float LABEL_R     =  90.f;   // labels outside arc, near clip boundary

    static constexpr int PR_X = 585,  PR_Y = 65, PR_W = 105, PR_H = 105;

    // Power toggle – near the bottom-right
    static constexpr int PWR_CX = 775;   // centre x
    static constexpr int PWR_CY = 190;   // centre y

    // Power LED – top-right corner of the silver panel area
    static constexpr int LED_CX = PLUGIN_W - EAR_W - 14;
    static constexpr int LED_CY = 14;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CouchLA2AEditor)
};

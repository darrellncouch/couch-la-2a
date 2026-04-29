#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
// Colour palette  — LA-2A: warm silver-gray panel, black knobs, amber meter
// ─────────────────────────────────────────────────────────────────────────────
namespace Col
{
    // Panel
    const juce::Colour panel      { 0xffc8c8bc };   // warm silver-gray
    const juce::Colour panelDark  { 0xffb0b0a4 };
    const juce::Colour panelLight { 0xffe0e0d4 };
    const juce::Colour chrome     { 0xff909090 };
    const juce::Colour chromeDark { 0xff606060 };

    // Text (engraved / silk-screened look)
    const juce::Colour labelDark  { 0xff1a1a1a };
    const juce::Colour labelMid   { 0xff404040 };

    // Knob
    const juce::Colour knobBody1  { 0xff181818 };
    const juce::Colour knobBody2  { 0xff050505 };
    const juce::Colour knobRing   { 0xff787878 };
    const juce::Colour knobPtr    { 0xffffffff };

    // Mode switch buttons
    const juce::Colour swOff      { 0xff8a8a7e };
    const juce::Colour swOn       { 0xfff0f0e8 };
    const juce::Colour swBorder   { 0xff505048 };
    const juce::Colour swTextOff  { 0xff282820 };
    const juce::Colour swTextOn   { 0xff0a0a08 };

    // VU meter face
    const juce::Colour meterBezel { 0xff202020 };
    const juce::Colour meterFaceA { 0xfff5e858 };   // warm amber top
    const juce::Colour meterFaceB { 0xffc89000 };   // darker amber bottom
    const juce::Colour meterPrint { 0xff0e0800 };   // dark brown scale
    const juce::Colour meterNeedl { 0xff0e0800 };

    // Power LED
    const juce::Colour ledGreen   { 0xff00dd44 };
}

// ─────────────────────────────────────────────────────────────────────────────
// LookAndFeel
// ─────────────────────────────────────────────────────────────────────────────

CouchLA2ALookAndFeel::CouchLA2ALookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId,    Col::knobRing);
    setColour (juce::Slider::rotarySliderOutlineColourId, Col::chromeDark);
}

void CouchLA2ALookAndFeel::drawRotarySlider (juce::Graphics& g,
                                              int x, int y, int w, int h,
                                              float sliderPos,
                                              float startAngle, float endAngle,
                                              juce::Slider& slider)
{
    juce::ignoreUnused (slider);

    const float cx  = x + w * 0.5f;
    const float cy  = y + h * 0.5f;
    const float rad = juce::jmin (w, h) * 0.5f - 4.f;

    // Chrome outer ring
    g.setColour (Col::knobRing);
    g.fillEllipse (cx - rad - 4.f, cy - rad - 4.f,
                   (rad + 4.f) * 2.f, (rad + 4.f) * 2.f);

    // Inner shadow rim
    g.setColour (juce::Colour (0xff0a0a0a));
    g.fillEllipse (cx - rad - 1.f, cy - rad - 1.f,
                   (rad + 1.f) * 2.f, (rad + 1.f) * 2.f);

    // Knob body gradient
    juce::ColourGradient bodyGrad (Col::knobBody1, cx - rad * 0.35f, cy - rad * 0.45f,
                                   Col::knobBody2, cx + rad * 0.5f,  cy + rad * 0.55f, false);
    g.setGradientFill (bodyGrad);
    g.fillEllipse (cx - rad, cy - rad, rad * 2.f, rad * 2.f);

    // Pointer (white line)
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float pLen  = rad * 0.73f;
    const float px    = cx + std::sin (angle) * pLen;
    const float py    = cy - std::cos (angle) * pLen;

    g.setColour (Col::knobPtr);
    g.drawLine (cx, cy, px, py, 3.f);

    // White dot at center to cap the line
    const float capR = rad * 0.10f;
    g.setColour (Col::knobBody1);
    g.fillEllipse (cx - capR, cy - capR, capR * 2.f, capR * 2.f);
}

juce::Label* CouchLA2ALookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setColour (juce::Label::textColourId,       juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId,    juce::Colours::transparentBlack);
    return l;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

CouchLA2AEditor::CouchLA2AEditor (CouchLA2AProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p)
{
    setLookAndFeel (&lnf);
    setSize (PLUGIN_W, PLUGIN_H);

    const float startA = juce::MathConstants<float>::pi * 1.25f;
    const float endA   = juce::MathConstants<float>::pi * 2.75f;

    auto configSlider = [&] (juce::Slider& s)
    {
        s.setSliderStyle   (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle  (juce::Slider::NoTextBox, true, 0, 0);
        s.setRotaryParameters (startA, endA, true);
        s.setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (s);
    };

    configSlider (gainSlider);
    configSlider (peakReductionSlider);

    gainAttach          = std::make_unique<SliderAttach> (p.apvts, "gain",          gainSlider);
    peakReductionAttach = std::make_unique<SliderAttach> (p.apvts, "peakReduction", peakReductionSlider);

    // Rest needle at 0 VU position
    needleAngle = needleAngleForVU (-18.f);

    startTimerHz (30);
}

CouchLA2AEditor::~CouchLA2AEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// resized
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::resized()
{
    gainSlider         .setBounds (GAIN_X, GAIN_Y, GAIN_W, GAIN_H);
    peakReductionSlider.setBounds (PR_X,   PR_Y,   PR_W,   PR_H);
}

// ─────────────────────────────────────────────────────────────────────────────
// Timer – needle physics
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::timerCallback()
{
    const float target = needleAngleForVU (processor.getOutputLevelDb());

    needleVelocity = needleVelocity * 0.78f + (target - needleAngle) * 0.10f;
    needleAngle   += needleVelocity;

    repaint (VM_X - 4, VM_Y - 4, VM_W + 8, VM_H + 8);
}

// ─────────────────────────────────────────────────────────────────────────────
// Needle angle  (clock-face: 0 = 12 o'clock, CW positive)
// VU scale: −20 VU = full left (−π/3),  +3 VU = full right (+π/3)
// Reference: 0 VU = −18 dBFS
// ─────────────────────────────────────────────────────────────────────────────

float CouchLA2AEditor::needleAngleForVU (float outDbFS) const noexcept
{
    const float vuLevel = outDbFS + 18.f;   // dBFS → VU
    // Map −20..+3 VU → norm 0..1 (left..right)
    const float norm = juce::jlimit (0.f, 1.f, (vuLevel + 20.f) / 23.f);
    // norm=0 → left (−π/3),  norm=1 → right (+π/3)
    return juce::MathConstants<float>::pi / 3.f * (2.f * norm - 1.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Mode switch helpers
// ─────────────────────────────────────────────────────────────────────────────

int CouchLA2AEditor::getCurrentMode() const noexcept
{
    auto* raw = processor.apvts.getRawParameterValue ("mode");
    return raw ? static_cast<int> (raw->load()) : 0;
}

juce::Rectangle<float> CouchLA2AEditor::getLimitBtnBounds() const noexcept
{
    return { (float) SW_X, (float) SW_LIMIT_Y, (float) SW_W, (float) SW_H };
}

juce::Rectangle<float> CouchLA2AEditor::getCompressBtnBounds() const noexcept
{
    return { (float) SW_X, (float) SW_COMPRESS_Y, (float) SW_W, (float) SW_H };
}

// ─────────────────────────────────────────────────────────────────────────────
// Mouse
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::mouseDown (const juce::MouseEvent& e)
{
    const auto pt = e.position;

    if (getLimitBtnBounds().contains (pt))
    {
        if (auto* param = processor.apvts.getParameter ("mode"))
            param->setValueNotifyingHost (1.f);
        repaint (SW_X - 4, SW_LIMIT_Y - 4, SW_W + 8, SW_COMPRESS_Y + SW_H - SW_LIMIT_Y + 8);
        return;
    }

    if (getCompressBtnBounds().contains (pt))
    {
        if (auto* param = processor.apvts.getParameter ("mode"))
            param->setValueNotifyingHost (0.f);
        repaint (SW_X - 4, SW_LIMIT_Y - 4, SW_W + 8, SW_COMPRESS_Y + SW_H - SW_LIMIT_Y + 8);
        return;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// paint
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::paint (juce::Graphics& g)
{
    drawPanel       (g);
    drawKnobLabels  (g);

    // Dial scales for both large knobs (drawn in editor space at larger radii)
    drawKnobScale (g,
        GAIN_X + GAIN_W * 0.5f, GAIN_Y + GAIN_H * 0.5f,
        GAIN_W * 0.5f - 1.f,
        GAIN_W * 0.5f + 11.f,
        GAIN_W * 0.5f + 19.f);

    drawKnobScale (g,
        PR_X + PR_W * 0.5f, PR_Y + PR_H * 0.5f,
        PR_W * 0.5f - 1.f,
        PR_W * 0.5f + 11.f,
        PR_W * 0.5f + 19.f);

    drawModeSwitch  (g);
    drawVUMeterFace (g);
    drawVUNeedle    (g);
    drawBranding    (g);
    drawPowerLED    (g);
}

// ─────────────────────────────────────────────────────────────────────────────
// Panel background  — warm silver-gray, brushed horizontal lines
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawPanel (juce::Graphics& g) const
{
    juce::ColourGradient panelGrad (Col::panelLight, 0.f, 0.f,
                                    Col::panelDark,  0.f, (float) PLUGIN_H, false);
    g.setGradientFill (panelGrad);
    g.fillAll();

    // Very subtle horizontal brushing
    g.setColour (juce::Colour (0x08000000));
    for (int y = 0; y < PLUGIN_H; y += 2)
        g.drawHorizontalLine (y, 0.f, (float) PLUGIN_W);

    // Outer chrome border
    g.setColour (Col::chrome.withAlpha (0.7f));
    g.drawRect (getLocalBounds(), 3);
    g.setColour (Col::chromeDark.withAlpha (0.4f));
    g.drawRect (getLocalBounds().reduced (3), 1);

    // Bottom bar slightly darker
    g.setColour (Col::panelDark.withAlpha (0.5f));
    g.fillRect (0, BOT_Y, PLUGIN_W, PLUGIN_H - BOT_Y);
    g.setColour (Col::chromeDark.withAlpha (0.35f));
    g.drawHorizontalLine (BOT_Y, 0.f, (float) PLUGIN_W);
}

// ─────────────────────────────────────────────────────────────────────────────
// Dial scale  — printed numbers around knob (0, 20, 40, 60, 80, 100)
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawKnobScale (juce::Graphics& g,
                                      float cx, float cy,
                                      float innerR, float outerR, float labelR) const
{
    // Match slider rotary params set in constructor
    const float startAngle = juce::MathConstants<float>::pi * 1.25f;
    const float endAngle   = juce::MathConstants<float>::pi * 2.75f;

    const int nTicks = 11;  // 0–10, scale labelled as 0,20,40,60,80,100

    g.setFont (juce::Font ("Arial", 8.5f, juce::Font::bold));

    for (int i = 0; i < nTicks; ++i)
    {
        const float t     = static_cast<float> (i) / static_cast<float> (nTicks - 1);
        const float angle = startAngle + t * (endAngle - startAngle);
        const float sa    = std::sin (angle);
        const float ca    = std::cos (angle);

        const bool  isMajor = (i % 2 == 0);  // 0, 20, 40, 60, 80, 100
        const float r1      = isMajor ? innerR : innerR + (outerR - innerR) * 0.3f;

        // Tick mark (dark, engraved look)
        g.setColour (isMajor ? Col::labelDark : Col::labelMid.withAlpha (0.6f));
        g.drawLine (cx + sa * r1,     cy - ca * r1,
                    cx + sa * outerR, cy - ca * outerR,
                    isMajor ? 1.8f : 0.9f);

        // Label at major ticks (every 20)
        if (isMajor)
        {
            const int    val = i * 10;   // 0, 20, 40, 60, 80, 100
            const float  lx  = cx + sa * labelR;
            const float  ly  = cy - ca * labelR;
            g.setColour (Col::labelDark);
            g.drawText (juce::String (val),
                        juce::Rectangle<float> (lx - 10.f, ly - 6.f, 20.f, 12.f),
                        juce::Justification::centred);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Knob labels
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawKnobLabels (juce::Graphics& g) const
{
    g.setFont (juce::Font ("Arial", 10.f, juce::Font::bold));
    g.setColour (Col::labelDark);

    // GAIN below knob
    g.drawText ("GAIN",
                GAIN_X, GAIN_Y + GAIN_H + 20, GAIN_W, 14,
                juce::Justification::centred);

    // PEAK REDUCTION below knob
    g.drawText ("PEAK REDUCTION",
                PR_X - 10, PR_Y + PR_H + 20, PR_W + 20, 14,
                juce::Justification::centred);
}

// ─────────────────────────────────────────────────────────────────────────────
// LIMIT / COMPRESS toggle switch
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawModeSwitch (juce::Graphics& g) const
{
    const int   mode = getCurrentMode();

    // Section label
    g.setFont (juce::Font ("Arial", 8.f, juce::Font::bold));
    g.setColour (Col::labelDark);
    g.drawText ("MODE", SW_X, SW_LIMIT_Y - 16, SW_W, 13,
                juce::Justification::centred);

    auto drawBtn = [&] (juce::Rectangle<float> b, const char* label, bool lit)
    {
        // Button body
        juce::ColourGradient grad (
            lit ? Col::swOn : Col::swOff, b.getX(), b.getY(),
            lit ? Col::panelDark : Col::chromeDark, b.getX(), b.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (b, 3.f);

        // Border
        g.setColour (Col::swBorder);
        g.drawRoundedRectangle (b, 3.f, 1.2f);

        // Inset shadow when not lit
        if (!lit)
        {
            g.setColour (juce::Colours::black.withAlpha (0.2f));
            g.drawRoundedRectangle (b.reduced (1.f), 2.5f, 0.7f);
        }

        // Text
        g.setFont (juce::Font ("Arial", 9.f, juce::Font::bold));
        g.setColour (lit ? Col::swTextOn : Col::swTextOff);
        g.drawText (label, b.toNearestInt(), juce::Justification::centred);
    };

    drawBtn (getLimitBtnBounds(),    "LIMIT",    mode == 1);
    drawBtn (getCompressBtnBounds(), "COMPRESS", mode == 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// VU Meter face
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawVUMeterFace (juce::Graphics& g) const
{
    const juce::Rectangle<float> face ((float) VM_X, (float) VM_Y,
                                       (float) VM_W, (float) VM_H);

    // ── Outer black bezel ──────────────────────────────────────────────────
    g.setColour (Col::meterBezel);
    g.fillRoundedRectangle (face.expanded (6.f), 5.f);
    g.setColour (juce::Colour (0xff404040));
    g.fillRoundedRectangle (face.expanded (3.f), 4.f);

    // ── Amber meter face ───────────────────────────────────────────────────
    juce::ColourGradient faceGrad (Col::meterFaceA, face.getCentreX(), face.getY(),
                                   Col::meterFaceB, face.getCentreX(), face.getBottom(), false);
    g.setGradientFill (faceGrad);
    g.fillRoundedRectangle (face, 3.f);

    // Amber glow overlay
    g.setColour (juce::Colour (0x22ffcc00));
    g.fillRoundedRectangle (face, 3.f);

    // ── VU scale tick marks ───────────────────────────────────────────────
    // Pivot in component space
    const float px = VM_PX;
    const float py = VM_PY;

    // Standard VU scale: −20, −10, −7, −5, −3, −2, −1, 0, +1, +2, +3
    // Needle formula: norm = (vu + 20) / 23, angle = π/3 * (2*norm - 1)
    struct Tick { float vu; const char* label; bool major; };
    const Tick ticks[] = {
        { -20.f, "-20", true  },
        { -10.f, "-10", true  },
        {  -7.f, "-7",  false },
        {  -5.f, "-5",  true  },
        {  -3.f, "-3",  false },
        {  -2.f, "-2",  false },
        {  -1.f, "-1",  false },
        {   0.f, "0",   true  },
        {   1.f, "+1",  false },
        {   2.f, "+2",  false },
        {   3.f, "+3",  true  },
    };

    for (const auto& t : ticks)
    {
        const float norm  = (t.vu + 20.f) / 23.f;
        const float angle = juce::MathConstants<float>::pi / 3.f * (2.f * norm - 1.f);
        const float sa    = std::sin (angle);
        const float ca    = std::cos (angle);

        const float r1 = t.major ? SCALE_R_IN - 4.f : SCALE_R_IN + 1.f;

        g.setColour (t.major ? Col::meterPrint : Col::meterPrint.withAlpha (0.55f));
        g.drawLine (px + sa * r1,          py - ca * r1,
                    px + sa * SCALE_R_OUT, py - ca * SCALE_R_OUT,
                    t.major ? 1.6f : 0.9f);

        if (t.major)
        {
            const float lx = px + sa * LABEL_R;
            const float ly = py - ca * LABEL_R;
            g.setFont (juce::Font ("Arial", 7.5f, juce::Font::bold));
            g.setColour (Col::meterPrint);
            g.drawText (t.label,
                        juce::Rectangle<float> (lx - 10.f, ly - 6.f, 20.f, 12.f),
                        juce::Justification::centred);
        }
    }

    // Thin arc baseline
    juce::Path arc;
    for (int i = 0; i <= 60; ++i)
    {
        const float f  = i / 60.f;
        const float a  = juce::MathConstants<float>::pi / 3.f * (2.f * f - 1.f);
        const float ax = px + std::sin (a) * SCALE_R_OUT;
        const float ay = py - std::cos (a) * SCALE_R_OUT;
        if (i == 0) arc.startNewSubPath (ax, ay);
        else        arc.lineTo (ax, ay);
    }
    g.setColour (Col::meterPrint.withAlpha (0.3f));
    g.strokePath (arc, juce::PathStrokeType (0.6f));

    // "VU" labels at left and right ends of scale
    g.setFont (juce::Font ("Arial", 8.f, juce::Font::bold));
    g.setColour (Col::meterPrint.withAlpha (0.7f));
    g.drawText ("VU", VM_X + 6,        (int) (VM_PY - 14.f), 20, 10,
                juce::Justification::centred);
    g.drawText ("VU", VM_X + VM_W - 26, (int) (VM_PY - 14.f), 20, 10,
                juce::Justification::centred);

    // "VU LEVEL INDICATOR" centre label
    g.setFont (juce::Font ("Arial", 7.5f, juce::Font::bold));
    g.setColour (Col::meterPrint.withAlpha (0.6f));
    g.drawText ("VU  LEVEL  INDICATOR",
                VM_X, (int) (VM_PY + 2.f), VM_W, 10,
                juce::Justification::centred);

    // Glass sheen
    juce::ColourGradient glassGrad (juce::Colour (0x12ffffff), face.getX(), face.getY(),
                                    juce::Colour (0x00000000), face.getX(), face.getCentreY(), false);
    g.setGradientFill (glassGrad);
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.45f), 3.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// VU Needle
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawVUNeedle (juce::Graphics& g) const
{
    const float px = VM_PX;
    const float py = VM_PY;

    const float maxAng = juce::MathConstants<float>::pi / 3.f;
    const float angle  = juce::jlimit (-maxAng, maxAng, needleAngle);

    const float sa = std::sin (angle);
    const float ca = std::cos (angle);

    const float tipX = px + sa * NEEDLE_R;
    const float tipY = py - ca * NEEDLE_R;
    const float midX = px + sa * NEEDLE_R * 0.67f;
    const float midY = py - ca * NEEDLE_R * 0.67f;

    // Shadow
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawLine (px + 1.f, py + 1.f, tipX + 1.f, tipY + 1.f, 1.5f);

    // Body
    g.setColour (Col::meterNeedl);
    g.drawLine (px, py, midX, midY, 1.8f);

    // Tip
    g.setColour (juce::Colour (0xff2a0e00));
    g.drawLine (midX, midY, tipX, tipY, 1.2f);

    // Pivot jewel (warm brown for the LA-2A aesthetic)
    g.setColour (juce::Colour (0xff2a1a00));
    g.fillEllipse (px - 4.5f, py - 4.5f, 9.f, 9.f);
    g.setColour (juce::Colour (0xff9a6020));
    g.fillEllipse (px - 2.5f, py - 2.5f, 5.f, 5.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Branding
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawBranding (juce::Graphics& g) const
{
    // "COUCH" company — top left
    g.setFont (juce::Font ("Arial", 13.f, juce::Font::bold));
    g.setColour (Col::labelDark);
    g.drawText ("COUCH", 10, 8, 110, 18, juce::Justification::centredLeft);

    // "AUDIO" sub-line
    g.setFont (juce::Font ("Arial", 8.f, juce::Font::plain));
    g.setColour (Col::labelMid);
    g.drawText ("AUDIO", 10, 24, 110, 10, juce::Justification::centredLeft);

    // "LEVELING AMPLIFIER" centered top
    g.setFont (juce::Font ("Arial", 9.f, juce::Font::bold));
    g.setColour (Col::labelMid);
    g.drawText ("LEVELING  AMPLIFIER", 120, 6, PLUGIN_W - 240, 13,
                juce::Justification::centred);

    // "MODEL LA-2A" sub-line centered
    g.setFont (juce::Font ("Arial", 8.f, juce::Font::plain));
    g.setColour (Col::labelMid.withAlpha (0.8f));
    g.drawText ("MODEL  LA-2A", 120, 18, PLUGIN_W - 240, 11,
                juce::Justification::centred);

    // Below meter: "OPTICAL LEVELING AMPLIFIER" small tagline
    g.setFont (juce::Font ("Arial", 7.5f, juce::Font::plain));
    g.setColour (Col::labelMid.withAlpha (0.6f));
    g.drawText ("OPTICAL  LEVELING  AMPLIFIER",
                VM_X, VM_Y + VM_H + 4, VM_W, 10,
                juce::Justification::centred);
}

// ─────────────────────────────────────────────────────────────────────────────
// Power LED
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawPowerLED (juce::Graphics& g) const
{
    const float lx = PLUGIN_W - 30.f;
    const float ly = 100.f;
    const float lr = 5.f;

    // "ON" label above
    g.setFont (juce::Font ("Arial", 8.f, juce::Font::bold));
    g.setColour (Col::labelDark);
    g.drawText ("ON", (int) (lx - 14.f), (int) (ly - lr - 16.f), 28, 12,
                juce::Justification::centred);

    // Glow
    g.setColour (Col::ledGreen.withAlpha (0.25f));
    g.fillEllipse (lx - lr * 2.2f, ly - lr * 2.2f, lr * 4.4f, lr * 4.4f);

    // LED body
    juce::ColourGradient ledGrad (Col::ledGreen.brighter(), lx - lr * 0.4f, ly - lr * 0.4f,
                                  Col::ledGreen.darker(),   lx + lr,        ly + lr, false);
    g.setGradientFill (ledGrad);
    g.fillEllipse (lx - lr, ly - lr, lr * 2.f, lr * 2.f);

    // Shine
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.fillEllipse (lx - lr * 0.45f, ly - lr * 0.55f, lr * 0.55f, lr * 0.45f);

    // "POWER" label below
    g.setFont (juce::Font ("Arial", 7.5f, juce::Font::plain));
    g.setColour (Col::labelMid);
    g.drawText ("POWER", (int) (lx - 18.f), (int) (ly + lr + 3.f), 36, 10,
                juce::Justification::centred);
}

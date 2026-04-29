#include "PluginEditor.h"
#include <BinaryData.h>

// ─────────────────────────────────────────────────────────────────────────────
// Colour palette
// ─────────────────────────────────────────────────────────────────────────────
namespace Col
{
    // Panel
    const juce::Colour panelBase   { 0xffc8c8be };
    const juce::Colour panelLight  { 0xffdcdcd0 };
    const juce::Colour panelDark   { 0xffababA0 };
    const juce::Colour panelShadow { 0xff909088 };
    const juce::Colour rackEar     { 0xff1e1e1e };
    const juce::Colour rackEarMid  { 0xff2e2e2e };

    // Chrome / metal
    const juce::Colour chrome      { 0xffb0b0b0 };
    const juce::Colour chromeMid   { 0xff808080 };
    const juce::Colour chromeDark  { 0xff404040 };
    const juce::Colour chromeShine { 0xffe0e0e0 };

    // Labels (silk-screened look)
    const juce::Colour labelDark   { 0xff0e0e0e };
    const juce::Colour labelMid    { 0xff383830 };
    const juce::Colour labelLight  { 0xff606058 };

    // Knob
    const juce::Colour knobShadow  { 0x88000000 };
    const juce::Colour knobRingH   { 0xffc8c8c8 };   // ring highlight
    const juce::Colour knobRingD   { 0xff484848 };   // ring shadow
    const juce::Colour knobBodyH   { 0xff555555 };   // body highlight
    const juce::Colour knobBodyM   { 0xff1a1a1a };   // body mid
    const juce::Colour knobBodyD   { 0xff020202 };   // body dark
    const juce::Colour knobSpecH   { 0xaaffffff };   // specular
    const juce::Colour knobPtr     { 0xffffffff };

    // VU meter
    const juce::Colour meterBezel  { 0xff0a0a0a };
    const juce::Colour meterBezel2 { 0xff303028 };
    const juce::Colour meterFaceH  { 0xfffff8e0 };   // warm ivory
    const juce::Colour meterFaceL  { 0xffe8d8a0 };   // aged tan
    const juce::Colour meterPrint  { 0xff1a0a00 };   // dark brown ink
    const juce::Colour meterRed    { 0xffcc1a00 };   // red zone ink
    const juce::Colour meterNeedle { 0xff0e0800 };

    // Mode toggle
    const juce::Colour toggleBase  { 0xff606058 };
    const juce::Colour toggleShine { 0xffa0a098 };
    const juce::Colour toggleDark  { 0xff282820 };
    const juce::Colour toggleBat   { 0xff888880 };
    const juce::Colour toggleBatH  { 0xffb0b0a8 };

    // LED
    const juce::Colour ledGreen    { 0xff22ee55 };
    const juce::Colour ledGlow     { 0x4400cc44 };
}

// ─────────────────────────────────────────────────────────────────────────────
// LookAndFeel  — hyper-realistic knob
// ─────────────────────────────────────────────────────────────────────────────

CouchLA2ALookAndFeel::CouchLA2ALookAndFeel() {}

void CouchLA2ALookAndFeel::drawRotarySlider (juce::Graphics& g,
                                              int x, int y, int w, int h,
                                              float sliderPos,
                                              float /*startAngle*/, float /*endAngle*/,
                                              juce::Slider& /*slider*/)
{
    static juce::Image filmstrip = juce::ImageCache::getFromMemory (
        BinaryData::knob_filmstrip_png, BinaryData::knob_filmstrip_pngSize);

    if (! filmstrip.isValid())
        return;

    const int nFrames    = 100;
    const int frameH     = filmstrip.getHeight() / nFrames;
    const int frameIndex = juce::jlimit (0, nFrames - 1,
                                         (int) (sliderPos * (nFrames - 1)));

    g.drawImage (filmstrip,
                 x, y, w, h,
                 0, frameIndex * frameH, filmstrip.getWidth(), frameH);
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
    : AudioProcessorEditor (&p), processor (p)
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
    repaint (VM_X - 8, VM_Y - 8, VM_W + 16, VM_H + 16);
}

// ─────────────────────────────────────────────────────────────────────────────
// Needle angle
// ─────────────────────────────────────────────────────────────────────────────

float CouchLA2AEditor::needleAngleForVU (float outDbFS) const noexcept
{
    const float vuLevel = outDbFS + 18.f;
    const float norm    = juce::jlimit (0.f, 1.f, (vuLevel + 20.f) / 23.f);
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
    return { (float) SW_X - 14.f, (float) SW_LIMIT_Y, (float) SW_W + 14.f, (float) SW_H };
}

juce::Rectangle<float> CouchLA2AEditor::getCompressBtnBounds() const noexcept
{
    return { (float) SW_X - 14.f, (float) SW_COMPRESS_Y, (float) SW_W + 14.f, (float) SW_H };
}

juce::Rectangle<float> CouchLA2AEditor::getPowerBtnBounds() const noexcept
{
    return { (float) PWR_CX - 44.f, (float) PWR_CY - 36.f, 88.f, 72.f };
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
        repaint();
        return;
    }

    if (getCompressBtnBounds().contains (pt))
    {
        if (auto* param = processor.apvts.getParameter ("mode"))
            param->setValueNotifyingHost (0.f);
        repaint();
        return;
    }

    if (getPowerBtnBounds().contains (pt))
    {
        powerOn = !powerOn;
        repaint();
        return;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// paint
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::paint (juce::Graphics& g)
{
    drawPanel      (g);
    drawBranding   (g);
    drawKnobScale (g,
        GAIN_X + GAIN_W * 0.5f, GAIN_Y + GAIN_H * 0.5f,
        GAIN_W * 0.5f,
        GAIN_W * 0.5f + 5.f,
        GAIN_W * 0.5f + 14.f);
    drawKnobScale (g,
        PR_X + PR_W * 0.5f, PR_Y + PR_H * 0.5f,
        PR_W * 0.5f,
        PR_W * 0.5f + 5.f,
        PR_W * 0.5f + 14.f);
    drawKnobLabels  (g);
    drawModeSwitch  (g);
    drawVUMeterFace (g);
    drawVUNeedle    (g);
    drawPowerSwitch (g);
    drawRackScrews  (g);
}

// ─────────────────────────────────────────────────────────────────────────────
// Panel background — brushed silver-gray with rack ears
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawPanel (juce::Graphics& g) const
{
    // ── Flat silver/gray panel ────────────────────────────────────────────
    g.setColour (juce::Colour (0xffc4c4c0));
    g.fillAll();

    // Subtle top-to-bottom shading for slight depth
    juce::ColourGradient pg (juce::Colour (0xffd0d0cc), 0.f, 0.f,
                             juce::Colour (0xffb8b8b4), 0.f, (float) PLUGIN_H, false);
    g.setGradientFill (pg);
    g.fillRect (EAR_W, 0, PLUGIN_W - EAR_W * 2, PLUGIN_H);

    // ── Dark rack ears (left and right) ───────────────────────────────────
    for (int side = 0; side < 2; ++side)
    {
        const int ex = (side == 0) ? 0 : PLUGIN_W - EAR_W;
        juce::ColourGradient eg (
            side == 0 ? Col::rackEarMid : Col::rackEar, (float) ex, 0.f,
            side == 0 ? Col::rackEar : Col::rackEarMid, (float)(ex + EAR_W), 0.f, false);
        g.setGradientFill (eg);
        g.fillRect (ex, 0, EAR_W, PLUGIN_H);

        g.setColour (juce::Colour (0xff505050));
        g.drawVerticalLine (side == 0 ? EAR_W : PLUGIN_W - EAR_W - 1, 0.f, (float) PLUGIN_H);
    }

    // ── Panel edge inset shadow ───────────────────────────────────────────
    {
        juce::ColourGradient ls (juce::Colour (0x28000000), (float) EAR_W, 0.f,
                                 juce::Colours::transparentBlack, (float)(EAR_W + 14), 0.f, false);
        g.setGradientFill (ls);
        g.fillRect ((float) EAR_W, 0.f, 14.f, (float) PLUGIN_H);

        juce::ColourGradient rs (juce::Colours::transparentBlack, (float)(PLUGIN_W - EAR_W - 14), 0.f,
                                 juce::Colour (0x28000000), (float)(PLUGIN_W - EAR_W), 0.f, false);
        g.setGradientFill (rs);
        g.fillRect ((float)(PLUGIN_W - EAR_W - 14), 0.f, 14.f, (float) PLUGIN_H);
    }

    // ── Border lines ──────────────────────────────────────────────────────
    g.setColour (juce::Colour (0xff484840));
    g.drawHorizontalLine (0,            (float) EAR_W, (float)(PLUGIN_W - EAR_W));
    g.drawHorizontalLine (PLUGIN_H - 1, (float) EAR_W, (float)(PLUGIN_W - EAR_W));
    g.setColour (juce::Colours::white.withAlpha (0.30f));
    g.drawHorizontalLine (1,            (float) EAR_W, (float)(PLUGIN_W - EAR_W));
}

// ─────────────────────────────────────────────────────────────────────────────
// Rack-mount corner screws
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawRackScrews (juce::Graphics& g) const
{
    const float screwPositions[][2] = {
        { (float)(EAR_W / 2),             (float)(PLUGIN_H / 2) },
        { (float)(PLUGIN_W - EAR_W / 2),  (float)(PLUGIN_H / 2) }
    };

    for (const auto& sp : screwPositions)
    {
        const float sx = sp[0];
        const float sy = sp[1];
        const float sr = 5.5f;

        // Outer shadow
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (sx - sr - 0.5f, sy - sr + 0.5f, (sr + 0.5f) * 2.f, (sr + 0.5f) * 2.f);

        // Screw body
        juce::ColourGradient sg (juce::Colour (0xff686868), sx - sr * 0.4f, sy - sr * 0.4f,
                                 juce::Colour (0xff222222), sx + sr * 0.5f, sy + sr * 0.5f, false);
        g.setGradientFill (sg);
        g.fillEllipse (sx - sr, sy - sr, sr * 2.f, sr * 2.f);

        // Screw ring highlight
        g.setColour (juce::Colour (0xff888888));
        g.drawEllipse (sx - sr, sy - sr, sr * 2.f, sr * 2.f, 0.8f);

        // Phillips cross slot
        g.setColour (juce::Colour (0xff0a0a0a));
        g.drawLine (sx - sr * 0.5f, sy, sx + sr * 0.5f, sy, 1.1f);
        g.drawLine (sx, sy - sr * 0.5f, sx, sy + sr * 0.5f, 1.1f);

        // Shine dot
        g.setColour (juce::Colours::white.withAlpha (0.3f));
        g.fillEllipse (sx - sr * 0.3f, sy - sr * 0.4f, sr * 0.28f, sr * 0.22f);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Dial scale  — printed numbers around knobs
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawKnobScale (juce::Graphics& g,
                                      float cx, float cy,
                                      float innerR, float outerR, float labelR) const
{
    const float startAngle = juce::MathConstants<float>::pi * 1.25f;
    const float endAngle   = juce::MathConstants<float>::pi * 2.75f;
    const int   nTicks     = 11;

    for (int i = 0; i < nTicks; ++i)
    {
        const float t     = (float) i / (float)(nTicks - 1);
        const float angle = startAngle + t * (endAngle - startAngle);
        const float sa    = std::sin (angle);
        const float ca    = std::cos (angle);
        const bool  major = (i % 2 == 0);
        const float r1    = major ? innerR + 1.f : innerR + (outerR - innerR) * 0.4f;

        // Engraved tick shadow
        g.setColour (juce::Colour (0xffffffff).withAlpha (major ? 0.18f : 0.10f));
        g.drawLine (cx + sa * (r1 + 1.f), cy - ca * (r1 + 1.f),
                    cx + sa * (outerR + 1.f), cy - ca * (outerR + 1.f),
                    major ? 2.f : 1.1f);

        // Dark tick
        g.setColour (major ? Col::labelDark : Col::labelMid.withAlpha (0.6f));
        g.drawLine (cx + sa * r1,      cy - ca * r1,
                    cx + sa * outerR,  cy - ca * outerR,
                    major ? 1.8f : 0.9f);

        if (true)   // label every tick (every 10)
        {
            const int   val = i * 10;
            const float lx  = cx + sa * labelR;
            const float ly  = cy - ca * labelR;
            g.setFont (juce::Font ("Arial", 8.f, juce::Font::bold));
            g.setColour (Col::labelDark);
            g.drawText (juce::String (val),
                        juce::Rectangle<float> (lx - 11.f, ly - 7.f, 22.f, 14.f),
                        juce::Justification::centred);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Knob labels
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawKnobLabels (juce::Graphics& g) const
{
    auto drawLabel = [&] (const juce::String& text, int bx, int by, int bw, int bh)
    {
        // Engraved shadow
        g.setFont (juce::Font ("Arial", 10.f, juce::Font::bold));
        g.setColour (juce::Colour (0xffffffff).withAlpha (0.35f));
        g.drawText (text, bx + 1, by + 1, bw, bh, juce::Justification::centred);
        g.setColour (Col::labelDark);
        g.drawText (text, bx, by, bw, bh, juce::Justification::centred);
    };

    drawLabel ("GAIN",
               GAIN_X, GAIN_Y + GAIN_H + 22, GAIN_W, 14);
    drawLabel ("PEAK REDUCTION",
               PR_X - 12, PR_Y + PR_H + 22, PR_W + 24, 14);
}

// ─────────────────────────────────────────────────────────────────────────────
// LIMIT / COMPRESS – 3-D metal bat toggle switches
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawModeSwitch (juce::Graphics& g) const
{
    const int mode = getCurrentMode();  // 0 = COMPRESS, 1 = LIMIT

    static juce::Image togUp   = juce::ImageCache::getFromMemory (
        BinaryData::toggle_up_png,   BinaryData::toggle_up_pngSize);
    static juce::Image togDown = juce::ImageCache::getFromMemory (
        BinaryData::toggle_down_png, BinaryData::toggle_down_pngSize);

    const juce::Image& img = (mode == 1) ? togUp : togDown;

    // Centre the toggle image over the combined LIMIT+COMPRESS click area
    const int tx = SW_X - 5;
    const int ty = SW_LIMIT_Y - 4;
    const int tw = SW_W + 10;
    const int th = SW_COMPRESS_Y + SW_H - SW_LIMIT_Y + 8;

    if (img.isValid())
        g.drawImage (img, tx, ty, tw, th,
                     0, 0, img.getWidth(), img.getHeight());

    // Labels (LIMIT above, COMPRESS below) — silk-screened look
    auto drawLabel = [&] (const juce::String& text, int lx, int ly, int lw, int lh)
    {
        g.setFont (juce::Font ("Arial", 8.5f, juce::Font::bold));
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawText (text, lx + 1, ly + 1, lw, lh, juce::Justification::centred);
        g.setColour (Col::labelDark.withAlpha (0.75f));
        g.drawText (text, lx, ly, lw, lh, juce::Justification::centred);
    };

    drawLabel ("LIMIT",    tx, ty - 8, tw, 12);
    drawLabel ("COMPRESS", tx, ty + th + 2, tw, 12);
}

// ─────────────────────────────────────────────────────────────────────────────
// VU Meter face  — vintage aged-paper look, red zone
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawVUMeterFace (juce::Graphics& g) const
{
    const float fx = (float) VM_X;
    const float fy = (float) VM_Y;
    const float fw = (float) VM_W;
    const float fh = (float) VM_H;
    const juce::Rectangle<float> face (fx, fy, fw, fh);

    // ── Window frame image ────────────────────────────────────────────────
    {
        static juce::Image windowImg = juce::ImageCache::getFromMemory (
            BinaryData::window_png, BinaryData::window_pngSize);

        if (windowImg.isValid())
            g.drawImage (windowImg,
                         fx - 9.f, fy - 9.f, fw + 18.f, fh + 18.f,
                         0, 0, windowImg.getWidth(), windowImg.getHeight());
    }

    // Clip all VU scale content to the inner opening of the window bezel
    g.saveState();
    g.reduceClipRegion (VM_X + VM_INSET, VM_Y + VM_INSET, VM_W - 2*VM_INSET, VM_H - 2*VM_INSET);

    // ── VU scale (pivot at VM_PX, VM_PY) ─────────────────────────────────
    const float px = VM_PX;
    const float py = VM_PY;

    // Standard VU ticks – more density between -20 and -10, every tick labelled
    struct Tick { float vu; const char* label; bool major; bool redZone; };
    const Tick ticks[] = {
        { -20.f, "-20", true,  false },
        { -18.f, "-18", false, false },
        { -16.f, "-16", false, false },
        { -14.f, "-14", false, false },
        { -12.f, "-12", false, false },
        { -10.f, "-10", true,  false },
        {  -7.f,  "-7", false, false },
        {  -5.f,  "-5", true,  false },
        {  -3.f,  "-3", false, false },
        {  -2.f,  "-2", false, false },
        {  -1.f,  "-1", false, false },
        {   0.f,   "0", true,  true  },
        {   1.f,  "+1", false, true  },
        {   2.f,  "+2", false, true  },
        {   3.f,  "+3", true,  true  },
    };

    // Filled arc band in red zone (0..+3 VU)
    {
        const float a0 = juce::MathConstants<float>::pi / 3.f
                       * (2.f * (20.f / 23.f) - 1.f);   // 0 VU
        const float a1 = juce::MathConstants<float>::pi / 3.f;  // +3 VU

        juce::Path redBand;
        const float rb1 = SCALE_R_IN - 2.f;
        const float rb2 = SCALE_R_OUT + 2.f;
        const int   nSeg = 24;
        for (int i = 0; i <= nSeg; ++i)
        {
            const float a  = a0 + (a1 - a0) * i / nSeg;
            const float sa = std::sin (a);
            const float ca = std::cos (a);
            if (i == 0)
                redBand.startNewSubPath (px + sa * rb1, py - ca * rb1);
            else
                redBand.lineTo          (px + sa * rb1, py - ca * rb1);
        }
        for (int i = nSeg; i >= 0; --i)
        {
            const float a  = a0 + (a1 - a0) * i / nSeg;
            const float sa = std::sin (a);
            const float ca = std::cos (a);
            redBand.lineTo (px + sa * rb2, py - ca * rb2);
        }
        redBand.closeSubPath();
        g.setColour (Col::meterRed.withAlpha (0.18f));
        g.fillPath (redBand);
    }

    // Tick marks and labels
    // Arc baseline sits at SCALE_R_IN; ticks extend OUTWARD to SCALE_R_OUT (upward on screen).
    // Labels are placed beyond SCALE_R_OUT, further from the pivot.
    for (const auto& t : ticks)
    {
        const float norm  = (t.vu + 20.f) / 23.f;
        const float angle = juce::MathConstants<float>::pi / 3.f * (2.f * norm - 1.f);
        const float sa    = std::sin (angle);
        const float ca    = std::cos (angle);
        // Ticks start at the arc baseline (SCALE_R_IN) and extend outward
        const float tickOuter = t.major ? SCALE_R_OUT + 2.f : SCALE_R_OUT - 3.f;

        const juce::Colour inkCol = t.redZone ? Col::meterRed : Col::meterPrint;

        g.setColour (inkCol.withAlpha (t.major ? 0.85f : 0.50f));
        g.drawLine (px + sa * SCALE_R_IN,    py - ca * SCALE_R_IN,
                    px + sa * tickOuter,     py - ca * tickOuter,
                    t.major ? 1.5f : 0.8f);

        // Label every tick, placed at LABEL_R (outside the arc)
        {
            const float lx = px + sa * LABEL_R;
            const float ly = py - ca * LABEL_R;
            g.setFont (juce::Font ("Arial", t.major ? 8.f : 7.f, juce::Font::bold));
            g.setColour (inkCol.withAlpha (t.major ? 0.9f : 0.65f));
            g.drawText (t.label,
                        juce::Rectangle<float> (lx - 13.f, ly - 7.f, 26.f, 14.f),
                        juce::Justification::centred);
        }
    }

    // Thin arc baseline
    {
        juce::Path arc;
        for (int i = 0; i <= 80; ++i)
        {
            const float f  = i / 80.f;
            const float a  = juce::MathConstants<float>::pi / 3.f * (2.f * f - 1.f);
            const float ax = px + std::sin (a) * SCALE_R_IN;
            const float ay = py - std::cos (a) * SCALE_R_IN;
            if (i == 0) arc.startNewSubPath (ax, ay);
            else        arc.lineTo (ax, ay);
        }
        g.setColour (Col::meterPrint.withAlpha (0.4f));
        g.strokePath (arc, juce::PathStrokeType (0.7f));
    }

    // "VU" text below each arc end (same X as arc endpoint, dropped below the arc)
    {
        g.setFont (juce::Font ("Arial", 8.f, juce::Font::bold));
        // Arc ends at ±π/3; Y at arc end = py - cos(π/3)*SCALE_R_IN = py - 0.5*SCALE_R_IN
        const float arcEndY = py - std::cos (juce::MathConstants<float>::pi / 3.f) * SCALE_R_IN;

        // left VU (−20 end)
        const float langle = -juce::MathConstants<float>::pi / 3.f;
        const float lvux   = px + std::sin (langle) * SCALE_R_IN;
        g.setColour (Col::meterPrint.withAlpha (0.55f));
        g.drawText ("VU", juce::Rectangle<float> (lvux - 10.f, arcEndY + 4.f, 20.f, 12.f),
                    juce::Justification::centred);

        // right VU (+3 end)
        const float rvux = px + std::sin (juce::MathConstants<float>::pi / 3.f) * SCALE_R_IN;
        g.setColour (Col::meterRed.withAlpha (0.65f));
        g.drawText ("VU", juce::Rectangle<float> (rvux - 10.f, arcEndY + 4.f, 20.f, 12.f),
                    juce::Justification::centred);
    }

    // Center text block removed

    // ── Glass reflection (top half) ───────────────────────────────────────
    {
        juce::ColourGradient glassGrad (
            juce::Colour (0x16ffffff), fx, fy,
            juce::Colours::transparentBlack, fx, fy + fh * 0.5f, false);
        g.setGradientFill (glassGrad);
        g.fillRoundedRectangle (face.withHeight (fh * 0.5f), 2.5f);

        // Thin bright top sheen line
        g.setColour (juce::Colour (0x18ffffff));
        g.drawHorizontalLine ((int) fy + 1, fx + 2.f, fx + fw - 2.f);
    }

    g.restoreState();  // end face clip
}

// ─────────────────────────────────────────────────────────────────────────────
// VU Needle  — tapered with jewel pivot
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawVUNeedle (juce::Graphics& g) const
{
    // Clip needle to the inner opening of the window bezel — hides the pivot
    g.saveState();
    g.reduceClipRegion (VM_X + VM_INSET, VM_Y + VM_INSET, VM_W - 2*VM_INSET, VM_H - 2*VM_INSET);

    const float px = VM_PX;
    const float py = VM_PY;
    const float maxAng = juce::MathConstants<float>::pi / 3.f;
    const float angle  = juce::jlimit (-maxAng, maxAng, needleAngle);
    const float sa     = std::sin (angle);
    const float ca     = std::cos (angle);

    const float tipX  = px + sa * NEEDLE_R;
    const float tipY  = py - ca * NEEDLE_R;
    const float baseX = px - sa * NEEDLE_R * 0.12f;
    const float baseY = py + ca * NEEDLE_R * 0.12f;

    // Shadow
    g.setColour (juce::Colours::black.withAlpha (0.20f));
    g.drawLine (baseX + 1.2f, baseY + 1.2f, tipX + 1.2f, tipY + 1.2f, 1.8f);

    // Tapered needle body
    {
        juce::Path needle;
        const float perpX = ca * 1.8f;
        const float perpY = sa * 1.8f;
        needle.startNewSubPath (tipX, tipY);
        needle.lineTo (baseX + perpX, baseY + perpY);
        needle.lineTo (baseX - perpX, baseY - perpY);
        needle.closeSubPath();

        // Gradient: slightly warmer near base
        juce::ColourGradient ng (juce::Colour (0xff2a1500), baseX, baseY,
                                 Col::meterNeedle, tipX, tipY, false);
        g.setGradientFill (ng);
        g.fillPath (needle);
    }

    g.restoreState();  // end face clip
}

// ─────────────────────────────────────────────────────────────────────────────
// Branding
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawBranding (juce::Graphics& g) const
{
    // Top header area

    // "LEVELING AMPLIFIER" centered between the knobs
    g.setFont (juce::Font ("Arial", 9.f, juce::Font::bold));
    g.setColour (Col::labelDark.withAlpha (0.75f));
    g.drawText ("LEVELING  AMPLIFIER",
                VM_X + VM_W + 4, 6, PLUGIN_W - EAR_W - (VM_X + VM_W + 4), 13,
                juce::Justification::centredLeft);

    // "DC-2A" sub-line
    g.setFont (juce::Font ("Arial", 8.f, juce::Font::plain));
    g.setColour (Col::labelMid.withAlpha (0.65f));
    g.drawText ("DC-2A",
                VM_X + VM_W + 4, 18, PLUGIN_W - EAR_W - (VM_X + VM_W + 4), 11,
                juce::Justification::centredLeft);

    // Top-left: "COUCH" logo
    g.setFont (juce::Font ("Arial", 14.f, juce::Font::bold));
    g.setColour (Col::labelDark.withAlpha (0.80f));
    g.drawText ("COUCH", EAR_W + 6, 7, 100, 18, juce::Justification::centredLeft);
    g.setFont (juce::Font ("Arial", 8.f, juce::Font::plain));
    g.setColour (Col::labelMid.withAlpha (0.60f));
    g.drawText ("AUDIO", EAR_W + 6, 23, 80, 10, juce::Justification::centredLeft);

    // Bottom: tagline below meter
    g.setFont (juce::Font ("Arial", 7.5f, juce::Font::plain));
    g.setColour (Col::labelLight.withAlpha (0.55f));
    g.drawText ("OPTICAL  LEVELING  AMPLIFIER",
                VM_X, VM_Y + VM_H + 5, VM_W, 11,
                juce::Justification::centred);
}

// ─────────────────────────────────────────────────────────────────────────────
// Power toggle switch — ON/OFF bat toggle with LED indicator, near bottom-right
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawPowerSwitch (juce::Graphics& g) const
{
    const float cx = (float) PWR_CX;
    const float cy = (float) PWR_CY;

    static juce::Image togUp   = juce::ImageCache::getFromMemory (
        BinaryData::toggle_up_png,   BinaryData::toggle_up_pngSize);
    static juce::Image togDown = juce::ImageCache::getFromMemory (
        BinaryData::toggle_down_png, BinaryData::toggle_down_pngSize);

    const juce::Image& img = powerOn ? togUp : togDown;

    // Toggle image centred at (cx, cy)
    const float tw = 88.f, th = 72.f;
    if (img.isValid())
        g.drawImage (img, (int)(cx - tw * 0.5f), (int)(cy - th * 0.5f), (int) tw, (int) th,
                     0, 0, img.getWidth(), img.getHeight());

    // Silk-screened labels
    auto drawLabel = [&] (const juce::String& text, float lx, float ly, float lw, float lh)
    {
        g.setFont (juce::Font ("Arial", 8.5f, juce::Font::bold));
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawText (text, (int)(lx + 1.f), (int)(ly + 1.f), (int) lw, (int) lh,
                    juce::Justification::centred);
        g.setColour (Col::labelDark.withAlpha (0.75f));
        g.drawText (text, (int) lx, (int) ly, (int) lw, (int) lh,
                    juce::Justification::centred);
    };

    drawLabel ("ON",  cx - 44.f, cy - th * 0.5f - 14.f, 88.f, 12.f);
    drawLabel ("OFF", cx - 44.f, cy + th * 0.5f +  2.f, 88.f, 12.f);

    // LED indicator – top-right corner of the silver panel
    const float lx = (float) LED_CX;
    const float ly = (float) LED_CY;
    const float lr = 5.5f;

    // Housing
    {
        juce::ColourGradient hg (juce::Colour (0xff383830), lx - lr, ly - lr,
                                 juce::Colour (0xff181810), lx + lr, ly + lr, false);
        g.setGradientFill (hg);
        g.fillEllipse (lx - lr - 1.f, ly - lr - 1.f, (lr + 1.f) * 2.f, (lr + 1.f) * 2.f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawEllipse (lx - lr - 1.f, ly - lr - 1.f, (lr + 1.f) * 2.f, (lr + 1.f) * 2.f, 0.8f);
    }

    if (powerOn)
    {
        // Glow aura
        juce::ColourGradient glow (Col::ledGlow, lx, ly,
                                   juce::Colours::transparentBlack, lx + 12.f, ly, true);
        g.setGradientFill (glow);
        g.fillEllipse (lx - 12.f, ly - 12.f, 24.f, 24.f);

        // Green jewel
        juce::ColourGradient lg (Col::ledGreen.brighter (0.3f), lx - lr * 0.35f, ly - lr * 0.4f,
                                 Col::ledGreen.darker  (0.4f), lx + lr,          ly + lr,       false);
        g.setGradientFill (lg);
        g.fillEllipse (lx - lr, ly - lr, lr * 2.f, lr * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillEllipse (lx - lr * 0.42f, ly - lr * 0.52f, lr * 0.5f, lr * 0.38f);
    }
    else
    {
        // Dark / off jewel
        g.setColour (juce::Colour (0xff303030));
        g.fillEllipse (lx - lr, ly - lr, lr * 2.f, lr * 2.f);
    }

    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawEllipse (lx - lr, ly - lr, lr * 2.f, lr * 2.f, 0.8f);

    // "POWER" label below the whole section
    g.setFont (juce::Font ("Arial", 7.5f, juce::Font::plain));
    g.setColour (Col::labelMid.withAlpha (0.55f));
    g.drawText ("POWER",
                (int)(cx - 30.f), (int)(cy + th * 0.5f + 16.f), 60, 10,
                juce::Justification::centred);
}

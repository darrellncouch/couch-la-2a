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
        GAIN_W * 0.5f + 12.f,
        GAIN_W * 0.5f + 22.f);
    drawKnobScale (g,
        PR_X + PR_W * 0.5f, PR_Y + PR_H * 0.5f,
        PR_W * 0.5f,
        PR_W * 0.5f + 12.f,
        PR_W * 0.5f + 22.f);
    drawKnobLabels  (g);
    drawModeSwitch  (g);
    drawVUMeterFace (g);
    drawVUNeedle    (g);
    drawPowerLED    (g);
    drawRackScrews  (g);
}

// ─────────────────────────────────────────────────────────────────────────────
// Panel background — brushed silver-gray with rack ears
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawPanel (juce::Graphics& g) const
{
    // ── Metal texture background ──────────────────────────────────────────
    static juce::Image metalTex = juce::ImageCache::getFromMemory (
        BinaryData::panel_metal_png, BinaryData::panel_metal_pngSize);

    if (metalTex.isValid())
    {
        // Draw texture stretched to full plugin size
        g.drawImage (metalTex, 0, 0, PLUGIN_W, PLUGIN_H,
                     0, 0, metalTex.getWidth(), metalTex.getHeight());

        // Subtle darkening overlay toward edges (vignette)
        juce::ColourGradient vig (juce::Colours::transparentBlack, PLUGIN_W * 0.5f, PLUGIN_H * 0.4f,
                                  juce::Colour (0x28000000), 0.f, (float) PLUGIN_H, true);
        g.setGradientFill (vig);
        g.fillAll();
    }
    else
    {
        // Fallback plain gradient
        juce::ColourGradient pg (Col::panelLight, 0.f, 0.f,
                                 Col::panelDark,  0.f, (float) PLUGIN_H, false);
        g.setGradientFill (pg);
        g.fillAll();
    }

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

    // ── Bottom darker strip ───────────────────────────────────────────────
    juce::ColourGradient bot (juce::Colours::transparentBlack, 0.f, (float) BOT_Y,
                              juce::Colour (0x22000000), 0.f, (float) PLUGIN_H, false);
    g.setGradientFill (bot);
    g.fillRect (EAR_W, BOT_Y, PLUGIN_W - EAR_W * 2, PLUGIN_H - BOT_Y);
    g.setColour (juce::Colour (0x60404038));
    g.drawHorizontalLine (BOT_Y, (float) EAR_W, (float)(PLUGIN_W - EAR_W));
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

        if (major)
        {
            const int   val = i * 10;
            const float lx  = cx + sa * labelR;
            const float ly  = cy - ca * labelR;
            g.setFont (juce::Font ("Arial", 9.f, juce::Font::bold));
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

    drawLabel ("LIMIT",    tx, ty - 13, tw, 12);
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

    // Standard VU ticks
    struct Tick { float vu; const char* label; bool major; bool redZone; };
    const Tick ticks[] = {
        { -20.f, "-20", true,  false },
        { -10.f, "-10", true,  false },
        {  -7.f, "-7",  false, false },
        {  -5.f, "-5",  true,  false },
        {  -3.f, "-3",  false, false },
        {  -2.f, "-2",  false, false },
        {  -1.f, "-1",  false, false },
        {   0.f,  "0",  true,  true  },
        {   1.f, "+1",  false, true  },
        {   2.f, "+2",  false, true  },
        {   3.f, "+3",  true,  true  },
    };

    // Filled arc band in red zone (0..+3 VU)
    {
        const float a0 = juce::MathConstants<float>::pi / 3.f
                       * (2.f * (20.f / 23.f) - 1.f);   // 0 VU
        const float a1 = juce::MathConstants<float>::pi / 3.f;  // +3 VU

        juce::Path redBand;
        const float rb1 = SCALE_R_IN - 5.f;
        const float rb2 = SCALE_R_OUT + 1.f;
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
    for (const auto& t : ticks)
    {
        const float norm  = (t.vu + 20.f) / 23.f;
        const float angle = juce::MathConstants<float>::pi / 3.f * (2.f * norm - 1.f);
        const float sa    = std::sin (angle);
        const float ca    = std::cos (angle);
        const float r1    = t.major ? SCALE_R_IN - 5.f : SCALE_R_IN;

        const juce::Colour inkCol = t.redZone ? Col::meterRed : Col::meterPrint;

        g.setColour (inkCol.withAlpha (t.major ? 0.85f : 0.50f));
        g.drawLine (px + sa * r1,          py - ca * r1,
                    px + sa * SCALE_R_OUT, py - ca * SCALE_R_OUT,
                    t.major ? 1.5f : 0.8f);

        if (t.major)
        {
            const float lx = px + sa * LABEL_R;
            const float ly = py - ca * LABEL_R;
            g.setFont (juce::Font ("Arial", 8.f, juce::Font::bold));
            g.setColour (inkCol.withAlpha (0.9f));
            g.drawText (t.label,
                        juce::Rectangle<float> (lx - 12.f, ly - 7.f, 24.f, 14.f),
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
            const float ax = px + std::sin (a) * SCALE_R_OUT;
            const float ay = py - std::cos (a) * SCALE_R_OUT;
            if (i == 0) arc.startNewSubPath (ax, ay);
            else        arc.lineTo (ax, ay);
        }
        g.setColour (Col::meterPrint.withAlpha (0.4f));
        g.strokePath (arc, juce::PathStrokeType (0.7f));
    }

    // "VU" text at left and right scale ends
    {
        g.setFont (juce::Font ("Arial", 8.f, juce::Font::bold));
        // left VU (−20 end)
        const float lnorm  = 0.f;
        const float langle = -juce::MathConstants<float>::pi / 3.f;
        const float lsa    = std::sin (langle);
        const float lca    = std::cos (langle);
        const float lvux   = px + lsa * (LABEL_R - 8.f);
        const float lvuy   = py - lca * (LABEL_R - 8.f);
        g.setColour (Col::meterPrint.withAlpha (0.55f));
        g.drawText ("VU", juce::Rectangle<float> (lvux - 10.f, lvuy - 6.f, 20.f, 12.f),
                    juce::Justification::centred);
        // right VU (+3 end)
        const float rvuAngle = juce::MathConstants<float>::pi / 3.f;
        const float rvuX = px + std::sin (rvuAngle) * (LABEL_R - 8.f);
        const float rvuY = py - std::cos (rvuAngle) * (LABEL_R - 8.f);
        g.setColour (Col::meterRed.withAlpha (0.65f));
        g.drawText ("VU", juce::Rectangle<float> (rvuX - 10.f, rvuY - 6.f, 20.f, 12.f),
                    juce::Justification::centred);
        juce::ignoreUnused (lnorm);
    }

    // Center text block
    {
        g.setFont (juce::Font ("Arial", 7.5f, juce::Font::bold));
        g.setColour (Col::meterPrint.withAlpha (0.50f));
        g.drawText ("VU  LEVEL  INDICATOR",
                    VM_X, (int)(VM_PY + 2.f), VM_W, 10,
                    juce::Justification::centred);
    }

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

    // "MODEL LA-2A" sub-line
    g.setFont (juce::Font ("Arial", 8.f, juce::Font::plain));
    g.setColour (Col::labelMid.withAlpha (0.65f));
    g.drawText ("MODEL  LA-2A",
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
// Power LED  — toggle switch style with green jewel
// ─────────────────────────────────────────────────────────────────────────────

void CouchLA2AEditor::drawPowerLED (juce::Graphics& g) const
{
    const float cx = PLUGIN_W - EAR_W - 22.f;
    const float cy = PLUGIN_H * 0.50f;

    // "ON" label
    g.setFont (juce::Font ("Arial", 8.f, juce::Font::bold));
    g.setColour (Col::labelDark.withAlpha (0.7f));
    g.drawText ("ON", (int)(cx - 16.f), (int)(cy - 34.f), 32, 12,
                juce::Justification::centred);

    // LED housing
    {
        juce::ColourGradient hg (juce::Colour (0xff383830), cx - 9.f, cy - 8.f,
                                 juce::Colour (0xff181810), cx + 9.f, cy + 8.f, false);
        g.setGradientFill (hg);
        g.fillRoundedRectangle (cx - 9.f, cy - 8.f, 18.f, 16.f, 3.f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawRoundedRectangle (cx - 9.f, cy - 8.f, 18.f, 16.f, 3.f, 0.8f);
    }

    // Glow aura
    {
        juce::ColourGradient glow (Col::ledGlow, cx, cy,
                                   juce::Colours::transparentBlack, cx + 14.f, cy, true);
        g.setGradientFill (glow);
        g.fillEllipse (cx - 14.f, cy - 14.f, 28.f, 28.f);
    }

    // LED jewel
    {
        const float lr = 6.f;
        juce::ColourGradient lg (Col::ledGreen.brighter (0.3f), cx - lr * 0.35f, cy - lr * 0.4f,
                                 Col::ledGreen.darker  (0.4f), cx + lr,         cy + lr,       false);
        g.setGradientFill (lg);
        g.fillEllipse (cx - lr, cy - lr, lr * 2.f, lr * 2.f);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawEllipse (cx - lr, cy - lr, lr * 2.f, lr * 2.f, 0.8f);

        // Specular
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillEllipse (cx - lr * 0.42f, cy - lr * 0.52f, lr * 0.5f, lr * 0.38f);
    }

    // "POWER" label below
    g.setFont (juce::Font ("Arial", 7.5f, juce::Font::plain));
    g.setColour (Col::labelMid.withAlpha (0.55f));
    g.drawText ("POWER", (int)(cx - 18.f), (int)(cy + 11.f), 36, 10,
                juce::Justification::centred);
}

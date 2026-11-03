#include "SA_Panel.h"
#include "../../PluginProcessor.h"
#include <cmath>
#include <complex>
#include <map>

namespace emberui
{
using juce::Colour; using juce::Graphics; using juce::Rectangle;

// ---- palette: charred-brown cabinet, amber light, cream print. flat, quiet, warm.
static const Colour kBg1 (0xFF1E1813), kBg2 (0xFF14100C), kCard (0xFF261F18), kCardEdge (0xFF3A2F23), kInk (0xFF120E0A), kField (0xFF4A3D2E);
static const Colour kAmber (0xFFEFAA3C), kGold (0xFFD9A441), kCream (0xFFEFE4CD), kDim (0xFF9C9080);

static juce::String pickFace (std::initializer_list<const char*> candidates)
{
    const auto all = juce::Font::findAllTypefaceNames();
    for (auto* c : candidates) if (all.contains (c)) return c;
    return juce::Font::getDefaultSansSerifFontName();
}
static juce::Font lab (float h, bool bold = false, float kern = 0.02f)
{
    static const juce::String face = pickFace ({ "Segoe UI", "Arial" });
    return juce::Font (juce::FontOptions (face, h, bold ? juce::Font::bold : juce::Font::plain).withKerningFactor (kern));
}
static juce::String fmt (double v)
{
    const double a = std::fabs (v);
    if (a >= 1000.0) return juce::String (v / 1000.0, 2) + "k";
    if (a >= 100.0) return juce::String (v, 0);
    if (a >= 10.0) return juce::String (v, 1);
    return juce::String (v, 2);
}

// ====================================================================================== wood trim (built once)
static float hash2 (int x, int y)
{
    unsigned h = (unsigned) x * 374761393u + (unsigned) y * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float) ((h ^ (h >> 16)) & 0xFFFFu) / 65535.0f;
}
static float vnoise (float x, float y)
{
    const int xi = (int) std::floor (x), yi = (int) std::floor (y);
    float fx = x - (float) xi, fy = y - (float) yi;
    fx = fx * fx * (3.0f - 2.0f * fx); fy = fy * fy * (3.0f - 2.0f * fy);
    const float a = hash2 (xi, yi), b = hash2 (xi + 1, yi), c = hash2 (xi, yi + 1), d = hash2 (xi + 1, yi + 1);
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy;
}
static juce::Image makeWood (int w, int h, bool grainVertical, int seed)
{
    juce::Image img (juce::Image::RGB, w, h, false);
    juce::Image::BitmapData bd (img, juce::Image::BitmapData::writeOnly);
    const Colour dark (0xFF1C0804), mid (0xFF4A1A0E), light (0xFF7A3218);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            const float u = grainVertical ? (float) x : (float) y, v = grainVertical ? (float) y : (float) x;
            const float across = u / (float) (grainVertical ? w : h);
            const float warp = vnoise (u * 0.07f + (float) seed, v * 0.0035f) * 6.0f + vnoise (u * 0.25f + (float) seed * 3.0f, v * 0.012f) * 2.0f;
            const float rings = 0.5f + 0.5f * std::sin ((u * 0.75f + warp) * 1.5f);
            const float streak = vnoise (u * 2.4f + (float) seed, v * 0.05f);
            const float cathedral = vnoise (u * 0.12f + 9.0f, v * 0.002f + (float) seed);
            float t = juce::jlimit (0.0f, 1.0f, 0.42f * rings + 0.34f * streak + 0.30f * cathedral - 0.06f);
            Colour c = t < 0.5f ? dark.interpolatedWith (mid, t * 2.0f) : mid.interpolatedWith (light, (t - 0.5f) * 2.0f);
            if (hash2 (x * 3 + seed, y * 7) > 0.985f) c = c.darker (0.45f);
            const float edge = std::fabs (across * 2.0f - 1.0f);
            bd.setPixelColour (x, y, c.withMultipliedBrightness (1.0f - 0.28f * edge * edge));
        }
    return img;
}
struct Textures
{
    juce::Image cheekL, cheekR, railT, railB;
    static constexpr int kCheek = 14, kRail = 6;
    Textures()
    {
        cheekL = makeWood (kCheek, 720, true, 11);
        cheekR = makeWood (kCheek, 720, true, 47);
        railT = makeWood (1280 - 2 * kCheek, kRail, false, 23);
        railB = makeWood (1280 - 2 * kCheek, kRail, false, 71);
    }
    static const Textures& get() { static const Textures t; return t; }
};

// ====================================================================================== look and feel (flat, warm)
class EmberLook : public juce::LookAndFeel_V4
{
public:
    EmberLook()
    {
        setColour (juce::ComboBox::backgroundColourId, kInk); setColour (juce::ComboBox::textColourId, kCream);
        setColour (juce::ComboBox::outlineColourId, kField); setColour (juce::ComboBox::arrowColourId, kAmber);
        setColour (juce::PopupMenu::backgroundColourId, Colour (0xFF18130E)); setColour (juce::PopupMenu::textColourId, kCream);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, kAmber.withAlpha (0.28f)); setColour (juce::PopupMenu::highlightedTextColourId, Colour (0xFFFFFFFF));
        setColour (juce::Label::textColourId, kCream.withAlpha (0.88f));
        setColour (juce::TextButton::textColourOffId, kCream); setColour (juce::TextButton::textColourOnId, kAmber);
    }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return lab (12.0f); }
    juce::Font getPopupMenuFont() override { return lab (13.0f); }
    juce::Font getLabelFont (juce::Label& l) override
    {
        if (dynamic_cast<juce::ComboBox*> (l.getParentComponent()) != nullptr) return lab (12.0f);
        return l.getProperties().contains ("val") ? lab (10.0f) : lab (10.0f);
    }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return lab (11.5f, true, 0.06f); }

    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override
    {
        const float R = (float) std::min (w, h) * 0.5f, cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f;
        const float body = R * 0.74f, ang = a0 + pos * (a1 - a0);
        const auto round = juce::PathStrokeType::rounded; const auto curved = juce::PathStrokeType::curved;
        {   // thin light ring: dim track + amber value
            const float ra = R * 0.93f;
            juce::Path track, val; track.addCentredArc (cx, cy, ra, ra, 0.0f, a0, a1, true);
            g.setColour (Colour (0x30FFFFFF)); g.strokePath (track, juce::PathStrokeType (1.4f, curved, round));
            if (pos > 0.003f)
            {
                val.addCentredArc (cx, cy, ra, ra, 0.0f, a0, ang, true);
                g.setColour (kAmber.withAlpha (0.20f)); g.strokePath (val, juce::PathStrokeType (std::max (4.0f, R * 0.16f), curved, round));
                g.setColour (kAmber); g.strokePath (val, juce::PathStrokeType (std::max (1.8f, R * 0.07f), curved, round));
            }
        }
        g.setColour (Colour (0x70000000)); g.fillEllipse (cx - body, cy - body + 1.6f, body * 2, body * 2);
        g.setGradientFill (juce::ColourGradient (Colour (0xFF39312A), cx - body * 0.5f, cy - body * 0.7f, Colour (0xFF0F0D0B), cx + body * 0.6f, cy + body * 0.9f, false));
        g.fillEllipse (cx - body, cy - body, body * 2, body * 2);
        g.setColour (Colour (0xFF5F5346)); g.drawEllipse (cx - body, cy - body, body * 2, body * 2, 1.0f);
        g.setColour (Colour (0x1CFFFFFF)); g.drawEllipse (cx - body + 2.0f, cy - body + 2.0f, body * 2 - 4.0f, body * 2 - 4.0f, 0.8f);
        const float s = std::sin (ang), c = -std::cos (ang);
        g.setColour (kCream); g.drawLine (cx + s * body * 0.30f, cy + c * body * 0.30f, cx + s * body * 0.90f, cy + c * body * 0.90f, std::max (1.8f, body * 0.11f));
    }

    void drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle style, juce::Slider&) override
    {
        auto cap = [&] (Rectangle<float> c)
        {
            g.setColour (Colour (0x80000000)); g.fillRoundedRectangle (c.translated (0.5f, 1.5f), 2.5f);
            g.setGradientFill (juce::ColourGradient (Colour (0xFFD9C9A6), 0, c.getY(), Colour (0xFF9C8A68), 0, c.getBottom(), false)); g.fillRoundedRectangle (c, 2.5f);
            g.setColour (Colour (0xFF3B3022)); g.drawRoundedRectangle (c, 2.5f, 0.8f);
        };
        if (style == juce::Slider::LinearVertical)
        {
            const float cx = (float) x + (float) w * 0.5f;
            g.setColour (kInk); g.fillRoundedRectangle (cx - 2.0f, (float) y + 4, 4.0f, (float) h - 8, 2.0f);
            g.setColour (Colour (0x30FFFFFF)); g.drawRoundedRectangle (cx - 2.0f, (float) y + 4, 4.0f, (float) h - 8, 2.0f, 0.8f);
            g.setColour (kAmber.withAlpha (0.8f)); g.fillRoundedRectangle (cx - 1.0f, pos, 2.0f, (float) y + (float) h - 6 - pos, 1.0f);
            auto c = Rectangle<float> (cx - 9, pos - 6, 18, 12); cap (c);
            g.setColour (Colour (0xFF2A2014)); g.drawLine (c.getX() + 3, c.getCentreY(), c.getRight() - 3, c.getCentreY(), 1.4f);
        }
        else
        {
            const float cy = (float) y + (float) h * 0.5f, mid = (float) x + (float) w * 0.5f;
            g.setColour (kInk); g.fillRoundedRectangle ((float) x + 2, cy - 2.0f, (float) w - 4, 4.0f, 2.0f);
            g.setColour (Colour (0x30FFFFFF)); g.drawRoundedRectangle ((float) x + 2, cy - 2.0f, (float) w - 4, 4.0f, 2.0f, 0.8f);
            g.setColour (kAmber.withAlpha (0.8f)); g.fillRoundedRectangle (std::min (mid, pos), cy - 1.0f, std::fabs (pos - mid), 2.0f, 1.0f);
            g.setColour (kCream.withAlpha (0.35f)); g.drawLine (mid, cy - 6, mid, cy + 6, 1.0f);
            auto c = Rectangle<float> (pos - 5, cy - 8, 10, 16);
            g.setColour (Colour (0x80000000)); g.fillRoundedRectangle (c.translated (0.5f, 1.5f), 2.5f);
            g.setGradientFill (juce::ColourGradient (Colour (0xFFD9C9A6), c.getX(), 0, Colour (0xFF9C8A68), c.getRight(), 0, false)); g.fillRoundedRectangle (c, 2.5f);
            g.setColour (Colour (0xFF3B3022)); g.drawRoundedRectangle (c, 2.5f, 0.8f);
        }
    }

    void drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setColour (kInk); g.fillRoundedRectangle (r, 5.0f);
        g.setColour (box.isEnabled() ? kField : kField.withAlpha (0.4f)); g.drawRoundedRectangle (r, 5.0f, 1.0f);
        juce::Path p; const float ax = (float) w - 12, ay = (float) h * 0.5f;
        p.startNewSubPath (ax - 4.0f, ay - 2.0f); p.lineTo (ax, ay + 2.0f); p.lineTo (ax + 4.0f, ay - 2.0f);
        g.setColour (kAmber); g.strokePath (p, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override { label.setBounds (7, 0, box.getWidth() - 24, box.getHeight()); label.setFont (getComboBoxFont (box)); }

    void drawToggleButton (Graphics& g, juce::ToggleButton& b, bool hl, bool) override
    {
        auto r = b.getLocalBounds().toFloat();
        const bool on = b.getToggleState();
        const float d = 8.0f, cy = r.getCentreY(), cx = 8.0f;
        g.setColour (Colour (0xFF0A0806)); g.fillEllipse (cx - d * 0.5f - 2, cy - d * 0.5f - 2, d + 4, d + 4);
        g.setColour (kField); g.drawEllipse (cx - d * 0.5f - 2, cy - d * 0.5f - 2, d + 4, d + 4, 1.0f);
        if (on)
        {
            g.setColour (kAmber.withAlpha (0.25f)); g.fillEllipse (cx - d, cy - d, d * 2, d * 2);
            g.setColour (kAmber); g.fillEllipse (cx - d * 0.5f, cy - d * 0.5f, d, d);
        }
        else { g.setColour (Colour (0xFF3A2B1F)); g.fillEllipse (cx - d * 0.5f, cy - d * 0.5f, d, d); }
        g.setColour (hl ? kAmber : (on ? kCream : kCream.withAlpha (0.75f))); g.setFont (lab (11.0f, true, 0.05f));
        g.drawText (b.getButtonText(), (int) (cx + d + 6), 0, b.getWidth() - (int) (cx + d + 6), b.getHeight(), juce::Justification::centredLeft);
    }

    void drawButtonBackground (Graphics& g, juce::Button& b, const Colour&, bool hl, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        g.setColour (down ? Colour (0xFF18130E) : Colour (0xFF2C241B)); g.fillRoundedRectangle (r, 6.0f);
        if (on) { g.setColour (kAmber.withAlpha (0.16f)); g.fillRoundedRectangle (r, 6.0f); }
        g.setColour (hl || on ? kAmber : kField); g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }
    void drawPopupMenuBackground (Graphics& g, int w, int h) override { g.fillAll (Colour (0xFF18130E)); g.setColour (kField); g.drawRect (0, 0, w, h, 1); }
};

// ====================================================================================== controls
struct EKnob : juce::Component
{
    juce::Slider s { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Label name, val;
    std::unique_ptr<juce::SliderParameterAttachment> att;
    juce::RangedAudioParameter& param;
    EKnob (juce::RangedAudioParameter& p, const juce::String& n) : param (p)
    {
        addAndMakeVisible (s); addAndMakeVisible (name); addAndMakeVisible (val);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        s.setMouseDragSensitivity (220);
        att = std::make_unique<juce::SliderParameterAttachment> (p, s, nullptr);
        s.setDoubleClickReturnValue (true, p.convertFrom0to1 (p.getDefaultValue()));
        name.setText (n, juce::dontSendNotification); name.setJustificationType (juce::Justification::centred); name.setInterceptsMouseClicks (false, false);
        val.setJustificationType (juce::Justification::centred); val.setColour (juce::Label::textColourId, kCream.withAlpha (0.8f)); val.setInterceptsMouseClicks (false, false);
        s.onValueChange = [this] { val.setText (fmt (s.getValue()), juce::dontSendNotification); };
        s.onValueChange();
    }
    void resized() override { auto r = getLocalBounds(); name.setBounds (r.removeFromTop (12)); val.setBounds (r.removeFromBottom (12)); s.setBounds (r); }
};

struct EFader : juce::Component
{
    juce::Slider s { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
    juce::Label name, val;
    std::unique_ptr<juce::SliderParameterAttachment> att;
    EFader (juce::RangedAudioParameter& p, const juce::String& n)
    {
        addAndMakeVisible (s); addAndMakeVisible (name); addAndMakeVisible (val);
        att = std::make_unique<juce::SliderParameterAttachment> (p, s, nullptr);
        s.setDoubleClickReturnValue (true, p.convertFrom0to1 (p.getDefaultValue()));
        name.setText (n, juce::dontSendNotification); name.setJustificationType (juce::Justification::centred); name.setInterceptsMouseClicks (false, false);
        val.setJustificationType (juce::Justification::centred); val.setColour (juce::Label::textColourId, kCream.withAlpha (0.8f)); val.setInterceptsMouseClicks (false, false);
        s.onValueChange = [this] { val.setText (fmt (s.getValue()), juce::dontSendNotification); };
        s.onValueChange();
    }
    void resized() override { auto r = getLocalBounds(); name.setBounds (r.removeFromTop (12)); val.setBounds (r.removeFromBottom (12)); s.setBounds (r); }
};

struct EHSlider : juce::Slider
{
    std::unique_ptr<juce::SliderParameterAttachment> att;
    EHSlider (juce::RangedAudioParameter& p) : juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox)
    {
        att = std::make_unique<juce::SliderParameterAttachment> (p, *this, nullptr);
        setDoubleClickReturnValue (true, p.convertFrom0to1 (p.getDefaultValue()));
    }
};

struct ECombo : juce::ComboBox
{
    std::unique_ptr<juce::ComboBoxParameterAttachment> att;
    ECombo (juce::RangedAudioParameter& p)
    {
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (&p)) addItemList (c->choices, 1);
        att = std::make_unique<juce::ComboBoxParameterAttachment> (p, *this, nullptr);
        setJustificationType (juce::Justification::centredLeft);
    }
};

struct EToggle : juce::ToggleButton
{
    std::unique_ptr<juce::ButtonParameterAttachment> att;
    EToggle (juce::RangedAudioParameter& p, const juce::String& t) { setButtonText (t); att = std::make_unique<juce::ButtonParameterAttachment> (p, *this, nullptr); }
};

struct EStepRow : juce::Component
{
    DuoProcessor& proc;
    std::vector<juce::RangedAudioParameter*> steps;
    float paintValue = 1.0f;
    EStepRow (DuoProcessor& p) : proc (p) { for (int i = 0; i < 16; ++i) steps.push_back (p.paramA (sa::gs1 + i)); }
    int indexAt (int x) const { return std::max (0, std::min (15, x * 16 / std::max (1, getWidth()))); }
    void set (int i, float v) { steps[(size_t) i]->setValueNotifyingHost (v); repaint(); }
    void mouseDown (const juce::MouseEvent& e) override { const int i = indexAt (e.x); paintValue = steps[(size_t) i]->getValue() > 0.5f ? 0.0f : 1.0f; steps[(size_t) i]->beginChangeGesture(); set (i, paintValue); }
    void mouseDrag (const juce::MouseEvent& e) override { set (indexAt (e.x), paintValue); }
    void mouseUp (const juce::MouseEvent&) override { for (auto* s : steps) s->endChangeGesture(); }
    void paint (Graphics& g) override
    {
        const int cur = proc.engineA().gateStep();
        const float w = (float) getWidth() / 16.0f;
        const bool gateOn = proc.paramA (sa::gate_on)->getValue() > 0.5f;
        for (int i = 0; i < 16; ++i)
        {
            auto r = Rectangle<float> (w * (float) i + 2, 2, w - 4, (float) getHeight() - 16);
            const bool on = steps[(size_t) i]->getValue() > 0.5f;
            if (on) { g.setColour (kAmber.withAlpha (gateOn ? 1.0f : 0.5f)); g.fillRoundedRectangle (r, 3.0f); }
            else { g.setColour (kInk); g.fillRoundedRectangle (r, 3.0f); g.setColour (kField); g.drawRoundedRectangle (r, 3.0f, 1.0f); }
            if (gateOn && i == cur) { g.setColour (Colour (0xFFFFFFFF)); g.drawRoundedRectangle (r.expanded (1.0f), 3.5f, 1.5f); }
            g.setColour (kDim); g.setFont (lab (9.5f));
            g.drawText (juce::String (i + 1), r.toNearestInt().withY (getHeight() - 13).withHeight (12), juce::Justification::centred);
        }
    }
};

struct EScope : juce::Component
{
    DuoProcessor& proc; juce::Image trail;
    EScope (DuoProcessor& p) : proc (p) {}
    void tick()
    {
        if (getWidth() < 4) return;
        if (! trail.isValid() || trail.getWidth() != getWidth() || trail.getHeight() != getHeight()) trail = juce::Image (juce::Image::ARGB, getWidth(), getHeight(), true);
        float buf[512]; proc.engineA().readScope (buf, 512);
        int start = 0; for (int i = 1; i < 256; ++i) if (buf[i - 1] < 0 && buf[i] >= 0) { start = i; break; }
        trail.multiplyAllAlphas (0.72f);
        Graphics g (trail);
        juce::Path p; const int n = 256;
        for (int i = 0; i < n; ++i)
        {
            const float x = (float) i / (float) (n - 1) * (float) getWidth(), y = (float) getHeight() * 0.5f - buf[start + i] * (float) getHeight() * 1.5f;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (kAmber.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (4.0f));
        g.setColour (Colour (0xFFFFCB75)); g.strokePath (p, juce::PathStrokeType (1.4f));
        repaint();
    }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (kInk); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (kAmber.withAlpha (0.08f));
        for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (r.getHeight() * (float) i / 4.0f), 0, r.getRight());
        for (int i = 1; i < 10; ++i) g.drawVerticalLine ((int) (r.getWidth() * (float) i / 10.0f), 0, r.getBottom());
        if (trail.isValid()) { g.setOpacity (0.95f); g.drawImageAt (trail, 0, 0); }
        g.setColour (kField); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    }
};

struct EFilterCurve : juce::Component
{
    DuoProcessor& proc; int which;
    EFilterCurve (DuoProcessor& p, int f) : proc (p), which (f) {}
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (kInk); g.fillRoundedRectangle (r, 6.0f);
        const int b = which == 0 ? sa::f1_model : sa::f2_model;
        auto rv = [this] (int idx) { return proc.paramA (idx)->convertFrom0to1 (proc.paramA (idx)->getValue()); };
        const int model = (int) (rv (b) + 0.5f), type = (int) (rv (b + 1) + 0.5f);
        const float cut = rv (b + 2), res = rv (b + 3);
        juce::Path p, fill;
        const float W = r.getWidth();
        for (int i = 0; i < (int) W; ++i)
        {
            const float f = 20.0f * std::pow (1000.0f, (float) i / W), x = f / cut;
            std::complex<float> jx (0, x), one (1, 0), H;
            if (model == 0) { auto s = one + jx; H = one / (s * s * s * s + std::complex<float> (res * 4.0f, 0)); }
            else
            {
                const float k = 2.0f * (1.0f - res * 0.98f) + 0.01f; const auto den = one - std::complex<float> (x * x, 0) + jx * k;
                if (type == 0) H = one / den; else if (type == 1) H = std::complex<float> (-x * x, 0) / den; else if (type == 2) H = jx * k / den; else H = (one - std::complex<float> (x * x, 0)) / den;
            }
            const float db = juce::Decibels::gainToDecibels (std::abs (H), -60.0f);
            const float y = r.getBottom() - 4 - (db + 40.0f) / 60.0f * (r.getHeight() - 8);
            const float yy = juce::jlimit (2.0f, r.getBottom() - 2, y);
            if (i == 0) { p.startNewSubPath (0, yy); fill.startNewSubPath (0, r.getBottom()); fill.lineTo (0, yy); } else { p.lineTo ((float) i, yy); fill.lineTo ((float) i, yy); }
        }
        fill.lineTo (W, r.getBottom()); fill.closeSubPath();
        g.setGradientFill (juce::ColourGradient (kAmber.withAlpha (0.30f), 0, 0, kAmber.withAlpha (0.0f), 0, r.getBottom(), false)); g.fillPath (fill);
        g.setColour (kAmber); g.strokePath (p, juce::PathStrokeType (1.6f));
        g.setColour (kField); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    }
};

// preset field: click opens the category menu
struct ELcd : juce::Component
{
    DuoProcessor& proc;
    std::function<void()> onClick;
    ELcd (DuoProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (kInk); g.fillRoundedRectangle (r, 6.0f);
        g.setColour (kField); g.drawRoundedRectangle (r, 6.0f, 1.0f);
        const auto& pr = sa::presets()[(size_t) std::min (proc.presetIndexA(), (int) sa::presets().size() - 1)];
        g.setColour (kCream); g.setFont (lab (14.0f, true, 0.01f)); g.drawText (juce::String (pr.name), 14, 0, getWidth() - 50, getHeight(), juce::Justification::centredLeft);
        juce::Path p; const float ax = (float) getWidth() - 18, ay = (float) getHeight() * 0.5f;
        p.startNewSubPath (ax - 5.0f, ay - 2.5f); p.lineTo (ax, ay + 2.5f); p.lineTo (ax + 5.0f, ay - 2.5f);
        g.setColour (kAmber); g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
};
struct EInfo : juce::Component
{
    DuoProcessor& proc;
    EInfo (DuoProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }
    void paint (Graphics& g) override
    {
        const auto& pr = sa::presets()[(size_t) std::min (proc.presetIndexA(), (int) sa::presets().size() - 1)];
        g.setColour (kGold); g.setFont (lab (10.5f, true, 0.1f));
        g.drawText (juce::String (pr.category).toUpperCase() + "   " + juce::String (proc.presetIndexA() + 1) + " / " + juce::String ((int) sa::presets().size())
                    + "   VOICES " + juce::String (proc.engineA().activeVoices()), getLocalBounds(), juce::Justification::centredLeft);
    }
};

// ====================================================================================== the panel
struct EmberPanel::Impl
{
    struct Face : juce::Component
    {
        Impl& im;
        explicit Face (Impl& i) : im (i) {}
        void paint (Graphics& g) override { im.paintFace (g, getWidth(), getHeight()); }
    };

    DuoProcessor& proc;
    EmberLook look;
    EmberPanel& owner;
    Face face { *this };
    struct Section { Rectangle<int> r; juce::String title; };
    std::vector<Section> sections;
    std::vector<std::unique_ptr<juce::Component>> owned;
    EScope* scope = nullptr; ELcd* lcd = nullptr; EStepRow* steps = nullptr; EInfo* info = nullptr;
    std::vector<EFilterCurve*> curves;
    juce::TextButton prevB { juce::CharPointer_UTF8 ("\xE2\x80\xB9") }, nextB { juce::CharPointer_UTF8 ("\xE2\x80\xBA") }, learnB { "LEARN CHORD" };

    Impl (EmberPanel& o, DuoProcessor& p) : proc (p), owner (o) {}

    juce::RangedAudioParameter& P (int i) { return *proc.paramA (i); }
    template <class T, class... A> T* add (A&&... a) { auto c = std::make_unique<T> (std::forward<A> (a)...); auto* raw = c.get(); face.addAndMakeVisible (*c); owned.push_back (std::move (c)); return raw; }
    void knob (int p, const juce::String& n, int x, int y, int w = 52, int h = 64) { add<EKnob> (P (p), n)->setBounds (x, y, w, h); }
    void bigKnob (int p, const juce::String& n, int x, int y, int w = 84, int h = 100) { add<EKnob> (P (p), n)->setBounds (x, y, w, h); }
    void fader (int p, const juce::String& n, int x, int y, int h) { add<EFader> (P (p), n)->setBounds (x, y, 28, h); }
    void combo (int p, int x, int y, int w, int h = 22) { add<ECombo> (P (p))->setBounds (x, y, w, h); }
    void toggle (int p, const juce::String& t, int x, int y, int w = 60, int h = 20) { add<EToggle> (P (p), t)->setBounds (x, y, w, h); }
    void section (int x, int y, int w, int h, const juce::String& t) { sections.push_back ({ { x, y, w, h }, t }); }
    void marker (int x, int y, const juce::String& t) { sections.push_back ({ { x, y, 1, 1 }, t }); }

    void build()
    {
        owner.setLookAndFeel (&look);
        owner.addAndMakeVisible (face);

        // ---- header: logo, preset field, quick stats, the signature controls
        lcd = add<ELcd> (proc); lcd->setBounds (250, 12, 330, 32);
        lcd->onClick = [this] { showPresetMenu(); };
        face.addAndMakeVisible (prevB); face.addAndMakeVisible (nextB);
        prevB.setBounds (250, 52, 36, 26); nextB.setBounds (290, 52, 36, 26);
        prevB.onClick = [this] { proc.loadPresetA ((proc.presetIndexA() + (int) sa::presets().size() - 1) % (int) sa::presets().size()); lcd->repaint(); };
        nextB.onClick = [this] { proc.loadPresetA ((proc.presetIndexA() + 1) % (int) sa::presets().size()); lcd->repaint(); };
        info = add<EInfo> (proc); info->setBounds (338, 54, 250, 22);
        scope = add<EScope> (proc); scope->setBounds (604, 14, 300, 64);
        knob (sa::age, "Age", 930, 12, 56, 66); knob (sa::warmth, "Warmth", 992, 12, 56, 66);
        combo (sa::flavor, 1066, 44, 104); marker (1066, 28, "FLAVOR");
        knob (sa::master, "Master", 1196, 12, 60, 66);

        const int Y1 = 98, H1 = 210, Y2 = 316, H2 = 186, Y3 = 510, H3 = 90, Y4 = 608, H4 = 108;

        // ---- oscillators
        auto oscCommon = [&] (int x, int base, const juce::String& title)
        {
            section (x, Y1, 224, H1, title);
            combo (base, x + 12, Y1 + 28, 118);
            knob (base + 1, "Oct", x + 8, Y1 + 62); knob (base + 2, "Semi", x + 58, Y1 + 62); knob (base + 3, "Fine", x + 108, Y1 + 62);
            knob (base + 4, "PW", x + 158, Y1 + 62);
            knob (base + 5, "Level", x + 158, Y1 + 134);
        };
        oscCommon (8, sa::o1_wave, "OSCILLATOR 01");
        oscCommon (236, sa::o2_wave, "OSCILLATOR 02");
        toggle (sa::o2_sync, "SYNC", 246, Y1 + 142, 70); knob (sa::o2_ring, "Ring", 292, Y1 + 134, 48, 64); knob (sa::o2_fm, "FM", 344, Y1 + 134, 48, 64);
        section (464, Y1, 224, H1, "OSC 3  /  SUB  /  NOISE");
        combo (sa::o3_wave, 474, Y1 + 28, 100); knob (sa::o3_oct, "Oct", 472, Y1 + 56, 48, 62); knob (sa::o3_semi, "Semi", 520, Y1 + 56, 48, 62); knob (sa::o3_fine, "Fine", 568, Y1 + 56, 48, 62); knob (sa::o3_lvl, "Level", 628, Y1 + 56, 52, 62);
        combo (sa::sub_wave, 474, Y1 + 128, 64); combo (sa::sub_oct, 542, Y1 + 128, 64); knob (sa::sub_lvl, "Sub", 622, Y1 + 126, 52, 66);
        combo (sa::noise_col, 474, Y1 + 160, 64); knob (sa::noise_lvl, "Noise", 542, Y1 + 144, 52, 62);
        section (692, Y1, 130, H1, "UNISON");
        knob (sa::uni_voices, "Voices", 698, Y1 + 30); knob (sa::uni_detune, "Detune", 756, Y1 + 30);
        knob (sa::uni_spread, "Spread", 698, Y1 + 112); knob (sa::uni_blend, "Blend", 756, Y1 + 112);
        auto filt = [&] (int x, int w, int b, const juce::String& title, int curveIdx)
        {
            section (x, Y1, w, H1, title);
            combo (b, x + 12, Y1 + 28, 126); combo (b + 1, x + 144, Y1 + 28, 66);
            bigKnob (b + 2, "Cutoff", x + 4, Y1 + 56);
            knob (b + 3, "Reso", x + 90, Y1 + 56, 44, 64); knob (b + 4, "Drive", x + 134, Y1 + 56, 44, 64); knob (b + 7, "Vel", x + 178, Y1 + 56, 44, 64);
            knob (b + 5, "Key", x + 90, Y1 + 126, 44, 64); knob (b + 6, "Env", x + 134, Y1 + 126, 44, 64);
            auto* c = add<EFilterCurve> (proc, curveIdx); curves.push_back (c); c->setBounds (x + 12, Y1 + 158, 70, 44);
        };
        filt (826, 222, sa::f1_model, "01  FILTER", 0);
        filt (1050, 222, sa::f2_model, "02  FILTER", 1);
        combo (sa::f_route, 1156, Y1 + 5, 110, 18);

        // ---- envelopes / lfo / matrix / macros
        section (8, Y2, 372, H2, "ENVELOPES");
        const char* en[3] = { "AMP", "FILTER", "FREE" }; const int eb[3] = { sa::e1_a, sa::e2_a, sa::e3_a };
        for (int e = 0; e < 3; ++e)
        {
            const int ex = 18 + e * 120;
            const char* nm[4] = { "A", "D", "S", "R" };
            for (int k = 0; k < 4; ++k) fader (eb[e] + k, nm[k], ex + k * 28, Y2 + 44, 134);
            marker (ex, Y2 + 28, en[e]);
        }
        section (388, Y2, 252, H2, "LFO 1 / 2 / 3");
        for (int l = 0; l < 3; ++l)
        {
            const int ly = Y2 + 26 + l * 52, b = sa::l1_shape + 3 * l;
            combo (b, 398, ly + 2, 100, 21); combo (b + 2, 398, ly + 26, 100, 21); knob (b + 1, "Rate", 520, ly + 0, 56, 52);
            marker (590, ly + 14, "LFO " + juce::String (l + 1));
        }
        section (648, Y2, 356, H2, "MODULATION  /  8 ROUTES");
        for (int s = 0; s < 8; ++s)
        {
            const int ry = Y2 + 28 + s * 19, b = sa::m1_src + 3 * s;
            combo (b, 658, ry, 110, 18); combo (b + 1, 772, ry, 110, 18);
            auto* sl = add<EHSlider> (P (b + 2)); sl->setBounds (890, ry - 1, 106, 20);
        }
        section (1012, Y2, 260, H2, "MACROS");
        for (int m = 0; m < 4; ++m) knob (sa::mac1 + m, "Macro " + juce::String (m + 1), 1030 + (m % 2) * 116, Y2 + 28 + (m / 2) * 76, 86, 72);

        // ---- arp / gate / play
        section (8, Y3, 366, H3, "ARPEGGIATOR");
        toggle (sa::arp_on, "ARP", 14, Y3 + 26, 60); toggle (sa::arp_hold, "HOLD", 14, Y3 + 54, 64);
        combo (sa::arp_mode, 80, Y3 + 26, 104); combo (sa::arp_rate, 80, Y3 + 54, 64);
        knob (sa::arp_oct, "Oct", 196, Y3 + 20, 52, 66); knob (sa::arp_gate, "Gate", 252, Y3 + 20, 52, 66); knob (sa::arp_swing, "Swing", 308, Y3 + 20, 52, 66);
        section (382, Y3, 500, H3, "TRANCE GATE");
        toggle (sa::gate_on, "GATE", 388, Y3 + 26, 62); combo (sa::gate_rate, 388, Y3 + 54, 62);
        knob (sa::gate_depth, "Depth", 454, Y3 + 20, 52, 66); knob (sa::gate_smooth, "Smooth", 508, Y3 + 20, 52, 66);
        steps = add<EStepRow> (proc); steps->setBounds (572, Y3 + 26, 302, 58);
        section (890, Y3, 382, H3, "PLAY");
        combo (sa::play_mode, 900, Y3 + 28, 118); toggle (sa::chord_on, "CHORD STAB", 900, Y3 + 58, 118);
        face.addAndMakeVisible (learnB); learnB.setBounds (1028, Y3 + 42, 112, 24);
        learnB.onClick = [this] { proc.learnChordA(); learnB.setButtonText (proc.chordTextA().isEmpty() ? "HOLD 2+ KEYS" : "LEARNED"); };
        knob (sa::glide, "Glide", 1146, Y3 + 20, 56, 66); knob (sa::bend_range, "Bend", 1206, Y3 + 20, 56, 66);
        // ---- effects strip
        const int fw[7] = { 160, 188, 212, 200, 170, 138, 148 };
        int fx = 8;
        auto fxCard = [&] (int idx) { section (fx, Y4, fw[idx], H4, ""); const int x = fx; fx += fw[idx] + 8; return x; };
        const int ky = Y4 + 32, kh = 70;
        { const int x = fxCard (0); toggle (sa::fx_drv_on, "DRIVE", x + 8, Y4 + 6, 70); combo (sa::fx_drv_type, x + 84, Y4 + 6, 68, 20); knob (sa::fx_drv_amt, "Amount", x + 8, ky, 56, kh); }
        { const int x = fxCard (1); toggle (sa::fx_ch_on, "CHORUS", x + 8, Y4 + 6, 78); combo (sa::fx_ch_mode, x + 96, Y4 + 6, 84, 20);
          knob (sa::fx_ch_rate, "Rate", x + 6, ky, 56, kh); knob (sa::fx_ch_depth, "Depth", x + 66, ky, 56, kh); knob (sa::fx_ch_mix, "Mix", x + 126, ky, 56, kh); }
        { const int x = fxCard (2); toggle (sa::fx_ph_on, "PHASER", x + 8, Y4 + 6, 90);
          knob (sa::fx_ph_rate, "Rate", x + 4, ky, 50, kh); knob (sa::fx_ph_depth, "Depth", x + 56, ky, 50, kh); knob (sa::fx_ph_fb, "Fdbk", x + 108, ky, 50, kh); knob (sa::fx_ph_mix, "Mix", x + 160, ky, 50, kh); }
        { const int x = fxCard (3); toggle (sa::fx_dl_on, "DELAY", x + 8, Y4 + 6, 62); combo (sa::fx_dl_time, x + 76, Y4 + 6, 56, 20); combo (sa::fx_dl_pp, x + 136, Y4 + 6, 56, 20);
          knob (sa::fx_dl_fb, "Fdbk", x + 8, ky, 56, kh); knob (sa::fx_dl_tone, "Tone", x + 70, ky, 56, kh); knob (sa::fx_dl_mix, "Mix", x + 132, ky, 56, kh); }
        { const int x = fxCard (4); toggle (sa::fx_rv_on, "REVERB", x + 8, Y4 + 6, 90);
          knob (sa::fx_rv_size, "Size", x + 4, ky, 54, kh); knob (sa::fx_rv_damp, "Damp", x + 60, ky, 54, kh); knob (sa::fx_rv_mix, "Mix", x + 116, ky, 50, kh); }
        { const int x = fxCard (5); toggle (sa::fx_eq_on, "EQ", x + 8, Y4 + 6, 60); knob (sa::fx_eq_lo, "Low", x + 8, ky, 58, kh); knob (sa::fx_eq_hi, "High", x + 70, ky, 58, kh); }
        { const int x = fxCard (6); toggle (sa::fx_pump_on, "PUMP", x + 8, Y4 + 6, 60); combo (sa::fx_pump_rate, x + 74, Y4 + 6, 66, 20); knob (sa::fx_pump_depth, "Depth", x + 8, ky, 62, kh); knob (sa::fx_pump_rel, "Release", x + 74, ky, 62, kh); }
    }

    void showPresetMenu()
    {
        juce::PopupMenu menu;
        std::map<std::string, juce::PopupMenu> cats;
        std::vector<std::string> order;
        const auto& all = sa::presets();
        for (size_t i = 0; i < all.size(); ++i)
        {
            if (! cats.count (all[i].category)) order.push_back (all[i].category);
            cats[all[i].category].addItem ((int) i + 1, all[i].name, true, (int) i == proc.presetIndexA());
        }
        for (auto& c : order) menu.addSubMenu (c, cats[c]);
        menu.setLookAndFeel (&look);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (lcd), [this] (int r) { if (r > 0) { proc.loadPresetA (r - 1); lcd->repaint(); } });
    }

    void paintFace (Graphics& g, int w, int h)
    {
        g.setGradientFill (juce::ColourGradient (kBg1, 0, 0, kBg2, 0, (float) h, false)); g.fillAll();
        // wordmark
        g.setColour (kCream); g.setFont (lab (38.0f, true, 0.02f)); g.drawText ("EMBER", 18, 6, 220, 44, juce::Justification::centredLeft, false);
        g.setColour (kGold); g.setFont (lab (10.5f, true, 0.12f)); g.drawText ("V A  /  ANALOG INSTRUMENT", 20, 52, 220, 14, juce::Justification::centredLeft, false);
        g.setColour (kDim); g.setFont (lab (9.5f, false, 0.04f)); g.drawText ("Moog-style ladder  -  drift  -  warmth", 20, 66, 220, 12, juce::Justification::centredLeft, false);
        for (auto& s : sections)
        {
            if (s.r.getWidth() > 4)
            {
                auto r = s.r.toFloat();
                g.setColour (Colour (0x66000000)); g.fillRoundedRectangle (r.translated (0, 2.0f), 9.0f);
                g.setGradientFill (juce::ColourGradient (Colour (0xFF2A2219), 0, r.getY(), Colour (0xFF221B14), 0, r.getBottom(), false)); g.fillRoundedRectangle (r, 9.0f);
                g.setColour (kCardEdge); g.drawRoundedRectangle (r.reduced (0.5f), 9.0f, 1.0f);
                if (s.title.isNotEmpty())
                {
                    g.setColour (kGold); g.setFont (lab (11.0f, true, 0.1f));
                    g.drawText (s.title.toUpperCase().replace (" ", juce::String::fromUTF8 ("\xC2\xA0")), (int) r.getX() + 14, (int) r.getY() + 7, (int) r.getWidth() - 28, 14, juce::Justification::centredLeft, false);
                }
            }
            else
            {
                g.setColour (kDim); g.setFont (lab (9.5f, true, 0.1f));
                g.drawText (s.title, s.r.getX(), s.r.getY(), 90, 12, juce::Justification::centredLeft, false);
            }
        }
        (void) w;
    }

    // ---- slim wooden trim around the face (real space 1280 x 720)
    void paintFrame (Graphics& g, int w, int h)
    {
        const auto& t = Textures::get();
        g.fillAll (Colour (0xFF1A0C06));
        const int c = Textures::kCheek, rl = Textures::kRail;
        g.drawImage (t.cheekL, 0, 0, c, h, 0, 0, t.cheekL.getWidth(), t.cheekL.getHeight());
        g.drawImage (t.cheekR, w - c, 0, c, h, 0, 0, t.cheekR.getWidth(), t.cheekR.getHeight());
        g.drawImage (t.railT, c, 0, w - 2 * c, rl, 0, 0, t.railT.getWidth(), t.railT.getHeight());
        g.drawImage (t.railB, c, h - rl, w - 2 * c, rl, 0, 0, t.railB.getWidth(), t.railB.getHeight());
    }
    void paintFrameOver (Graphics& g, int w, int h)
    {
        const float c = (float) Textures::kCheek, rl = (float) Textures::kRail;
        const float fx0 = c, fx1 = (float) w - c, fy0 = rl, fy1 = (float) h - rl;
        g.setGradientFill (juce::ColourGradient (Colour (0x99000000), fx0, 0, Colour (0x00000000), fx0 + 8, 0, false)); g.fillRect (fx0, fy0, 8.0f, fy1 - fy0);
        g.setGradientFill (juce::ColourGradient (Colour (0x99000000), fx1, 0, Colour (0x00000000), fx1 - 8, 0, false)); g.fillRect (fx1 - 8, fy0, 8.0f, fy1 - fy0);
        g.setColour (Colour (0xFF0A0503)); g.drawRect (fx0 - 0.5f, fy0 - 0.5f, fx1 - fx0 + 1.0f, fy1 - fy0 + 1.0f, 1.0f);
        g.setColour (Colour (0x40FFD9A0)); g.drawLine (0.5f, 0.0f, 0.5f, (float) h, 1.0f); g.drawLine (1.0f, 0.5f, (float) w - 1.0f, 0.5f, 1.0f);
        for (float x0 : { 0.0f, (float) w - c })
        { g.setGradientFill (juce::ColourGradient (Colour (0x30FFFFFF), x0, 0, Colour (0x00FFFFFF), x0 + c, (float) h * 0.7f, false)); g.fillRect (x0, 0.0f, c, (float) h); }
    }
};

EmberPanel::EmberPanel (DuoProcessor& p) : impl (std::make_unique<Impl> (*this, p))
{
    setSize (kW, kH);
    impl->build();
    resized();
}
EmberPanel::~EmberPanel() { stopTimer(); setLookAndFeel (nullptr); }
void EmberPanel::paint (Graphics& g) { impl->paintFrame (g, getWidth(), getHeight()); }
void EmberPanel::paintOverChildren (Graphics& g) { impl->paintFrameOver (g, getWidth(), getHeight()); }
void EmberPanel::resized()
{
    // the 1280-wide face is scaled to sit between the wooden cheeks
    const float s = (float) (getWidth() - 2 * Textures::kCheek) / 1280.0f;
    impl->face.setBounds (0, 0, 1280, (int) std::ceil ((float) (getHeight() - 2 * Textures::kRail) / s));
    impl->face.setTransform (juce::AffineTransform::scale (s).translated ((float) Textures::kCheek, (float) Textures::kRail));
}
void EmberPanel::visibilityChanged() { if (isVisible()) startTimerHz (30); else stopTimer(); }
void EmberPanel::timerCallback()
{
    impl->scope->tick();
    impl->lcd->repaint();
    impl->info->repaint();
    impl->steps->repaint();
    for (auto* c : impl->curves) c->repaint();
}
} // namespace emberui

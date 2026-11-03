#include "SB_Panel.h"
#include "../../PluginProcessor.h"
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <map>
#include <complex>

namespace prismui
{
using juce::Colour; using juce::Graphics; using juce::Rectangle;

static const Colour kBase (0xFF0A1022), kSurface (0xFF101632), kGlass (0xFF1A2250), kIce (0xFFEAF2FF), kAqua (0xFF4FD6F0), kViolet (0xFF8F7BFF), kMagenta (0xFFFF6AD5), kMint (0xFF6CF2B8);

static juce::Font sans (float h, bool bold = false) { return juce::Font (juce::FontOptions ("Segoe UI", h, bold ? juce::Font::bold : juce::Font::plain)); }
static juce::String fmt (double v)
{
    const double a = std::fabs (v);
    if (a >= 1000.0) return juce::String (v / 1000.0, 2) + "k";
    if (a >= 100.0) return juce::String (v, 0);
    if (a >= 10.0) return juce::String (v, 1);
    return juce::String (v, 2);
}
static Colour srcColour (int src)
{
    using namespace sb;
    switch (src)
    {
        case S_ENV1: return kAqua; case S_ENV2: return kMint; case S_ENV3: return Colour (0xFF7FD6FF);
        case S_LFO1: return kViolet; case S_LFO2: return kMagenta; case S_LFO3: return Colour (0xFFFFB86C); case S_LFO4: return Colour (0xFFFF7A7A);
        default: return src >= S_MAC1 && src <= S_MAC8 ? Colour (0xFFFFD166) : Colour (0xFF9FB0D6);
    }
}
static const char* srcName (int s)
{
    static const char* n[] = { "-", "LFO1", "LFO2", "LFO3", "LFO4", "ENV1", "ENV2", "ENV3", "M1", "M2", "M3", "M4", "M5", "M6", "M7", "M8", "VEL", "KEY", "MOD", "AT", "RND", "BEND" };
    return n[s];
}

// ====================================================================================== helpers
static juce::String pickFace (std::initializer_list<const char*> candidates)
{
    const auto all = juce::Font::findAllTypefaceNames();
    for (auto* c : candidates) if (all.contains (c)) return c;
    return juce::Font::getDefaultSansSerifFontName();
}
static juce::Font tech (float h, bool bold = true, float kern = 0.1f)
{
    static const juce::String face = pickFace ({ "Bahnschrift", "Segoe UI", "Arial" });
    return juce::Font (juce::FontOptions (face, h, bold ? juce::Font::bold : juce::Font::plain).withKerningFactor (kern));
}
static juce::Font sansK (float h, bool bold, float kern)
{
    return juce::Font (juce::FontOptions ("Segoe UI", h, bold ? juce::Font::bold : juce::Font::plain).withKerningFactor (kern));
}
static const Colour kEdge (0xFF27345F), kField (0xFF0C1330), kFieldEdge (0xFF2C3B6B);

// rounded panel outline (the name is historic; c is the corner size)
static juce::Path facetShape (Rectangle<float> r, float c)
{
    juce::Path p; p.addRoundedRectangle (r, std::min (c, 12.0f)); return p;
}
static juce::ColourGradient prismGrad (float x0, float y0, float x1, float y1, float a = 1.0f)
{
    juce::ColourGradient g (kAqua.withAlpha (a), x0, y0, kViolet.withAlpha (a), x1, y1, false);
    return g;
}
static Colour spectrumAt (float t, float a = 1.0f)      // 0 = aqua, 0.5 = violet, 1 = magenta
{
    t = juce::jlimit (0.0f, 1.0f, t);
    return (t < 0.5f ? kAqua.interpolatedWith (kViolet, t * 2.0f) : kViolet.interpolatedWith (kMagenta, (t - 0.5f) * 2.0f)).withAlpha (a);
}

// ====================================================================================== look and feel (flat, clean, luminous)
class PrismLook : public juce::LookAndFeel_V4
{
public:
    PrismLook()
    {
        setColour (juce::ComboBox::backgroundColourId, kField); setColour (juce::ComboBox::textColourId, kIce); setColour (juce::ComboBox::outlineColourId, kFieldEdge);
        setColour (juce::ComboBox::arrowColourId, kAqua);
        setColour (juce::PopupMenu::backgroundColourId, Colour (0xFF0E1530)); setColour (juce::PopupMenu::textColourId, kIce);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, kViolet.withAlpha (0.4f)); setColour (juce::PopupMenu::highlightedTextColourId, Colours_white());
        setColour (juce::Label::textColourId, kIce.withAlpha (0.8f));
        setColour (juce::TextButton::textColourOffId, kIce.withAlpha (0.9f)); setColour (juce::TextButton::textColourOnId, Colours_white());
        setColour (juce::TextEditor::backgroundColourId, kField); setColour (juce::TextEditor::textColourId, kIce); setColour (juce::TextEditor::outlineColourId, kFieldEdge);
        setColour (juce::TextEditor::focusedOutlineColourId, kAqua); setColour (juce::CaretComponent::caretColourId, kAqua);
    }
    static Colour Colours_white() { return Colour (0xFFFFFFFF); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return sans (12.0f); }
    juce::Font getPopupMenuFont() override { return sans (13.0f); }
    juce::Font getLabelFont (juce::Label& l) override { return l.getProperties().contains ("caps") ? sansK (10.0f, false, 0.01f) : (l.getProperties().contains ("own") ? l.getFont() : sans (11.0f)); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return sansK (12.0f, true, 0.03f); }

    void drawLabel (Graphics& g, juce::Label& l) override
    {
        g.setFont (getLabelFont (l)); g.setColour (l.findColour (juce::Label::textColourId));
        g.drawText (l.getText(), l.getLocalBounds(), l.getJustificationType(), true);
    }

    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override
    {
        const float d = (float) std::min (w, h) - 8.0f, cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f, r = d * 0.5f;
        const float ang = a0 + pos * (a1 - a0);
        const auto round = juce::PathStrokeType::rounded; const auto curved = juce::PathStrokeType::curved;
        juce::Path track, val;
        track.addCentredArc (cx, cy, r, r, 0, a0, a1, true);
        g.setColour (Colour (0x26FFFFFF)); g.strokePath (track, juce::PathStrokeType (2.2f, curved, round));
        if (pos > 0.003f)
        {
            val.addCentredArc (cx, cy, r, r, 0, a0, ang, true);
            g.setGradientFill (prismGrad (cx - r, cy, cx + r, cy, 0.20f)); g.strokePath (val, juce::PathStrokeType (6.5f, curved, round));
            g.setGradientFill (prismGrad (cx - r, cy, cx + r, cy)); g.strokePath (val, juce::PathStrokeType (2.6f, curved, round));
        }
        const float ri = r - 5.0f;
        if (ri > 4.0f)
        {
            g.setGradientFill (juce::ColourGradient (Colour (0xFF1D2850), cx, cy - ri, Colour (0xFF0D1330), cx, cy + ri, false)); g.fillEllipse (cx - ri, cy - ri, ri * 2, ri * 2);
            g.setColour (Colour (0x22FFFFFF)); g.drawEllipse (cx - ri, cy - ri, ri * 2, ri * 2, 1.0f);
        }
        const float s = std::sin (ang), c = -std::cos (ang);
        g.setColour (Colours_white()); g.drawLine (cx + s * ri * 0.35f, cy + c * ri * 0.35f, cx + s * (r + 0.5f), cy + c * (r + 0.5f), 1.8f);
        g.setColour (kAqua.withAlpha (0.35f)); g.fillEllipse (cx + s * r - 4.0f, cy + c * r - 4.0f, 8.0f, 8.0f);
        g.setColour (Colours_white()); g.fillEllipse (cx + s * r - 2.4f, cy + c * r - 2.4f, 4.8f, 4.8f);
    }
    void drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        if (style == juce::Slider::LinearVertical)
        {
            const float cx = (float) x + (float) w * 0.5f;
            g.setColour (Colour (0x26FFFFFF)); g.fillRoundedRectangle (cx - 1.5f, (float) y, 3.0f, (float) h, 1.5f);
            g.setGradientFill (juce::ColourGradient (kAqua, 0, pos, kViolet, 0, (float) (y + h), false)); g.fillRoundedRectangle (cx - 1.5f, pos, 3.0f, (float) (y + h) - pos, 1.5f);
            g.setColour (kAqua.withAlpha (0.25f)); g.fillEllipse (cx - 9, pos - 9, 18, 18);
            g.setColour (kIce); g.fillRoundedRectangle (cx - 7.0f, pos - 3.0f, 14.0f, 6.0f, 3.0f);
            return;
        }
        const float cy = (float) y + (float) h * 0.5f;
        g.setColour (Colour (0x26FFFFFF)); g.fillRoundedRectangle ((float) x, cy - 1.5f, (float) w, 3.0f, 1.5f);
        const bool bip = s.getMinimum() < 0 && s.getMaximum() > 0;
        const float x0 = bip ? (float) x + (float) w * 0.5f : (float) x;
        g.setGradientFill (prismGrad ((float) x, cy, (float) (x + w), cy)); g.fillRoundedRectangle (std::min (x0, pos), cy - 1.5f, std::fabs (pos - x0), 3.0f, 1.5f);
        g.setColour (kAqua.withAlpha (0.25f)); g.fillEllipse (pos - 8, cy - 8, 16, 16);
        g.setColour (kIce); g.fillEllipse (pos - 4.5f, cy - 4.5f, 9, 9);
    }
    void drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setColour (kField); g.fillRoundedRectangle (r, 7.0f);
        g.setColour (kFieldEdge); g.drawRoundedRectangle (r, 7.0f, 1.0f);
        juce::Path p; const float ax = (float) w - 13, ay = (float) h * 0.5f;
        p.startNewSubPath (ax - 4.0f, ay - 2.0f); p.lineTo (ax, ay + 2.0f); p.lineTo (ax + 4.0f, ay - 2.0f);
        g.setColour (kAqua); g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override { label.setBounds (9, 0, box.getWidth() - 26, box.getHeight()); label.setFont (getComboBoxFont (box)); }
    void drawToggleButton (Graphics& g, juce::ToggleButton& b, bool hl, bool) override
    {
        auto r = b.getLocalBounds().toFloat();
        const bool on = b.getToggleState();
        const float tw = 28, th = 15, ty = r.getCentreY() - th / 2;
        if (on)
        {
            g.setColour (kAqua.withAlpha (0.18f)); g.fillRoundedRectangle (-1, ty - 3, tw + 6, th + 6, (th + 6) / 2);
            g.setGradientFill (prismGrad (2, ty, 2 + tw, ty)); g.fillRoundedRectangle (2, ty, tw, th, th / 2);
        }
        else { g.setColour (Colour (0x26FFFFFF)); g.fillRoundedRectangle (2, ty, tw, th, th / 2); }
        g.setColour (on ? Colour (0xFFFFFFFF) : kIce.withAlpha (0.5f)); g.fillEllipse (on ? 2 + tw - th + 2 : 4.0f, ty + 2, th - 4, th - 4);
        g.setColour (hl ? kIce : kIce.withAlpha (0.82f)); g.setFont (sans (11.0f));
        g.drawText (b.getButtonText(), (int) tw + 8, 0, b.getWidth() - (int) tw - 8, b.getHeight(), juce::Justification::centredLeft);
    }
    void drawButtonBackground (Graphics& g, juce::Button& b, const Colour&, bool hl, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState(), accent = b.getProperties().contains ("accent"), field = b.getProperties().contains ("preset");
        const float rad = field ? 7.0f : std::min (8.0f, r.getHeight() * 0.5f);
        if (field) g.setColour (kField);
        else if (accent) g.setGradientFill (prismGrad (r.getX(), r.getY(), r.getRight(), r.getBottom(), down ? 0.9f : 0.62f));
        else if (on) g.setColour (Colour (0xFF39508F));
        else if (down) g.setColour (kViolet.withAlpha (0.5f));
        else g.setColour (hl ? Colour (0xFF22305F) : Colour (0xFF1A2548));
        g.fillRoundedRectangle (r, rad);
        g.setColour (field ? kFieldEdge : (hl || on || accent ? kAqua.withAlpha (0.85f) : kEdge)); g.drawRoundedRectangle (r, rad, 1.0f);
    }
    void drawButtonText (Graphics& g, juce::TextButton& b, bool, bool) override
    {
        if (b.getProperties().contains ("preset"))
        {
            const auto t = b.getButtonText(); auto r = b.getLocalBounds().reduced (12, 0);
            const auto name = t.upToFirstOccurrenceOf ("   -   ", false, false), cat = t.fromFirstOccurrenceOf ("   -   ", false, false);
            g.setColour (kIce); g.setFont (sans (13.5f, true)); g.drawText (name, r.withTrimmedRight (60), juce::Justification::centredLeft);
            g.setColour (kAqua.withAlpha (0.85f)); g.setFont (sansK (9.5f, true, 0.12f)); g.drawText (cat.toUpperCase(), r.withTrimmedRight (22), juce::Justification::centredRight);
            juce::Path p; const float ax = (float) b.getWidth() - 14, ay = (float) b.getHeight() * 0.5f;
            p.startNewSubPath (ax - 4.0f, ay - 2.0f); p.lineTo (ax, ay + 2.0f); p.lineTo (ax + 4.0f, ay - 2.0f);
            g.setColour (kAqua); g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            return;
        }
        g.setFont (getTextButtonFont (b, b.getHeight())); g.setColour (b.findColour (b.getToggleState() ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId));
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, false);
    }
    void drawPopupMenuBackground (Graphics& g, int w, int h) override { g.fillAll (Colour (0xFF0C1228)); g.setColour (kFieldEdge); g.drawRect (0, 0, w, h, 1); }
};

// ====================================================================================== controls
struct ModKnob;
struct Host
{
    DuoProcessor* proc = nullptr;
    std::function<void (int src, int dest)> assign;
};

struct PKnob : juce::Component, juce::DragAndDropTarget
{
    juce::Slider s { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Label name, val;
    std::unique_ptr<juce::SliderParameterAttachment> att;
    juce::RangedAudioParameter& param;
    DuoProcessor& proc; Host& host; const int pidx, dest; bool dropHover = false;
    PKnob (DuoProcessor& pr, Host& h, int paramIndex, const juce::String& n) : param (*pr.paramB (paramIndex)), proc (pr), host (h), pidx (paramIndex), dest (sb::destForParam (paramIndex))
    {
        addAndMakeVisible (s); addAndMakeVisible (name); addAndMakeVisible (val);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        s.setMouseDragSensitivity (220);
        att = std::make_unique<juce::SliderParameterAttachment> (param, s, nullptr);
        s.setDoubleClickReturnValue (true, param.convertFrom0to1 (param.getDefaultValue()));
        name.setText (n, juce::dontSendNotification); name.setJustificationType (juce::Justification::centred); name.setInterceptsMouseClicks (false, false);
        name.getProperties().set ("caps", true);
        val.setJustificationType (juce::Justification::centred); val.setColour (juce::Label::textColourId, kAqua.withAlpha (0.85f)); val.setInterceptsMouseClicks (false, false);
        s.onValueChange = [this] { val.setText (fmt (s.getValue()), juce::dontSendNotification); };
        s.onValueChange();
    }
    void resized() override { auto r = getLocalBounds(); name.setBounds (r.removeFromTop (12)); val.setBounds (r.removeFromBottom (12)); s.setBounds (r); }
    bool isInterestedInDragSource (const SourceDetails& d) override { return dest != 0 && d.description.isInt(); }
    void itemDragEnter (const SourceDetails&) override { dropHover = true; repaint(); }
    void itemDragExit (const SourceDetails&) override { dropHover = false; repaint(); }
    void itemDropped (const SourceDetails& d) override { dropHover = false; if (host.assign) host.assign ((int) d.description, dest); repaint(); }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && dest != 0) showModMenu();
    }
    void showModMenu()
    {
        juce::PopupMenu m; std::vector<int> slots;
        for (int i = 0; i < 10; ++i)
        {
            const int sbase = sb::m1_src + 3 * i;
            if ((int) (proc.paramB (sbase + 1)->convertFrom0to1 (proc.paramB (sbase + 1)->getValue()) + 0.5f) == dest && (int) (proc.paramB (sbase)->convertFrom0to1 (proc.paramB (sbase)->getValue()) + 0.5f) > 0)
            {
                const int src = (int) (proc.paramB (sbase)->convertFrom0to1 (proc.paramB (sbase)->getValue()) + 0.5f);
                m.addItem ((int) slots.size() + 1, juce::String ("Remove ") + srcName (src)); slots.push_back (i);
            }
        }
        if (slots.empty()) m.addItem (1000, "Drag a coloured source chip onto this knob", false);
        m.showMenuAsync ({}, [this, slots] (int r)
        {
            if (r >= 1 && r <= (int) slots.size()) { auto* p = proc.paramB (sb::m1_src + 3 * slots[(size_t) r - 1]); p->setValueNotifyingHost (0.0f); }
        });
    }
    void paintOverChildren (Graphics& g) override
    {
        auto sb_ = s.getBounds().toFloat();
        const float d = std::min (sb_.getWidth(), sb_.getHeight()) - 8.0f, r = d * 0.5f + 5.0f, cx = sb_.getCentreX(), cy = sb_.getCentreY();
        const float a0 = juce::MathConstants<float>::pi * 1.25f, a1 = juce::MathConstants<float>::pi * 2.75f;
        const float pos = (float) s.valueToProportionOfLength (s.getValue());
        if (dropHover) { g.setColour (kAqua.withAlpha (0.25f)); g.fillEllipse (cx - r - 4, cy - r - 4, (r + 4) * 2, (r + 4) * 2); }
        if (dest == 0) return;
        int ring = 0;
        for (int i = 0; i < 10; ++i)
        {
            const int sbase = sb::m1_src + 3 * i;
            const int src = (int) (proc.paramB (sbase)->convertFrom0to1 (proc.paramB (sbase)->getValue()) + 0.5f), dst = (int) (proc.paramB (sbase + 1)->convertFrom0to1 (proc.paramB (sbase + 1)->getValue()) + 0.5f);
            if (src == 0 || dst != dest) continue;
            const float amt = proc.paramB (sbase + 2)->convertFrom0to1 (proc.paramB (sbase + 2)->getValue()) * 2.0f - 1.0f;
            const float rr = r + 3.0f * (float) ring++;
            const float lo = juce::jlimit (0.0f, 1.0f, std::min (pos, pos + amt * 0.4f)), hi = juce::jlimit (0.0f, 1.0f, std::max (pos, pos + amt * 0.4f));
            juce::Path p; p.addCentredArc (cx, cy, rr, rr, 0, a0 + lo * (a1 - a0), a0 + hi * (a1 - a0), true);
            g.setColour (srcColour (src).withAlpha (0.85f)); g.strokePath (p, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            const float live = juce::jlimit (0.0f, 1.0f, pos + proc.engineB().srcShownValue (src) * amt * 0.4f);       // animated marker
            const float la = a0 + live * (a1 - a0);
            g.setColour (Colour (0xFFFFFFFF)); g.fillEllipse (cx + std::sin (la) * rr - 2.2f, cy - std::cos (la) * rr - 2.2f, 4.4f, 4.4f);
        }
    }
};

struct PCombo : juce::ComboBox
{
    std::unique_ptr<juce::ComboBoxParameterAttachment> att;
    PCombo (juce::RangedAudioParameter& p)
    {
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (&p)) addItemList (c->choices, 1);
        att = std::make_unique<juce::ComboBoxParameterAttachment> (p, *this, nullptr);
    }
};
struct PToggle : juce::ToggleButton
{
    std::unique_ptr<juce::ButtonParameterAttachment> att;
    PToggle (juce::RangedAudioParameter& p, const juce::String& t) { setButtonText (t); att = std::make_unique<juce::ButtonParameterAttachment> (p, *this, nullptr); }
};
struct PSlider : juce::Slider
{
    std::unique_ptr<juce::SliderParameterAttachment> att;
    PSlider (juce::RangedAudioParameter& p) : juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::NoTextBox)
    {
        att = std::make_unique<juce::SliderParameterAttachment> (p, *this, nullptr);
        setDoubleClickReturnValue (true, p.convertFrom0to1 (p.getDefaultValue()));
    }
};

// horizontal slider that is also a modulation drop target (Position, Warp amounts) with a live coloured range bar
struct PModSlider : PSlider, juce::DragAndDropTarget
{
    DuoProcessor& proc; Host& host; const int dest; bool hover = false;
    PModSlider (DuoProcessor& p, Host& h, int pidx) : PSlider (*p.paramB (pidx)), proc (p), host (h), dest (sb::destForParam (pidx)) {}
    bool isInterestedInDragSource (const SourceDetails& d) override { return dest != 0 && d.description.isInt(); }
    void itemDragEnter (const SourceDetails&) override { hover = true; repaint(); }
    void itemDragExit (const SourceDetails&) override { hover = false; repaint(); }
    void itemDropped (const SourceDetails& d) override { hover = false; if (host.assign) host.assign ((int) d.description, dest); repaint(); }
    void paintOverChildren (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        if (hover) { g.setColour (kAqua.withAlpha (0.25f)); g.fillRoundedRectangle (b, 6.0f); }
        const float pos = (float) valueToProportionOfLength (getValue());
        const float x0 = b.getX() + 4, w = b.getWidth() - 8;
        int ring = 0;
        for (int i = 0; i < 10; ++i)
        {
            const int sbase = sb::m1_src + 3 * i;
            auto rvv = [this] (int idx) { return proc.paramB (idx)->convertFrom0to1 (proc.paramB (idx)->getValue()); };
            const int src = (int) (rvv (sbase) + 0.5f), dst = (int) (rvv (sbase + 1) + 0.5f);
            if (src == 0 || dst != dest) continue;
            const float amt = rvv (sbase + 2);
            const float lo = juce::jlimit (0.0f, 1.0f, std::min (pos, pos + amt * 0.5f)), hi = juce::jlimit (0.0f, 1.0f, std::max (pos, pos + amt * 0.5f));
            const float y = b.getY() + 2.0f + 3.0f * (float) ring++;
            g.setColour (srcColour (src).withAlpha (0.9f)); g.fillRoundedRectangle (x0 + lo * w, y, std::max (2.0f, (hi - lo) * w), 2.5f, 1.2f);
            const float live = juce::jlimit (0.0f, 1.0f, pos + proc.engineB().srcShownValue (src) * amt * 0.5f);
            g.setColour (Colour (0xFFFFFFFF)); g.fillEllipse (x0 + live * w - 2.5f, y - 1.2f, 5.0f, 5.0f);
        }
    }
};
struct PChip : juce::Component
{
    int src; Colour col;
    PChip (int s) : src (s), col (srcColour (s)) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.5f);
        auto shape = facetShape (r, 6.0f);
        g.setColour (col.withAlpha (0.14f)); g.strokePath (shape, juce::PathStrokeType (4.0f));
        g.setGradientFill (juce::ColourGradient (col.withAlpha (0.42f), 0, r.getY(), col.withAlpha (0.10f), 0, r.getBottom(), false)); g.fillPath (shape);
        g.setColour (col); g.strokePath (shape, juce::PathStrokeType (1.2f));
        g.setColour (Colour (0x50FFFFFF)); g.drawLine (r.getX() + 8, r.getY() + 1.5f, r.getRight() - 3, r.getY() + 1.5f, 1.0f);
        g.setColour (Colour (0xFFFFFFFF)); g.setFont (sansK (11.0f, true, 0.03f)); g.drawText (srcName (src), getLocalBounds(), juce::Justification::centred);
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.getDistanceFromDragStart() < 4) return;
        if (auto* c = juce::DragAndDropContainer::findParentDragContainerFor (this))
            if (! c->isDragAndDropActive())
                c->startDragging (src, this, juce::ScaledImage (createComponentSnapshot (getLocalBounds(), true, 1.0f)));
    }
};

// 3D stacked-frames wavetable view
struct PWaveView : juce::Component
{
    DuoProcessor& proc; const int osc;
    int cachedTable = -1; std::vector<juce::Path> frames;
    PWaveView (DuoProcessor& p, int o) : proc (p), osc (o) {}
    void rebuild (int table, float w, float h)
    {
        cachedTable = table; frames.clear();
        const auto& wt = sb::wavetable (table); const int mip = 3, N = sb::kBaseN >> mip, pts = 80;
        const float dx = w * 0.14f / (float) sb::kFrames, dy = h * 0.52f / (float) sb::kFrames, ww = w * 0.8f, amp = h * 0.15f, baseY = h * 0.34f;
        for (int f = 0; f < sb::kFrames; ++f)
        {
            juce::Path p;
            for (int i = 0; i <= pts; ++i)
            {
                const float v = wt.mip[mip][(size_t) (f * N + (i * (N - 1)) / pts)];
                const float x = (float) i / (float) pts * ww + dx * (float) f, y = baseY + dy * (float) (sb::kFrames - 1 - f) - v * amp * 1.6f;
                if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
            }
            frames.push_back (p);
        }
    }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        auto shape = facetShape (r, 10.0f);
        g.setGradientFill (juce::ColourGradient (Colour (0xFF0D1534), 0, 0, Colour (0xFF050918), 0, r.getBottom(), false)); g.fillPath (shape);
        g.saveState(); g.reduceClipRegion (shape, juce::AffineTransform());
        g.setColour (Colour (0x0DFFFFFF));
        for (int i = 1; i < 8; ++i) g.drawHorizontalLine ((int) (r.getHeight() * (float) i / 8.0f), 0, r.getRight());
        for (int i = 1; i < 12; ++i) g.drawVerticalLine ((int) (r.getWidth() * (float) i / 12.0f), 0, r.getBottom());
        const int b = osc == 0 ? sb::o1_table : sb::o2_table;
        auto rv = [this] (int i) { return proc.paramB (i)->convertFrom0to1 (proc.paramB (i)->getValue()); };
        const int table = (int) (rv (b) + 0.5f);
        if (table != cachedTable) rebuild (table, r.getWidth(), r.getHeight());
        const float pos = juce::jlimit (0.0f, 1.0f, rv (b + 1) + proc.engineB().modShownDest (osc == 0 ? sb::D_O1POS : sb::D_O2POS) * 0.5f);
        const int cur = juce::jlimit (0, sb::kFrames - 1, (int) std::lround (pos * (float) (sb::kFrames - 1)));
        for (int f = sb::kFrames - 1; f >= 0; --f)          // back to front
        {
            const float t = (float) f / (float) (sb::kFrames - 1);
            if (f == cur) continue;
            g.setColour (spectrumAt (t, 0.26f + 0.38f * (1.0f - t)));
            g.strokePath (frames[(size_t) f], juce::PathStrokeType (1.0f));
        }
        {   // the active frame: filled, glowing
            const float dx = r.getWidth() * 0.14f / (float) sb::kFrames, dy = r.getHeight() * 0.52f / (float) sb::kFrames, ww = r.getWidth() * 0.8f, baseY = r.getHeight() * 0.34f;
            const float zero = baseY + dy * (float) (sb::kFrames - 1 - cur), x0 = dx * (float) cur;
            auto fill = frames[(size_t) cur]; fill.lineTo (x0 + ww, zero); fill.lineTo (x0, zero); fill.closeSubPath();
            const float t = (float) cur / (float) (sb::kFrames - 1);
            g.setGradientFill (juce::ColourGradient (spectrumAt (t, 0.42f), 0, zero - r.getHeight() * 0.3f, spectrumAt (t, 0.0f), 0, zero + 4, false)); g.fillPath (fill);
            g.setColour (spectrumAt (t, 0.18f)); g.strokePath (frames[(size_t) cur], juce::PathStrokeType (13.0f)); g.setColour (spectrumAt (t, 0.34f)); g.strokePath (frames[(size_t) cur], juce::PathStrokeType (6.0f));
            g.setColour (kIce); g.strokePath (frames[(size_t) cur], juce::PathStrokeType (1.8f));
        }
        g.restoreState();
        g.setGradientFill (prismGrad (0, 0, r.getRight(), r.getBottom(), 0.55f)); g.strokePath (shape, juce::PathStrokeType (1.0f));
    }
};

static float filterMagDb (int type, float cut, float res, float f)
{
    const float x = f / cut;
    const std::complex<float> jx (0, x), one (1, 0);
    float k = 2.0f * (1.0f - res * 0.97f) + 0.02f;
    std::complex<float> H;
    switch (type)
    {
        case 0: H = one / (one - std::complex<float> (x * x, 0) + jx * k); break;
        case 1: { k = 2.0f * (1.0f - res * 0.9f) + 0.1f; auto h = one / (one - std::complex<float> (x * x, 0) + jx * k); H = h * h; } break;
        case 2: H = std::complex<float> (-x * x, 0) / (one - std::complex<float> (x * x, 0) + jx * k); break;
        case 3: H = jx * k / (one - std::complex<float> (x * x, 0) + jx * k); break;
        case 4: H = (one - std::complex<float> (x * x, 0)) / (one - std::complex<float> (x * x, 0) + jx * k); break;
        case 5: { const float fb = 0.2f + res * 0.76f, ph = 2.0f * 3.14159f * f / cut; H = one / (one - fb * std::polar (1.0f, -ph)); H *= 0.55f; } break;
        case 6: { const float c[3] = { 1.0f, 2.2f, 4.0f }; const float q = 2.0f * (1.0f - res * 0.9f) + 0.12f; H = 0;
                  for (int j = 0; j < 3; ++j) { const float xx = f / (cut * c[j]); const std::complex<float> jj (0, xx); H += jj * q / (one - std::complex<float> (xx * xx, 0) + jj * q) * (1.0f - 0.25f * (float) j); } H *= 1.4f; } break;
        default: { const float g = std::pow (10.0f, res * 12.0f / 20.0f); const float t = 1.0f / (1.0f + std::pow (cut / std::max (f, 1.0f), 2.0f)); H = one + (g - 1.0f) * t; } break;
    }
    return juce::Decibels::gainToDecibels (std::abs (H), -60.0f);
}

struct PFilterCurve : juce::Component
{
    DuoProcessor& proc; const int base;
    PFilterCurve (DuoProcessor& p, int f) : proc (p), base (f == 0 ? sb::f1_type : sb::f2_type) {}
    float rv (int i) const { return proc.paramB (i)->convertFrom0to1 (proc.paramB (i)->getValue()); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        auto shape = facetShape (r, 8.0f);
        g.setGradientFill (juce::ColourGradient (Colour (0xFF0D1534), 0, 0, Colour (0xFF050918), 0, r.getBottom(), false)); g.fillPath (shape);
        g.saveState(); g.reduceClipRegion (shape, juce::AffineTransform());
        g.setColour (Colour (0x10FFFFFF));
        for (float fr : { 100.0f, 1000.0f, 10000.0f }) g.drawVerticalLine ((int) (std::log (fr / 20.0f) / std::log (1000.0f) * r.getWidth()), 0, r.getBottom());
        for (int i = 1; i < 4; ++i) g.drawHorizontalLine ((int) (r.getHeight() * (float) i / 4.0f), 0, r.getRight());
        const int type = (int) (rv (base) + 0.5f); const float cut = rv (base + 1), res = rv (base + 2);
        juce::Path line, fill;
        const int W = (int) r.getWidth();
        for (int i = 0; i < W; ++i)
        {
            const float f = 20.0f * std::pow (1000.0f, (float) i / (float) W);
            const float db = filterMagDb (type, cut, res, f);
            const float y = juce::jlimit (3.0f, r.getBottom() - 3, r.getBottom() - 6 - (db + 36.0f) / 54.0f * (r.getHeight() - 12));
            if (i == 0) { line.startNewSubPath (0, y); fill.startNewSubPath (0, r.getBottom()); fill.lineTo (0, y); } else { line.lineTo ((float) i, y); fill.lineTo ((float) i, y); }
        }
        fill.lineTo ((float) W, r.getBottom()); fill.closeSubPath();
        g.setGradientFill (juce::ColourGradient (kViolet.withAlpha (0.45f), 0, 0, kAqua.withAlpha (0.0f), 0, r.getBottom(), false)); g.fillPath (fill);
        g.setGradientFill (prismGrad (0, 0, r.getRight(), 0, 0.30f)); g.strokePath (line, juce::PathStrokeType (5.0f));
        g.setGradientFill (prismGrad (0, 0, r.getRight(), 0)); g.strokePath (line, juce::PathStrokeType (1.8f));
        const float hx = std::log (cut / 20.0f) / std::log (1000.0f) * r.getWidth(), hy = juce::jlimit (3.0f, r.getBottom() - 3, r.getBottom() - 6 - (juce::jlimit (-36.0f, 18.0f, filterMagDb (type, cut, res, cut)) + 36.0f) / 54.0f * (r.getHeight() - 12));
        g.setColour (kAqua.withAlpha (0.18f)); g.drawVerticalLine ((int) hx, 0, r.getBottom());
        g.setColour (kAqua.withAlpha (0.25f)); g.fillEllipse (hx - 9, hy - 9, 18, 18);
        g.setColour (kIce); g.fillEllipse (hx - 4.5f, hy - 4.5f, 9, 9);
        g.restoreState();
        g.setGradientFill (prismGrad (0, 0, r.getRight(), r.getBottom(), 0.5f)); g.strokePath (shape, juce::PathStrokeType (1.0f));
    }
    void drag (const juce::MouseEvent& e)
    {
        auto setN = [this] (int idx, float n) { proc.paramB (idx)->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, n)); };
        const float f = 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (float) e.x / (float) getWidth()));
        auto* pc = proc.paramB (base + 1); setN (base + 1, pc->convertTo0to1 (juce::jlimit (20.0f, 20000.0f, f)));
        setN (base + 2, 1.0f - (float) e.y / (float) getHeight());
    }
    void mouseDown (const juce::MouseEvent& e) override { proc.paramB (base + 1)->beginChangeGesture(); proc.paramB (base + 2)->beginChangeGesture(); drag (e); }
    void mouseDrag (const juce::MouseEvent& e) override { drag (e); }
    void mouseUp (const juce::MouseEvent&) override { proc.paramB (base + 1)->endChangeGesture(); proc.paramB (base + 2)->endChangeGesture(); }
};

struct PSpectrum : juce::Component
{
    DuoProcessor& proc; juce::dsp::FFT fft { 10 }; std::array<float, 512> mag {};
    PSpectrum (DuoProcessor& p) : proc (p) { setInterceptsMouseClicks (false, false); }
    void tick()
    {
        float buf[1024]; proc.engineB().readScope (buf, 1024);
        std::array<float, 2048> d {};
        for (int i = 0; i < 1024; ++i) d[(size_t) i] = buf[i] * (0.5f - 0.5f * std::cos (2.0f * 3.14159f * (float) i / 1023.0f));
        fft.performFrequencyOnlyForwardTransform (d.data());
        for (int i = 0; i < 512; ++i) { const float v = juce::jlimit (0.0f, 1.0f, (juce::Decibels::gainToDecibels (d[(size_t) i] / 64.0f, -90.0f) + 80.0f) / 70.0f); mag[(size_t) i] = std::max (v, mag[(size_t) i] * 0.88f); }
        repaint();
    }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        juce::Path p, edge; p.startNewSubPath (0, r.getBottom());
        for (int x = 0; x < (int) r.getWidth(); x += 3)
        {
            const float fr = std::pow ((float) x / r.getWidth(), 2.2f) * 511.0f;       // roughly log-ish axis
            const int i0 = (int) fr; const float t = fr - (float) i0;
            const float v = mag[(size_t) i0] * (1 - t) + mag[(size_t) std::min (511, i0 + 1)] * t;
            const float y = r.getBottom() - v * r.getHeight() * 0.9f;
            p.lineTo ((float) x, y);
            if (x == 0) edge.startNewSubPath (0, y); else edge.lineTo ((float) x, y);
        }
        p.lineTo (r.getRight(), r.getBottom()); p.closeSubPath();
        g.setGradientFill (juce::ColourGradient (kViolet.withAlpha (0.24f), 0, r.getY(), kAqua.withAlpha (0.02f), 0, r.getBottom(), false)); g.fillPath (p);
        g.setGradientFill (prismGrad (0, 0, r.getRight(), 0, 0.30f)); g.strokePath (edge, juce::PathStrokeType (1.2f));
    }
};

struct PLfoEditor : juce::Component
{
    DuoProcessor& proc; int lfo = 0; int dragIdx = -1;
    PLfoEditor (DuoProcessor& p) : proc (p) {}
    float px (int i) const { return 12.0f + (float) i / 15.0f * ((float) getWidth() - 24.0f); }
    float py (float v) const { return (float) getHeight() * 0.5f - v * ((float) getHeight() * 0.5f - 14.0f); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Colour (0xFF0A0F22)); g.fillRoundedRectangle (r, 10.0f);
        g.setColour (Colour (0x14FFFFFF)); g.drawHorizontalLine ((int) py (0), 8, r.getRight() - 8);
        juce::Path p;
        for (int i = 0; i <= 15; ++i) { const float v = proc.engineB().getCustomShape (lfo, i); if (i == 0) p.startNewSubPath (px (i), py (v)); else p.lineTo (px (i), py (v)); }
        const float v0 = proc.engineB().getCustomShape (lfo, 0); p.lineTo (r.getRight() - 12.0f + 0.0f, py (v0));
        g.setColour (srcColour (sb::S_LFO1 + lfo).withAlpha (0.9f)); g.strokePath (p, juce::PathStrokeType (2.0f));
        for (int i = 0; i < 16; ++i) { g.setColour (kIce); g.fillEllipse (px (i) - 4, py (proc.engineB().getCustomShape (lfo, i)) - 4, 8, 8); }
        const float ph = proc.engineB().lfoValue (lfo);
        g.setColour (srcColour (sb::S_LFO1 + lfo)); g.fillEllipse (r.getRight() - 24, py (ph) - 5, 10, 10);
        g.setColour (kIce.withAlpha (0.5f)); g.setFont (sans (10.0f)); g.drawText ("Drag the points - used when the LFO shape is set to Drawn", 10, 2, getWidth() - 20, 12, juce::Justification::centredLeft);
        g.setColour (Colour (0x26FFFFFF)); g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
    }
    void edit (const juce::MouseEvent& e)
    {
        int best = 0; float bd = 1e9f;
        for (int i = 0; i < 16; ++i) { const float d = std::fabs (px (i) - (float) e.x); if (d < bd) { bd = d; best = i; } }
        if (dragIdx < 0) dragIdx = best;
        const float v = juce::jlimit (-1.0f, 1.0f, ((float) getHeight() * 0.5f - (float) e.y) / ((float) getHeight() * 0.5f - 14.0f));
        proc.setCustomShapeB (lfo, dragIdx, v); repaint();
    }
    void mouseDown (const juce::MouseEvent& e) override { dragIdx = -1; edit (e); }
    void mouseDrag (const juce::MouseEvent& e) override { if (dragIdx >= 0) { const int keep = dragIdx; (void) keep; edit (e); } }
    void mouseUp (const juce::MouseEvent&) override { dragIdx = -1; }
};

struct PEnvView : juce::Component
{
    DuoProcessor& proc; const int base;
    PEnvView (DuoProcessor& p, int b) : proc (p), base (b) { setInterceptsMouseClicks (false, false); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.0f);
        auto rv = [this] (int i) { return proc.paramB (i)->convertFrom0to1 (proc.paramB (i)->getValue()); };
        const float a = rv (base), d = rv (base + 1), s = rv (base + 2), rl = rv (base + 3);
        const float total = a + d + 0.4f + rl, w = r.getWidth();
        float x = r.getX(); const float bot = r.getBottom(), top = r.getY() + 3;
        juce::Path p; p.startNewSubPath (x, bot);
        x += a / total * w; p.lineTo (x, top);
        for (int i = 1; i <= 12; ++i) { const float t = (float) i / 12.0f; p.lineTo (x + t * d / total * w, top + (1 - s) * (bot - top) * (1 - std::exp (-4.0f * t)) / (1 - std::exp (-4.0f))); }
        x += d / total * w; p.lineTo (x + 0.4f / total * w, top + (1 - s) * (bot - top)); x += 0.4f / total * w;
        for (int i = 1; i <= 12; ++i) { const float t = (float) i / 12.0f; p.lineTo (x + t * rl / total * w, bot - (bot - top) * s * std::exp (-4.0f * t)); }
        auto fill = p; fill.lineTo (r.getRight(), bot); fill.closeSubPath();
        g.setColour (kAqua.withAlpha (0.12f)); g.fillPath (fill);
        g.setColour (kAqua); g.strokePath (p, juce::PathStrokeType (1.8f));
    }
};

// ====================================================================================== the panel
struct PrismPanel::Impl
{
    DuoProcessor& proc;
    PrismLook look;
    PrismPanel& owner;
    Host host;
    struct Card { Rectangle<int> r; juce::String title; };
    std::vector<Card> cards;
    std::vector<std::unique_ptr<juce::Component>> owned;
    std::vector<PKnob*> knobs; std::vector<juce::Component*> modSliders;
    std::vector<PWaveView*> waves; std::vector<PFilterCurve*> curves;
    PSpectrum* spectrum = nullptr; PLfoEditor* lfoEd = nullptr;
    juce::Component* tabPages[5] = {};
    juce::TextButton tabBtn[5], prevB { "<" }, nextB { ">" }, presetB { "Init" };
    juce::TextEditor search;
    juce::TextButton lfoSel[4];
    int tab = 0;
    bool locks[5] = { false, false, false, false, false };
    struct SlotUi { juce::Label labels[3]; PCombo* type = nullptr; PKnob* k[4] = {}; };
    SlotUi slotUi[8];
    juce::Rectangle<int> fanRect;

    Impl (PrismPanel& o, DuoProcessor& p) : proc (p), owner (o) { host.proc = &p; }

    juce::RangedAudioParameter& P (int i) { return *proc.paramB (i); }
    template <class T, class... A> T* addTo (juce::Component& parent, A&&... a) { auto c = std::make_unique<T> (std::forward<A> (a)...); auto* raw = c.get(); parent.addAndMakeVisible (*c); owned.push_back (std::move (c)); return raw; }
    PKnob* knob (juce::Component& parent, int p, const juce::String& n, int x, int y, int w = 54, int h = 66)
    {
        auto* k = addTo<PKnob> (parent, proc, host, p, n); k->setBounds (x, y, w, h); knobs.push_back (k); return k;
    }
    void combo (juce::Component& parent, int p, int x, int y, int w, int h = 22) { addTo<PCombo> (parent, P (p))->setBounds (x, y, w, h); }
    void toggle (juce::Component& parent, int p, const juce::String& t, int x, int y, int w = 70, int h = 18) { addTo<PToggle> (parent, P (p), t)->setBounds (x, y, w, h); }
    void hslider (juce::Component& parent, int p, int x, int y, int w, int h = 16)
    {
        if (sb::destForParam (p) != 0) { auto* s = addTo<PModSlider> (parent, proc, host, p); s->setBounds (x, y, w, h); modSliders.push_back (s); }
        else addTo<PSlider> (parent, P (p))->setBounds (x, y, w, h);
    }
    void card (int x, int y, int w, int h, const juce::String& t) { cards.push_back ({ { x, y, w, h }, t }); }
    juce::Label* label (juce::Component& parent, const juce::String& t, int x, int y, int w, int h = 14, float size = 11.0f)
    {
        auto* l = addTo<juce::Label> (parent); l->setText (t, juce::dontSendNotification); l->setBounds (x, y, w, h); l->setFont (sans (size)); l->getProperties().set ("own", true); l->setInterceptsMouseClicks (false, false); return l;
    }
    float rv (int i) { return P (i).convertFrom0to1 (P (i).getValue()); }
    void setReal (int i, float v) { P (i).beginChangeGesture(); P (i).setValueNotifyingHost (P (i).convertTo0to1 (v)); P (i).endChangeGesture(); }

    void assignMod (int src, int dest)
    {
        int slot = -1;
        for (int i = 0; i < 10 && slot < 0; ++i) if ((int) (rv (sb::m1_src + 3 * i) + 0.5f) == src && (int) (rv (sb::m1_dst + 3 * i) + 0.5f) == dest) slot = i;
        for (int i = 0; i < 10 && slot < 0; ++i) if ((int) (rv (sb::m1_src + 3 * i) + 0.5f) == 0) slot = i;
        if (slot < 0) slot = 9;
        setReal (sb::m1_src + 3 * slot, (float) src); setReal (sb::m1_dst + 3 * slot, (float) dest);
        if (std::fabs (rv (sb::m1_amt + 3 * slot)) < 0.01f) setReal (sb::m1_amt + 3 * slot, 0.5f);
    }

    static const char* fxLabel (int type, int i)
    {
        static const char* L[11][3] = { { "A", "B", "C" }, { "Rate", "Depth", "Spread" }, { "Rate", "Depth", "Fdbk" }, { "Time", "Fdbk", "X-Feed" }, { "Decay", "Bright", "Air" },
                                        { "Drive", "Fold/Crush", "Tone" }, { "Low", "Mid", "High" }, { "Thresh", "Ratio", "Makeup" }, { "Cutoff", "Reso", "Mode" }, { "Width", "Haas", "Bass Mono" }, { "Rate", "Depth", "Fdbk" } };
        return L[std::max (0, std::min (10, type))][i];
    }
    void refreshFxLabels() { for (int s = 0; s < 8; ++s) for (int i = 0; i < 3; ++i) if (slotUi[s].k[i]) slotUi[s].k[i]->name.setText (fxLabel ((int) (rv (sb::r1_type + 6 * s) + 0.5f), i), juce::dontSendNotification); }
    void swapSlots (int a, int b)
    {
        if (b < 0 || b > 7) return;
        for (int i = 0; i < 6; ++i)
        {
            auto &pa = P (sb::r1_type + 6 * a + i), &pb = P (sb::r1_type + 6 * b + i);
            const float va = pa.getValue(), vb = pb.getValue();
            pa.setValueNotifyingHost (vb); pb.setValueNotifyingHost (va);
        }
    }

    // ---- "Surprise me": musical randomisation with locks (0 osc, 1 filter, 2 env, 3 mod, 4 fx)
    void surprise()
    {
        juce::Random rnd;
        auto rn = [&] (float a, float b) { return a + (b - a) * rnd.nextFloat(); };
        auto setN = [this] (int i, float n) { P (i).setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, n)); };
        if (! locks[0])
            for (int o = 0; o < 2; ++o)
            {
                const int b = o == 0 ? sb::o1_table : sb::o2_table;
                setReal (b, (float) rnd.nextInt (sb::kTables)); setReal (b + 1, rn (0, 1));
                setReal (b + 9, rnd.nextFloat() < 0.5f ? 0.0f : (float) (1 + rnd.nextInt (10))); setReal (b + 10, rn (0, 0.6f));
                setReal (b + 11, rnd.nextFloat() < 0.8f ? 0.0f : (float) (1 + rnd.nextInt (10))); setReal (b + 12, rn (0, 0.5f));
                if (o == 1) setReal (b + 5, rnd.nextFloat() < 0.6f ? rn (0.3f, 0.8f) : 0.0f);
                setReal (b + 3, o == 1 && rnd.nextFloat() < 0.4f ? (float) (rnd.nextBool() ? 7 : 12) : 0.0f);
            }
        if (! locks[0]) { setReal (sb::uni_voices, (float) (1 + rnd.nextInt (8))); setReal (sb::uni_detune, rn (0.05f, 0.6f)); setReal (sb::uni_wtspread, rnd.nextFloat() < 0.4f ? rn (0, 0.5f) : 0.0f); }
        if (! locks[1])
        {
            static const int types[] = { 0, 1, 1, 3, 6, 5, 2 };
            setReal (sb::f1_type, (float) types[rnd.nextInt (7)]); setReal (sb::f1_cut, 300.0f * std::pow (60.0f, rnd.nextFloat())); setReal (sb::f1_res, rn (0, 0.55f));
            setReal (sb::f1_env, rn (-0.2f, 0.7f)); setReal (sb::f1_drive, rnd.nextFloat() < 0.3f ? rn (0, 0.5f) : 0.0f);
        }
        if (! locks[2])
        {
            setReal (sb::e1_a, 0.001f + std::pow (rnd.nextFloat(), 3.0f) * 1.5f); setReal (sb::e1_d, rn (0.1f, 1.5f)); setReal (sb::e1_s, rn (0.0f, 1.0f)); setReal (sb::e1_r, rn (0.1f, 2.5f));
            setReal (sb::e2_a, 0.001f + std::pow (rnd.nextFloat(), 3.0f) * 0.8f); setReal (sb::e2_d, rn (0.1f, 1.2f)); setReal (sb::e2_s, rn (0.0f, 0.8f)); setReal (sb::e2_r, rn (0.1f, 1.5f));
        }
        if (! locks[3])
        {
            for (int i = 0; i < 10; ++i) { setReal (sb::m1_src + 3 * i, 0); setReal (sb::m1_dst + 3 * i, 0); setReal (sb::m1_amt + 3 * i, 0); }
            static const int dsts[] = { sb::D_O1POS, sb::D_O2POS, sb::D_F1CUT, sb::D_O1W1, sb::D_DETUNE, sb::D_PAN, sb::D_O1PITCH };
            static const int srcs[] = { sb::S_LFO1, sb::S_LFO2, sb::S_LFO3, sb::S_ENV3, sb::S_ENV2, sb::S_MAC1, sb::S_LFO4 };
            for (int i = 0; i < 3; ++i) { setReal (sb::m1_src + 3 * i, (float) srcs[rnd.nextInt (7)]); setReal (sb::m1_dst + 3 * i, (float) dsts[rnd.nextInt (7)]); setReal (sb::m1_amt + 3 * i, rn (-0.7f, 0.7f)); }
            setReal (sb::l1_rate, rn (0.2f, 6)); setReal (sb::l2_rate, rn (0.1f, 2)); setReal (sb::l1_shape, (float) rnd.nextInt (5));
        }
        if (! locks[4])
        {
            for (int s = 0; s < 8; ++s) setReal (sb::r1_type + 6 * s, 0.0f);
            static const int types[] = { sb::FX_CHORUS, sb::FX_DELAY, sb::FX_REVERB, sb::FX_PHASER, sb::FX_WIDENER, sb::FX_DIST };
            for (int s = 0; s < 3; ++s) { const int b = sb::r1_type + 6 * s; setReal (b, (float) types[rnd.nextInt (6)]); setReal (b + 1, 1); setN (b + 2, rnd.nextFloat()); setN (b + 3, rn (0.2f, 0.7f)); setN (b + 4, rnd.nextFloat()); setReal (b + 5, rn (0.15f, 0.45f)); }
        }
        (void) setN;
    }

    void showTab (int t)
    {
        tab = t;
        for (int i = 0; i < 5; ++i) { tabPages[i]->setVisible (i == t); tabBtn[i].setToggleState (i == t, juce::dontSendNotification); }
    }

    juce::Label infoL;
    juce::TextButton surpriseB { "SURPRISE ME" }, locksB { "SECTION LOCKS" };

    void buildPresetBar()
    {
        owner.addAndMakeVisible (presetB); presetB.setBounds (176, 12, 290, 32); presetB.getProperties().set ("preset", true);
        owner.addAndMakeVisible (prevB); owner.addAndMakeVisible (nextB); prevB.setBounds (176, 52, 36, 26); nextB.setBounds (216, 52, 36, 26);
        owner.addAndMakeVisible (search); search.setBounds (474, 12, 190, 32); search.setTextToShowWhenEmpty ("Search names + tags...", kIce.withAlpha (0.4f)); search.setFont (sans (12.5f));
        owner.addAndMakeVisible (infoL); infoL.setBounds (262, 54, 300, 22); infoL.setColour (juce::Label::textColourId, kAqua.withAlpha (0.9f));
        owner.addAndMakeVisible (surpriseB); surpriseB.setBounds (690, 12, 190, 32); surpriseB.getProperties().set ("accent", true); surpriseB.onClick = [this] { surprise(); };
        owner.addAndMakeVisible (locksB); locksB.setBounds (690, 52, 190, 26); locksB.onClick = [this] { showTab (4); };
        presetB.onClick = [this] { showPresetMenu(); };
        auto step = [this] (int d) { const int n = (int) sb::presets().size(); proc.loadPresetB ((proc.presetIndexB() + n + d) % n); refreshPresetName(); };
        prevB.onClick = [step] { step (-1); }; nextB.onClick = [step] { step (1); };
        search.onReturnKey = [this] { showPresetMenu (search.getText()); };
        refreshPresetName();
    }
    void refreshPresetName()
    {
        const int idx = std::min (proc.presetIndexB(), (int) sb::presets().size() - 1);
        const auto& p = sb::presets()[(size_t) idx];
        presetB.setButtonText (juce::String (p.name) + "   -   " + juce::String (p.category));
        infoL.setText (juce::String (idx + 1) + " / " + juce::String ((int) sb::presets().size()) + "   -   " + juce::String (p.category).toUpperCase(), juce::dontSendNotification);
        refreshFxLabels();
    }
    void showPresetMenu (const juce::String& filter = {})
    {
        juce::PopupMenu menu;
        const auto& all = sb::presets();
        if (filter.isEmpty())
        {
            std::map<std::string, juce::PopupMenu> cats; std::vector<std::string> order;
            for (size_t i = 0; i < all.size(); ++i) { if (! cats.count (all[i].category)) order.push_back (all[i].category); cats[all[i].category].addItem ((int) i + 1, all[i].name, true, (int) i == proc.presetIndexB()); }
            for (auto& c : order) menu.addSubMenu (c, cats[c]);
        }
        else
            for (size_t i = 0; i < all.size(); ++i)
                if (juce::String (all[i].name).containsIgnoreCase (filter) || juce::String (all[i].category).containsIgnoreCase (filter)) menu.addItem ((int) i + 1, juce::String (all[i].name) + "  (" + all[i].category + ")");
        if (menu.getNumItems() == 0) menu.addItem (9999, "no match", false);
        menu.setLookAndFeel (&look);
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetB), [this] (int r) { if (r > 0 && r < 9999) { proc.loadPresetB (r - 1); refreshPresetName(); } });
    }

    void build()
    {
        owner.setLookAndFeel (&look);
        host.assign = [this] (int s, int d) { assignMod (s, d); };
        buildPresetBar();
        knob (owner, sb::master, "Output", 1196, 8, 62, 72);

        // ---- macro bar
        card (8, 90, 1264, 72, "");
        label (owner, "MACRO", 22, 112, 90, 14, 11.0f)->setFont (sansK (11.0f, true, 0.12f));
        label (owner, "CONTROL", 22, 128, 90, 14, 9.5f)->setColour (juce::Label::textColourId, kIce.withAlpha (0.5f));
        for (int m = 0; m < 8; ++m) knob (owner, sb::mac1 + m, "Macro " + juce::String (m + 1), 126 + m * 142, 94, 110, 66);

        // ---- oscillators
        const int Y1 = 168, H1 = 212;
        for (int o = 0; o < 2; ++o)
        {
            const int x = 8 + o * 296, y = Y1, b = o == 0 ? sb::o1_table : sb::o2_table;
            card (x, y, 288, H1, o == 0 ? "OSC 1  /  WAVETABLE" : "OSC 2  /  WAVETABLE");
            combo (owner, b, x + 12, y + 30, 138);
            auto* wv = addTo<PWaveView> (owner, proc, o); wv->setBounds (x + 12, y + 56, 138, 76); waves.push_back (wv);
            hslider (owner, b + 1, x + 12, y + 136, 138); label (owner, "POSITION", x + 12, y + 152, 100, 12, 9.5f);
            combo (owner, b + 9, x + 12, y + 168, 66); combo (owner, b + 11, x + 84, y + 168, 66);
            hslider (owner, b + 10, x + 12, y + 194, 66, 14); hslider (owner, b + 12, x + 84, y + 194, 66, 14);
            const int kx = x + 158;
            knob (owner, b + 2, "Oct", kx, y + 28, 42, 62); knob (owner, b + 3, "Semi", kx + 43, y + 28, 42, 62); knob (owner, b + 4, "Fine", kx + 86, y + 28, 42, 62);
            knob (owner, b + 5, "Level", kx, y + 96, 42, 62); knob (owner, b + 6, "Pan", kx + 43, y + 96, 42, 62); knob (owner, b + 7, "Phase", kx + 86, y + 96, 42, 62);
            combo (owner, b + 8, kx, y + 172, 128);
        }
        // ---- sub / noise / unison
        card (600, Y1, 216, 84, "MIX");
        knob (owner, sb::sub_lvl, "Sub", 608, Y1 + 20, 56, 62); knob (owner, sb::noise_lvl, "Noise", 666, Y1 + 20, 56, 62);
        combo (owner, sb::sub_wave, 728, Y1 + 24, 80, 18); combo (owner, sb::sub_oct, 728, Y1 + 44, 80, 18); combo (owner, sb::noise_col, 728, Y1 + 64, 80, 18);
        card (600, Y1 + 92, 216, 120, "UNISON");
        const int uy = Y1 + 92 + 22;
        knob (owner, sb::uni_voices, "Voices", 606, uy, 42, 62); knob (owner, sb::uni_detune, "Detune", 647, uy, 42, 62); knob (owner, sb::uni_spread, "Spread", 688, uy, 42, 62);
        knob (owner, sb::uni_blend, "Blend", 729, uy, 42, 62); knob (owner, sb::uni_wtspread, "WT Sprd", 770, uy, 42, 62);
        combo (owner, sb::uni_curve, 608, Y1 + 92 + 90, 96, 20);
        fanRect = { 712, Y1 + 92 + 88, 96, 24 };
        // ---- filters
        for (int f = 0; f < 2; ++f)
        {
            const int x = 824, y = Y1 + f * 110, b = f == 0 ? sb::f1_type : sb::f2_type;
            card (x, y, 448, 102, f == 0 ? "FILTER 1" : "FILTER 2");
            combo (owner, b, x + 448 - 118, y + 6, 108, 20);
            auto* c = addTo<PFilterCurve> (owner, proc, f); c->setBounds (x + 12, y + 32, 120, 62); curves.push_back (c);
            const int kx = x + 138;
            knob (owner, b + 1, "Cutoff", kx, y + 34, 43, 62); knob (owner, b + 2, "Reso", kx + 43, y + 34, 43, 62); knob (owner, b + 3, "Drive", kx + 86, y + 34, 43, 62);
            knob (owner, b + 4, "Mix", kx + 129, y + 34, 43, 62); knob (owner, b + 5, "Key", kx + 172, y + 34, 43, 62); knob (owner, b + 6, "Env", kx + 215, y + 34, 43, 62); knob (owner, b + 7, "Vel", kx + 258, y + 34, 43, 62);
        }
        combo (owner, sb::f_route, 912, Y1 + 6, 96, 20);

        // ---- source chips
        const int cy = 386;
        card (8, cy, 1264, 34, "");
        label (owner, "DRAG SOURCE", 20, cy + 9, 100, 16, 9.5f)->setColour (juce::Label::textColourId, kIce.withAlpha (0.5f));
        const int order[] = { sb::S_ENV1, sb::S_ENV2, sb::S_ENV3, sb::S_LFO1, sb::S_LFO2, sb::S_LFO3, sb::S_LFO4, sb::S_MAC1, sb::S_MAC2, sb::S_MAC3, sb::S_MAC4, sb::S_MAC5, sb::S_MAC6, sb::S_MAC7, sb::S_MAC8, sb::S_VEL, sb::S_KEY, sb::S_MOD, sb::S_AT, sb::S_RAND };
        int cx = 122;
        for (int s : order) { auto* c = addTo<PChip> (owner, s); c->setBounds (cx, cy + 5, 52, 24); cx += 54; if (s == sb::S_ENV3 || s == sb::S_LFO4 || s == sb::S_MAC8) cx += 10; }

        // ---- lower tabs
        const int ty = 426;
        card (8, ty, 920, 170, "");
        static const char* tn[] = { "ENVELOPES", "LFO", "MOD MATRIX", "ARP / SEQ", "SURPRISE" };
        for (int i = 0; i < 5; ++i)
        {
            owner.addAndMakeVisible (tabBtn[i]); tabBtn[i].setButtonText (tn[i]); tabBtn[i].setBounds (16 + i * 118, ty + 6, 112, 26); tabBtn[i].setClickingTogglesState (false);
            tabBtn[i].onClick = [this, i] { showTab (i); };
            auto pg = std::make_unique<juce::Component>(); owner.addAndMakeVisible (*pg); pg->setBounds (8, ty + 36, 920, 134); tabPages[i] = pg.get(); owned.push_back (std::move (pg));
        }
        // ENV
        {
            auto& pg = *tabPages[0];
            const int eb[3] = { sb::e1_a, sb::e2_a, sb::e3_a }; const char* en[3] = { "ENV 1  -  AMPLITUDE", "ENV 2  -  FILTER", "ENV 3  -  FREE" }; const Colour ec[3] = { kAqua, kMint, Colour (0xFF9FB4FF) };
            for (int e = 0; e < 3; ++e)
            {
                const int ex = 10 + e * 302;
                auto* l = label (pg, en[e], ex, 0, 240, 16, 11.0f); l->setColour (juce::Label::textColourId, ec[e]);
                addTo<PEnvView> (pg, proc, eb[e])->setBounds (ex, 18, 286, 48);
                for (int k = 0; k < 4; ++k) knob (pg, eb[e] + k, k == 0 ? "Attack" : k == 1 ? "Decay" : k == 2 ? "Sustain" : "Release", ex + k * 72, 68, 70, 64);
            }
        }
        // LFO
        {
            auto& pg = *tabPages[1];
            lfoEd = addTo<PLfoEditor> (pg, proc); lfoEd->setBounds (10, 32, 470, 98);
            for (int l = 0; l < 4; ++l)
            {
                auto& b = lfoSel[l]; pg.addAndMakeVisible (b); b.setButtonText ("LFO " + juce::String (l + 1)); b.setBounds (10 + l * 70, 2, 64, 26); b.setClickingTogglesState (false);
                b.onClick = [this, l] { lfoEd->lfo = l; for (int k = 0; k < 4; ++k) lfoSel[k].setToggleState (k == l, juce::dontSendNotification); lfoEd->repaint(); updateLfoCtl(); };
            }
            lfoSel[0].setToggleState (true, juce::dontSendNotification);
            lfoCtl = &pg;
            for (int l = 0; l < 4; ++l)
            {
                const int b = sb::l1_shape + 3 * l;
                lfoShape[l] = addTo<PCombo> (pg, P (b)); lfoShape[l]->setBounds (500, 32, 200, 26);
                lfoSync[l] = addTo<PCombo> (pg, P (b + 2)); lfoSync[l]->setBounds (500, 66, 200, 26);
                lfoRate[l] = addTo<PKnob> (pg, proc, host, b + 1, "Rate"); lfoRate[l]->setBounds (730, 28, 90, 90); knobs.push_back (lfoRate[l]);
            }
            updateLfoCtl();
        }
        // MATRIX
        {
            auto& pg = *tabPages[2];
            for (int col = 0; col < 2; ++col)
            {
                const int x0 = 10 + col * 456;
                label (pg, "SOURCE", x0 + 4, 2, 100, 14, 9.5f); label (pg, "DESTINATION", x0 + 140, 2, 100, 14, 9.5f); label (pg, "AMOUNT", x0 + 278, 2, 100, 14, 9.5f);
                for (int r = 0; r < 5; ++r)
                {
                    const int b = sb::m1_src + 3 * (col * 5 + r), ry = 20 + r * 22;
                    combo (pg, b, x0, ry, 130, 20); combo (pg, b + 1, x0 + 136, ry, 130, 20); hslider (pg, b + 2, x0 + 274, ry + 2, 150);
                }
            }
        }
        // ARP / SEQ
        {
            auto& pg = *tabPages[3];
            toggle (pg, sb::arp_on, "ARP", 10, 2, 60); toggle (pg, sb::arp_hold, "HOLD", 76, 2, 70); combo (pg, sb::arp_mode, 10, 28, 124); combo (pg, sb::arp_rate, 140, 28, 70);
            knob (pg, sb::arp_oct, "Octaves", 10, 62, 60, 66); knob (pg, sb::arp_gate, "Gate", 72, 62, 60, 66); knob (pg, sb::arp_swing, "Swing", 134, 62, 60, 66);
            toggle (pg, sb::seq_on, "STEP SEQ", 232, 2, 100); combo (pg, sb::seq_rate, 338, 2, 70, 20); hslider (pg, sb::seq_len, 440, 4, 130); label (pg, "LENGTH", 576, 3, 60, 14, 9.5f);
            for (int i = 0; i < 16; ++i)
            {
                const int sx = 230 + i * 42;
                knob (pg, sb::sp1 + 3 * i, "P" + juce::String (i + 1), sx, 24, 42, 58);
                hslider (pg, sb::sv1 + 3 * i, sx + 2, 84, 38, 14);
                toggle (pg, sb::sg1 + 3 * i, "", sx + 8, 102, 34, 16);
            }
            label (pg, "pitch", 196, 44, 32, 12, 9.0f); label (pg, "vel", 204, 84, 24, 12, 9.0f); label (pg, "gate", 198, 104, 30, 12, 9.0f);
        }
        // SURPRISE
        {
            auto& pg = *tabPages[4];
            auto* big = addTo<juce::TextButton> (pg, "SURPRISE ME"); big->setBounds (14, 12, 220, 46); big->getProperties().set ("accent", true); big->onClick = [this] { surprise(); };
            label (pg, "Creates a new sound inside musical ranges. Switch on a lock to keep that section as it is.", 250, 12, 600, 18, 12.0f);
            static const char* ln[5] = { "OSCILLATORS", "FILTER", "ENVELOPES", "MODULATION", "FX RACK" };
            for (int i = 0; i < 5; ++i)
            {
                auto* t = addTo<juce::ToggleButton> (pg, ln[i]); t->setButtonText (juce::String ("Lock ") + ln[i]); t->setBounds (14 + i * 178, 84, 170, 22);
                t->onClick = [this, i, t] { locks[i] = t->getToggleState(); };
            }
        }
        showTab (0);

        // ---- spectral field
        card (936, ty, 336, 170, "SPECTRAL FIELD");
        spectrum = addTo<PSpectrum> (owner, proc); spectrum->setBounds (946, ty + 30, 316, 114);

        // ---- FX rack: eight slots in a row
        const int fy = 602;
        for (int s = 0; s < 8; ++s)
        {
            const int x = 8 + s * 159, b = sb::r1_type + 6 * s;
            card (x, fy, 151, 114, "");
            toggle (owner, b + 1, "", x + 4, fy + 7, 34, 18);
            slotUi[s].type = addTo<PCombo> (owner, P (b)); slotUi[s].type->setBounds (x + 42, fy + 5, 66, 22);
            slotUi[s].type->onChange = [this] { refreshFxLabels(); };
            auto* lb = addTo<juce::TextButton> (owner, "<"); lb->setBounds (x + 112, fy + 5, 17, 22); lb->onClick = [this, s] { swapSlots (s, s - 1); refreshFxLabels(); };
            auto* rb = addTo<juce::TextButton> (owner, ">"); rb->setBounds (x + 130, fy + 5, 17, 22); rb->onClick = [this, s] { swapSlots (s, s + 1); refreshFxLabels(); };
            for (int k = 0; k < 3; ++k) slotUi[s].k[k] = knob (owner, b + 2 + k, "", x + 2 + k * 49, fy + 28, 49, 62);
            label (owner, "MIX", x + 8, fy + 94, 28, 12, 9.0f); hslider (owner, b + 5, x + 36, fy + 94, 106, 14);
        }
        refreshFxLabels();
    }

    PCombo *lfoShape[4] = {}, *lfoSync[4] = {}; PKnob* lfoRate[4] = {}; juce::Component* lfoCtl = nullptr;
    void updateLfoCtl() { for (int l = 0; l < 4; ++l) { const bool v = (l == (lfoEd ? lfoEd->lfo : 0)); lfoShape[l]->setVisible (v); lfoSync[l]->setVisible (v); lfoRate[l]->setVisible (v); } }

    float sheen = -1.0f;
    juce::Rectangle<int> logoRect { 4, 2, 170, 60 };

    void paintBack (Graphics& g, int w, int h)
    {
        g.setGradientFill (juce::ColourGradient (Colour (0xFF0F1736), 0, 0, Colour (0xFF090E22), 0, (float) h, false)); g.fillAll();
        g.setGradientFill (juce::ColourGradient (kViolet.withAlpha (0.10f), 240, 0, kViolet.withAlpha (0.0f), 760, 360, true)); g.fillRect (0, 0, w, h);
        // wordmark
        g.setColour (kIce); g.setFont (tech (36.0f, true, 0.06f)); g.drawText ("PRISM", 18, 6, 150, 42, juce::Justification::centredLeft, false);
        g.setColour (kAqua); g.setFont (tech (10.5f, true, 0.42f)); g.drawText ("WAVETABLE", 21, 50, 150, 14, juce::Justification::centredLeft, false);
        for (auto& c : cards)
        {
            auto r = c.r.toFloat();
            auto shape = facetShape (r, 12.0f);
            g.setColour (Colour (0x55000000)); g.fillPath (shape, juce::AffineTransform::translation (0, 2.0f));
            g.setGradientFill (juce::ColourGradient (Colour (0xFF19234A), 0, r.getY(), Colour (0xFF131B3B), 0, r.getBottom(), false)); g.fillPath (shape);
            g.setColour (kEdge); g.strokePath (shape, juce::PathStrokeType (1.0f));
            if (c.title.isNotEmpty())
            {
                g.setColour (kAqua); g.fillRoundedRectangle (r.getX() + 12, r.getY() + 9, 3.0f, 14.0f, 1.5f);
                g.setColour (kIce.withAlpha (0.92f)); g.setFont (sansK (11.0f, true, 0.1f)); g.drawText (c.title, (int) r.getX() + 22, (int) r.getY() + 7, 220, 18, juce::Justification::centredLeft, false);
            }
        }
        g.setColour (kIce.withAlpha (0.4f)); g.setFont (sans (9.5f));
        g.drawText ("20 Hz", 948, 426 + 150, 60, 14, juce::Justification::centredLeft); g.drawText ("20 kHz", 1200, 426 + 150, 60, 14, juce::Justification::centredRight);
        // unison fan
        const int n = juce::jlimit (1, 16, (int) std::round (rv (sb::uni_voices))); const float det = rv (sb::uni_detune);
        for (int i = 0; i < n; ++i)
        {
            const float x = n > 1 ? ((float) i / (float) (n - 1) * 2.0f - 1.0f) : 0.0f;
            const float px = (float) fanRect.getCentreX() + x * (5.0f + det * 40.0f), py = (float) fanRect.getCentreY();
            g.setColour (spectrumAt (n > 1 ? (float) i / (float) (n - 1) : 0.0f, 0.22f)); g.fillEllipse (px - 6.0f, py - 6.0f, 12, 12);
            juce::Path dia; dia.addQuadrilateral (px, py - 4.0f, px + 3.2f, py, px, py + 4.0f, px - 3.2f, py);
            g.setColour (spectrumAt (n > 1 ? (float) i / (float) (n - 1) : 0.0f)); g.fillPath (dia);
        }
    }
};

PrismPanel::PrismPanel (DuoProcessor& p) : impl (std::make_unique<Impl> (*this, p))
{
    setSize (kW, kH);
    impl->build();
}
PrismPanel::~PrismPanel() { stopTimer(); setLookAndFeel (nullptr); }
void PrismPanel::paint (Graphics& g) { impl->paintBack (g, getWidth(), getHeight()); }
void PrismPanel::showTab (int tab) { impl->showTab (juce::jlimit (0, 4, tab)); }
void PrismPanel::visibilityChanged() { if (isVisible()) startTimerHz (30); else stopTimer(); }
void PrismPanel::timerCallback()
{
    impl->spectrum->tick();
   for (auto* w : impl->waves) w->repaint();
    for (auto* c : impl->curves) c->repaint();
    for (auto* k : impl->knobs) if (k->dest != 0 && k->isShowing()) k->repaint();
    for (auto* s : impl->modSliders) if (s->isShowing()) s->repaint();
    if (impl->lfoEd && impl->lfoEd->isShowing()) impl->lfoEd->repaint();
    repaint (impl->fanRect.expanded (10, 4));
}
} // namespace prismui
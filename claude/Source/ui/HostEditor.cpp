#include "HostEditor.h"
#include "../PluginProcessor.h"
#include "../SynthA/ui/SA_Panel.h"
#include "../SynthB/ui/SB_Panel.h"

namespace
{
using juce::Colour; using juce::Graphics; using juce::Rectangle;
const Colour kBg (0xFF1B1C1F), kBar (0xFF232427), kCard (0xFF2A2C30), kLine (0xFF3A3D42), kText (0xFFD6D8DC), kDim (0xFF8E9299), kAccent (0xFFB8BCC4), kEmber (0xFFFFA928), kPrism (0xFF4FD6F0), kWarn (0xFFE5584A);

juce::Font ui (float h, bool bold = false) { return juce::Font (juce::FontOptions ("Segoe UI", h, bold ? juce::Font::bold : juce::Font::plain)); }

// neutral chrome: flat, grey, nothing that competes with either synth's own design
class HostLook : public juce::LookAndFeel_V4
{
public:
    HostLook()
    {
        setColour (juce::ComboBox::backgroundColourId, kCard); setColour (juce::ComboBox::textColourId, kText); setColour (juce::ComboBox::outlineColourId, kLine);
        setColour (juce::PopupMenu::backgroundColourId, kCard); setColour (juce::PopupMenu::textColourId, kText); setColour (juce::PopupMenu::highlightedBackgroundColourId, kLine);
        setColour (juce::Label::textColourId, kText);
        setColour (juce::Slider::textBoxTextColourId, kText); setColour (juce::Slider::textBoxBackgroundColourId, kCard); setColour (juce::Slider::textBoxOutlineColourId, kLine);
        setColour (juce::TextButton::textColourOffId, kText); setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return ui (12.0f, true); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return ui (12.0f); }
    juce::Font getLabelFont (juce::Label&) override { return ui (12.0f); }
    juce::Font getPopupMenuFont() override { return ui (13.0f); }
    void drawButtonBackground (Graphics& g, juce::Button& b, const Colour&, bool hl, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        Colour tint = kAccent;
        if (b.getRadioGroupId() == 11) tint = b.getButtonText() == "EMBER" ? kEmber : kPrism;       // the two synth switches glow in their own colour
        g.setGradientFill (juce::ColourGradient (down ? kBg : kCard.brighter (hl ? 0.22f : 0.10f), 0, r.getY(), kBar.darker (0.25f), 0, r.getBottom(), false)); g.fillRoundedRectangle (r, 6.0f);
        if (on)
        {
            g.setColour (tint.withAlpha (0.20f)); g.fillRoundedRectangle (r, 6.0f);
            g.setColour (tint.withAlpha (0.22f)); g.drawRoundedRectangle (r.expanded (1.5f), 7.0f, 3.0f);
        }
        g.setColour (Colour (0x22FFFFFF)); g.drawLine (r.getX() + 5, r.getY() + 1.0f, r.getRight() - 5, r.getY() + 1.0f, 1.0f);
        g.setColour (on ? tint : (hl ? kDim : kLine)); g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }
    void drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override
    {
        const float d = (float) std::min (w, h) - 6.0f, cx = (float) x + (float) w * 0.5f, cy = (float) y + (float) h * 0.5f, r = d * 0.5f;
        juce::Path tr, vl; tr.addCentredArc (cx, cy, r, r, 0, a0, a1, true); vl.addCentredArc (cx, cy, r, r, 0, a0, a0 + pos * (a1 - a0), true);
        g.setColour (kLine); g.strokePath (tr, juce::PathStrokeType (3.0f)); g.setColour (kAccent); g.strokePath (vl, juce::PathStrokeType (3.0f));
        const float ang = a0 + pos * (a1 - a0);
        g.setColour (kText); g.drawLine (cx, cy, cx + std::sin (ang) * r * 0.7f, cy - std::cos (ang) * r * 0.7f, 2.0f);
    }
    void drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle st, juce::Slider&) override
    {
        if (st == juce::Slider::LinearVertical)
        {
            const float cx = (float) x + (float) w * 0.5f;
            g.setColour (kLine); g.fillRoundedRectangle (cx - 2, (float) y, 4, (float) h, 2); g.setColour (kAccent); g.fillRoundedRectangle (cx - 9, pos - 4, 18, 8, 3);
        }
        else
        {
            const float cy = (float) y + (float) h * 0.5f;
            g.setColour (kLine); g.fillRoundedRectangle ((float) x, cy - 2, (float) w, 4, 2); g.setColour (kAccent); g.fillRoundedRectangle (pos - 4, cy - 8, 8, 16, 3);
        }
    }
};

struct Knob : juce::Component
{
    juce::Slider s { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Label name, val; std::unique_ptr<juce::SliderParameterAttachment> att; juce::RangedAudioParameter& p; juce::String suffix;
    Knob (juce::RangedAudioParameter& param, const juce::String& n, const juce::String& unit = {}) : p (param), suffix (unit)
    {
        addAndMakeVisible (s); addAndMakeVisible (name); addAndMakeVisible (val);
        s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        att = std::make_unique<juce::SliderParameterAttachment> (param, s, nullptr);
        s.setDoubleClickReturnValue (true, param.convertFrom0to1 (param.getDefaultValue()));
        name.setText (n, juce::dontSendNotification); name.setJustificationType (juce::Justification::centred); name.setFont (ui (11.0f)); name.setColour (juce::Label::textColourId, kDim);
        val.setJustificationType (juce::Justification::centred); val.setFont (ui (11.0f));
        name.setInterceptsMouseClicks (false, false); val.setInterceptsMouseClicks (false, false);
        s.onValueChange = [this] { const double v = s.getValue(); val.setText ((std::fabs (v) >= 100 ? juce::String (v, 0) : std::fabs (v) >= 10 ? juce::String (v, 1) : juce::String (v, 2)) + suffix, juce::dontSendNotification); };
        s.onValueChange();
    }
    void resized() override { auto r = getLocalBounds(); name.setBounds (r.removeFromTop (14)); val.setBounds (r.removeFromBottom (14)); s.setBounds (r); }
};

struct Meter : juce::Component
{
    std::function<std::pair<float, float>()> src; float l = 0, r = 0; bool gr = false;
    void tick() { if (! src) return; auto v = src(); l = std::max (v.first, l * 0.82f); r = std::max (v.second, r * 0.82f); repaint(); }
    void paint (Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        auto bar = [&] (Rectangle<float> rc, float level)
        {
            g.setColour (kBg); g.fillRoundedRectangle (rc, 2);
            if (gr) { g.setColour (kWarn); g.fillRoundedRectangle (rc.withWidth (rc.getWidth() * juce::jlimit (0.0f, 1.0f, level / 18.0f)), 2); return; }
            const float db = juce::Decibels::gainToDecibels (level, -60.0f), t = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
            g.setColour (db > -1.0f ? kWarn : (db > -9.0f ? Colour (0xFFE6C34A) : Colour (0xFF6CCB7A)));
            g.fillRoundedRectangle (rc.withWidth (rc.getWidth() * t), 2);
        };
        if (gr) bar (b, l); else { bar (b.removeFromTop (b.getHeight() * 0.5f - 1), l); b.removeFromTop (2); bar (b, r); }
    }
};
// window background: graphite bars with a thin amber-to-aqua line that ties the two synths together
struct Root : juce::Component
{
    void paint (Graphics& g) override
    {
        const float w = (float) getWidth();
        g.fillAll (kBg);
        g.setGradientFill (juce::ColourGradient (Colour (0xFF30323A), 0, 0, Colour (0xFF15161A), 0, 46, false)); g.fillRect (0.0f, 0.0f, w, 46.0f);
        g.setColour (Colour (0x20FFFFFF)); g.fillRect (0.0f, 0.0f, w, 1.0f);
        g.setGradientFill (juce::ColourGradient (kEmber.withAlpha (0.75f), 0, 0, kPrism.withAlpha (0.75f), w, 0, false)); g.fillRect (0.0f, 45.0f, w, 1.0f);
        g.setGradientFill (juce::ColourGradient (Colour (0xFF24262B), 0, 766, Colour (0xFF121316), 0, (float) getHeight(), false)); g.fillRect (0.0f, 766.0f, w, (float) getHeight() - 766.0f);
        g.setGradientFill (juce::ColourGradient (kEmber.withAlpha (0.75f), 0, 0, kPrism.withAlpha (0.75f), w, 0, false)); g.fillRect (0.0f, 766.0f, w, 1.0f);
        g.setColour (Colour (0x30000000)); g.fillRect (0.0f, 767.0f, w, 3.0f);
    }
};
} // namespace

struct DuoEditor::Impl
{
    DuoProcessor& proc; DuoEditor& ed; HostLook look;
    Root root;
    emberui::EmberPanel ember; prismui::PrismPanel prism;
    juce::TextButton swA { "EMBER" }, swB { "PRISM" }, mLayer { "LAYER" }, mSplit { "SPLIT" }, mChan { "CHANNEL" }, tap { "TAP" }, panicB { "PANIC" }, saveB { "SAVE" }, loadB { "LOAD" }, masterB { "MASTER" }, swap { "SWAP" };
    juce::ToggleButton syncT { "DAW" };
    juce::Slider bpm { juce::Slider::LinearBarVertical, juce::Slider::TextBoxLeft };
    std::unique_ptr<juce::SliderParameterAttachment> bpmAtt;
    std::unique_ptr<juce::ButtonParameterAttachment> syncAtt;
    juce::Slider splitS { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft }, chA { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft }, chB { juce::Slider::IncDecButtons, juce::Slider::TextBoxLeft };
    std::unique_ptr<juce::SliderParameterAttachment> splitAtt, chAAtt, chBAtt;
    juce::Label routeInfo, tempoLabel;
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::FileChooser> chooser;

    struct Strip : juce::Component
    {
        juce::String name; Colour col;
        juce::Slider vol { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight }, pan { juce::Slider::LinearHorizontal, juce::Slider::NoTextBox };
        juce::TextButton mute { "M" }, solo { "S" };
        std::unique_ptr<juce::SliderParameterAttachment> va, pa; std::unique_ptr<juce::ButtonParameterAttachment> ma, sa_;
        Meter meter;
        Strip (DuoProcessor& p, const juce::String& n, Colour c, const juce::String& suffix) : name (n), col (c)
        {
            addAndMakeVisible (vol); addAndMakeVisible (pan); addAndMakeVisible (mute); addAndMakeVisible (solo); addAndMakeVisible (meter);
            va = std::make_unique<juce::SliderParameterAttachment> (*p.hostParam ("host_vol" + suffix), vol, nullptr);
            pa = std::make_unique<juce::SliderParameterAttachment> (*p.hostParam ("host_pan" + suffix), pan, nullptr);
            ma = std::make_unique<juce::ButtonParameterAttachment> (*p.hostParam ("host_mute" + suffix), mute, nullptr);
            sa_ = std::make_unique<juce::ButtonParameterAttachment> (*p.hostParam ("host_solo" + suffix), solo, nullptr);
            vol.setNumDecimalPlacesToDisplay (1); vol.setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20); vol.setDoubleClickReturnValue (true, 0.0);
            vol.textFromValueFunction = [] (double v) { return v <= -59.5 ? juce::String ("-inf dB") : juce::String (v, 1) + " dB"; }; vol.valueFromTextFunction = [] (const juce::String& s) { return s.getDoubleValue(); }; vol.updateText(); pan.setDoubleClickReturnValue (true, 0.0);
            mute.setClickingTogglesState (true); solo.setClickingTogglesState (true);
        }
        void resized() override
        {
            auto r = getLocalBounds().reduced (8, 6);
            r.removeFromLeft (70);
            meter.setBounds (r.removeFromBottom (16).reduced (0, 3));
            auto top = r.removeFromTop (30);
            solo.setBounds (top.removeFromRight (30).reduced (0, 2)); top.removeFromRight (4); mute.setBounds (top.removeFromRight (30).reduced (0, 2)); top.removeFromRight (8);
            pan.setBounds (top.removeFromRight (62)); top.removeFromRight (6);
            vol.setBounds (top);
        }        void paint (Graphics& g) override
        {
            auto r = getLocalBounds().toFloat().reduced (2);
            g.setGradientFill (juce::ColourGradient (kCard.brighter (0.08f), 0, r.getY(), kBar.darker (0.2f), 0, r.getBottom(), false)); g.fillRoundedRectangle (r, 6);
            g.setGradientFill (juce::ColourGradient (col.withAlpha (0.18f), r.getX(), 0, col.withAlpha (0.0f), r.getX() + 120, 0, false)); g.fillRoundedRectangle (r, 6);
            g.setColour (kLine); g.drawRoundedRectangle (r, 6, 1);
            g.setColour (col); g.fillRoundedRectangle (r.getX() + 1, r.getY() + 8, 3.0f, r.getHeight() - 16, 1.5f);
            g.setColour (col.withAlpha (0.3f)); g.fillEllipse (11, 11, 15, 15);
            g.setColour (col); g.fillEllipse (14, 14, 9, 9);
            g.setColour (kText); g.setFont (ui (13.0f, true)); g.drawText (name, 28, 8, 50, 22, juce::Justification::centredLeft);
            g.setColour (kDim); g.setFont (ui (10.0f)); g.drawText ("PAN", pan.getX(), pan.getBottom() - 2, pan.getWidth(), 10, juce::Justification::centred); g.drawText ("VOLUME", vol.getX(), vol.getBottom() - 2, 80, 10, juce::Justification::centredLeft);
        }
    };
    Strip stripA, stripB;
    Meter masterMeter;

    // ---- master drawer
    struct Drawer : juce::Component
    {
        DuoProcessor& proc; juce::OwnedArray<juce::Component> items; Meter gr, lim, mm;
        juce::ToggleButton cOn { "ON" }, rOn { "ON" }; std::unique_ptr<juce::ButtonParameterAttachment> cAtt, rAtt;
        Drawer (DuoProcessor& p) : proc (p)
        {
            auto k = [&] (const char* id, const char* n, int x, int y, const char* unit = "") { auto* c = new Knob (*proc.hostParam (id), n, unit); items.add (c); addAndMakeVisible (c); c->setBounds (x, y, 64, 72); };
            addAndMakeVisible (cOn); cOn.setBounds (150, 14, 60, 20); cAtt = std::make_unique<juce::ButtonParameterAttachment> (*proc.hostParam ("host_c_on"), cOn, nullptr);
            k ("host_c_thr", "Thresh", 10, 44, " dB"); k ("host_c_ratio", "Ratio", 80, 44, ":1"); k ("host_c_att", "Attack", 150, 44, " ms"); k ("host_c_rel", "Release", 220, 44, " ms");
            k ("host_c_knee", "Knee", 290, 44, " dB"); k ("host_c_make", "Makeup", 10, 120, " dB"); k ("host_c_mix", "Dry/Wet", 80, 120);
            addAndMakeVisible (gr); gr.gr = true; gr.setBounds (160, 146, 230, 14);
            gr.src = [this] { const float v = proc.meters().gr; return std::make_pair (v, v); };
            addAndMakeVisible (rOn); rOn.setBounds (150, 194, 60, 20); rAtt = std::make_unique<juce::ButtonParameterAttachment> (*proc.hostParam ("host_r_on"), rOn, nullptr);
            k ("host_r_size", "Size", 10, 224); k ("host_r_decay", "Decay", 80, 224, " s"); k ("host_r_damp", "Damping", 150, 224); k ("host_r_pre", "Pre-delay", 220, 224, " ms");
            k ("host_r_low", "Low cut", 290, 224, " Hz"); k ("host_r_wet", "Wet", 10, 300);
            addAndMakeVisible (lim); lim.gr = true; lim.setBounds (160, 346, 230, 14);
            lim.src = [this] { const float v = proc.meters().limGr; return std::make_pair (v * 3.0f, v * 3.0f); };
            k ("host_master", "Master", 300, 410, " dB");
            addAndMakeVisible (mm); mm.setBounds (10, 440, 270, 24);
            mm.src = [this] { return std::make_pair ((float) proc.meters().mL, (float) proc.meters().mR); };
        }
        void paint (Graphics& g) override
        {
            g.fillAll (kBar.withAlpha (0.98f)); g.setColour (kLine); g.drawVerticalLine (0, 0, (float) getHeight());
            g.setColour (kText); g.setFont (ui (13.0f, true));
            g.drawText ("GLUE COMPRESSOR", 10, 12, 140, 22, juce::Justification::centredLeft);
            g.drawText ("MASTER REVERB", 10, 192, 140, 22, juce::Justification::centredLeft);
            g.drawText ("SAFETY LIMITER", 10, 340, 140, 22, juce::Justification::centredLeft);
            g.drawText ("MASTER OUT", 10, 410, 140, 22, juce::Justification::centredLeft);
            g.setColour (kDim); g.setFont (ui (11.0f));
            g.drawText ("GR", 130, 144, 26, 16, juce::Justification::centredRight); g.drawText ("GR", 130, 344, 26, 16, juce::Justification::centredRight);
            g.drawText ("Ceiling -0.3 dBFS, 2 ms look-ahead, fixed.", 10, 366, 380, 16, juce::Justification::centredLeft);
            g.drawText ("Chain: A+B -> glue comp -> reverb (parallel) -> limiter -> master volume", 10, 480, 380, 16, juce::Justification::centredLeft);
            g.setColour (kLine);
            for (int y : { 180, 330, 400 }) g.drawHorizontalLine (y, 10.0f, (float) getWidth() - 10);
        }
    };
    Drawer drawer;

    Impl (DuoEditor& e, DuoProcessor& p)
        : proc (p), ed (e), ember (p), prism (p),
          keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard),
          stripA (p, "EMBER", kEmber, "A"), stripB (p, "PRISM", kPrism, "B"), drawer (p) {}

    void setParamNorm (const juce::String& id, float n) { if (auto* pr = proc.hostParam (id)) { pr->beginChangeGesture(); pr->setValueNotifyingHost (n); pr->endChangeGesture(); } }

    void build()
    {
        root.setLookAndFeel (&look);
        ed.addAndMakeVisible (root);
        root.setBounds (0, 0, kW, kH);

        root.addAndMakeVisible (ember); root.addAndMakeVisible (prism);
        ember.setBounds (0, 46, 1280, 720); prism.setBounds (0, 46, 1280, 720);

        // panel switch
        for (auto* b : { &swA, &swB }) { root.addChildComponent (b); b->setVisible (true); b->setClickingTogglesState (false); b->setRadioGroupId (11); }
        swA.setBounds (212, 8, 92, 30); swB.setBounds (308, 8, 92, 30);
        swA.onClick = [this] { showPanel (0); }; swB.onClick = [this] { showPanel (1); };

        // routing
        for (auto* b : { &mLayer, &mSplit, &mChan }) { root.addAndMakeVisible (b); b->setClickingTogglesState (false); }
        mLayer.setBounds (420, 8, 62, 30); mSplit.setBounds (486, 8, 62, 30); mChan.setBounds (552, 8, 76, 30);
        mLayer.onClick = [this] { setParamNorm ("host_mode", 0.0f); }; mSplit.onClick = [this] { setParamNorm ("host_mode", 0.5f); }; mChan.onClick = [this] { setParamNorm ("host_mode", 1.0f); };
        root.addAndMakeVisible (routeInfo); routeInfo.setBounds (634, 8, 30, 30); routeInfo.setVisible (false);
        auto stepper = [&] (juce::Slider& s, const char* id, int x, int w) { root.addAndMakeVisible (s); s.setBounds (x, 10, w, 26); s.setTextBoxStyle (juce::Slider::TextBoxLeft, false, w - 40, 26); (void) id; };
        stepper (splitS, "host_split", 636, 96); stepper (chA, "host_chA", 636, 72); stepper (chB, "host_chB", 716, 72);
        splitAtt = std::make_unique<juce::SliderParameterAttachment> (*proc.hostParam ("host_split"), splitS, nullptr);
        chAAtt = std::make_unique<juce::SliderParameterAttachment> (*proc.hostParam ("host_chA"), chA, nullptr);
        chBAtt = std::make_unique<juce::SliderParameterAttachment> (*proc.hostParam ("host_chB"), chB, nullptr);
        splitS.textFromValueFunction = [] (double v) { return juce::MidiMessage::getMidiNoteName ((int) v, true, true, 3); };
        chA.textFromValueFunction = [] (double v) { return v < 0.5 ? juce::String ("A:omni") : "A:ch" + juce::String ((int) v); };
        chB.textFromValueFunction = [] (double v) { return v < 0.5 ? juce::String ("B:omni") : "B:ch" + juce::String ((int) v); };
        splitS.updateText(); chA.updateText(); chB.updateText();
        root.addAndMakeVisible (swap); swap.setBounds (736, 8, 54, 30);
        swap.onClick = [this] { auto* p = proc.hostParam ("host_swap"); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); };

        // tempo
        root.addAndMakeVisible (bpm); bpm.setSliderStyle (juce::Slider::IncDecButtons); bpm.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 56, 26); bpm.setBounds (812, 10, 90, 26);
        bpmAtt = std::make_unique<juce::SliderParameterAttachment> (*proc.hostParam ("host_bpm"), bpm, nullptr); bpm.setTextValueSuffix (" bpm");
        root.addAndMakeVisible (tap); tap.setBounds (908, 8, 42, 30); tap.onClick = [this] { proc.tapTempo(); };
        root.addAndMakeVisible (syncT); syncT.setBounds (954, 12, 62, 22); syncAtt = std::make_unique<juce::ButtonParameterAttachment> (*proc.hostParam ("host_sync"), syncT, nullptr);
        syncT.setTooltip ("Follow the DAW tempo (VST3). In the standalone app the BPM field is used.");

        root.addAndMakeVisible (panicB); panicB.setBounds (1022, 8, 60, 30); panicB.setColour (juce::TextButton::textColourOffId, kWarn); panicB.onClick = [this] { proc.requestPanic(); };
        root.addAndMakeVisible (saveB); saveB.setBounds (1090, 8, 50, 30); saveB.onClick = [this] { saveSession(); };
        root.addAndMakeVisible (loadB); loadB.setBounds (1144, 8, 50, 30); loadB.onClick = [this] { loadSession(); };
        root.addAndMakeVisible (masterB); masterB.setBounds (1200, 8, 72, 30); masterB.setClickingTogglesState (true);
        masterB.onClick = [this] { drawer.setVisible (masterB.getToggleState()); };

        // mixer + keyboard
        root.addAndMakeVisible (stripA); root.addAndMakeVisible (stripB);
        stripA.setBounds (6, 770, 382, 88); stripB.setBounds (392, 770, 382, 88);
        stripA.meter.src = [this] { return std::make_pair ((float) proc.meters().aL, (float) proc.meters().aR); };
        stripB.meter.src = [this] { return std::make_pair ((float) proc.meters().bL, (float) proc.meters().bR); };
        root.addAndMakeVisible (keyboard);
        keyboard.setBounds (780, 770, 494, 88); keyboard.setAvailableRange (24, 108); keyboard.setKeyWidth (22.0f); keyboard.setLowestVisibleKey (48); keyboard.setScrollButtonsVisible (true);
        keyboard.setMidiChannel (1); keyboard.setKeyPressBaseOctave (3);
        keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, Colour (0xFFEDEAE2)); keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, Colour (0xFF17181B));
        keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, kAccent.withAlpha (0.8f)); keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, kAccent.withAlpha (0.3f));
        keyboard.setWantsKeyboardFocus (true);

        // master drawer (slides over the panel's right side)
        root.addChildComponent (drawer); drawer.setBounds (kW - 400, 46, 400, 520);

        showPanel (proc.activePanel());
        refreshRouting();
    }

    void showPanel (int p)
    {
        proc.setActivePanel (p);
        ember.setVisible (p == 0); prism.setVisible (p == 1);
        swA.setToggleState (p == 0, juce::dontSendNotification); swB.setToggleState (p == 1, juce::dontSendNotification);
        keyboard.grabKeyboardFocus();
    }

    void refreshRouting()
    {
        const int mode = (int) (proc.rawHost ("host_mode") + 0.5f);
        mLayer.setToggleState (mode == 0, juce::dontSendNotification); mSplit.setToggleState (mode == 1, juce::dontSendNotification); mChan.setToggleState (mode == 2, juce::dontSendNotification);
        splitS.setVisible (mode == 1); swap.setVisible (mode == 1); chA.setVisible (mode == 2); chB.setVisible (mode == 2);
        swap.setButtonText (proc.rawHost ("host_swap") > 0.5f ? "B<A" : "A<B");
    }

    void saveSession()
    {
        chooser = std::make_unique<juce::FileChooser> ("Save session", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("aether-duo-session.json"), "*.json");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting, [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult(); if (f == juce::File()) return;
            f.replaceWithText (proc.sessionToJson());
        });
    }
    void loadSession()
    {
        chooser = std::make_unique<juce::FileChooser> ("Load session", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.json");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult(); if (! f.existsAsFile()) return;
            juce::String err;
            if (! proc.sessionFromJson (f.loadFileAsString(), err)) juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Could not load session", err);
        });
    }
};

DuoEditor::DuoEditor (DuoProcessor& p) : juce::AudioProcessorEditor (&p), impl (std::make_unique<Impl> (*this, p))
{
    setOpaque (true);
    impl->build();
    setResizable (true, true);
    setResizeLimits (640, 431, 2560, 1724);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setSize (kW, kH);
    startTimerHz (30);
}
DuoEditor::~DuoEditor() { stopTimer(); impl->root.setLookAndFeel (nullptr); }

void DuoEditor::resized()
{
    const float s = (float) getWidth() / (float) kW;
    impl->root.setTransform (juce::AffineTransform::scale (s));
}
void DuoEditor::mouseDown (const juce::MouseEvent&) { impl->keyboard.grabKeyboardFocus(); }
void DuoEditor::showPanel (int panel) { impl->showPanel (panel); }
void DuoEditor::showPrismTab (int tab) { impl->prism.showTab (tab); }
void DuoEditor::setDrawerOpen (bool open) { impl->masterB.setToggleState (open, juce::dontSendNotification); impl->drawer.setVisible (open); }

void DuoEditor::timerCallback()
{
    impl->stripA.meter.tick(); impl->stripB.meter.tick();
    if (impl->drawer.isVisible()) { impl->drawer.gr.tick(); impl->drawer.lim.tick(); impl->drawer.mm.tick(); }
    impl->refreshRouting();
}

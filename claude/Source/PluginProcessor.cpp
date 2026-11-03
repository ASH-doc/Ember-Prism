#include "PluginProcessor.h"
#include "ui/HostEditor.h"

namespace
{
using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;

bool isIntParam (const juce::String& id)
{
    static const char* ends[] = { "_oct", "_semi", "_fine", "uni_voices", "bend_range", "arp_oct", "seq_len" };
    for (auto* e : ends) if (id.endsWith (e)) return true;
    if (id.startsWith ("sb_sp") && id.length() <= 7 && id.substring (5).containsOnly ("0123456789")) return true;
    return false;
}

template <class DefT>
void addSynthParams (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& v, const DefT* defs, int count)
{
    for (int i = 0; i < count; ++i)
    {
        const auto& d = defs[i];
        const juce::String id (d.id);
        if (d.choices[0] != 0)
        {
            v.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, d.name, juce::StringArray::fromTokens (d.choices, "|", ""), (int) std::lround (d.def)));
        }
        else
        {
            const bool isInt = isIntParam (id);
            juce::NormalisableRange<float> r (d.mn, d.mx, isInt ? 1.0f : 0.0f, d.skew);
            v.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, d.name, r, d.def));
        }
    }
}

void addFloat (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& v, const char* id, const char* name, float mn, float mx, float def, float skew = 1.0f, float interval = 0.0f)
{
    v.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (mn, mx, interval, skew), def));
}
void addChoice (std::vector<std::unique_ptr<juce::RangedAudioParameter>>& v, const char* id, const char* name, const char* items, int def)
{
    v.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, juce::StringArray::fromTokens (items, "|", ""), def));
}
} // namespace

Layout DuoProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
    addSynthParams (v, sa::defs(), sa::COUNT);
    addSynthParams (v, sb::defs(), sb::COUNT);
    // ---- host: mixer
    addFloat (v, "host_volA", "Mixer A Volume", -60.0f, 6.0f, 0.0f, 2.2f); addFloat (v, "host_volB", "Mixer B Volume", -60.0f, 6.0f, 0.0f, 2.2f);
    addFloat (v, "host_panA", "Mixer A Pan", -1.0f, 1.0f, 0.0f);          addFloat (v, "host_panB", "Mixer B Pan", -1.0f, 1.0f, 0.0f);
    addChoice (v, "host_muteA", "Mute A", "Off|On", 0); addChoice (v, "host_muteB", "Mute B", "Off|On", 0);
    addChoice (v, "host_soloA", "Solo A", "Off|On", 0); addChoice (v, "host_soloB", "Solo B", "Off|On", 0);
    // ---- host: routing
    addChoice (v, "host_mode", "Routing Mode", "Layer|Split|Channel", 0);
    addFloat (v, "host_split", "Split Note", 0.0f, 127.0f, 60.0f, 1.0f, 1.0f);
    addChoice (v, "host_swap", "Split Swap", "A low / B high|B low / A high", 0);
    addFloat (v, "host_chA", "Channel A (0 = omni)", 0.0f, 16.0f, 1.0f, 1.0f, 1.0f); addFloat (v, "host_chB", "Channel B (0 = omni)", 0.0f, 16.0f, 2.0f, 1.0f, 1.0f);
    // ---- host: master
    addFloat (v, "host_master", "Master Volume", -60.0f, 0.0f, -1.0f, 2.0f);
    addChoice (v, "host_c_on", "Glue On", "Off|On", 1);
    addFloat (v, "host_c_thr", "Glue Threshold", -40.0f, 0.0f, -14.0f);
    addFloat (v, "host_c_ratio", "Glue Ratio", 1.5f, 4.0f, 2.0f);
    addFloat (v, "host_c_att", "Glue Attack", 1.0f, 100.0f, 10.0f, 0.5f);
    addFloat (v, "host_c_rel", "Glue Release", 30.0f, 1000.0f, 150.0f, 0.5f);
    addFloat (v, "host_c_knee", "Glue Knee", 0.0f, 12.0f, 6.0f);
    addFloat (v, "host_c_make", "Glue Makeup", 0.0f, 12.0f, 1.0f);
    addFloat (v, "host_c_mix", "Glue Mix", 0.0f, 1.0f, 1.0f);
    addChoice (v, "host_r_on", "Reverb On", "Off|On", 1);
    addFloat (v, "host_r_size", "Reverb Size", 0.0f, 1.0f, 0.5f);
    addFloat (v, "host_r_decay", "Reverb Decay", 0.3f, 10.0f, 2.0f, 0.5f);
    addFloat (v, "host_r_damp", "Reverb Damping", 0.0f, 1.0f, 0.5f);
    addFloat (v, "host_r_pre", "Reverb Pre-delay", 0.0f, 100.0f, 15.0f);
    addFloat (v, "host_r_low", "Reverb Low Cut", 20.0f, 1000.0f, 200.0f, 0.4f);
    addFloat (v, "host_r_wet", "Reverb Wet", 0.0f, 1.0f, 0.1f, 0.7f);
    addFloat (v, "host_bpm", "Tempo", 40.0f, 220.0f, 138.0f, 1.0f, 0.1f);
    addChoice (v, "host_sync", "Follow DAW tempo", "Off|On", 1);
    return { v.begin(), v.end() };
}

DuoProcessor::DuoProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AETHERDUO", createLayout()),
      synA (std::make_unique<sa::Engine>()), synB (std::make_unique<sb::Engine>())
{
    for (int i = 0; i < sa::COUNT; ++i) { pA.push_back (static_cast<juce::RangedAudioParameter*> (apvts.getParameter (sa::defs()[i].id))); rawA.push_back (apvts.getRawParameterValue (sa::defs()[i].id)); }
    for (int i = 0; i < sb::COUNT; ++i) { pB.push_back (static_cast<juce::RangedAudioParameter*> (apvts.getParameter (sb::defs()[i].id))); rawB.push_back (apvts.getRawParameterValue (sb::defs()[i].id)); }
    snapA.assign ((size_t) sa::COUNT, 0.0f); snapB.assign ((size_t) sb::COUNT, 0.0f);
    auto r = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    hr = { r ("host_volA"), r ("host_volB"), r ("host_panA"), r ("host_panB"), r ("host_muteA"), r ("host_muteB"), r ("host_soloA"), r ("host_soloB"), r ("host_mode"), r ("host_split"), r ("host_swap"),
           r ("host_chA"), r ("host_chB"), r ("host_master"), r ("host_c_on"), r ("host_c_thr"), r ("host_c_ratio"), r ("host_c_att"), r ("host_c_rel"), r ("host_c_knee"), r ("host_c_make"), r ("host_c_mix"),
           r ("host_r_on"), r ("host_r_size"), r ("host_r_decay"), r ("host_r_damp"), r ("host_r_pre"), r ("host_r_low"), r ("host_r_wet"), r ("host_bpm"), r ("host_sync") };
    apvts.state.setProperty ("panel", 0, nullptr);
}

DuoProcessor::~DuoProcessor() = default;

bool DuoProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() && l.getMainInputChannelSet().isDisabled();
}

void DuoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    host.prepare (sampleRate, samplesPerBlock, synA.get(), synB.get());
    setLatencySamples (host.latencySamples());
    pushExtrasToEngines();
}

void DuoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();

    for (int i = 0; i < sa::COUNT; ++i) snapA[(size_t) i] = *rawA[(size_t) i];
    for (int i = 0; i < sb::COUNT; ++i) snapB[(size_t) i] = *rawB[(size_t) i];
    synA->setParamSnapshot (snapA.data());
    synB->setParamSnapshot (snapB.data());

    // computer keyboard / on-screen keyboard share the same MIDI path as the hardware input
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    if (panicFlag.exchange (false)) host.panic();

    duo::HostParams hp;
    hp.gainA = juce::Decibels::decibelsToGain ((float) *hr.volA, -60.0f); hp.gainB = juce::Decibels::decibelsToGain ((float) *hr.volB, -60.0f);
    if (*hr.volA <= -59.9f) hp.gainA = 0.0f;
    if (*hr.volB <= -59.9f) hp.gainB = 0.0f;
    hp.panA = *hr.panA; hp.panB = *hr.panB;
    hp.muteA = *hr.muteA > 0.5f; hp.muteB = *hr.muteB > 0.5f; hp.soloA = *hr.soloA > 0.5f; hp.soloB = *hr.soloB > 0.5f;
    hp.mode = (int) (*hr.mode + 0.5f); hp.splitNote = (int) (*hr.split + 0.5f); hp.splitSwap = *hr.swap > 0.5f;
    hp.channelA = (int) (*hr.chA + 0.5f); hp.channelB = (int) (*hr.chB + 0.5f);
    hp.masterGain = juce::Decibels::decibelsToGain ((float) *hr.master, -60.0f);
    hp.comp.on = *hr.cOn > 0.5f; hp.comp.thresholdDb = *hr.cThr; hp.comp.ratio = *hr.cRatio; hp.comp.attackMs = *hr.cAtt; hp.comp.releaseMs = *hr.cRel;
    hp.comp.kneeDb = *hr.cKnee; hp.comp.makeupDb = *hr.cMake; hp.comp.mix = *hr.cMix;
    hp.reverb.on = *hr.rOn > 0.5f; hp.reverb.size = *hr.rSize; hp.reverb.decay = *hr.rDecay; hp.reverb.damp = *hr.rDamp;
    hp.reverb.preDelayMs = *hr.rPre; hp.reverb.lowCutHz = *hr.rLow; hp.reverb.wet = *hr.rWet;

    duo::TransportInfo tr;
    tr.bpm = *hr.bpm;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (*hr.sync > 0.5f)
                if (auto bpm = pos->getBpm()) tr.bpm = *bpm;
            tr.playing = pos->getIsPlaying();
            if (auto ppq = pos->getPpqPosition()) tr.ppq = *ppq;
        }

    host.processBlock (buffer.getWritePointer (0), buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : buffer.getWritePointer (0), n, midi, tr, hp);
    midi.clear();
}

juce::AudioProcessorEditor* DuoProcessor::createEditor() { return new DuoEditor (*this); }

// ------------------------------------------------------------------------------------ presets and extras
static void applyValues (const std::vector<juce::RangedAudioParameter*>& params, const std::vector<float>& vals)
{
    for (size_t i = 0; i < params.size(); ++i)
    {
        auto* p = params[i];
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (vals[i]));
        p->endChangeGesture();
    }
}

void DuoProcessor::loadPresetA (int index)
{
    const int n = (int) sa::presets().size();
    index = std::max (0, std::min (n - 1, index));
    applyValues (pA, sa::presetValues (index));
    apvts.state.setProperty ("presetA", index, nullptr);
}

void DuoProcessor::loadPresetB (int index)
{
    const int n = (int) sb::presets().size();
    index = std::max (0, std::min (n - 1, index));
    applyValues (pB, sb::presetValues (index));
    for (int l = 0; l < 4; ++l) for (int p = 0; p < 16; ++p) setCustomShapeB (l, p, std::sin (2.0f * 3.14159265f * (float) p / 16.0f));
    for (auto& s : sb::presets()[(size_t) index].shapes)
        for (int p = 0; p < 16 && p < (int) s.second.size(); ++p) setCustomShapeB (s.first, p, s.second[(size_t) p]);
    apvts.state.setProperty ("presetB", index, nullptr);
}

void DuoProcessor::setCustomShapeB (int lfo, int point, float v)
{
    synB->setCustomShape (lfo, point, v);
    juce::String s;
    for (int l = 0; l < 4; ++l) for (int p = 0; p < 16; ++p) s << juce::String (synB->getCustomShape (l, p), 3) << (l == 3 && p == 15 ? "" : ",");
    apvts.state.setProperty ("shapesB", s, nullptr);
}

void DuoProcessor::learnChordA()
{
    synA->learnChord();
    apvts.state.setProperty ("chordA", chordTextA(), nullptr);
}

juce::String DuoProcessor::chordTextA() const
{
    juce::StringArray sa_;
    for (int iv : synA->getChord()) sa_.add (juce::String (iv));
    return sa_.joinIntoString (",");
}

void DuoProcessor::pushExtrasToEngines()
{
    const auto shapes = apvts.state.getProperty ("shapesB").toString();
    if (shapes.isNotEmpty())
    {
        auto t = juce::StringArray::fromTokens (shapes, ",", "");
        for (int i = 0; i < 64 && i < t.size(); ++i) synB->setCustomShape (i / 16, i % 16, t[i].getFloatValue());
    }
    std::vector<int> chord;
    for (auto& s : juce::StringArray::fromTokens (apvts.state.getProperty ("chordA").toString(), ",", "")) if (s.isNotEmpty()) chord.push_back (s.getIntValue());
    synA->setChord (chord);
}

void DuoProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void DuoProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            pushExtrasToEngines();
        }
}

void DuoProcessor::tapTempo()
{
    const double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
    if (! tapTimes.empty() && now - tapTimes.back() > 2.0) tapTimes.clear();
    tapTimes.push_back (now);
    if (tapTimes.size() > 6) tapTimes.erase (tapTimes.begin());
    if (tapTimes.size() >= 2)
    {
        const double avg = (tapTimes.back() - tapTimes.front()) / (double) (tapTimes.size() - 1);
        if (auto* p = apvts.getParameter ("host_bpm")) p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (40.0, 220.0, 60.0 / avg)));
        if (auto* p = apvts.getParameter ("host_sync")) p->setValueNotifyingHost (0.0f);    // tapping means "use my tempo"
    }
}

// ------------------------------------------------------------------------------------ sessions (JSON)
static double sig6 (float v) { return juce::String::toDecimalStringWithSignificantFigures ((double) v, 6).getDoubleValue(); }

juce::var DuoProcessor::patchToVar (int engine)
{
    auto* o = new juce::DynamicObject();
    auto* params = new juce::DynamicObject();
    juce::Array<juce::var> mod, fx;
    const bool isA = engine == 0;
    o->setProperty ("format", "dualsynth-patch"); o->setProperty ("version", 1); o->setProperty ("engine", isA ? "A" : "B");
    const int idx = isA ? presetIndexA() : presetIndexB();
    o->setProperty ("name", isA ? juce::String (sa::presets()[(size_t) std::min (idx, (int) sa::presets().size() - 1)].name) : juce::String (sb::presets()[(size_t) std::min (idx, (int) sb::presets().size() - 1)].name));
    o->setProperty ("category", isA ? juce::String (sa::presets()[(size_t) std::min (idx, (int) sa::presets().size() - 1)].category) : juce::String (sb::presets()[(size_t) std::min (idx, (int) sb::presets().size() - 1)].category));
    o->setProperty ("tags", juce::Array<juce::var>());
    const int count = isA ? sa::COUNT : sb::COUNT;
    for (int i = 0; i < count; ++i)
    {
        const char* id = isA ? sa::defs()[i].id : sb::defs()[i].id;
        params->setProperty (juce::Identifier (id), sig6 (*(isA ? rawA : rawB)[(size_t) i]));
    }
    const int slots = isA ? 8 : 10;
    for (int s = 0; s < slots; ++s)
    {
        const int base = isA ? sa::m1_src + 3 * s : sb::m1_src + 3 * s;
        const int src = (int) (*(isA ? rawA : rawB)[(size_t) base] + 0.5f), dst = (int) (*(isA ? rawA : rawB)[(size_t) base + 1] + 0.5f);
        if (src > 0 && dst > 0)
        {
            auto* m = new juce::DynamicObject();
            m->setProperty ("src", src); m->setProperty ("dst", dst); m->setProperty ("amount", sig6 (*(isA ? rawA : rawB)[(size_t) base + 2]));
            mod.add (juce::var (m));
        }
    }
    if (isA)
    {
        const char* names[] = { "drive", "chorus", "phaser", "delay", "reverb", "eq", "pump" };
        const int ons[] = { sa::fx_drv_on, sa::fx_ch_on, sa::fx_ph_on, sa::fx_dl_on, sa::fx_rv_on, sa::fx_eq_on, sa::fx_pump_on };
        for (int i = 0; i < 7; ++i) { auto* f = new juce::DynamicObject(); f->setProperty ("type", names[i]); f->setProperty ("on", *rawA[(size_t) ons[i]] > 0.5f); fx.add (juce::var (f)); }
        o->setProperty ("chord", chordTextA());
    }
    else
    {
        for (int r = 0; r < 8; ++r)
        {
            auto* f = new juce::DynamicObject(); const int b = sb::r1_type + 6 * r;
            f->setProperty ("slot", r + 1); f->setProperty ("type", (int) (*rawB[(size_t) b] + 0.5f)); f->setProperty ("on", *rawB[(size_t) b + 1] > 0.5f);
            auto* fp = new juce::DynamicObject(); fp->setProperty ("a", sig6 (*rawB[(size_t) b + 2])); fp->setProperty ("b", sig6 (*rawB[(size_t) b + 3])); fp->setProperty ("c", sig6 (*rawB[(size_t) b + 4])); fp->setProperty ("mix", sig6 (*rawB[(size_t) b + 5]));
            f->setProperty ("params", juce::var (fp)); fx.add (juce::var (f));
        }
        juce::Array<juce::var> shapes;
        for (int i = 0; i < 64; ++i) shapes.add (sig6 (synB->getCustomShape (i / 16, i % 16)));
        o->setProperty ("lfoShapes", shapes);
    }
    o->setProperty ("params", juce::var (params)); o->setProperty ("mod", mod); o->setProperty ("fx", fx);
    return juce::var (o);
}

void DuoProcessor::patchFromVar (int engine, const juce::var& v)
{
    if (! v.isObject()) return;
    const bool isA = engine == 0;
    auto params = v.getProperty ("params", {});
    if (auto* po = params.getDynamicObject())
    {
        const int count = isA ? sa::COUNT : sb::COUNT;
        for (int i = 0; i < count; ++i)
        {
            const char* id = isA ? sa::defs()[i].id : sb::defs()[i].id;
            if (po->hasProperty (juce::Identifier (id)))
            {
                auto* p = (isA ? pA : pB)[(size_t) i];
                p->setValueNotifyingHost (p->convertTo0to1 ((float) (double) po->getProperty (juce::Identifier (id))));
            }
        }
    }
    {
        const juce::String nm = v.getProperty ("name", "").toString(), cat = v.getProperty ("category", "").toString();
        const int n = isA ? (int) sa::presets().size() : (int) sb::presets().size();
        for (int i = 0; i < n; ++i)
        {
            const juce::String pn = isA ? juce::String (sa::presets()[(size_t) i].name) : juce::String (sb::presets()[(size_t) i].name);
            const juce::String pc = isA ? juce::String (sa::presets()[(size_t) i].category) : juce::String (sb::presets()[(size_t) i].category);
            if (pn == nm && pc == cat) { apvts.state.setProperty (isA ? "presetA" : "presetB", i, nullptr); break; }
        }
    }
    if (isA)
    {
        std::vector<int> chord;
        for (auto& s : juce::StringArray::fromTokens (v.getProperty ("chord", "").toString(), ",", "")) if (s.isNotEmpty()) chord.push_back (s.getIntValue());
        synA->setChord (chord); apvts.state.setProperty ("chordA", chordTextA(), nullptr);
    }
    else if (auto* sh = v.getProperty ("lfoShapes", {}).getArray())
        for (int i = 0; i < 64 && i < sh->size(); ++i) setCustomShapeB (i / 16, i % 16, (float) (double) (*sh)[i]);
}

juce::String DuoProcessor::sessionToJson()
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("format", "dualsynth-session"); root->setProperty ("version", 1);
    auto grab = [this] (std::initializer_list<const char*> ids)
    {
        auto* o = new juce::DynamicObject();
        for (auto* id : ids) o->setProperty (juce::Identifier (id), sig6 (rawHost (id)));
        return juce::var (o);
    };
    root->setProperty ("mixer", grab ({ "host_volA", "host_volB", "host_panA", "host_panB", "host_muteA", "host_muteB", "host_soloA", "host_soloB" }));
    root->setProperty ("routing", grab ({ "host_mode", "host_split", "host_swap", "host_chA", "host_chB" }));
    root->setProperty ("master", grab ({ "host_master", "host_c_on", "host_c_thr", "host_c_ratio", "host_c_att", "host_c_rel", "host_c_knee", "host_c_make", "host_c_mix",
                                         "host_r_on", "host_r_size", "host_r_decay", "host_r_damp", "host_r_pre", "host_r_low", "host_r_wet", "host_bpm", "host_sync" }));
    root->setProperty ("synthA", patchToVar (0));
    root->setProperty ("synthB", patchToVar (1));
    return juce::JSON::toString (juce::var (root), false);
}

bool DuoProcessor::sessionFromJson (const juce::String& text, juce::String& error)
{
    auto v = juce::JSON::parse (text);
    if (! v.isObject() || v.getProperty ("format", "").toString() != "dualsynth-session") { error = "This is not a dualsynth-session file."; return false; }
    if ((int) v.getProperty ("version", 0) != 1) { error = "Unsupported session version."; return false; }
    for (auto group : { "mixer", "routing", "master" })
        if (auto* o = v.getProperty (group, {}).getDynamicObject())
            for (auto& nv : o->getProperties())
                if (auto* p = apvts.getParameter (nv.name.toString()))
                    p->setValueNotifyingHost (p->convertTo0to1 ((float) (double) nv.value));
    patchFromVar (0, v.getProperty ("synthA", {}));
    patchFromVar (1, v.getProperty ("synthB", {}));
    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DuoProcessor(); }

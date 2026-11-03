#include "HostEngine.h"

namespace duo
{
void HostEngine::prepare (double sampleRate, int maxBlock, SynthContract* a, SynthContract* b)
{
    sr = sampleRate; syn[0] = a; syn[1] = b;
    const size_t n = (size_t) std::max (64, maxBlock) + 16;
    aL.assign (n, 0); aR.assign (n, 0); bL.assign (n, 0); bR.assign (n, 0); sumL.assign (n, 0); sumR.assign (n, 0);
    comp.prepare (sampleRate); reverb.prepare (sampleRate); limiter.prepare (sampleRate);
    for (auto& ch : held) ch.fill (0);
    lastMode = -1; audible[0] = audible[1] = true;
    if (a) a->prepare (sampleRate, maxBlock);
    if (b) b->prepare (sampleRate, maxBlock);
}

int HostEngine::heldCount (int synth) const
{
    int c = 0;
    for (auto& ch : held) for (auto v : ch) if (v & (1 << synth)) ++c;
    return c;
}

void HostEngine::allNotesOffBoth()
{
    for (auto* s : syn) if (s) s->allNotesOff();
    for (auto& ch : held) ch.fill (0);
}

void HostEngine::panic()
{
    for (auto* s : syn) if (s) s->panic();
    for (auto& ch : held) ch.fill (0);
    for (auto& s : sustainState) s = false;
    comp.reset(); reverb.reset(); limiter.reset();       // panic also kills the master reverb tail
}

void HostEngine::noteOnTo (int mask, int note, float vel, int ch)
{
    if (ch < 0 || ch > 15) ch = 0;
    for (int i = 0; i < 2; ++i)
        if ((mask & (1 << i)) && syn[i] && audible[i])
        {
            syn[i]->noteOn (note, vel, ch + 1);
            held[(size_t) ch][(size_t) note] |= (uint8_t) (1 << i);
        }
}

// audibility = not muted and (nobody soloed, or this strip is soloed). A synth that goes silent gets allNotesOff so nothing hangs.
void HostEngine::applyAudibility (const HostParams& p)
{
    const bool anySolo = p.soloA || p.soloB;
    const bool aud[2] = { ! p.muteA && (! anySolo || p.soloA), ! p.muteB && (! anySolo || p.soloB) };
    for (int i = 0; i < 2; ++i)
    {
        if (audible[i] && ! aud[i])
        {
            if (syn[i]) syn[i]->allNotesOff();
            for (auto& ch : held) for (auto& v : ch) v &= (uint8_t) ~(1 << i);
        }
        audible[i] = aud[i];
    }
}

void HostEngine::handle (const juce::MidiMessage& msg, const HostParams& p)
{
    const int ch = msg.getChannel() - 1;                         // 0..15
    auto targetMask = [&] (int note) -> int
    {
        switch (p.mode)
        {
            case Split:   { const bool high = note >= p.splitNote; const bool toB = p.splitSwap ? ! high : high; return toB ? 2 : 1; }
            case Channel: { int mk = 0; if (p.channelA == 0 || p.channelA == ch + 1) mk |= 1; if (p.channelB == 0 || p.channelB == ch + 1) mk |= 2; return mk; }
            default:      return 3;
        }
    };
    auto channelMask = [&]() -> int      // for controllers: split mode sends them to both synths
    {
        if (p.mode == Channel) { int mk = 0; if (p.channelA == 0 || p.channelA == ch + 1) mk |= 1; if (p.channelB == 0 || p.channelB == ch + 1) mk |= 2; return mk; }
        return 3;
    };

    if (msg.isNoteOn())
    {
        const int note = msg.getNoteNumber();
        // a retrigger of a key that is still down: release the old one first so held tracking stays exact
        const uint8_t old = held[(size_t) ch][(size_t) note];
        for (int i = 0; i < 2; ++i) if ((old & (1 << i)) && syn[i]) syn[i]->noteOff (note, ch + 1);
        held[(size_t) ch][(size_t) note] = 0;
        noteOnTo (targetMask (note), note, msg.getFloatVelocity(), ch);
    }
    else if (msg.isNoteOff())
    {
        const int note = msg.getNoteNumber();
        const uint8_t mk = held[(size_t) ch][(size_t) note];
        for (int i = 0; i < 2; ++i) if ((mk & (1 << i)) && syn[i]) syn[i]->noteOff (note, ch + 1);
        held[(size_t) ch][(size_t) note] = 0;
    }
    else if (msg.isPitchWheel())
    {
        const float v = ((float) msg.getPitchWheelValue() - 8192.0f) / 8192.0f;
        const int mk = channelMask();
        for (int i = 0; i < 2; ++i) if ((mk & (1 << i)) && syn[i]) syn[i]->pitchBend (std::max (-1.0f, std::min (1.0f, v)));
    }
    else if (msg.isController())
    {
        const int cc = msg.getControllerNumber(), val = msg.getControllerValue();
        if (cc == 120) { panic(); return; }
        if (cc == 123) { allNotesOffBoth(); return; }
        const int mk = channelMask();
        for (int i = 0; i < 2; ++i) if ((mk & (1 << i)) && syn[i]) syn[i]->controlChange (cc, val);
    }
    else if (msg.isChannelPressure())
    {
        const int mk = channelMask();
        for (int i = 0; i < 2; ++i) if ((mk & (1 << i)) && syn[i]) syn[i]->aftertouch (msg.getChannelPressureValue());
    }
    else if (msg.isAftertouch())
    {
        const int mk = channelMask();
        for (int i = 0; i < 2; ++i) if ((mk & (1 << i)) && syn[i]) syn[i]->aftertouch (msg.getAfterTouchValue());
    }
    else if (msg.isAllNotesOff() || msg.isAllSoundOff()) allNotesOffBoth();
}

void HostEngine::render (int start, int len, const TransportInfo& t, const HostParams& p)
{
    if (len <= 0) return;
    TransportInfo ts = t;
    ts.ppq = blockPpq + (double) start * t.bpm / 60.0 / sr;
    float* a0 = aL.data() + start; float* a1 = aR.data() + start; float* b0 = bL.data() + start; float* b1 = bR.data() + start;
    if (syn[0]) syn[0]->process (a0, a1, len, ts); else { std::fill (a0, a0 + len, 0.0f); std::fill (a1, a1 + len, 0.0f); }
    if (syn[1]) syn[1]->process (b0, b1, len, ts); else { std::fill (b0, b0 + len, 0.0f); std::fill (b1, b1 + len, 0.0f); }
    (void) p;
}

void HostEngine::processBlock (float* outL, float* outR, int n, const juce::MidiBuffer& midi, const TransportInfo& transport, const HostParams& p)
{
    if (n > (int) aL.size()) { const size_t sz = (size_t) n + 16; aL.resize (sz); aR.resize (sz); bL.resize (sz); bR.resize (sz); sumL.resize (sz); sumR.resize (sz); }
    blockPpq = transport.ppq;

    // mode change -> release everything so nothing is stuck between routings
    if (lastMode != p.mode) { if (lastMode >= 0) allNotesOffBoth(); lastMode = p.mode; }
    applyAudibility (p);

    int pos = 0;
    for (const auto meta : midi)
    {
        const int evPos = std::min (n, std::max (0, meta.samplePosition));
        render (pos, evPos - pos, transport, p);
        pos = std::max (pos, evPos);
        handle (meta.getMessage(), p);
    }
    render (pos, n - pos, transport, p);

    // ---- mixer: smoothed gain / pan / mute (no zipper), meters
    const float tgtA = p.gainA, tgtB = p.gainB;
    const float mA = audible[0] ? 1.0f : 0.0f, mB = audible[1] ? 1.0f : 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float k = 0.0015f;
        gA += (tgtA - gA) * k; gB += (tgtB - gB) * k; panAs += (p.panA - panAs) * k; panBs += (p.panB - panBs) * k;
        muteGA += (mA - muteGA) * 0.004f; muteGB += (mB - muteGB) * 0.004f;
        const float angA = (panAs + 1.0f) * kPi * 0.25f, angB = (panBs + 1.0f) * kPi * 0.25f;
        const float aLg = std::cos (angA) * 1.4142f * gA * muteGA, aRg = std::sin (angA) * 1.4142f * gA * muteGA;
        const float bLg = std::cos (angB) * 1.4142f * gB * muteGB, bRg = std::sin (angB) * 1.4142f * gB * muteGB;
        const float la = aL[(size_t) i] * aLg, ra = aR[(size_t) i] * aRg, lb = bL[(size_t) i] * bLg, rb = bR[(size_t) i] * bRg;
        peakA[0] = std::max (peakA[0] * 0.9995f, std::fabs (la)); peakA[1] = std::max (peakA[1] * 0.9995f, std::fabs (ra));
        peakB[0] = std::max (peakB[0] * 0.9995f, std::fabs (lb)); peakB[1] = std::max (peakB[1] * 0.9995f, std::fabs (rb));
        sumL[(size_t) i] = la + lb; sumR[(size_t) i] = ra + rb;
    }

    // ---- master chain: sum -> glue compressor -> reverb (parallel) -> safety limiter -> master volume
    float gr = comp.process (sumL.data(), sumR.data(), n, p.comp);
    reverb.process (sumL.data(), sumR.data(), n, p.reverb);
    const float lg = limiter.process (sumL.data(), sumR.data(), n);
    for (int i = 0; i < n; ++i)
    {
        mastS += (std::min (1.0f, p.masterGain) - mastS) * 0.002f;
        outL[i] = sumL[(size_t) i] * mastS; outR[i] = sumR[(size_t) i] * mastS;
        peakM[0] = std::max (peakM[0] * 0.9995f, std::fabs (outL[i])); peakM[1] = std::max (peakM[1] * 0.9995f, std::fabs (outR[i]));
    }
    m.aL = peakA[0]; m.aR = peakA[1]; m.bL = peakB[0]; m.bR = peakB[1]; m.mL = peakM[0]; m.mR = peakM[1]; m.gr = gr; m.limGr = lg;
}
} // namespace duo


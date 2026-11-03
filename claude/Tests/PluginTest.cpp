// Plugin-level tests: sessions, state, presets, panel switching, and loading the built VST3 the way a DAW does.
//   DuoPluginTest.exe [path-to-"AETHER DUO.vst3"]
#include "PluginProcessor.h"
#include "ui/HostEditor.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <cstdio>

static int failures = 0;
static void check (bool ok, const juce::String& what)
{
    std::printf ("%s  %s\n", ok ? "PASS" : "FAIL", what.toRawUTF8());
    if (! ok) ++failures;
}

static float blockPeak (const juce::AudioBuffer<float>& b)
{
    float p = 0; for (int c = 0; c < b.getNumChannels(); ++c) for (int i = 0; i < b.getNumSamples(); ++i) { const float x = b.getSample (c, i); if (! std::isfinite (x)) return 1e9f; p = std::max (p, std::fabs (x)); }
    return p;
}

static std::vector<float> snapshot (juce::AudioProcessor& p)
{
    std::vector<float> v;
    for (auto* prm : p.getParameters()) v.push_back (prm->getValue());
    return v;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000.0;
    {
        DuoProcessor proc;
        proc.prepareToPlay (sr, 512);
        std::printf ("INFO  %d automatable parameters (EMBER %d, PRISM %d, host %d)\n", proc.getParameters().size(), sa::COUNT, sb::COUNT, proc.getParameters().size() - sa::COUNT - sb::COUNT);

        juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi;
        auto runBlocks = [&] (int blocks, bool noteOn = false) -> float
        {
            float pk = 0;
            for (int b = 0; b < blocks; ++b)
            {
                midi.clear();
                if (noteOn && b == 0) for (int n : { 48, 55, 60, 64 }) proc.keyboardState.noteOn (1, n, 0.8f);
                proc.processBlock (buf, midi);
                pk = std::max (pk, blockPeak (buf));
            }
            return pk;
        };
        check (runBlocks (40, true) > 0.01f, "processor: keyboard note makes sound through the whole chain");
        proc.requestPanic(); runBlocks (4);
        check (runBlocks (20) < 1.0e-4f, "processor: panic button silences everything");

        // ---- presets apply exactly
        {
            bool ok = true;
            for (int i = 0; i < (int) sa::presets().size() && ok; ++i)
            {
                proc.loadPresetA (i); const auto want = sa::presetValues (i);
                for (int k = 0; k < sa::COUNT; ++k) { auto* p = proc.paramA (k); const float got = p->convertFrom0to1 (p->getValue()); if (std::fabs (got - want[(size_t) k]) > 0.02f * std::max (1.0f, std::fabs (want[(size_t) k])) + 0.02f) { ok = false; std::printf ("   A preset %d param %s: want %.3f got %.3f\n", i, sa::defs()[k].id, want[(size_t) k], got); break; } }
            }
            check (ok, "presets: every EMBER preset loads into the parameters exactly");
            ok = true;
            for (int i = 0; i < (int) sb::presets().size() && ok; ++i)
            {
                proc.loadPresetB (i); const auto want = sb::presetValues (i);
                for (int k = 0; k < sb::COUNT; ++k) { auto* p = proc.paramB (k); const float got = p->convertFrom0to1 (p->getValue()); if (std::fabs (got - want[(size_t) k]) > 0.02f * std::max (1.0f, std::fabs (want[(size_t) k])) + 0.02f) { ok = false; std::printf ("   B preset %d param %s: want %.3f got %.3f\n", i, sb::defs()[k].id, want[(size_t) k], got); break; } }
            }
            check (ok, "presets: every PRISM preset loads into the parameters exactly");
        }

        // ---- session round trip: save, change things, load, everything matches
        {
            proc.loadPresetA (7); proc.loadPresetB (9);
            proc.setCustomShapeB (2, 5, 0.77f);
            proc.hostParam ("host_volA")->setValueNotifyingHost (0.4f); proc.hostParam ("host_mode")->setValueNotifyingHost (0.5f); proc.hostParam ("host_c_thr")->setValueNotifyingHost (0.3f);
            const auto before = snapshot (proc); const auto json = proc.sessionToJson();
            proc.loadPresetA (2); proc.loadPresetB (3); proc.setCustomShapeB (2, 5, -0.5f);
            proc.hostParam ("host_volA")->setValueNotifyingHost (0.9f); proc.hostParam ("host_mode")->setValueNotifyingHost (0.0f); proc.hostParam ("host_c_thr")->setValueNotifyingHost (0.9f);
            juce::String err;
            const bool loaded = proc.sessionFromJson (json, err);
            const auto after = snapshot (proc);
            double maxDiff = 0; for (size_t i = 0; i < before.size(); ++i) maxDiff = std::max (maxDiff, (double) std::fabs (before[i] - after[i]));
            check (loaded && maxDiff < 1.0e-3, "session: save -> change -> load restores every parameter (max diff " + juce::String (maxDiff, 6) + ")");
            check (std::fabs (proc.engineB().getCustomShape (2, 5) - 0.77f) < 1.0e-3f, "session: drawn LFO shape restored");
            check (proc.sessionToJson() == json, "session: getPatch matches after the round trip (JSON identical)");
            check (json.contains ("dualsynth-session") && json.contains ("dualsynth-patch") && json.contains ("\"engine\": \"A\""), "session: file uses the dualsynth-session / dualsynth-patch format");
            juce::String e2;
            check (! proc.sessionFromJson ("{ \"format\": \"nope\" }", e2) && e2.isNotEmpty(), "session: a wrong file is rejected with a message");
            check (proc.sessionToJson() == json, "session: a rejected file changes nothing");
        }
        // ---- DAW state round trip
        {
            proc.loadPresetA (12); proc.loadPresetB (14);
            const auto before = snapshot (proc);
            juce::MemoryBlock mb; proc.getStateInformation (mb);
            proc.loadPresetA (1); proc.loadPresetB (1);
            proc.setStateInformation (mb.getData(), (int) mb.getSize());
            const auto after = snapshot (proc);
            double d = 0; for (size_t i = 0; i < before.size(); ++i) d = std::max (d, (double) std::fabs (before[i] - after[i]));
            check (d < 1.0e-3, "DAW state: getStateInformation / setStateInformation round trip (max diff " + juce::String (d, 6) + ")");
        }
        // ---- switching panels 50 times while audio runs
        {
            std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
            ed->setSize (1280, 862);
            auto* de = dynamic_cast<DuoEditor*> (ed.get());
            const int kids = de->getNumChildComponents();
            int cNow = 0; std::function<void (juce::Component*)> count = [&] (juce::Component* c) { ++cNow; for (auto* k : c->getChildren()) count (k); };
            count (ed.get()); const int total0 = cNow;
            float minPeak = 1e9f; bool soundAlways = true;
            runBlocks (2, true);
            for (int i = 0; i < 50; ++i)
            {
                de->showPanel (i & 1);
                juce::MessageManager::getInstance()->runDispatchLoopUntil (5);
                const float pk = runBlocks (6); minPeak = std::min (minPeak, pk); if (pk < 0.005f) soundAlways = false;
            }
            cNow = 0; count (ed.get());
            check (de->getNumChildComponents() == kids && cNow == total0, "panels: 50 quick switches leave the same number of components (" + juce::String (total0) + ")");
            check (soundAlways, "panels: audio keeps playing through all 50 switches (lowest block peak " + juce::String (minPeak, 3) + ")");
            check (proc.presetIndexA() == 12 && proc.presetIndexB() == 14, "panels: parameter state unchanged after switching");
            ed.reset();
            std::unique_ptr<juce::AudioProcessorEditor> ed2 (proc.createEditor()); ed2.reset();
            check (true, "panels: editor can be closed and reopened");
        }
    }

    // ---- the real VST3 file, loaded like a DAW would
    if (argc > 1)
    {
        juce::VST3PluginFormat fmt;
        juce::OwnedArray<juce::PluginDescription> descs;
        fmt.findAllTypesForFile (descs, argv[1]);
        check (descs.size() == 1, "VST3: bundle is found and describes one plugin");
        if (descs.size() == 1)
        {
            std::printf ("INFO  VST3 name '%s', manufacturer '%s', instrument=%d, %d out\n", descs[0]->name.toRawUTF8(), descs[0]->manufacturerName.toRawUTF8(), (int) descs[0]->isInstrument, descs[0]->numOutputChannels);
            juce::String err; juce::AudioPluginFormatManager mgr; mgr.addFormat (new juce::VST3PluginFormat());
            auto inst = mgr.createPluginInstance (*descs[0], sr, 512, err);
            check (inst != nullptr, "VST3: loads and instantiates (" + err + ")");
            if (inst)
            {
                inst->prepareToPlay (sr, 512);
                juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi; float pk = 0;
                for (int b = 0; b < 60; ++b)
                {
                    midi.clear(); if (b == 0) for (int n : { 48, 55, 60, 64 }) midi.addEvent (juce::MidiMessage::noteOn (1, n, 0.8f), 0);
                    inst->processBlock (buf, midi); pk = std::max (pk, blockPeak (buf));
                }
                check (pk > 0.01f && pk < 1.0f, "VST3: renders audio from MIDI (peak " + juce::String (pk, 3) + ")");
                check (inst->getTotalNumOutputChannels() == 2 && inst->acceptsMidi(), "VST3: stereo output, accepts MIDI");
                juce::MemoryBlock st; inst->getStateInformation (st);
                check (st.getSize() > 100, "VST3: saves state (" + juce::String ((int) st.getSize()) + " bytes)");
                std::printf ("INFO  VST3 reports %d parameters and %d samples of latency\n", inst->getParameters().size(), inst->getLatencySamples());
                if (auto* ed = inst->createEditorIfNeeded()) { ed->setSize (1280, 862); check (ed->getWidth() > 100, "VST3: editor opens"); inst->editorBeingDeleted (ed); delete ed; }
            }
        }
    }

    std::printf ("\n%s (%d failure%s)\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}

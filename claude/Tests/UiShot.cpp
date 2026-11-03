// Renders the real AETHER DUO editor to a PNG (so the UI can be checked without a DAW).
//   DuoUiShot.exe --out shot.png [--panel 0|1] [--size 1280x862] [--presetA N] [--presetB N] [--master 1] [--hold 0.9]
#include "PluginProcessor.h"
#include "ui/HostEditor.h"

namespace
{
struct Win : juce::DocumentWindow
{
    Win() : juce::DocumentWindow ("AETHER DUO", juce::Colours::black, 0) { setUsingNativeTitleBar (false); }
    void closeButtonPressed() override {}
};
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile ("duo-ui.png");
    int w = 1280, h = 862, panel = 0, pa = -1, pb = -1, master = 0, tab = 0, mode = 0;
    double hold = 0.9;
    for (int i = 1; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        if (a == "--out" && i + 1 < argc)     out = juce::File::getCurrentWorkingDirectory().getChildFile (argv[++i]);
        if (a == "--panel" && i + 1 < argc)   panel = juce::String (argv[++i]).getIntValue();
        if (a == "--presetA" && i + 1 < argc) pa = juce::String (argv[++i]).getIntValue();
        if (a == "--presetB" && i + 1 < argc) pb = juce::String (argv[++i]).getIntValue();
        if (a == "--master" && i + 1 < argc)  master = juce::String (argv[++i]).getIntValue();
        if (a == "--tab" && i + 1 < argc)     tab = juce::String (argv[++i]).getIntValue();
        if (a == "--mode" && i + 1 < argc)    mode = juce::String (argv[++i]).getIntValue();
        if (a == "--hold" && i + 1 < argc)    hold = juce::String (argv[++i]).getDoubleValue();
        if (a == "--size" && i + 1 < argc)
        {
            const auto s = juce::String (argv[++i]);
            w = s.upToFirstOccurrenceOf ("x", false, false).getIntValue(); h = s.fromFirstOccurrenceOf ("x", false, false).getIntValue();
        }
    }

    DuoProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    if (pa >= 0) proc.loadPresetA (pa);
    if (pb >= 0) proc.loadPresetB (pb);
    proc.setActivePanel (panel);
    if (mode > 0) proc.hostParam ("host_mode")->setValueNotifyingHost (mode == 1 ? 0.5f : 1.0f);

    Win window;
    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    editor->setSize (w, h);
    window.setContentNonOwned (editor.get(), true);
    window.setBounds (20, 20, w, h);
    window.setVisible (true);
    if (auto* de = dynamic_cast<DuoEditor*> (editor.get())) { de->showPanel (panel); de->showPrismTab (tab); de->setDrawerOpen (master != 0); }
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);

    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    const int blocks = (int) (hold * 48000.0 / 512.0);
    for (int b = 0; b < blocks + 20; ++b)
    {
        midi.clear();
        if (b == 0) for (int n : { 48, 55, 60, 64, 67, 72 }) proc.keyboardState.noteOn (1, n, 0.8f);
        proc.processBlock (buf, midi);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
    }
    juce::MessageManager::getInstance()->runDispatchLoopUntil (200);

    auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
    out.deleteFile();
    juce::PNGImageFormat png;
    if (auto stream = std::unique_ptr<juce::FileOutputStream> (out.createOutputStream())) png.writeImageToStream (img, *stream);
    std::printf ("wrote %s (%dx%d)\n", out.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    window.setContentNonOwned (nullptr, false);
    editor.reset();
    return 0;
}

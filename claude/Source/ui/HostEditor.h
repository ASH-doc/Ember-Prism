// AETHER DUO host window: neutral dark-gray chrome around the two synth panels (switch, routing, tempo, mixer, keyboard, master drawer).
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

class DuoProcessor;

class DuoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit DuoEditor (DuoProcessor&);
    ~DuoEditor() override;
    void paint (juce::Graphics&) override {}
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void showPanel (int panel);
    void showPrismTab (int tab);
    void setDrawerOpen (bool open);          // 0 = EMBER, 1 = PRISM (only changes what is visible)

    static constexpr int kW = 1280, kH = 862;

private:
    void timerCallback() override;
    struct Impl;
    std::unique_ptr<Impl> impl;
};

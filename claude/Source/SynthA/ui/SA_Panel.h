// EMBER (Synth A) panel: warm, dark, vintage-hardware look. Designed at 1280 x 720; the host scales it.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

class DuoProcessor;

namespace emberui
{
class EmberLook;
class EmberPanel : public juce::Component, private juce::Timer
{
public:
    static constexpr int kW = 1280, kH = 720;
    explicit EmberPanel (DuoProcessor&);
    ~EmberPanel() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

private:
    void timerCallback() override;
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace emberui

// PRISM (Synth B) panel: bright, glassy, animated modern look. Designed at 1280 x 720; the host scales it.
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

class DuoProcessor;

namespace prismui
{
class PrismPanel : public juce::Component, public juce::DragAndDropContainer, private juce::Timer
{
public:
    static constexpr int kW = 1280, kH = 720;
    explicit PrismPanel (DuoProcessor&);
    ~PrismPanel() override;
    void paint (juce::Graphics&) override;
    void visibilityChanged() override;
    void showTab (int tab);               // lower area: 0 ENV, 1 LFO, 2 MATRIX, 3 ARP/SEQ, 4 SURPRISE

private:
    void timerCallback() override;
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace prismui

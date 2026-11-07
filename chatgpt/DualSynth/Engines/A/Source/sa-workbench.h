#pragma once

#include "sa-editor.h"

namespace SynthAUI {
void drawWorkbenchKnob(juce::Graphics&, juce::Rectangle<int>, float value, float start, float end, int detents);

class Workbench final : public juce::Component {
public:
 Workbench(SynthAProcessor&, std::function<void(const juce::String&)> inspect);
 ~Workbench() override;
 void paint(juce::Graphics&) override;

private:
 class Route;
 class FilterBank;
 SynthAProcessor& proc;
 std::vector<std::unique_ptr<Control>> oscillators;
 std::vector<std::unique_ptr<juce::TextButton>> details;
 std::vector<std::unique_ptr<Route>> routes;
 std::array<std::unique_ptr<FilterBank>, 2> filters;
 std::array<juce::TextButton, 2> tabs{juce::TextButton{"FILTER 1"}, juce::TextButton{"FILTER 2"}};
 void selectFilter(int);
};
}

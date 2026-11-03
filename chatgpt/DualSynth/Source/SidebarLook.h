#pragma once
#include <JuceHeader.h>

namespace Sidebar {
inline juce::Colour graphite() { return juce::Colour(0xff171819); }
inline juce::Colour silver()   { return juce::Colour(0xffdedbd3); }
inline juce::Colour accent()   { return juce::Colour(0xffb7cb91); }

class Look final : public juce::LookAndFeel_V4 {
public:
 Look() {
  using namespace juce;
  setColour(Label::textColourId, silver());
  setColour(Slider::thumbColourId, accent());
  setColour(Slider::trackColourId, Colour(0xff899a6e));
  setColour(Slider::backgroundColourId, Colour(0xff101111));
  setColour(Slider::textBoxBackgroundColourId, Colour(0xff111212));
  setColour(Slider::textBoxTextColourId, silver());
  setColour(Slider::textBoxOutlineColourId, Colour(0xff30312e));
  setColour(ComboBox::backgroundColourId, Colour(0xff151615));
  setColour(ComboBox::outlineColourId, Colour(0xff51534a));
  setColour(ComboBox::textColourId, silver());
  setColour(ComboBox::arrowColourId, accent());
  setColour(PopupMenu::backgroundColourId, graphite());
  setColour(PopupMenu::textColourId, silver());
  setColour(PopupMenu::highlightedBackgroundColourId, Colour(0xff373d30));
  setColour(PopupMenu::highlightedTextColourId, Colours::white);
 }

 void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float value,
                       float start, float end, juce::Slider&) override {
  using namespace juce;
  const auto centre = Rectangle<float>(float(x), float(y), float(w), float(h)).getCentre();
  const float radius = jmin(w, h) * .44f;
  if (radius < 2) return;
  const float angle = start + value * (end - start);
  auto arc = [&](float to, Colour colour, float thickness) {
   Path path;
   path.addCentredArc(centre.x, centre.y, radius, radius, 0, start, to, true);
   g.setColour(colour);
   g.strokePath(path, PathStrokeType(thickness, PathStrokeType::curved, PathStrokeType::rounded));
  };
  arc(end, Colour(0xff42443e), 2);
  arc(angle, accent(), 2);
  auto cap = Rectangle<float>(radius * 1.49f, radius * 1.49f).withCentre(centre);
  g.setColour(Colours::black.withAlpha(.7f));
  g.fillEllipse(cap.translated(0, 1.5f).expanded(1));
  g.setGradientFill(ColourGradient(Colour(0xff555651), cap.getX(), cap.getY(),
                                  Colour(0xff191a19), cap.getRight(), cap.getBottom(), false));
  g.fillEllipse(cap);
  g.setColour(Colour(0xff63645b));
  g.drawEllipse(cap, .6f);
  g.setColour(silver());
  g.drawLine(Line<float>::fromStartAndAngle(centre.getPointOnCircumference(radius * .34f, angle),
                                          radius * .29f, angle), 1.6f);
 }

 void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool hover, bool down) override {
  using namespace juce;
  const auto face = button.getLocalBounds().toFloat().reduced(.5f, 3);
  const bool on = button.getToggleState();
  g.setColour(Colour(on ? 0xff373d2f : 0xff232424).brighter(hover ? .12f : 0).darker(down ? .1f : 0));
  g.fillRoundedRectangle(face, 4);
  g.setColour(Colour(on ? 0xff777f66 : 0xff40413d));
  g.drawRoundedRectangle(face, 4, .8f);
  g.setColour(on ? accent() : Colour(0xff595b55));
  g.fillEllipse(7, face.getCentreY() - 2, 4, 4);
  g.setColour(silver());
  g.setFont(FontOptions(10.f, Font::bold));
  g.drawFittedText(button.getButtonText(), button.getLocalBounds().withTrimmedLeft(16).withTrimmedRight(3),
                   Justification::centredLeft, 1);
 }
};

inline void paintChassis(juce::Graphics& g, bool effectsOn) {
 using namespace juce;
 const Rectangle<float> frame(1290, 76, 298, 752);
 g.setColour(Colours::black.withAlpha(.45f));
 g.fillRoundedRectangle(frame.translated(0, 3), 12);
 g.setGradientFill(ColourGradient(Colour(0xff292a29), 1290, 76, Colour(0xff141515), 1588, 828, false));
 g.fillRoundedRectangle(frame, 12);
 g.setColour(Colour(0xff50514a));
 g.drawRoundedRectangle(frame, 12, 1);
 for (float x : {1298.f, 1580.f})
  for (float y : {85.f, 819.f}) {
   g.setColour(Colour(0xff101111));
   g.fillEllipse(x - 2.5f, y - 2.5f, 5, 5);
   g.setColour(Colour(0xff66665e));
   g.drawLine(x - 1.4f, y + 1, x + 1.4f, y - 1, .7f);
  }

 g.setColour(Colour(0xff101111));
 g.fillRoundedRectangle(1290, 15, 298, 48, 8);
 g.setColour(accent());
 g.fillRoundedRectangle(1307, 25, 3, 27, 1.5f);
 g.setFont(FontOptions(15.f, Font::bold));
 g.setColour(silver());
 g.drawText("CONTROL BAY", 1318, 20, 160, 22, Justification::centredLeft);
 g.setFont(FontOptions(9.f));
 g.setColour(Colour(0xffaaa99e));
 g.drawText("DUAL SYNTH / SIGNAL PATH", 1318, 41, 230, 13, Justification::centredLeft);
 g.drawText("01 / 03", 1510, 22, 58, 17, Justification::centredRight);

 auto section = [&](int y, int height, const String& number, const String& title) {
  g.setColour(Colour(0xff1c1d1c));
  g.fillRoundedRectangle(1302.f, float(y), 274.f, float(height), 7);
  g.setColour(Colour(0xff383934));
  g.drawRoundedRectangle(1302.f, float(y), 274.f, float(height), 7, .8f);
  g.setColour(accent());
  g.setFont(FontOptions(10.f, Font::bold));
  g.drawText(number, 1310, y + 2, 24, 17, Justification::centredLeft);
  g.setColour(silver());
  g.drawText(title, 1334, y + 2, 178, 17, Justification::centredLeft);
 };
 section(88, 276, "01", "KEYBOARD ROUTING");
 section(369, 139, "02", "OUTPUT MIXER");
 section(514, 303, "03", "MASTER GLUE");

 g.setColour(effectsOn ? accent() : Colour(0xffa6a79f));
 g.setFont(FontOptions(9.f, Font::bold));
 g.drawText(effectsOn ? "ACTIVE" : "BYPASSED", 1498, 516, 69, 17, Justification::centredRight);
 g.setColour(Colour(0xff44453e));
 g.drawVerticalLine(1440, 392, 496);

 g.setFont(FontOptions(9.f, Font::bold));
 g.setColour(silver());
 g.drawText("A  /  EMBER VA", 1310, 200, 114, 15, Justification::centredLeft);
 g.drawText("B  /  PRISM FLUX", 1310, 278, 114, 15, Justification::centredLeft);
 g.setColour(Colour(0xffaaa99e));
 g.setFont(FontOptions(9.f));
 g.drawText("NOTE RANGE + CHANNEL", 1413, 200, 154, 15, Justification::centredRight);
 g.drawText("NOTE RANGE + CHANNEL", 1413, 278, 154, 15, Justification::centredRight);
 g.drawText("CH 0 = OMNI   /   SPLIT NOTE STARTS B", 1310, 350, 259, 12, Justification::centredLeft);
}
} // namespace Sidebar

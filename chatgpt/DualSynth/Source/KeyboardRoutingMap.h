#pragma once

#include "EditorWidgets.h"

class KeyboardRoutingMap final : public juce::Component, public juce::SettableTooltipClient {
public:
 KeyboardRoutingMap(juce::AudioProcessorValueTreeState& state, const juce::MidiKeyboardState& keys)
     : keyboard(keys), splitEdit(*state.getParameter("split"), [this](float){ repaint(); }){
  for(size_t i=0;i<values.size();++i)values[i]=state.getRawParameterValue(ids[i]);
  setTitle("Keyboard routing map");
  setWantsKeyboardFocus(true);
  setTooltip("The colored lanes show each engine's playable key range. In Split mode, drag to move the split, or use the arrow keys.");
 }
 ~KeyboardRoutingMap() override { if(dragging)splitEdit.endGesture(); }
 void refresh(){
  const bool split=read(0)==1;
  setMouseCursor(split?juce::MouseCursor::LeftRightResizeCursor:juce::MouseCursor::NormalCursor);
  repaint();
 }

 void paint(juce::Graphics& g) override {
  using namespace juce;
  const int mode=read(0),split=read(1);
  const bool swapped=read(2)!=0;
  const auto area=getLocalBounds().toFloat().reduced(.5f);
  g.setColour(Colour(0xff151716));
  g.fillRoundedRectangle(area,6.f);
  g.setColour(hasKeyboardFocus(false)?ControlBay::signal:ControlBay::border);
  g.drawRoundedRectangle(area,6.f,1.f);

  g.setFont(FontOptions(9.f,Font::bold));
  g.setColour(ControlBay::secondary);
  g.drawText(mode==1?"SPLIT AT "+MidiMessage::getMidiNoteName(split,true,true,4):mode==0?"LAYER MAP":"SINGLE ENGINE",
             9,4,118,14,Justification::centredLeft);
  if(mode==1)g.drawText("DRAG TO SPLIT",getWidth()-104,4,95,14,Justification::centredRight);

  const auto laneWidth=float(getWidth())-36.f;
  const auto noteWidth=laneWidth/128.f;
  for(int engine=0;engine<2;++engine){
   const bool ember=engine==0;
   const int y=22+engine*15,first=3+engine*3;
   const auto accent=ember?ControlBay::ember:ControlBay::prism;
   const int channel=read(size_t(first+2));
   const int channelMask=channel==0?0xffff:1<<(channel-1);
   int low=read(size_t(first)),high=read(size_t(first+1));
   if(mode==1){
    if(ember!=swapped)high=jmin(high,split-1);
    else low=jmax(low,split);
   }
   if((mode==2&&!ember)||(mode==3&&ember))high=-1;

   g.setColour(accent);
   g.drawText(ember?"A":"B",8,y-1,12,13,Justification::centred);
   const Rectangle<float> lane(26.f,float(y),laneWidth,11.f);
   g.setColour(ControlBay::plate);
   g.fillRoundedRectangle(lane,2.f);
   if(low<=high){
    const auto range=lane.withX(noteX(low)).withWidth(float(high-low+1)*noteWidth);
    g.setColour(accent.withAlpha(.30f));
    g.fillRoundedRectangle(range,2.f);
    g.setColour(accent.withAlpha(.8f));
    g.drawRoundedRectangle(range,2.f,1.f);
    for(int n=low;n<=high;++n)if(keyboard.isNoteOnForChannels(channelMask,n)){
     g.setColour(accent);
     g.fillRect(noteX(n),lane.getY()+1.f,jmax(1.f,noteWidth),lane.getHeight()-2.f);
    }
   }
  }

  const Rectangle<float> keys(26.f,53.f,laneWidth,19.f);
  g.setColour(Colour(0xff8c9083));
  g.fillRoundedRectangle(keys,2.f);
  for(int n=0;n<128;++n){
   const float x=noteX(n);
   if(MidiMessage::isMidiNoteBlack(n)){
    g.setColour(ControlBay::ink);
    g.fillRect(x,keys.getY(),noteWidth,keys.getHeight()*.62f);
   }else if(n%12==0){
    g.setColour(Colour(0xff53594e));
    g.drawVerticalLine(roundToInt(x),keys.getY(),keys.getBottom());
   }
   if(keyboard.isNoteOnForChannels(0xffff,n)){
    g.setColour(ControlBay::signal);
    g.fillRect(x,keys.getY()+1.f,jmax(noteWidth,1.f),keys.getHeight()-2.f);
   }
  }
  if(mode==1){
   const float x=noteX(split);
   g.setColour(ControlBay::signal);
   g.drawLine(x,19.f,x,74.f,1.4f);
   Path marker;
   marker.addTriangle(x-3.5f,17.f,x+3.5f,17.f,x,22.f);
   g.fillPath(marker);
  }
 }
 void mouseDown(const juce::MouseEvent& e) override {
  if(read(0)!=1||!e.mods.isLeftButtonDown())return;
  grabKeyboardFocus();
  dragging=true;
  splitEdit.beginGesture();
  dragTo(e.position.x);
 }
 void mouseDrag(const juce::MouseEvent& e) override { if(dragging&&read(0)==1)dragTo(e.position.x); }
 void mouseUp(const juce::MouseEvent&) override {
  if(dragging){dragging=false;splitEdit.endGesture();}
 }
 bool keyPressed(const juce::KeyPress& key) override {
  using namespace juce;
  if(read(0)!=1)return false;
  const int direction=key.getKeyCode()==KeyPress::leftKey?-1:key.getKeyCode()==KeyPress::rightKey?1:0;
  if(direction==0)return false;
  splitEdit.setValueAsCompleteGesture(float(jlimit(0,127,read(1)+direction)));
  repaint();
  return true;
 }
private:
 static constexpr std::array<const char*,9> ids{"routing","split","swap","a.low","a.high","a.channel","b.low","b.high","b.channel"};
 std::array<std::atomic<float>*,ids.size()> values{};
 const juce::MidiKeyboardState& keyboard;
 juce::ParameterAttachment splitEdit;
 bool dragging=false;
 int read(size_t index) const { return int(values[index]->load(std::memory_order_relaxed)); }
 float noteX(int note) const { return 26.f+float(note)*(float(getWidth())-36.f)/128.f; }
 void dragTo(float x){
  const int note=juce::jlimit(0,127,juce::roundToInt((x-26.f)*128.f/(float(getWidth())-36.f)));
  splitEdit.setValueAsPartOfGesture(float(note));
  repaint();
 }
};

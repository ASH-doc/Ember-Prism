#pragma once
#include <JuceHeader.h>

namespace ControlBay {
inline const juce::Colour ink{0xff111211};
inline const juce::Colour plate{0xff252625};
inline const juce::Colour border{0xff4b4d45};
inline const juce::Colour text{0xffe4e1d8};
inline const juce::Colour secondary{0xffb4b6a9};
inline const juce::Colour signal{0xffb7cb91};

inline void buttonColours(juce::TextButton& button) {
 button.setColour(juce::TextButton::buttonColourId, plate);
 button.setColour(juce::TextButton::buttonOnColourId, signal);
 button.setColour(juce::TextButton::textColourOffId, text);
 button.setColour(juce::TextButton::textColourOnId, ink);
}
} // namespace ControlBay
// Small custom components for the host strip: the old-school gain-reduction needle, stereo level bars and the ratio buttons.

// Analog-style gain-reduction meter: cream backlit face, needle at rest on the right (0 dB) and swinging left as the
// compressor works, with VU-style ballistics (quick to swing in, slow to fall back).
class GainReductionMeter final:public juce::Component{
public:
 void setReduction(float db){target=juce::jlimit(0.f,24.f,db);}
 void setTitle(const juce::String& text){if(text!=title){title=text;repaint();}}
 // Call at the UI refresh rate (about 24 Hz).
 void tick(){const float k=target>needle?.55f:.13f;needle+=(target-needle)*k;if(std::abs(target-needle)<.004f)needle=target;repaint();}
 float needlePosition()const{return needle;}
 void paint(juce::Graphics& g)override{
  using namespace juce;
  const auto area=getLocalBounds().toFloat();
  // Face and backlight.
  g.setGradientFill(ColourGradient(Colour(0xffefe4c2),area.getCentreX(),area.getY()+area.getHeight()*.35f,Colour(0xffc9b987),area.getX(),area.getBottom(),true));
  g.fillRoundedRectangle(area,7.f);
  g.setColour(Colour(0xff2b2217));g.drawRoundedRectangle(area.reduced(1.f),7.f,2.f);
  g.setColour(Colour(0x30000000));g.drawRoundedRectangle(area.reduced(4.f),5.f,1.f);
  Graphics::ScopedSaveState state(g);
  Path clip;clip.addRoundedRectangle(area.reduced(2.f),6.f);g.reduceClipRegion(clip);
  // Pivot well below the face so the scale is a wide shallow arc: 0 dB on the right, the end of the scale on the left.
  const float pivotX=area.getCentreX(),pivotY=area.getY()+area.getHeight()*1.72f,radius=area.getHeight()*1.62f;
  auto point=[&](float position,float r){const float angle=juce::degreesToRadians((.5f-position)*100.f);return Point<float>(pivotX+r*std::sin(angle),pivotY-r*std::cos(angle));};
  // Red zone from 10 dB to the end of the scale.
  {Path zone;const float from=positionFor(10.f),to=1.f;const float a0=juce::degreesToRadians((.5f-from)*100.f),a1=juce::degreesToRadians((.5f-to)*100.f);
   zone.addCentredArc(pivotX,pivotY,radius-3.f,radius-3.f,0.f,a1,a0,true);g.setColour(Colour(0xffb3261e));g.strokePath(zone,PathStrokeType(4.f));}
  // Scale: minor ticks every dB up to 10, numbered ticks at the classic positions.
  g.setColour(Colour(0xff2b2217));
  for(int db=0;db<=24;++db){const float position=positionFor(float(db));const bool numbered=db==0||db==3||db==5||db==7||db==10||db==20;
   const float length=numbered?10.f:5.f;g.drawLine({point(position,radius-8.f),point(position,radius-8.f-length)},numbered?1.6f:1.f);}
  g.setFont(FontOptions(10.f,Font::bold));
  for(int db:{0,3,5,7,10,20}){const auto at=point(positionFor(float(db)),radius-27.f);g.setColour(db>=10?Colour(0xffa31f18):Colour(0xff2b2217));g.drawText(String(db),Rectangle<float>(26.f,12.f).withCentre(at),Justification::centred,false);}
  g.setColour(Colour(0xff2b2217));g.setFont(FontOptions(9.f,Font::bold));
  g.drawText("GAIN REDUCTION",area.withTrimmedTop(area.getHeight()-17.f).withTrimmedBottom(3.f),Justification::centred,false);
  g.setFont(FontOptions(9.f));g.drawText(title,area.reduced(9.f,5.f),Justification::topLeft,false);g.drawText("dB",area.reduced(9.f,5.f),Justification::topRight,false);
  // Needle with a soft shadow and a hub.
  const auto tip=point(positionFor(needle),radius-4.f);
  g.setColour(Colour(0x40000000));g.drawLine({Point<float>(pivotX+1.5f,pivotY+1.5f),Point<float>(tip.x+1.5f,tip.y+1.5f)},2.f);
  g.setColour(Colour(0xff17110a));g.drawLine({Point<float>(pivotX,pivotY),tip},1.7f);
 }
private:
 float target=0,needle=0;juce::String title{"FET 76"};
 // Position along the scale (0 = right end, 1 = left end) for a gain reduction in dB; the spacing follows the cramped
 // top end of a real GR meter: 0..3 dB take a lot of room, 10..20 dB very little.
 static float positionFor(float db){
  static const float reduction[]={0.f,3.f,5.f,7.f,10.f,15.f,20.f,24.f},position[]={0.f,.34f,.5f,.62f,.76f,.9f,.97f,1.f};
  db=juce::jlimit(0.f,24.f,db);for(int i=1;i<8;++i)if(db<=reduction[i]){const float t=(db-reduction[i-1])/(reduction[i]-reduction[i-1]);return position[i-1]+t*(position[i]-position[i-1]);}
  return 1.f;
 }
};

// Look for the host strip: thin value arc, dark knob body and a pointer, readable even at 36 pixels.
class HostLook final:public juce::LookAndFeel_V4{
public:
 void drawRotarySlider(juce::Graphics& g,int x,int y,int width,int height,float position,float startAngle,float endAngle,juce::Slider& slider)override{
  using namespace juce;
  const auto bounds=Rectangle<float>(float(x),float(y),float(width),float(height)).reduced(1.5f);const float diameter=jmin(bounds.getWidth(),bounds.getHeight());
  const auto centre=bounds.getCentre();const float radius=diameter*.5f,angle=startAngle+position*(endAngle-startAngle);const bool on=slider.isEnabled();
  Path track;track.addCentredArc(centre.x,centre.y,radius-2.f,radius-2.f,0.f,startAngle,endAngle,true);
  g.setColour(ControlBay::border);g.strokePath(track,PathStrokeType(3.f,PathStrokeType::curved,PathStrokeType::rounded));
  Path value;value.addCentredArc(centre.x,centre.y,radius-2.f,radius-2.f,0.f,startAngle,angle,true);
  g.setColour(on?ControlBay::signal:ControlBay::border);g.strokePath(value,PathStrokeType(3.f,PathStrokeType::curved,PathStrokeType::rounded));
  const float body = radius * .69f;
  const auto cap = Rectangle<float>(2 * body, 2 * body).withCentre(centre);
  g.setColour(Colours::black.withAlpha(.7f));
  g.fillEllipse(cap.translated(0, 1.4f));
  g.setGradientFill(ColourGradient(Colour(0xff555650), cap.getX(), cap.getY(),
                                  ControlBay::ink, cap.getRight(), cap.getBottom(), false));
  g.fillEllipse(cap);
  g.setColour(Colour(0xff68695f));
  g.drawEllipse(cap, .7f);
  const Point<float> from(centre.x+body*.25f*std::sin(angle),centre.y-body*.25f*std::cos(angle)),to(centre.x+body*.92f*std::sin(angle),centre.y-body*.92f*std::cos(angle));
  g.setColour(ControlBay::text);g.drawLine({from,to},1.6f);
 }

 void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool hover, bool down) override {
  using namespace juce;
  const auto face = button.getLocalBounds().toFloat().reduced(.5f, 2.5f);
  const bool on = button.getToggleState();
  g.setColour(Colour(on ? 0xff363d2e : 0xff232424).brighter(hover ? .12f : 0).darker(down ? .1f : 0));
  g.fillRoundedRectangle(face, 4);
  g.setColour(on ? Colour(0xff737e63) : ControlBay::border);
  g.drawRoundedRectangle(face, 4, .8f);
  g.setColour(on ? ControlBay::signal : Colour(0xff5e6158));
  g.fillEllipse(7, face.getCentreY() - 2, 4, 4);
  g.setColour(ControlBay::text);
  g.setFont(FontOptions(10.f, Font::bold));
  g.drawFittedText(button.getButtonText(), button.getLocalBounds().withTrimmedLeft(16).withTrimmedRight(3),
                   Justification::centredLeft, 1);
 }
};

// Two level bars (left and right), vertical or horizontal, -60 dB to 0 dB, quick attack and gentle fall.
class LevelBars final:public juce::Component{
public:
 explicit LevelBars(bool horizontalBars=false):horizontal(horizontalBars){}
 void setLevels(float left,float right){const float in[2]={left,right};for(int c=0;c<2;++c){const float db=in[c]>1.e-6f?juce::Decibels::gainToDecibels(in[c]):-90.f;shown[c]=db>shown[c]?db:shown[c]+(db-shown[c])*.22f;}repaint();}
 void paint(juce::Graphics& g)override{
  using namespace juce;
  const auto area=getLocalBounds().toFloat();const float gap=2.f;
  for(int c=0;c<2;++c){
   Rectangle<float> track=horizontal?Rectangle<float>(area.getX(),area.getY()+c*(area.getHeight()+gap)/2.f,area.getWidth(),(area.getHeight()-gap)/2.f):Rectangle<float>(area.getX()+c*(area.getWidth()+gap)/2.f,area.getY(),(area.getWidth()-gap)/2.f,area.getHeight());
   g.setColour(ControlBay::ink);g.fillRoundedRectangle(track,2.f);
   const float fill=jlimit(0.f,1.f,(shown[c]+60.f)/60.f);
   Rectangle<float> lit=horizontal?track.withWidth(track.getWidth()*fill):track.withTrimmedTop(track.getHeight()*(1.f-fill));
   g.setColour(shown[c]>-1.f?Colour(0xffff5a4d):shown[c]>-8.f?Colour(0xffffc24a):ControlBay::signal);g.fillRoundedRectangle(lit,2.f);
  }
 }
private:
 bool horizontal;float shown[2]={-90.f,-90.f};
};

// Five ratio buttons (4, 8, 12, 20, ALL) bound to one choice parameter.
class RatioButtons final:public juce::Component{
public:
 RatioButtons(juce::AudioProcessorValueTreeState& state,const juce::String& parameterId,const juce::StringArray& labels){
  auto* parameter=state.getParameter(parameterId);
  for(int i=0;i<labels.size();++i){
   auto button=std::make_unique<juce::TextButton>(labels[i]);button->setClickingTogglesState(false);button->setRadioGroupId(0);
   ControlBay::buttonColours(*button);
   button->setConnectedEdges((i>0?juce::Button::ConnectedOnLeft:0)|(i<labels.size()-1?juce::Button::ConnectedOnRight:0));
   button->onClick=[this,i]{if(attachment)attachment->setValueAsCompleteGesture(float(i));};
   addAndMakeVisible(*button);buttons.push_back(std::move(button));
  }
  attachment=std::make_unique<juce::ParameterAttachment>(*parameter,[this](float value){for(size_t i=0;i<buttons.size();++i)buttons[i]->setToggleState(int(value+.5f)==int(i),juce::dontSendNotification);});
  attachment->sendInitialUpdate();
 }
 void resized()override{const int count=int(buttons.size());for(int i=0;i<count;++i)buttons[size_t(i)]->setBounds(getWidth()*i/count,0,getWidth()*(i+1)/count-getWidth()*i/count,getHeight());}
private:
 std::vector<std::unique_ptr<juce::TextButton>> buttons;std::unique_ptr<juce::ParameterAttachment> attachment;
};

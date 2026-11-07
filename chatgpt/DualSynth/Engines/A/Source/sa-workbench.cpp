#include "sa-workbench.h"

using namespace juce;
namespace SynthAUI {
namespace {
const Colour brass{0xffc5a275}, edge{0xff6f563b}, panel{0xff2b231b}, well{0xff15110d};
constexpr int oscillatorWidth = 190;
constexpr int filterLeft = 601;
constexpr int routeTop = 277;

void plate(Graphics& g, Rectangle<int> bounds){
 const auto r=bounds.toFloat();
 g.setGradientFill(ColourGradient(panel.brighter(.12f),r.getTopLeft(),panel.darker(.23f),r.getBottomLeft(),false));
 g.fillRoundedRectangle(r,5.f);
 g.setColour(edge);
 g.drawRoundedRectangle(r.reduced(.5f),5.f,1.f);
}
void caption(Graphics& g, const String& text, Rectangle<int> bounds, Colour colour=brass){
 g.setFont(FontOptions(11.f,Font::bold));
 g.setColour(colour);
 g.drawText(text,bounds,Justification::centredLeft,true);
}
std::unique_ptr<Control> makeControl(SynthAProcessor& proc, const String& id, int detents=0){
 auto control=std::make_unique<Control>(proc,SynthA::indexOf(id.toStdString()),false,false,detents>0);
 if(control->slider.isVisible()){
  control->slider.getProperties().set("emberWorkbenchKnob",detents);
  control->slider.setTextBoxStyle(Slider::TextBoxBelow,false,72,15);
  if(detents>0)control->slider.setMouseDragSensitivity(96);
 }
 return control;
}

Colour sourceColour(int source){
 static const Colour palette[]{Colour(0xffd8a368),Colour(0xffbcbb78),Colour(0xffd58465),Colour(0xff98b396)};
 return palette[size_t((source-1)%4)];
}
}

void drawWorkbenchKnob(Graphics& g, Rectangle<int> bounds, float value, float start, float end, int detents){
 const auto centre=bounds.toFloat().getCentre();
 const float radius=.5f*float(jmin(bounds.getWidth(),bounds.getHeight()))-9.f;
 if(radius<4.f)return;
 const auto radial=[&](float angle,float distance){
  return centre+Point<float>{std::sin(angle)*distance,-std::cos(angle)*distance};
 };
 const int intervals=detents>1?detents-1:10;
 for(int i=0;i<=intervals;++i){
  const float angle=jmap(float(i)/float(intervals),start,end);
  g.setColour((detents>1 && std::abs(value*intervals-float(i))<.1f)?amber():brass.withAlpha(.65f));
  g.drawLine({radial(angle,radius+3.f),radial(angle,radius+7.f)},detents>1?1.7f:1.f);
 }
 const auto disc=Rectangle<float>(radius*2.f,radius*2.f).withCentre(centre);
 g.setColour(Colours::black.withAlpha(.65f));
 g.fillEllipse(disc.translated(1.f,3.f));
 g.setGradientFill(ColourGradient(Colour(0xff5f5548),disc.getTopLeft(),Colour(0xff130f0d),disc.getBottomRight(),false));
 g.fillEllipse(disc);
 for(int i=0;i<32;++i){
  const float angle=MathConstants<float>::twoPi*float(i)/32.f;
  g.setColour(i<16?Colour(0xff100e0b):Colour(0xff76685a));
  g.drawLine({radial(angle,radius-5.f),radial(angle,radius-1.f)},1.25f);
 }
 g.setColour(Colour(0xff201b16));
 g.fillEllipse(disc.reduced(6.f));
 g.setColour(Colour(0xff8c7961));
 g.drawEllipse(disc.reduced(6.f),.8f);
 const float angle=jmap(value,start,end);
 g.setColour(cream());
 if(detents>1){
  Path pointer;
  pointer.startNewSubPath(radial(angle,radius+1.f));
  pointer.lineTo(radial(angle-.26f,radius*.38f));
  pointer.lineTo(radial(angle+.26f,radius*.38f));
  pointer.closeSubPath();
  g.fillPath(pointer);
 }else{
  g.drawLine({radial(angle,radius*.34f),radial(angle,radius-3.f)},2.5f);
 }
}

class Workbench::Route final : public Component {
public:
 Route(SynthAProcessor& p,int number):proc(p),slot(number),
     prefix("mod."+String(number)+"."),source(p,SynthA::indexOf((prefix+"source").toStdString())),
     destination(p,SynthA::indexOf((prefix+"dest").toStdString())),
     amount(p,SynthA::indexOf((prefix+"amount").toStdString())){
  for(auto* control:{&source,&destination,&amount}){
   addAndMakeVisible(*control);
   control->label.setVisible(false);
  }
  source.combo.setTitle("Route "+String(slot)+" source");
  destination.combo.setTitle("Route "+String(slot)+" destination");
  amount.slider.setTitle("Route "+String(slot)+" depth");
  amount.slider.setSliderStyle(Slider::LinearHorizontal);
  amount.slider.getProperties().set("emberRouteDepth",true);
  amount.slider.setTextBoxStyle(Slider::TextBoxRight,false,44,16);
  amount.slider.setDoubleClickReturnValue(true,0);
  amount.slider.textFromValueFunction=[](double value){
   const int percent=roundToInt(value*100);
   return (percent>0?"+":"")+String(percent)+"%";
  };
  amount.slider.valueFromTextFunction=[](const String& text){return text.getDoubleValue()*.01;};
  amount.slider.updateText();
 }
 void resized() override {
  source.setBounds(32,0,143,getHeight());
  source.combo.setBounds(source.getLocalBounds().reduced(0,1));
  destination.setBounds(224,0,215,getHeight());
  destination.combo.setBounds(destination.getLocalBounds().reduced(0,1));
  amount.setBounds(449,0,getWidth()-453,getHeight());
  amount.slider.setBounds(amount.getLocalBounds());
 }
 void paint(Graphics& g) override {
  const int activeSource=int(proc.getParam(prefix+"source"));
  const bool patched=activeSource>0&&proc.getParam(prefix+"dest")>0.f;
  const auto tint=patched?sourceColour(activeSource):edge.darker(.35f);
  g.setColour(slot%2==0?Colour(0xff261d15):Colour(0xff30251a));
  g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(0,1),3.f);
  caption(g,String(slot).paddedLeft('0',2),{8,0,24,getHeight()},patched?tint:brass);
  const float middle=float(getHeight())*.5f;
  g.setColour(tint);
  Path cable;
  cable.startNewSubPath(180.f,middle);
  cable.cubicTo(191.f,middle-8.f,207.f,middle+8.f,219.f,middle);
  g.strokePath(cable,PathStrokeType(patched?2.f:1.f));
  for(float x:{180.f,219.f})g.fillEllipse(x-2.2f,middle-2.2f,4.4f,4.4f);
 }
private:
 SynthAProcessor& proc;
 int slot;
 String prefix;
 Control source,destination,amount;
};

class Workbench::FilterBank final : public Component {
public:
 FilterBank(SynthAProcessor& p,int number):proc(p),prefix("flt"+String(number)+"."),
     powerAttachment(p.state,prefix+"on",power){
  addAndMakeVisible(power);
  power.setBounds(16,6,66,23);
  const auto add=[this](const char* suffix,Rectangle<int> bounds){
   auto control=makeControl(proc,prefix+suffix);
   addAndMakeVisible(*control);
   control->setBounds(bounds);
   controls.push_back(std::move(control));
  };
  add("model",{266,3,150,38});
  add("mode",{427,3,157,38});
  add("cutoff",{261,48,118,119});
  add("res",{385,62,91,104});
  add("drive",{487,62,91,104});
 }
 void paint(Graphics& g) override {
  const Rectangle<float> graph(13,39,232,123);
  g.setColour(well);
  g.fillRoundedRectangle(graph,4.f);
  for(int i=1;i<6;++i){
   g.setColour(edge.withAlpha(.35f));
   const float x=graph.getX()+float(i)*graph.getWidth()/6.f;
   g.drawVerticalLine(roundToInt(x),graph.getY(),graph.getBottom());
  }
  for(int i=1;i<4;++i){
   g.setColour(edge.withAlpha(.35f));
   const float y=graph.getY()+float(i)*graph.getHeight()/4.f;
   g.drawHorizontalLine(roundToInt(y),graph.getX(),graph.getRight());
  }
  const float cutoff=proc.getParam(prefix+"cutoff"),res=proc.getParam(prefix+"res");
  const float slope=proc.getParam(prefix+"model")<.5f?8.f:4.f;
  const int response=int(proc.getParam(prefix+"mode"));
  const bool enabled=power.getToggleState();
  Path trace;
  for(int i=0;i<=116;++i){
   const float t=float(i)/116.f,ratio=20.f*std::pow(1000.f,t)/cutoff;
   const float low=1.f/std::sqrt(1.f+std::pow(ratio,slope));
   const float high=1.f/std::sqrt(1.f+std::pow(1.f/ratio,slope));
   float magnitude=low;
   if(response==1)magnitude=high;
   if(response==2)magnitude=2.f*low*high;
   if(response==3)magnitude=std::abs(low-high);
   if(response!=3)magnitude+=res*.55f*std::exp(-3.f*std::pow(std::log2(ratio),2.f));
   const float db=enabled?Decibels::gainToDecibels(magnitude,-42.f):0.f;
   const auto point=Point<float>(graph.getX()+graph.getWidth()*t,
       graph.getBottom()-3.f-(graph.getHeight()-7.f)*jlimit(0.f,1.f,(db+42.f)/50.f));
   if(i==0)trace.startNewSubPath(point);else trace.lineTo(point);
  }
  g.setColour(enabled?amber():brass.withAlpha(.5f));
  g.strokePath(trace,PathStrokeType(1.8f));
  g.setFont(FontOptions(9.f));
  g.setColour(brass);
  g.drawText("20 Hz  /  20 kHz  /  RESPONSE GUIDE",13,163,232,15,Justification::centred);
 }
private:
 SynthAProcessor& proc;
 String prefix;
 ToggleButton power{"POWER"};
 AudioProcessorValueTreeState::ButtonAttachment powerAttachment;
 std::vector<std::unique_ptr<Control>> controls;
};

Workbench::Workbench(SynthAProcessor& p,std::function<void(const String&)> inspect):proc(p){
 const auto detail=[this,inspect](String text,String group,Rectangle<int> bounds){
  auto button=std::make_unique<TextButton>(text);
  button->setBounds(bounds);
  button->onClick=[inspect,group]{inspect(group);};
  addAndMakeVisible(*button);
  details.push_back(std::move(button));
 };
 for(int o=0;o<3;++o){
  const int x=o*200;
  const String number(o+1),prefix="osc"+number+".";
  detail("EDIT","Oscillator "+number,{x+130,9,50,19});
  auto shape=makeControl(proc,prefix+"wave",5);
  shape->label.setText("Waveform",dontSendNotification);
  shape->slider.setTitle("Oscillator "+number+" waveform");
  shape->setBounds(x+8,43,84,104);
  addAndMakeVisible(*shape);
  oscillators.push_back(std::move(shape));
  const char* ids[]{"coarse","fine","pw","level"};
  for(int j=0;j<4;++j){
   auto control=makeControl(proc,prefix+ids[j]);
   control->setBounds(x+4+j*46,153,45,62);
   control->slider.setTextBoxStyle(Slider::TextBoxBelow,false,42,14);
   addAndMakeVisible(*control);
   oscillators.push_back(std::move(control));
  }
 }
 for(int i=0;i<2;++i){
  filters[size_t(i)]=std::make_unique<FilterBank>(proc,i+1);
  filters[size_t(i)]->setBounds(filterLeft+1,41,614,181);
  addChildComponent(*filters[size_t(i)]);
  auto& tab=tabs[size_t(i)];
  tab.setBounds(filterLeft+177+i*87,8,82,22);
  tab.onClick=[this,i]{selectFilter(i);};
  addAndMakeVisible(tab);
 }
 detail("ROUTING","Filter routing",{1059,8,71,22});
 detail("FILTER EDIT","Filter",{1138,8,74,22});
 details.back()->onClick=[this,inspect]{inspect(filters[0]->isVisible()?"Filter 1":"Filter 2");};
 selectFilter(0);
 for(int i=0;i<12;++i){
  auto route=std::make_unique<Route>(proc,i+1);
  route->setBounds(9+(i/6)*607,routeTop+(i%6)*23,598,23);
  addAndMakeVisible(*route);
  routes.push_back(std::move(route));
 }
}

Workbench::~Workbench()=default;

void Workbench::selectFilter(int index){
 for(size_t i=0;i<filters.size();++i){
  filters[i]->setVisible(int(i)==index);
  tabs[i].setToggleState(int(i)==index,dontSendNotification);
 }
}

void Workbench::paint(Graphics& g){
 g.fillAll(Colour(0xff1b1612));
 for(int o=0;o<3;++o){
  const int x=o*200;
  plate(g,{x,0,oscillatorWidth,226});
  caption(g,"OSCILLATOR 0"+String(o+1),{12+x,8,115,22});
  const Rectangle<float> display(float(x+95),58.f,84.f,75.f);
  g.setColour(well);
  g.fillRoundedRectangle(display,4.f);
  g.setColour(edge);
  g.drawHorizontalLine(roundToInt(display.getCentreY()),display.getX()+3.f,display.getRight()-3.f);
  const auto prefix="osc"+String(o+1)+".";
  const int wave=int(proc.getParam(prefix+"wave"));
  const float width=proc.getParam(prefix+"pw");
  Path shape;
  for(int i=0;i<=80;++i){
   const float t=float(i)/81.f;
   const float sample=SynthA::wave(t,1.f/81.f,wave,width);
   const Point<float> point(display.getX()+4.f+76.f*t,display.getCentreY()-sample*27.f);
   if(i==0)shape.startNewSubPath(point);else shape.lineTo(point);
  }
  g.setColour(amber());
  g.strokePath(shape,PathStrokeType(1.6f));
 }
 plate(g,{filterLeft,0,getWidth()-filterLeft,226});
 caption(g,"FILTER / RESPONSE",{filterLeft+14,8,158,22});
 plate(g,{0,232,getWidth(),195});
 caption(g,"MODULATION PATCHBOARD",{13,239,245,23});
 caption(g,"12 ROUTES  /  CHOOSE A SOURCE AND DESTINATION, THEN SET DEPTH",{274,239,865,23});
 for(int x:{9,616}){
  caption(g,"SOURCE",{x+32,261,143,15});
  caption(g,"DESTINATION",{x+224,261,215,15});
  caption(g,"BIPOLAR DEPTH",{x+449,261,145,15});
 }
}
}

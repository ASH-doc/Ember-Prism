// sb-ui-signal.cpp — Signal view: large wavetable cards, one large filter graph, and the Motion lanes.
#include "sb-ui.h"
using namespace juce;
namespace PrismUI {
namespace {
Colour laneColour(int lane){return lane==0?Colour(0xff4fe3f5):lane==1?Colour(0xff6cf2b8):Colour(0xffff6ad5);}
String hertzText(float hz){return hz>=1000?String(hz/1000,2)+" kHz":String(int(std::lround(hz)))+" Hz";}
}
// ---- FilterGraph -----------------------------------------------------------------------------------------------------------
void FilterGraph::paint(Graphics& g){
 const auto& schema=SynthB::Schema::get();const auto& layout=schema.loc;
 const auto area=getLocalBounds().toFloat();const float w=area.getWidth(),h=area.getHeight(),top=24.f,bottom=h-18.f;
 g.setColour(Colour(0xff0d1528));g.fillRoundedRectangle(area,9);
 auto xFor=[&](float hz){return std::log(hz/20.f)/std::log(1000.f)*w;};
 g.setFont(FontOptions(9.f));
 for(float hz:{20.f,50.f,100.f,200.f,500.f,1000.f,2000.f,5000.f,10000.f,20000.f}){
  const bool major=hz==100.f||hz==1000.f||hz==10000.f;const float x=xFor(hz);
  g.setColour(Colours::white.withAlpha(major?.10f:.045f));g.drawVerticalLine(int(std::min(x,w-1)),top,bottom);
  if(major){g.setColour(Colour(0xff8295b8));g.drawText(hz==100.f?"100":hz==1000.f?"1k":"10k",Rectangle<float>(x+3,bottom+2,34,12),Justification::left,false);}
 }
 for(int line=0;line<=4;++line){g.setColour(Colours::white.withAlpha(line==4?.12f:.045f));g.drawHorizontalLine(int(top+(bottom-top)*float(line)/4),0.f,w);}
 for(int pass=0;pass<2;++pass){
  const int f=pass==0?1-selected:selected;const int base=layout.filter[f];const float cutoff=processor.read(base+SynthB::cutoff),res=processor.read(base+SynthB::resonance);
  const int type=int(processor.read(base+SynthB::type));const bool on=processor.read(base+SynthB::enabled)>.5f,chosen=pass==1;
  const Colour colour=f==0?aqua():violet();Path path;
  for(int i=0;i<=int(w);i+=2){const float hz=20*std::pow(1000.f,float(i)/w);const float m=std::min(1.25f,filterResponse(type,cutoff,res,hz));const float y=bottom-m/1.25f*(bottom-top);if(i==0)path.startNewSubPath(0,y);else path.lineTo(float(i),y);}
  if(chosen){Path fill(path);fill.lineTo(w,bottom);fill.lineTo(0,bottom);fill.closeSubPath();g.setGradientFill(ColourGradient(colour.withAlpha(on?.28f:.08f),0,top,colour.withAlpha(0.f),0,bottom,false));g.fillPath(fill);}
  g.setColour(colour.withAlpha(chosen?(on?1.f:.4f):.3f));g.strokePath(path,PathStrokeType(chosen?2.2f:1.2f));
 }
 // Cutoff / resonance handle of the selected filter.
 {const int base=layout.filter[selected];const float cutoff=processor.read(base+SynthB::cutoff),res=processor.read(base+SynthB::resonance);const Colour colour=selected==0?aqua():violet();
  const float x=jlimit(0.f,w,xFor(cutoff)),y=top+(1-res)*(bottom-top);
  g.setColour(colour.withAlpha(.35f));g.drawVerticalLine(int(std::min(x,w-1)),top,bottom);
  g.setColour(colour.withAlpha(.25f));g.fillEllipse(x-11,y-11,22,22);g.setColour(colour);g.fillEllipse(x-6,y-6,12,12);g.setColour(ice());g.drawEllipse(x-6,y-6,12,12,1.2f);
  const auto& type=schema.specs[size_t(base+SynthB::type)];const int typeIndex=jlimit(0,int(type.options.size())-1,int(processor.read(base+SynthB::type)));
  g.setColour(ice());g.setFont(FontOptions(11.f,Font::bold));
  g.drawText("FILTER "+String(selected+1)+"  /  "+String(type.options[size_t(typeIndex)])+"  /  CUTOFF "+hertzText(cutoff)+"  /  RESONANCE "+String(res,2),Rectangle<float>(10,4,w-20,16),Justification::centredLeft,false);
  g.setColour(Colour(0xff8295b8));g.setFont(FontOptions(9.f));
  g.drawText(processor.read(base+SynthB::enabled)>.5f?"ILLUSTRATIVE RESPONSE":"BYPASSED  /  ILLUSTRATIVE RESPONSE",Rectangle<float>(10,4,w-20,16),Justification::centredRight,false);
  g.drawText("drag to edit",Rectangle<float>(10,h-14,70,12),Justification::centredLeft,false);}
 if(hasKeyboardFocus(false)){g.setColour(aqua().withAlpha(.7f));g.drawRoundedRectangle(area.reduced(1),9,1.6f);}
}
void FilterGraph::begin(){if(dragging)return;dragging=true;const auto& layout=SynthB::Schema::get().loc;const auto& specs=SynthB::Schema::get().specs;const int base=layout.filter[selected];
 for(int index:{base+SynthB::cutoff,base+SynthB::resonance})if(auto* p=processor.parameters.getParameter(specs[size_t(index)].key))p->beginChangeGesture();}
void FilterGraph::end(){if(!dragging)return;dragging=false;const auto& layout=SynthB::Schema::get().loc;const auto& specs=SynthB::Schema::get().specs;const int base=layout.filter[selected];
 for(int index:{base+SynthB::cutoff,base+SynthB::resonance})if(auto* p=processor.parameters.getParameter(specs[size_t(index)].key))p->endChangeGesture();}
void FilterGraph::drag(const MouseEvent& e){
 const auto& layout=SynthB::Schema::get().loc;const auto& specs=SynthB::Schema::get().specs;const int base=layout.filter[selected];
 const float top=24.f,bottom=float(getHeight())-18.f;
 const float cutoff=20*std::pow(1000.f,SynthB::bounded(float(e.x)/float(std::max(1,getWidth())),0.f,1.f)),res=SynthB::bounded(1-(float(e.y)-top)/(bottom-top),0.f,1.f);
 if(auto* p=processor.parameters.getParameter(specs[size_t(base+SynthB::cutoff)].key))p->setValueNotifyingHost(p->convertTo0to1(specs[size_t(base+SynthB::cutoff)].valid(cutoff)));
 if(auto* p=processor.parameters.getParameter(specs[size_t(base+SynthB::resonance)].key))p->setValueNotifyingHost(p->convertTo0to1(res));
 repaint();
}
void FilterGraph::mouseDown(const MouseEvent& e){grabKeyboardFocus();begin();drag(e);}
void FilterGraph::mouseDrag(const MouseEvent& e){drag(e);}
void FilterGraph::mouseUp(const MouseEvent&){end();}
bool FilterGraph::keyPressed(const KeyPress& key){
 const auto& layout=SynthB::Schema::get().loc;const int base=layout.filter[selected];const bool fine=key.getModifiers().isShiftDown();
 if(key==KeyPress::leftKey||key==KeyPress::rightKey){const float semitones=(fine?.25f:1.f)*(key==KeyPress::rightKey?1.f:-1.f);processor.write(base+SynthB::cutoff,processor.read(base+SynthB::cutoff)*std::pow(2.f,semitones/12));repaint();return true;}
 if(key==KeyPress::upKey||key==KeyPress::downKey){processor.write(base+SynthB::resonance,processor.read(base+SynthB::resonance)+(key==KeyPress::upKey?1.f:-1.f)*(fine?.005f:.02f));repaint();return true;}
 return false;
}
// ---- MotionSteps -----------------------------------------------------------------------------------------------------------
void MotionSteps::paint(Graphics& g){
 const auto& layout=SynthB::Schema::get().loc;const int base=layout.motion[size_t(lane)];const int length=int(processor.read(base+SynthB::laneLength))+2;
 const bool running=processor.read(base+SynthB::laneDest)>.5f&&processor.read(layout.motionAmount)>.0001f;const int playing=running?processor.engine.motionSteps[size_t(lane)].load(std::memory_order_relaxed):-1;
 const float cw=float(getWidth())/16,h=float(getHeight());const Colour colour=laneColour(lane);
 for(int j=0;j<16;++j){
  const auto cell=Rectangle<float>(float(j)*cw+1.5f,1.f,cw-3.f,h-2.f);const bool inLoop=j<length;const float v=processor.read(base+SynthB::laneValue0+j);
  g.setColour(Colour(0xff10182c));g.fillRoundedRectangle(cell,3.f);g.setColour(Colours::white.withAlpha(j%4==0?.22f:.1f));g.drawRoundedRectangle(cell,3.f,1.f);
  g.setColour(Colours::white.withAlpha(.12f));g.drawHorizontalLine(int(cell.getCentreY()),cell.getX()+2,cell.getRight()-2);
  const float half=cell.getHeight()*.5f-3.f,y=cell.getCentreY()-v*half;
  g.setColour(colour.withAlpha(inLoop?.9f:.22f));g.fillRect(Rectangle<float>(cell.getX()+4,std::min(y,cell.getCentreY()),cell.getWidth()-8,std::max(1.5f,std::abs(y-cell.getCentreY()))));
  if(!inLoop){g.setColour(Colour(0x66080c18));g.fillRoundedRectangle(cell,3.f);}
  if(j==playing){g.setColour(ice());g.drawRoundedRectangle(cell,3.f,1.8f);}
 }
}
void MotionSteps::edit(const MouseEvent& e){
 const auto& layout=SynthB::Schema::get().loc;const int base=layout.motion[size_t(lane)]+SynthB::laneValue0;const float cw=float(getWidth())/16;
 const int step=jlimit(0,15,int(float(e.x)/cw));float value=SynthB::bounded(1-2*float(e.y)/float(std::max(1,getHeight())),-1.f,1.f);if(std::abs(value)<.07f)value=0;
 if(previous>=0&&previous!=step){const int direction=step>previous?1:-1;for(int i=previous+direction;i!=step;i+=direction){const float t=float(i-previous)/float(step-previous);processor.write(base+i,previousValue+(value-previousValue)*t);}}
 if(std::abs(processor.read(base+step)-value)>.002f)processor.write(base+step,value);
 previous=step;previousValue=value;repaint();
}
void MotionSteps::shapes(const MouseEvent&){
 PopupMenu menu;menu.addItem(1,"Clear (all zero)");menu.addItem(2,"Ramp up");menu.addItem(3,"Ramp down");menu.addItem(4,"Sine");menu.addItem(5,"Alternate");menu.addItem(6,"Random");
 Component::SafePointer<MotionSteps> safe(this);
 menu.showMenuAsync(PopupMenu::Options().withTargetComponent(this),[safe](int result){
  if(!safe||result==0)return;const auto& layout=SynthB::Schema::get().loc;const int base=layout.motion[size_t(safe->lane)]+SynthB::laneValue0;Random random;safe->processor.undo.beginNewTransaction("Motion lane shape");
  for(int j=0;j<16;++j){const float t=float(j)/15;float value=0;if(result==2)value=-1+2*t;else if(result==3)value=1-2*t;else if(result==4)value=std::sin(float(SynthB::tau)*float(j)/16);else if(result==5)value=j%2?-.8f:.8f;else if(result==6)value=random.nextFloat()*2-1;safe->processor.write(base+j,value);}
  safe->repaint();});
}
void MotionSteps::mouseDown(const MouseEvent& e){if(e.mods.isPopupMenu()){shapes(e);return;}previous=-1;processor.undo.beginNewTransaction("Motion lane step");edit(e);}
void MotionSteps::mouseDrag(const MouseEvent& e){if(!e.mods.isPopupMenu())edit(e);}
void MotionSteps::mouseDoubleClick(const MouseEvent& e){const auto& layout=SynthB::Schema::get().loc;const int step=jlimit(0,15,int(float(e.x)/(float(getWidth())/16)));processor.write(layout.motion[size_t(lane)]+SynthB::laneValue0+step,0);repaint();}
// ---- SignalView ------------------------------------------------------------------------------------------------------------
Dial& SignalView::dial(const String& key,Rectangle<int> bounds,bool menuOnly){
 const int index=SynthB::Schema::get().find(key.toStdString());jassert(index>=0);
 auto control=std::make_unique<Dial>(processor,style,index);control->setBounds(bounds);control->inspect=inspect;addAndMakeVisible(*control);
 if(menuOnly){control->label.setVisible(false);control->choice.setBounds(0,0,bounds.getWidth(),bounds.getHeight());}
 dials.push_back(std::move(control));return *dials.back();
}
SignalView::SignalView(PrismProcessor& p,Style& look,std::function<void(String)> inspectCallback,std::function<void(int)> importWave):processor(p),style(look),inspect(std::move(inspectCallback)){
 setOpaque(true);setLookAndFeel(&style);
 auto button=[&](const String& text,Rectangle<int> bounds,std::function<void()> action)->TextButton&{auto b=std::make_unique<TextButton>(text);b->setBounds(bounds);b->onClick=std::move(action);addAndMakeVisible(*b);buttons.push_back(std::move(b));return *buttons.back();};
 // Oscillators
 for(int o=0;o<2;++o){const int X=o*684;const String prefix="osc"+String(o+1)+".";
  button("Import WAV",{X+388,6,78,19},[importWave,o]{if(importWave)importWave(o);});button("Edit",{X+474,6,62,19},[this,o]{if(inspect)inspect("Oscillator "+String(o+1));});
  dial(prefix+"table",{X+14,28,300,26},true);
  waves[size_t(o)]=std::make_unique<WaveView>(processor,o);waves[size_t(o)]->setBounds(X+14,58,300,142);addAndMakeVisible(*waves[size_t(o)]);
  dial(prefix+"pos",{X+326,28,56,66});dial(prefix+"level",{X+384,28,56,66});dial(prefix+"uni.voices",{X+442,28,56,66});dial(prefix+"uni.detune",{X+500,28,56,66});
  dial(prefix+"warp1.type",{X+326,100,112,42});dial(prefix+"warp1.amount",{X+442,100,56,48});dial(prefix+"warp2.type",{X+326,150,112,42});dial(prefix+"warp2.amount",{X+442,150,56,48});
 }
 dial("mix.sub",{574,28,48,68});dial("mix.noise",{626,28,48,68});dial("voice.glide",{574,106,48,68});
 // Filters
 graph=std::make_unique<FilterGraph>(processor);graph->setBounds(14,252,640,146);addAndMakeVisible(*graph);
 for(int i=0;i<2;++i){auto& tab=filterTabs[size_t(i)];tab.setButtonText("FILTER "+String(i+1));tab.setClickingTogglesState(false);tab.setBounds(14+i*92,222,86,22);tab.onClick=[this,i]{chooseFilter(i);};addAndMakeVisible(tab);}
 for(int f=0;f<2;++f){const String prefix="flt"+String(f+1)+".";
  auto power=std::make_unique<ToggleButton>("Power");power->setBounds(204,222,76,22);addAndMakeVisible(*power);toggleBindings.push_back(std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(processor.parameters,prefix+"on",*power));perFilter[size_t(f)].push_back(power.get());toggles.push_back(std::move(power));
  perFilter[size_t(f)].push_back(&dial(prefix+"type",{290,220,150,26},true));
  const char* keys[]{"cutoff","res","drive","mix","key","env","velocity","vowel"};
  for(int k=0;k<8;++k)perFilter[size_t(f)].push_back(&dial(prefix+keys[k],{672+k*68,226,66,76}));
 }
 button("All filter controls",{452,222,140,22},[this]{if(inspect)inspect("Filter "+String(selectedFilter+1));});
 dial("filter.route",{672,326,170,50});dial("filter.balance",{852,316,66,76});
 // Motion
 dial("motion.amount",{14,430,84,76});
 for(int l=0;l<3;++l){const String prefix="motion.lane"+String(l+1)+".";const int y=429+l*30;
  dial(prefix+"dest",{116,y,132,27},true);dial(prefix+"division",{254,y,66,27},true);dial(prefix+"length",{326,y,50,27},true);dial(prefix+"interp",{382,y,66,27},true);
  auto depth=std::make_unique<Slider>(Slider::LinearHorizontal,Slider::NoTextBox);depth->setLookAndFeel(&style);depth->setBounds(454,y,82,27);depth->setTooltip("Lane depth");addAndMakeVisible(*depth);
  sliderBindings.push_back(std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(processor.parameters,prefix+"depth",*depth));sliders.push_back(std::move(depth));
  steps[size_t(l)]=std::make_unique<MotionSteps>(processor,l);steps[size_t(l)]->setBounds(544,y,688,27);addAndMakeVisible(*steps[size_t(l)]);
 }
 chooseFilter(0);
}
void SignalView::chooseFilter(int f){
 selectedFilter=jlimit(0,1,f);graph->select(selectedFilter);
 for(int i=0;i<2;++i){filterTabs[size_t(i)].setToggleState(i==selectedFilter,dontSendNotification);for(auto* component:perFilter[size_t(i)])component->setVisible(i==selectedFilter);}
 repaint();
}
void SignalView::refresh(){graph->repaint();for(auto& lane:steps)lane->repaint();for(auto& wave:waves)wave->repaint();}
void SignalView::paint(Graphics& g){
 g.fillAll(Colour(0xff0b1020));
 g.setColour(ice());g.setFont(FontOptions(12.f,Font::bold));
 for(int o=0;o<2;++o){card(g,{float(o*684),0,560,208},o==0?aqua():violet());g.setColour(ice());g.drawText("OSC "+String(o+1)+" / WAVETABLE",float(o*684+28),6.f,240.f,18.f,Justification::centredLeft,false);}
 card(g,{568,0,108,208},Colour(0xff6cf2b8));g.setColour(ice());g.setFont(FontOptions(10.f,Font::bold));g.drawText("MIX",594.f,6.f,60.f,18.f,Justification::centredLeft,false);
 card(g,{0,214,1244,190},selectedFilter==0?aqua():violet());
 g.setColour(Colour(0xff8295b8));g.setFont(FontOptions(9.f));g.drawText("ROUTING",684.f,310.f,120.f,12.f,Justification::centredLeft,false);
 card(g,{0,412,1244,108},Colour(0xffff6ad5));g.setColour(ice());g.setFont(FontOptions(10.f,Font::bold));g.drawText("MOTION",34.f,416.f,80.f,14.f,Justification::centredLeft,false);
 g.setColour(Colour(0xff8295b8));g.setFont(FontOptions(8.f));
 const struct{int x,w;const char* text;} captions[]{{116,132,"DESTINATION"},{254,66,"DIVISION"},{326,50,"STEPS"},{382,66,"INTERP"},{454,82,"DEPTH"},{544,688,"STEPS  /  drag to draw  /  right-click for shapes  /  steps past the loop length are dimmed"}};
 for(const auto& caption:captions)g.drawText(caption.text,caption.x,415,caption.w,11,Justification::centredLeft,false);
 for(int l=0;l<3;++l){g.setColour(laneColour(l));g.setFont(FontOptions(10.f,Font::bold));g.drawText(String(l+1),100,429+l*30,14,27,Justification::centred,false);}
}
}

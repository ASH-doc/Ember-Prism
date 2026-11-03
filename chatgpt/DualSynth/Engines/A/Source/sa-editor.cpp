// sa-editor.cpp — Drawn hardware controls, amber display, full matrix, scope and parameter inspector.
#include "PresetBrowser.h"
#include "sa-editor.h"
using namespace juce;
namespace SynthAUI {
HardwareLook::HardwareLook(){
 setColour(Slider::textBoxTextColourId,cream());setColour(Slider::textBoxBackgroundColourId,Colours::transparentBlack);setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);
 setColour(ComboBox::backgroundColourId,Colour(0xff17130f));setColour(ComboBox::textColourId,cream());setColour(ComboBox::outlineColourId,Colour(0xff5a4935));setColour(ComboBox::arrowColourId,amber());
 setColour(PopupMenu::backgroundColourId,Colour(0xff211b16));setColour(PopupMenu::textColourId,cream());setColour(PopupMenu::highlightedBackgroundColourId,Colour(0xff654924));
 setColour(TextButton::buttonColourId,Colour(0xff332a21));setColour(TextButton::textColourOffId,cream());setColour(TextButton::buttonOnColourId,Colour(0xff745222));
 setColour(Label::textColourId,cream());setColour(TextEditor::backgroundColourId,Colour(0xff17130f));setColour(TextEditor::textColourId,amber());setColour(TextEditor::outlineColourId,Colour(0xff5a4935));
 setColour(TooltipWindow::backgroundColourId,Colour(0xff211b16));setColour(TooltipWindow::textColourId,cream());
}
Label* HardwareLook::createSliderTextBox(Slider& slider){auto* box=LookAndFeel_V4::createSliderTextBox(slider);box->setFont(FontOptions(9.f));box->setColour(Label::textColourId,cream());box->setColour(Label::outlineColourId,Colours::transparentBlack);box->setColour(Label::backgroundColourId,Colours::transparentBlack);return box;}
void HardwareLook::drawRotarySlider(Graphics& g,int x,int y,int w,int h,float value,float start,float end,Slider&){
 const float size=float(std::min(w,h))-9,r=size*.5f,cx=float(x)+float(w)*.5f,cy=float(y)+float(h)*.5f;
 for(int i=0;i<=10;++i){float a=start+(end-start)*float(i)/10;Point<float> a1(cx+std::sin(a)*(r+2),cy-std::cos(a)*(r+2)),a2(cx+std::sin(a)*(r+4),cy-std::cos(a)*(r+4));g.setColour(Colour(0xff897350));g.drawLine({a1,a2},.8f);}
 g.setColour(Colours::black.withAlpha(.65f));g.fillEllipse(cx-r+1,cy-r+3,size,size);
 g.setGradientFill(ColourGradient(Colour(0xff595149),cx-r,cy-r,Colour(0xff100e0c),cx+r,cy+r,false));g.fillEllipse(cx-r,cy-r,size,size);
 g.setColour(Colour(0xff777069));g.drawEllipse(cx-r,cy-r,size,size,.6f);
 g.setColour(Colour(0xff201c18));g.fillEllipse(cx-r+3,cy-r+3,size-6,size-6);
 for(int i=0;i<24;++i){const float a=float(2*SynthA::pi*i)/24;g.setColour(Colours::black.withAlpha(.4f));g.drawLine(cx+std::sin(a)*(r-3),cy+std::cos(a)*(r-3),cx+std::sin(a)*(r-1),cy+std::cos(a)*(r-1),.6f);}
 const float a=start+value*(end-start);g.setColour(cream());g.drawLine(cx+std::sin(a)*(r*.45f),cy-std::cos(a)*(r*.45f),cx+std::sin(a)*(r-4),cy-std::cos(a)*(r-4),2);
}
void HardwareLook::drawLinearSlider(Graphics& g,int x,int y,int w,int h,float pos,float,float,Slider::SliderStyle style,Slider& s){
 if(style==Slider::LinearVertical){float cx=float(x)+float(w)*.5f;g.setColour(Colour(0xff100c09));g.fillRoundedRectangle(cx-2,float(y),4,float(h),2);for(int i=0;i<6;++i){g.setColour(Colour(0xff705b3d));float yy=float(y)+float(h*i)/5;g.drawHorizontalLine(int(yy),cx-8,cx-4);}
 g.setGradientFill(ColourGradient(Colour(0xff96816a),cx-8,pos-4,Colour(0xff2a241e),cx+8,pos+5,false));g.fillRoundedRectangle(cx-8,pos-5,16,10,2);g.setColour(cream());g.drawHorizontalLine(int(pos),cx-6,cx+6);
 }else {g.setColour(Colour(0xff0f0c09));g.fillRoundedRectangle(float(x),float(y+h/2-2),float(w),4,2);g.setColour(s.getName().startsWith("seq.gate.step")?amber():Colour(0xffb18b52));g.fillRoundedRectangle(float(x),float(y+h/2-2),std::max(1.f,pos-float(x)),4,2);g.setColour(cream());g.fillRoundedRectangle(pos-2,float(y+h/2-5),4,10,1);}
}
void HardwareLook::drawToggleButton(Graphics& g,ToggleButton& b,bool,bool){auto r=b.getLocalBounds();g.setColour(Colour(0xff100d0a));g.fillRoundedRectangle(r.toFloat(),3);g.setColour(b.getToggleState()?amber():Colour(0xff59442a));g.fillEllipse(5,float(r.getCentreY())-3,6,6);g.setColour(cream());g.setFont(FontOptions(9.f));g.drawText(b.getButtonText(),r.withTrimmedLeft(15),Justification::centredLeft);}
Control::Control(SynthAProcessor& p,int i,bool f,bool in):parameter(i),proc(p),fader(f),inlineLabel(in){
 const auto& def=SynthA::parameters()[size_t(i)];label.setText(def.label,dontSendNotification);label.setFont(FontOptions(in?11.f:9.f));label.setJustificationType(in?Justification::centredLeft:Justification::centred);addAndMakeVisible(label);
 if(!def.choices.empty()){for(size_t n=0;n<def.choices.size();++n)combo.addItem(def.choices[n],int(n+1));addAndMakeVisible(combo);ca=std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(proc.state,def.id,combo);combo.setTooltip(def.id);}
 else{slider.setName(def.id);slider.setSliderStyle(in?Slider::LinearHorizontal:f?Slider::LinearVertical:Slider::RotaryVerticalDrag);slider.setTextBoxStyle(Slider::TextBoxBelow,false,in?60:48,13);slider.setRotaryParameters(float(SynthA::pi)*1.25f,float(SynthA::pi)*2.75f,true);slider.setVelocityBasedMode(false);slider.setVelocityModeParameters(.2,1,0,true,ModifierKeys::shiftModifier);slider.setMouseDragSensitivity(160);slider.setDoubleClickReturnValue(true,def.initial);slider.setTextValueSuffix(def.unit.empty()?"":" "+def.unit);slider.setTooltip(def.id+" | Shift: fine | Double-click: reset | Right-click label: MIDI learn");slider.setNumDecimalPlacesToDisplay(def.discrete?0:def.max>100?0:2);addAndMakeVisible(slider);sa=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(proc.state,def.id,slider);slider.setPopupMenuEnabled(false);
 slider.textFromValueFunction=[def](double value){
  if(def.unit=="Hz"&&value>=1000)return String(value/1000,2)+"k";
  if(def.unit=="s")return value<1?String(value*1000,0)+"ms":String(value,2)+"s";
  if(def.discrete)return String(int(std::round(value)));
  if(def.unit=="BPM"||def.unit=="ct")return String(value,1);
  if(def.unit=="dB")return String(value,1);
  return String(value,2);
 };
 slider.valueFromTextFunction=[def](const String& text){double value=text.getDoubleValue();if(def.unit=="Hz"&&text.containsIgnoreCase("k"))value*=1000;if(def.unit=="s"&&text.containsIgnoreCase("ms"))value/=1000;return value;};
 slider.setTextValueSuffix("");slider.updateText();}
 label.addMouseListener(this,false);
}
void Control::resized(){auto r=getLocalBounds();if(inlineLabel){label.setBounds(r.removeFromLeft(std::min(260,getWidth()/2)));if(ca)combo.setBounds(r.reduced(4));else slider.setBounds(r);}else{label.setBounds(r.removeFromTop(13));if(ca)combo.setBounds(r.reduced(1));else slider.setBounds(r);}}
void Control::paint(Graphics& g){for(int n=1;n<=12;++n)if(int(proc.getParam("mod."+String(n)+".dest"))==parameter+1&&proc.getParam("mod."+String(n)+".source")>0){g.setColour(amber());g.fillEllipse(float(getWidth()-5),3,3,3);break;}}
void Control::mouseDown(const MouseEvent& e){if(e.mods.isPopupMenu())menu(e);}
void Control::menu(const MouseEvent&){PopupMenu menu;menu.addItem(1,"Learn MIDI CC");menu.addItem(2,"Remove MIDI mapping");menu.addItem(3,"Reset to default");Component::SafePointer<Control> safe(this);menu.showMenuAsync(PopupMenu::Options().withTargetComponent(this),[safe](int result){if(!safe)return;if(result==1)safe->proc.learnParameter.store(safe->parameter);if(result==2)for(auto& cc:safe->proc.ccMap)if(cc.load()==safe->parameter)cc.store(-1);if(result==3){auto& p=SynthA::parameters()[size_t(safe->parameter)];safe->proc.setParam(p.id,p.initial);}});}
Inspector::Inspector(SynthAProcessor& p,const String& initial,HardwareLook& look):proc(p){setLookAndFeel(&look);addAndMakeVisible(search);search.setTextToShowWhenEmpty("Search any parameter or section",Colour(0xff9c876a));search.setText(initial);addAndMakeVisible(viewport);viewport.setViewedComponent(&list,false);
 for(size_t i=0;i<SynthA::parameters().size();++i){auto c=std::make_unique<Control>(proc,int(i),false,true);const auto& def=SynthA::parameters()[i];c->label.setText(def.group+" / "+def.label+"  ["+def.id+"]",dontSendNotification);list.addAndMakeVisible(*c);c->slider.setLookAndFeel(&look);controls.push_back(std::move(c));}search.onTextChange=[this]{filter();};setSize(760,640);filter();}
void Inspector::resized(){search.setBounds(12,10,getWidth()-24,28);viewport.setBounds(8,46,getWidth()-16,getHeight()-54);filter();}
void Inspector::filter(){const auto query=search.getText().trim();int y=0;for(size_t i=0;i<controls.size();++i){const auto& p=SynthA::parameters()[i];bool show=query.isEmpty()||String(p.id+" "+p.group+" "+p.label).containsIgnoreCase(query);controls[i]->setVisible(show);if(show){controls[i]->setBounds(8,y,std::max(600,viewport.getWidth()-32),38);y+=40;}}list.setSize(std::max(620,viewport.getWidth()-20),y);}
void GateGrid::paint(Graphics& g){
 for(int i=0;i<16;++i){const float x=float(i*31),level=proc.getParam("seq.gate.step"+String(i+1));g.setColour(Colour(0xff17120e));g.fillRoundedRectangle(x+2,12,26,25,2);g.setColour(amber().withAlpha(.25f+.75f*level));g.fillRoundedRectangle(x+3,36-23*level,24,std::max(1.f,23*level),2);g.setFont(FontOptions(8.f));g.setColour(cream());g.drawText(String(i+1),int(x),0,30,11,Justification::centred);if(proc.getParam("seq.gate.tie"+String(i+1))>.5f){g.setColour(cream());g.drawHorizontalLine(14,x+6,x+23);}}
}
void GateGrid::mouseDown(const MouseEvent& e){previous=-1;ties=e.mods.isShiftDown()||e.mods.isPopupMenu();edit(e);}
void GateGrid::mouseDrag(const MouseEvent& e){edit(e);}
void GateGrid::edit(const MouseEvent& e){int step=SynthA::clamp(e.x/31,0,15);float value=SynthA::clamp(float(36-e.y)/23,0.f,1.f);if(ties){if(step!=previous){auto id="seq.gate.tie"+String(step+1);proc.setParam(id,proc.getParam(id)>.5f?0.f:1.f);}}else{if(previous>=0&&previous!=step){int direction=step>previous?1:-1;for(int i=previous+direction;i!=step;i+=direction){float t=float(i-previous)/float(step-previous);proc.setParam("seq.gate.step"+String(i+1),previousValue+(value-previousValue)*t);}}proc.setParam("seq.gate.step"+String(step+1),value);}previous=step;previousValue=value;repaint();}
// ---- Arp and gate board -------------------------------------------------------------------------------------------------
namespace {
Colour boardMuted(){return Colour(0xffc8a782);}Colour boardSage(){return Colour(0xffb7c582);}Colour boardAmber(){return Colour(0xffef9c43);}
const char* const laneNames[3]={"VELOCITY","GATE LEVEL","TIE"};const char* const laneNotes[3]={"arp notes","output rhythm","hold into next"};
}
Rectangle<float> StepMatrix::laneRect(Lane lane)const{
 const float tieHeight=36.f,top=22.f,gap=8.f,laneHeight=(float(getHeight())-top-tieHeight-gap*2-2.f)/2.f,width=float(getWidth()-gridLeft-2);
 if(lane==Velocity)return {float(gridLeft),top,width,laneHeight};
 if(lane==Gate)return {float(gridLeft),top+laneHeight+gap,width,laneHeight};
 return {float(gridLeft),top+2*laneHeight+2*gap,width,tieHeight};
}
int StepMatrix::stepAt(float x)const{return jlimit(0,15,int(std::floor((x-float(gridLeft))/columnWidth())));}
StepMatrix::Lane StepMatrix::laneAt(float y)const{if(y<laneRect(Velocity).getBottom()+4)return Velocity;if(y<laneRect(Gate).getBottom()+4)return Gate;return Tie;}
String StepMatrix::idFor(Lane lane,int step)const{return String(lane==Velocity?"arp.velocity":lane==Gate?"seq.gate.step":"seq.gate.tie")+String(step+1);}
float StepMatrix::valueFor(Lane lane,float y)const{const auto r=laneRect(lane);const float t=jlimit(0.f,1.f,(r.getBottom()-y)/r.getHeight());return lane==Velocity?.05f+.95f*t:t;}
void StepMatrix::apply(Lane lane,int step,float value){const auto id=idFor(lane,step);if(std::abs(proc.getParam(id)-value)>.001f)proc.setParam(id,value);}
void StepMatrix::report(Lane lane,int step){
 if(!onReadout)return;const float v=proc.getParam(idFor(lane,step));
 onReadout(String(laneNames[lane])+"  STEP "+String(step+1).paddedLeft('0',2)+"  /  "+(lane==Tie?String(v>.5f?"TIED":"OFF"):String(int(std::lround(v*100)))+"%"));
}
void StepMatrix::edit(const MouseEvent& e,bool first){
 const int step=stepAt(float(e.x));
 if(first){selectedLane=laneAt(float(e.y));previous=-1;if(selectedLane==Tie)paintTie=proc.getParam(idFor(Tie,step))<.5f;}
 selectedStep=step;
 if(selectedLane==Tie){const int from=previous<0?step:previous,direction=step>=from?1:-1;for(int i=from;;i+=direction){apply(Tie,i,paintTie?1.f:0.f);if(i==step)break;}}
 else{
  const float value=valueFor(selectedLane,float(e.y));
  if(previous>=0&&previous!=step){const int direction=step>previous?1:-1;for(int i=previous+direction;i!=step;i+=direction){const float t=float(i-previous)/float(step-previous);apply(selectedLane,i,previousValue+(value-previousValue)*t);}}
  apply(selectedLane,step,value);previousValue=value;
 }
 previous=step;report(selectedLane,step);repaint();
}
void StepMatrix::mouseDown(const MouseEvent& e){grabKeyboardFocus();edit(e,true);}
void StepMatrix::mouseDrag(const MouseEvent& e){edit(e,false);}
void StepMatrix::mouseDoubleClick(const MouseEvent& e){
 const Lane lane=laneAt(float(e.y));const int step=stepAt(float(e.x));
 apply(lane,step,lane==Velocity?(step%4==0?1.f:.8f):lane==Gate?(step%4==3?0.f:1.f):0.f);report(lane,step);repaint();
}
bool StepMatrix::keyPressed(const KeyPress& key){
 const auto code=key.getKeyCode();const float fine=key.getModifiers().isShiftDown()?.01f:.05f;
 if(code==KeyPress::leftKey){selectedStep=std::max(0,selectedStep-1);}
 else if(code==KeyPress::rightKey){selectedStep=std::min(15,selectedStep+1);}
 else if(code==KeyPress::upKey||code==KeyPress::downKey){
  const float sign=code==KeyPress::upKey?1.f:-1.f;
  if(selectedLane==Tie)apply(Tie,selectedStep,sign>0?1.f:0.f);
  else apply(selectedLane,selectedStep,jlimit(selectedLane==Velocity?.05f:0.f,1.f,proc.getParam(idFor(selectedLane,selectedStep))+sign*fine));
 }
 else if(code==KeyPress::spaceKey||code==KeyPress::returnKey){if(selectedLane==Tie)apply(Tie,selectedStep,proc.getParam(idFor(Tie,selectedStep))>.5f?0.f:1.f);else return false;}
 else if(key.getTextCharacter()=='v'||key.getTextCharacter()=='V')selectedLane=Velocity;
 else if(key.getTextCharacter()=='g'||key.getTextCharacter()=='G')selectedLane=Gate;
 else if(key.getTextCharacter()=='t'||key.getTextCharacter()=='T')selectedLane=Tie;
 else return false;
 report(selectedLane,selectedStep);repaint();return true;
}
void StepMatrix::paint(Graphics& g){
 const float cw=columnWidth();
 g.setFont(FontOptions(12.f,Font::bold));
 for(int step=0;step<16;++step){g.setColour(step==selectedStep?cream():boardMuted());g.drawText(String(step+1).paddedLeft('0',2),Rectangle<float>(float(gridLeft)+step*cw,0.f,cw,18.f),Justification::centred,false);}
 for(int lane=0;lane<3;++lane){
  const auto r=laneRect(Lane(lane));
  g.setColour(lane==Velocity?boardAmber():lane==Gate?boardSage():cream());g.setFont(FontOptions(12.f,Font::bold));g.drawText(laneNames[lane],Rectangle<float>(0.f,r.getY()+2,float(gridLeft)-8.f,16.f),Justification::centredLeft,false);
  g.setColour(boardMuted());g.setFont(FontOptions(10.f));g.drawText(laneNotes[lane],Rectangle<float>(0.f,r.getY()+18,float(gridLeft)-8.f,14.f),Justification::centredLeft,false);
  for(int step=0;step<16;++step){
   const auto cell=Rectangle<float>(float(gridLeft)+step*cw+1.5f,r.getY(),cw-3.f,r.getHeight());const float v=proc.getParam(idFor(Lane(lane),step));
   g.setColour(Colour(0xff211811));g.fillRoundedRectangle(cell,3.f);g.setColour(step%4==0?Colour(0xff9a7550):Colour(0xff6a4f38));g.drawRoundedRectangle(cell,3.f,step%4==0?1.4f:1.f);
   if(lane==Tie){
    const bool on=v>.5f;
    if(on){auto bar=cell.reduced(3.f,7.f);if(step<15)bar=bar.withWidth(bar.getWidth()+6.f);g.setColour(boardSage());g.fillRoundedRectangle(bar,3.f);g.setColour(Colour(0xff1d230f));g.setFont(FontOptions(11.f,Font::bold));g.drawText("TIE",cell,Justification::centred,false);}
    else{g.setColour(Colour(0xff6a4f38));g.drawLine(cell.getCentreX()-6,cell.getCentreY(),cell.getCentreX()+6,cell.getCentreY(),1.5f);}
   }else{
    const float norm=lane==Velocity?(v-.05f)/.95f:v;const auto inner=cell.reduced(4.f,4.f);const float height=std::max(2.f,inner.getHeight()*norm);
    const auto bar=Rectangle<float>(inner.getX(),inner.getBottom()-height,inner.getWidth(),height);
    const Colour base=lane==Velocity?boardAmber():boardSage();
    g.setGradientFill(ColourGradient(base.brighter(.25f),bar.getX(),bar.getY(),base.darker(.45f),bar.getX(),bar.getBottom(),false));g.fillRoundedRectangle(bar,2.f);
    g.setColour(norm>.82f?Colour(0xff2a1706):cream().withAlpha(.9f));g.setFont(FontOptions(11.f,Font::bold));g.drawText(String(int(std::lround(v*100))),cell.withHeight(16.f).translated(0,2),Justification::centred,false);
   }
   if(step==selectedStep&&lane==int(selectedLane)){g.setColour(hasKeyboardFocus(false)?Colour(0xffffdc91):cream().withAlpha(.5f));g.drawRoundedRectangle(cell.expanded(1.f),3.f,hasKeyboardFocus(false)?2.f:1.2f);}
  }
 }
}
ArpBoard::ArpBoard(SynthAProcessor& p,HardwareLook& l):proc(p),matrix(p),look(l){
 setOpaque(true);setWantsKeyboardFocus(true);
 addAndMakeVisible(matrix);addAndMakeVisible(closeButton);addAndMakeVisible(readout);
 closeButton.onClick=[this]{if(onClose)onClose();};
 readout.setFont(FontOptions(12.f,Font::bold));readout.setColour(Label::textColourId,boardAmber());readout.setText("READY",dontSendNotification);readout.setJustificationType(Justification::centredLeft);
 matrix.onReadout=[this](const String& text){readout.setText(text,dontSendNotification);};
 sw("arp.on","ARP",{34,100,84,26});knob("arp.mode",130,84,130,46);knob("arp.rate",270,84,100,46);sw("arp.hold","HOLD",{388,100,84,26});
 knob("arp.octaves",490,76,64,68);knob("arp.swing",562,76,64,68);knob("arp.gate",634,76,64,68);
 sw("seq.gate.on","GATE",{34,198,84,26});knob("seq.gate.rate",130,182,100,46);knob("seq.gate.depth",244,174,64,68);knob("seq.gate.smooth",316,174,64,68);knob("seq.gate.shape",388,174,64,68);sw("seq.gate.restart","KEY RESTART",{470,198,130,26});
}
Control& ArpBoard::knob(const String& id,int x,int y,int w,int h){auto c=std::make_unique<Control>(proc,SynthA::indexOf(id.toStdString()),false,false);c->setBounds(x,y,w,h);addAndMakeVisible(*c);c->slider.setLookAndFeel(&look);controls.push_back(std::move(c));return *controls.back();}
void ArpBoard::sw(const String& id,const String& text,Rectangle<int> bounds){auto b=std::make_unique<ToggleButton>(text);b->setBounds(bounds);addAndMakeVisible(*b);attachments.push_back(std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(proc.state,id,*b));switches.push_back(std::move(b));}
void ArpBoard::resized(){closeButton.setBounds(getWidth()-112,12,96,28);matrix.setBounds(16,256,getWidth()-32,268);readout.setBounds(16,532,520,24);}
bool ArpBoard::keyPressed(const KeyPress& key){if(key==KeyPress::escapeKey){if(onClose)onClose();return true;}return false;}
void ArpBoard::paint(Graphics& g){
 g.setGradientFill(ColourGradient(Colour(0xff403026),0.f,0.f,Colour(0xff261d17),float(getWidth()),float(getHeight()),false));g.fillAll();
 for(int y=0;y<getHeight();y+=3){g.setColour(Colours::white.withAlpha(.01f));g.drawHorizontalLine(y,0.f,float(getWidth()));}
 g.setColour(Colour(0xff76563d));g.drawRect(getLocalBounds(),2);
 for(auto point:{Point<float>(10,10),Point<float>(float(getWidth())-10,10),Point<float>(10,float(getHeight())-10),Point<float>(float(getWidth())-10,float(getHeight())-10)}){g.setColour(Colour(0xff74614e));g.fillEllipse(point.x-4,point.y-4,8,8);g.setColour(Colour(0xff18130f));g.drawLine(point.x-2.5f,point.y-1,point.x+2.5f,point.y+1,1.f);}
 g.setColour(Colour(0xfff7dfba));g.setFont(FontOptions("Georgia",26.f,Font::bold));g.drawText("Arp & Gate",34,10,300,34,Justification::centredLeft,false);
 g.setColour(boardMuted());g.setFont(FontOptions(11.f,Font::bold));g.drawText("EMBER  /  16-STEP PERFORMANCE BOARD",290,18,420,18,Justification::centredLeft,false);
 g.setColour(Colour(0xff856347));g.drawHorizontalLine(46,16.f,float(getWidth()-16));
 auto panel=[&](Rectangle<int> r,const String& title,const String& note){
  g.setColour(Colour(0xff332a21));g.fillRoundedRectangle(r.toFloat(),5.f);g.setColour(Colour(0xff71543c));g.drawRoundedRectangle(r.toFloat(),5.f,1.f);
  g.setColour(Colour(0xfff6c687));g.setFont(FontOptions(11.f,Font::bold));g.drawText(title,r.getX()+14,r.getY()+6,260,16,Justification::centredLeft,false);
  g.setColour(boardMuted());g.setFont(FontOptions(11.f));g.drawFittedText(note,Rectangle<int>(r.getRight()-440,r.getY()+30,424,r.getHeight()-38),Justification::centredRight,3);
 };
 panel({16,56,getWidth()-32,92},"01 / ARPEGGIATOR","Velocity shapes the notes the arpeggiator plays. Pattern, rate and length stay where they are on the compact strip.");
 panel({16,154,getWidth()-32,92},"02 / TRANCE GATE","Gate level and ties shape the output rhythm. The arp and the gate run on their own clocks, so a different rate on each makes the lanes drift against each other.");
 g.setColour(boardMuted());g.setFont(FontOptions(10.f));g.drawText("Drag to draw  /  double-click resets a step  /  arrow keys move and change  /  V G T pick a lane  /  Esc closes",Rectangle<int>(getWidth()-700,532,684,24),Justification::centredRight,false);
}
void Face::openArpBoard(){if(arpBoard==nullptr)return;arpBoard->setVisible(true);arpBoard->toFront(false);arpBoard->focusMatrix();}
void Face::closeArpBoard(){if(arpBoard==nullptr)return;arpBoard->setVisible(false);if(arpMore!=nullptr)arpMore->grabKeyboardFocus();}
Control& Face::control(const String& id,int x,int y,int w,int h,bool f,bool in){int index=SynthA::indexOf(id.toStdString());jassert(index>=0);auto c=std::make_unique<Control>(proc,index,f,in);c->setBounds(x,y,w,h);addAndMakeVisible(*c);c->slider.setLookAndFeel(&look);controls.push_back(std::move(c));return *controls.back();}
void Face::button(const String& text,Rectangle<int> bounds,std::function<void()> fn){auto b=std::make_unique<TextButton>(text);b->setBounds(bounds);b->onClick=std::move(fn);addAndMakeVisible(*b);buttons.push_back(std::move(b));}
void Face::toggle(const String& id,const String& text,Rectangle<int> bounds){auto b=std::make_unique<ToggleButton>(text);b->setBounds(bounds);addAndMakeVisible(*b);switchAttachments.push_back(std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(proc.state,id,*b));switches.push_back(std::move(b));}
Face::Face(SynthAProcessor& p):proc(p),keyboard(p.keyboard,MidiKeyboardComponent::horizontalKeyboard){
 setLookAndFeel(&look);setSize(1280,720);setWantsKeyboardFocus(true);
 presetBox.setBounds(245,25,335,28);addAndMakeVisible(presetBox);presetBox.onChange=[this]{int id=presetBox.getSelectedId();if(id>0){selectedPreset=id-1;proc.loadPreset(selectedPreset);}};
 categoryBox.setBounds(590,25,176,28);PresetBrowser::categories(categoryBox, proc.presetCount(), [this](int i){return proc.preset(i);});categoryBox.onChange=[this]{browse();};addAndMakeVisible(categoryBox);
 search.setBounds(776,25,180,28);search.setTextToShowWhenEmpty("Search sounds + tags",Colour(0xff9c876a));search.onTextChange=[this]{browse();};addAndMakeVisible(search);
 button("<",{245,59,28,22},[this]{const int next=PresetBrowser::nextIndex(presetBox,selectedPreset,-1);if(next>=0){selectedPreset=next;proc.loadPreset(next);browse();}});
 button(">",{278,59,28,22},[this]{const int next=PresetBrowser::nextIndex(presetBox,selectedPreset,1);if(next>=0){selectedPreset=next;proc.loadPreset(next);browse();}});
 button("LOAD",{315,59,51,22},[this]{loadFile();});button("SAVE",{372,59,51,22},[this]{saveFile();});button("A / B",{430,59,51,22},[this]{proc.compare();});
 button("UNDO",{488,59,51,22},[this]{proc.undo.undo();});button("MUTATE",{546,59,65,22},[this]{proc.mutate();});
 button("ALL CONTROLS",{622,59,102,22},[this]{showInspector("");});button("PANIC",{731,59,57,22},[this]{proc.panicRequested.store(true);});
 status.setBounds(800,58,365,25);status.setFont(FontOptions(10.f));status.setColour(Label::textColourId,amber());addAndMakeVisible(status);
 control("master.volume",1177,16,65,69);control("clock.bpm",975,18,75,64);control("voice.mode",1055,29,109,44);
 for(int o=0;o<3;++o){int x=28+o*192;String prefix="osc"+String(o+1)+".";
   button("...",{x+156,102,23,16},[this,o]{showInspector("Oscillator "+String(o+1));});
   control(prefix+"wave",x+8,122,82,39);control(prefix+"flavor",x+96,122,82,39);
   const char* top[]={"coarse","fine","pw","level"};const char* bottom[]={"uni.voices","uni.detune","uni.spread","uni.blend"};
   for(int k=0;k<4;++k){control(prefix+top[k],x+5+k*44,164,44,54);control(prefix+bottom[k],x+5+k*44,222,44,54);}
 }
 control("mix.sub",611,123,42,62);control("mix.noise",657,123,42,62);control("analog.age",611,190,42,61);control("osc1.fm",657,190,42,61);
 toggle("osc2.sync","SYNC",{614,256,82,18});button("...",{676,102,23,16},[this]{showInspector("Mixer");});
 for(int f=0;f<2;++f){int x=710+f*270;String pre="flt"+String(f+1)+".";
   toggle(pre+"on","ON",{x+161,102,50,18});button("...",{x+232,102,23,16},[this,f]{showInspector("Filter "+String(f+1));});
   control(pre+"model",x+8,123,118,39);control(pre+"mode",x+133,123,120,39);
   control(pre+"cutoff",x+8,166,96,105);control(pre+"res",x+112,170,65,58);control(pre+"drive",x+184,170,65,58);
   control(pre+"env",x+112,228,65,49);control(pre+"key",x+184,228,65,49);
 }
 for(int e=0;e<3;++e){int x=28+e*178;button("...",{x+145,291,22,16},[this,e]{showInspector("Envelope "+String(e+1));});const char* ids[]={"attack","decay","sustain","release"};for(int k=0;k<4;++k)control("env"+String(e+1)+"."+ids[k],x+7+k*41,316,39,105,true);}
 for(int l=0;l<3;++l){int x=28+l*178;button("...",{x+145,437,22,16},[this,l]{showInspector("LFO "+String(l+1));});control("lfo"+String(l+1)+".shape",x+8,462,80,40);control("lfo"+String(l+1)+".rate",x+92,458,72,59);}
 for(int row=0;row<12;++row){int y=313+row*17;String prefix="mod."+String(row+1)+".";
   auto& a=control(prefix+"source",590,y,106,16);a.label.setVisible(false);a.combo.setBounds(0,0,106,16);
   auto& b=control(prefix+"dest",702,y,194,16);b.label.setVisible(false);b.combo.setBounds(0,0,194,16);
   auto& amount=control(prefix+"amount",902,y,96,16,false,true);amount.label.setVisible(false);amount.slider.setSliderStyle(Slider::LinearHorizontal);amount.slider.setTextBoxStyle(Slider::TextBoxRight,false,34,14);amount.slider.setBounds(0,0,96,16);
 }
 for(int i=0;i<6;++i){int x=1025+(i%3)*73,y=315+(i/3)*91;auto id=i<4?"macro."+String(i+1):i==4?"macro.warmth":"macro.saturation";auto& c=control(id,x,y,68,81);
   if(i<4){c.label.setVisible(false);auto& label=macroLabels[i];label.setText(proc.macroName(i),dontSendNotification);label.setFont(FontOptions(9.f));label.setEditable(false,true);label.setJustificationType(Justification::centred);label.setBounds(x,y,68,13);label.onTextChange=[this,i]{proc.renameMacro(i,macroLabels[i].getText());};addAndMakeVisible(label);}
 }
 button("ROUTING",{1027,497,80,20},[this]{showInspector("Filter routing");});button("PERFORM",{1114,497,125,20},[this]{showInspector("Performance");});
 toggle("arp.on","ARP",{36,541,60,22});control("arp.mode",102,533,108,40);control("arp.rate",215,533,74,40);button("...",{296,544,24,20},[this]{openArpBoard();});arpMore=buttons.back().get();arpMore->setTooltip("Open the arp and gate board");
 toggle("seq.gate.on","GATE",{338,541,64,22});
 gateGrid=std::make_unique<GateGrid>(proc);gateGrid->setBounds(412,535,496,39);addAndMakeVisible(*gateGrid);
 button("GATE +",{916,544,67,20},[this]{showInspector("gate");});button("LEARN CHORD",{998,544,105,20},[this]{proc.learnChord();});button("CLOCK +",{1114,544,123,20},[this]{showInspector("Clock");});
 const char* effects[]={"drive","chorus","flanger","phaser","delay","reverb","eq","pump"};
 const char* main[]={"amount","rate","rate","rate","feedback","size","low","depth"};const char* extra[]={"tone","depth","depth","depth","tone","damp","high","curve"};
 for(int n=0;n<8;++n){int x=28+n*153;String pre="fx."+String(effects[n])+".";toggle(pre+"on",String(effects[n]).toUpperCase(),{x+7,586,110,17});button("...",{x+122,586,22,17},[this,n,effects]{showInspector("FX "+String(effects[n]));});control(pre+"mix",x+3,609,48,48);control(pre+main[n],x+51,609,48,48);control(pre+extra[n],x+99,609,48,48);}
 keyboard.setBounds(118,668,1129,39);keyboard.setAvailableRange(24,108);keyboard.setLowestVisibleKey(36);keyboard.setKeyWidth(20);keyboard.setVelocity(.8f,true);keyboard.setColour(MidiKeyboardComponent::whiteNoteColourId,Colour(0xffd5c5a8));keyboard.setColour(MidiKeyboardComponent::blackNoteColourId,Colour(0xff1c1814));keyboard.setColour(MidiKeyboardComponent::keyDownOverlayColourId,amber().withAlpha(.6f));addAndMakeVisible(keyboard);
 pitch.setSliderStyle(Slider::LinearVertical);pitch.setTextBoxStyle(Slider::NoTextBox,false,0,0);pitch.setRange(-1,1);pitch.setBounds(36,668,28,38);pitch.onValueChange=[this]{proc.uiPitch.store(float(pitch.getValue()));};pitch.onDragEnd=[this]{pitch.setValue(0);};addAndMakeVisible(pitch);
 mod.setSliderStyle(Slider::LinearVertical);mod.setTextBoxStyle(Slider::NoTextBox,false,0,0);mod.setRange(0,127,1);mod.setBounds(77,668,28,38);mod.onValueChange=[this]{proc.uiMod.store(float(mod.getValue()));};addAndMakeVisible(mod);
 arpBoard=std::make_unique<ArpBoard>(proc,look);arpBoard->setBounds(28,92,1224,568);arpBoard->onClose=[this]{closeArpBoard();};addChildComponent(*arpBoard);
 phosphor=Image(Image::ARGB,160,28,true);browse();timerCallback();startTimerHz(24);
}
Face::~Face(){stopTimer();inspectorWindow.reset();setLookAndFeel(nullptr);}
void Face::browse(){
 PresetBrowser::rebuild(presetBox, categoryBox, search.getText(), proc.presetCount(), [this](int i){ return proc.preset(i); });
 presetBox.setSelectedId(selectedPreset+1, dontSendNotification);
 if (presetBox.getSelectedId() == 0) presetBox.setText(proc.patchName(), dontSendNotification);
}
void Face::section(Graphics& g,Rectangle<int> r,const String& title){g.setGradientFill(ColourGradient(Colour(0xff332a21),float(r.getX()),float(r.getY()),Colour(0xff221c16),float(r.getX()),float(r.getBottom()),false));g.fillRoundedRectangle(r.toFloat(),4);g.setColour(Colour(0xff655037));g.drawRoundedRectangle(r.toFloat(),4,.6f);g.setColour(Colour(0xffb9955d));g.setFont(FontOptions(10.f,Font::bold));g.drawText(title,r.getX()+9,r.getY()+6,r.getWidth()-40,15,Justification::centredLeft);g.setColour(Colour(0xff59452e));g.drawHorizontalLine(r.getY()+24,float(r.getX()+7),float(r.getRight()-7));}
void Face::paint(Graphics& g){
 g.fillAll(Colour(0xff1b1612));
 for(int side:{0,1262}){g.setGradientFill(ColourGradient(Colour(0xff5b341d),float(side),0,Colour(0xff28180f),float(side+18),0,false));g.fillRect(side,0,18,720);for(int n=0;n<18;n+=3){g.setColour(Colour(0xff93603b).withAlpha(.15f));g.drawVerticalLine(side+n,0,720);}}
 for(int y=0;y<720;y+=3){g.setColour(Colours::white.withAlpha(.008f));g.drawHorizontalLine(y,19,1261);}
 g.setColour(cream());g.setFont(FontOptions(30.f,Font::bold));g.drawText("EMBER",34,22,180,35,Justification::centredLeft);g.setColour(amber());g.setFont(FontOptions(11.f));g.drawText("V A  /  ANALOG INSTRUMENT",36,60,200,18,Justification::centredLeft);
 for(int o=0;o<3;++o)section(g,{28+o*192,96,186,186},"OSCILLATOR 0"+String(o+1));section(g,{604,96,100,186},"MIXER");section(g,{710,96,264,186},"01 / EMBER FILTER");section(g,{980,96,272,186},"02 / COLOR FILTER");
 for(int e=0;e<3;++e)section(g,{28+e*178,287,172,141},String(e==0?"AMP":e==1?"FILTER":"FREE")+" / ENV 0"+String(e+1));
 for(int l=0;l<3;++l)section(g,{28+l*178,433,172,90},"LFO 0"+String(l+1));
 section(g,{566,287,444,236},"MODULATION / 12 ROUTES");section(g,{1016,287,236,236},"MACROS / CHARACTER");
 g.setFont(FontOptions(9.f));for(int n=0;n<12;++n){g.setColour(Colour(0xffa88b61));g.drawText(String(n+1),570,313+n*17,16,16,Justification::centred);}
 g.setColour(Colour(0xff332a21));g.fillRoundedRectangle(28,529,1224,48,4);for(int n=0;n<8;++n)section(g,{28+n*153,581,147,80},"");
 g.setColour(Colour(0xff0c0b08));g.fillRoundedRectangle(974,1,192,14,2);g.setColour(amber());g.setFont(FontOptions(8.f));g.drawText("2x ENGINE  /  16 VOICES  /  STEREO",974,1,192,14,Justification::centred);
 g.drawImage(phosphor,800,61,160,24,0,0,160,28);
 // Small live response sketches are visual guides to cutoff/resonance, not measured transfer plots.
 for(int f=0;f<2;++f){const int x=710+f*270;float cutoff=proc.getParam("flt"+String(f+1)+".cutoff"),res=proc.getParam("flt"+String(f+1)+".res");Path response;for(int j=0;j<82;++j){float hz=20*std::pow(1000.f,float(j)/81),ratio=hz/cutoff;float magnitude=1/std::sqrt(1+std::pow(ratio,proc.getParam("flt"+String(f+1)+".model")<.5f?8.f:4.f));magnitude+=res*.45f*std::exp(-std::pow(std::log2(std::max(.0001f,ratio))*2.5f,2.f));float y=184-std::min(1.3f,magnitude)*10;if(j==0)response.startNewSubPath(float(x+14),y);else response.lineTo(float(x+14+j),y);}g.setColour(amber().withAlpha(.35f));g.strokePath(response,PathStrokeType(.8f));}
 for(int o=0;o<3;++o){int x=28+o*192;int wave=int(proc.getParam("osc"+String(o+1)+".wave"));Path path;for(int j=0;j<26;++j){float v=SynthA::wave(float(j)/26,.01f,wave,.5f);if(j==0)path.startNewSubPath(float(x+125),109-v*4);else path.lineTo(float(x+125+j),109-v*4);}g.setColour(amber().withAlpha(.7f));g.strokePath(path,PathStrokeType(.8f));}
 for(auto point:{Point<int>(23,14),Point<int>(1257,14),Point<int>(23,712),Point<int>(1257,712)}){g.setColour(Colour(0xff74614e));g.fillEllipse(float(point.x-3),float(point.y-3),6,6);g.setColour(Colour(0xff18130f));g.drawLine(float(point.x-2),float(point.y-1),float(point.x+2),float(point.y+1));}
}
void Face::timerCallback(){
 if(lastName!=proc.patchName()){lastName=proc.patchName();presetBox.setText(lastName,dontSendNotification);for(int i=0;i<4;++i)macroLabels[i].setText(proc.macroName(i),dontSendNotification);}
 status.setBounds(36,80,200,12);status.setText(proc.learnParameter.load()>=0?"MOVE A MIDI CONTROL...":String(proc.voicesPlaying.load())+" / 16   CPU "+String(proc.cpu.load()*100,0)+"%",dontSendNotification);
 Graphics g(phosphor);g.setColour(Colour(0xff17130c).withAlpha(.25f));g.fillAll();Path path;const unsigned w=proc.engine.scopeWrite.load(std::memory_order_acquire);for(int x=0;x<160;++x){float value=proc.engine.scope[(w+512-160+unsigned(x))%512].load(std::memory_order_relaxed);float y=14-value*70;if(x==0)path.startNewSubPath(0,y);else path.lineTo(float(x),y);}g.setColour(amber().withAlpha(.8f));g.strokePath(path,PathStrokeType(1));repaint();
}
void Face::showInspector(const String& query){
 struct Window:DocumentWindow {Window():DocumentWindow("Ember VA - All controls",Colour(0xff211b16),DocumentWindow::closeButton){}void closeButtonPressed()override{setVisible(false);}};
 inspectorWindow=std::make_unique<Window>();inspectorWindow->setLookAndFeel(&look);inspectorWindow->setUsingNativeTitleBar(true);inspectorWindow->setContentOwned(new Inspector(proc,query,look),true);inspectorWindow->setResizable(true,true);inspectorWindow->centreWithSize(760,640);inspectorWindow->setVisible(true);
}
void Face::loadFile(){chooser=std::make_unique<FileChooser>("Load Ember VA patch",File::getSpecialLocation(File::userDocumentsDirectory),"*.json");Component::SafePointer<Face> safe(this);chooser->launchAsync(FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles,[safe](const FileChooser& fc){if(!safe)return;auto f=fc.getResult();if(f.existsAsFile()){auto result=safe->proc.loadPatchFile(f);if(result.failed())AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,"Patch could not load",result.getErrorMessage());}});}
void Face::saveFile(){chooser=std::make_unique<FileChooser>("Save Ember VA patch",File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(proc.patchName()+".json"),"*.json");Component::SafePointer<Face> safe(this);chooser->launchAsync(FileBrowserComponent::saveMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::warnAboutOverwriting,[safe](const FileChooser& fc){if(!safe)return;auto f=fc.getResult();if(f!=File()){auto result=safe->proc.savePatchFile(f.withFileExtension("json"));if(result.failed())AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,"Patch could not save",result.getErrorMessage());}});}
}
SynthAEditor::SynthAEditor(SynthAProcessor& p):AudioProcessorEditor(p),face(p){addAndMakeVisible(face);setResizable(true,true);ratio.setFixedAspectRatio(16.0/9);ratio.setSizeLimits(1000,562,2560,1440);setConstrainer(&ratio);setSize(1280,720);}
void SynthAEditor::resized(){face.setBounds(0,0,1280,720);face.setTransform(AffineTransform::scale(float(getWidth())/1280,float(getHeight())/720));}

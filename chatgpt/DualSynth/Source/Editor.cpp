#include "Host.h"
#include "sa-editor.h"
#include "sb-ui.h"
using namespace juce;
namespace {
Colour navy(){return Colour(0xff0b1020);}Colour aqua(){return Colour(0xff4fe3f5);}Colour ice(){return Colour(0xffeaf2ff);}
}
DualEditor::DualEditor(DualProcessor& processor):AudioProcessorEditor(processor),p(processor),keys(p.keyboard,MidiKeyboardComponent::horizontalKeyboard){
 // Read the remembered width first: setting the resize limits below resizes the editor and would overwrite it.
 const int savedWidth=int(p.state.state.getProperty("uiWidth",1600));
 look.setColour(ResizableWindow::backgroundColourId,navy());look.setColour(Label::textColourId,ice());look.setColour(Slider::thumbColourId,aqua());look.setColour(Slider::rotarySliderFillColourId,aqua());look.setColour(Slider::rotarySliderOutlineColourId,Colour(0xff2e3c59));look.setColour(TextButton::buttonColourId,Colour(0xff202d49));look.setColour(TextButton::buttonOnColourId,Colour(0xff4d4783));look.setColour(Slider::textBoxBackgroundColourId,Colour(0xff101a2e));look.setColour(Slider::textBoxTextColourId,ice());look.setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);look.setColour(ComboBox::backgroundColourId,Colour(0xff131e34));look.setColour(ComboBox::outlineColourId,Colour(0xff33425d));setLookAndFeel(&look);
 addAndMakeVisible(surface);surface.setSize(1600,840);surface.addAndMakeVisible(clip);clip.setBounds(0,64,1280,720);
 editorA.reset(p.a.createEditor());editorB.reset(p.b.createEditor());for(auto* editor:{editorA.get(),editorB.get()}){editor->setResizable(false,false);editor->setSize(1280,720);editor->setTopLeftPosition(0,0);clip.addAndMakeVisible(editor);}
 // Original editors remain mounted. Each is told explicitly to hide its own keyboard and wheels; the host provides shared ones.
 if(auto* embedded=dynamic_cast<SynthAEditor*>(editorA.get()))embedded->setEmbedded(true);
 if(auto* embedded=dynamic_cast<PrismEditor*>(editorB.get()))embedded->setEmbedded(true);
 for(auto* button:{&showA,&showB,&panicButton,&save,&load})surface.addAndMakeVisible(button);
 showA.setBounds(247,15,192,34);showB.setBounds(449,15,192,34);showA.onClick=[this]{select(0);};showB.onClick=[this]{select(1);};save.setBounds(823,18,142,28);load.setBounds(975,18,68,28);panicButton.setBounds(1177,18,85,28);save.onClick=[this]{file(true);};load.onClick=[this]{file(false);};panicButton.onClick=[this]{p.panic.store(true);};
 keys.setBounds(114,750,1148,32);keys.setAvailableRange(24,108);keys.setLowestVisibleKey(36);keys.setKeyWidth(20);keys.setVelocity(.8f,true);keys.setColour(MidiKeyboardComponent::whiteNoteColourId,Colour(0xffc8d5ed));keys.setColour(MidiKeyboardComponent::blackNoteColourId,Colour(0xff101b30));keys.setColour(MidiKeyboardComponent::keyDownOverlayColourId,aqua().withAlpha(.6f));surface.addAndMakeVisible(keys);
 for(auto* slider:{&pitch,&mod}){slider->setSliderStyle(Slider::LinearVertical);slider->setTextBoxStyle(Slider::NoTextBox,false,0,0);surface.addAndMakeVisible(slider);}pitch.setRange(-1,1);pitch.setBounds(28,749,28,34);pitch.onValueChange=[this]{p.bend.store(float(pitch.getValue()));};pitch.onDragEnd=[this]{pitch.setValue(0);};mod.setRange(0,127,1);mod.setBounds(70,749,28,34);mod.onValueChange=[this]{p.wheel.store(float(mod.getValue()));};
 dial("master",1077,8,76,49);
 // ---- Keyboard routing: mode, split note and swap, tap tempo -------------------------------------------------------------
 menu("routing",{"Layer","Split","A only","B only"},1308,102,264,26);
 dial("split",1308,132,170,44);toggle("swap","SWAP",1488,142,84);
 ControlBay::buttonColours(tapButton);surface.addAndMakeVisible(tapButton);tapButton.setBounds(1308,181,56,24);tapButton.onClick=[this]{p.tapTempo();};
 surface.addAndMakeVisible(tapLabel);tapLabel.setBounds(1368,181,76,24);tapLabel.setFont(FontOptions(10.f));tapLabel.setColour(Label::textColourId,ControlBay::secondary);tapLabel.setJustificationType(Justification::centredLeft);
 menu("tap.target",{"Tap: both","Tap: A","Tap: B"},1448,181,124,24);
 // ---- Per-engine note range and channel ------------------------------------------------------------------------------------
 dial("a.low",1308,224,74,50);dial("a.high",1390,224,74,50);dial("a.channel",1472,224,100,50);
 dial("b.low",1308,292,74,50);dial("b.high",1390,292,74,50);dial("b.channel",1472,292,100,50);
 // ---- Output mixer: gain, pan, mute, solo and level bars for each strip ----------------------------------------------------
 dial("a.gain",1308,384,86,72);surface.addAndMakeVisible(barsA);barsA.setBounds(1402,388,30,64);dial("a.pan",1308,456,128,44);toggle("a.mute","MUTE",1308,502,62);toggle("a.solo","SOLO",1374,502,62);
 dial("b.gain",1444,384,86,72);surface.addAndMakeVisible(barsB);barsB.setBounds(1538,388,30,64);dial("b.pan",1444,456,128,44);toggle("b.mute","MUTE",1444,502,62);toggle("b.solo","SOLO",1510,502,62);
 // ---- Master section: switch, then three tabs ------------------------------------------------------------------------------
 toggle("glue","MASTER FX",1458,532,114);
 const char* tabNames[3]={"COMP","REVERB","TONE"};
 for(int i=0;i<3;++i){auto& tab=tabButtons[size_t(i)];tab.setButtonText(tabNames[i]);tab.setClickingTogglesState(false);tab.setBounds(1308+i*89,558,86,22);ControlBay::buttonColours(tab);surface.addAndMakeVisible(tab);tab.onClick=[this,i]{showMasterTab(i);};}
 currentGroup=CompShared;menu("comp.mode",{"Glue","FET 76"},1308,586,100,24);toggle("comp","COMP ON",1414,586,100);
 surface.addAndMakeVisible(reductionMeter);reductionMeter.setBounds(1308,614,264,80);
 reductionMeter.onEnable=[this]{if(auto* master=p.state.getParameter("glue")){master->beginChangeGesture();master->setValueNotifyingHost(1.f);master->endChangeGesture();}};grouped.push_back({&reductionMeter,CompShared});
 currentGroup=CompFet;
 toggle("fet.hpf","90 Hz SC",1520,586,52);
 ratioButtons=std::make_unique<RatioButtons>(p.state,"fet.ratio",StringArray{"4","8","12","20","ALL"});surface.addAndMakeVisible(*ratioButtons);ratioButtons->setBounds(1308,698,264,22);grouped.push_back({ratioButtons.get(),CompFet});
 dial("fet.input",1308,724,52,70);dial("fet.output",1361,724,52,70);dial("fet.attack",1414,724,52,70);dial("fet.release",1467,724,52,70);dial("fet.mix",1520,724,52,70);
 currentGroup=CompGlue;dial("threshold",1308,724,62,70);dial("ratio",1375,724,62,70);dial("attack",1442,724,62,70);dial("release",1509,724,63,70);
 currentGroup=ReverbTab;toggle("verb","REVERB ON",1308,586,120);dial("room",1308,618,84,72);dial("damp",1398,618,84,72);dial("wet",1488,618,84,72);dial("verb.predelay",1308,698,84,72);dial("verb.lowcut",1398,698,84,72);
 currentGroup=ToneTab;toggle("eq","TONE EQ ON",1308,586,130);dial("low",1308,618,84,72);dial("air",1398,618,84,72);dial("width",1488,618,84,72);
 currentGroup=Always;
 surface.addAndMakeVisible(barsOut);barsOut.setBounds(1334,800,238,16);
 meter.setColour(Label::textColourId,aqua());meter.setFont(FontOptions(10.f));meter.setJustificationType(Justification::centredRight);surface.addAndMakeVisible(meter);
 status.setBounds(28,796,830,24);status.setFont(FontOptions(11.f));status.setColour(Label::textColourId,Colour(0xff94a6c7));surface.addAndMakeVisible(status);
 showMasterTab(0);
 // Size: the design is 1600 x 840 and is drawn as vectors, so it is sharpest at exactly 100% (and at 150%, 200%).
 // The chosen width is remembered with the project; a new instance opens at 100%.
 meter.setBounds(880,796,284,24);
 sizeBox.addItemList({"90%","100%","125%","150%","200%"},1);sizeBox.setTooltip("Window size. 100% is sharpest.");sizeBox.setBounds(1172,796,90,24);surface.addAndMakeVisible(sizeBox);
 sizeBox.onChange=[this]{static const double steps[]={.9,1.,1.25,1.5,2.};const int index=jlimit(0,4,sizeBox.getSelectedId()-1);setSize(int(std::lround(1600*steps[index])),int(std::lround(840*steps[index])));};
 select(p.panel.load());setResizable(true,true);setResizeLimits(1200,630,3200,1680);getConstrainer()->setFixedAspectRatio(1600.0/840.0);
 {const int width=jlimit(1200,3200,savedWidth);setSize(width,int(std::lround(width*840.0/1600.0)));}timerCallback();startTimerHz(24);
}
DualEditor::~DualEditor(){stopTimer();editorB.reset();editorA.reset();setLookAndFeel(nullptr);}
void DualEditor::select(int which){p.panel.store(which);editorA->setVisible(which==0);editorB->setVisible(which==1);showA.setToggleState(which==0,dontSendNotification);showB.setToggleState(which==1,dontSendNotification);repaint();}
void DualEditor::resized(){surface.setBounds(0,0,1600,840);surface.setTransform(AffineTransform::scale(float(getWidth())/1600,float(getHeight())/840));
 if(getWidth()>0){p.state.state.setProperty("uiWidth",getWidth(),nullptr);
  static const double steps[]={.9,1.,1.25,1.5,2.};int match=0;for(int i=0;i<5;++i)if(std::abs(getWidth()-1600*steps[i])<std::abs(getWidth()-1600*steps[match]))match=i;
  if(std::abs(getWidth()-1600*steps[match])<2)sizeBox.setSelectedId(match+1,dontSendNotification);else sizeBox.setText(String(int(std::lround(100.0*getWidth()/1600)))+"%",dontSendNotification);}}
void DualEditor::paint(Graphics& g){g.fillAll(navy());g.addTransform(AffineTransform::scale(float(getWidth())/1600,float(getHeight())/840));g.setColour(ice());g.setFont(FontOptions(23.f,Font::bold));g.drawText("AETHER v4.1",26,13,218,35,Justification::centredLeft);g.setColour(aqua());g.setFont(FontOptions(10.f));g.drawText("TWO ENGINES  /  ONE PERFORMANCE",26,47,300,12,Justification::centredLeft);
 const Rectangle<float> bay(1290, 76, 298, 752);
 g.setColour(Colours::black.withAlpha(.5f));
 g.fillRoundedRectangle(bay.translated(0, 3), 12);
 g.setGradientFill(ColourGradient(Colour(0xff2a2b29), 1290, 76, ControlBay::ink, 1588, 828, false));
 g.fillRoundedRectangle(bay, 12);
 g.setColour(ControlBay::border);
 g.drawRoundedRectangle(bay, 12, 1);
 for (float x : {1298.f, 1580.f})
  for (float y : {85.f, 819.f}) {
   g.setColour(ControlBay::ink);
   g.fillEllipse(x - 2.5f, y - 2.5f, 5, 5);
   g.setColour(Colour(0xff62645b));
   g.drawLine(x - 1.4f, y + 1, x + 1.4f, y - 1, .7f);
  }

 g.setColour(ControlBay::ink);
 g.fillRoundedRectangle(1290, 15, 298, 48, 8);
 g.setColour(ControlBay::signal);
 g.fillRoundedRectangle(1307, 25, 3, 27, 1.5f);
 g.setColour(ControlBay::text);
 g.setFont(FontOptions(15.f, Font::bold));
 g.drawText("CONTROL BAY", 1318, 20, 160, 22, Justification::centredLeft);
 g.setColour(ControlBay::secondary);
 g.setFont(FontOptions(9.f));
 g.drawText("AETHER v4.1 / SIGNAL PATH", 1318, 41, 230, 13, Justification::centredLeft);
 g.drawText("01 / 03", 1510, 22, 58, 17, Justification::centredRight);

 auto section = [&](int y, int height, const String& number, const String& title) {
  g.setColour(Colour(0xff1c1d1c));
  g.fillRoundedRectangle(1302.f, float(y), 274.f, float(height), 7);
  g.setColour(Colour(0xff393b35));
  g.drawRoundedRectangle(1302.f, float(y), 274.f, float(height), 7, .8f);
  g.setFont(FontOptions(10.f, Font::bold));
  g.setColour(ControlBay::signal);
  g.drawText(number, 1310, y + 3, 23, 17, Justification::centredLeft);
  g.setColour(ControlBay::text);
  g.drawText(title, 1334, y + 3, 125, 17, Justification::centredLeft);
 };
 section(80, 278, "01", "KEYBOARD ROUTING");
 section(362, 166, "02", "OUTPUT MIXER");
 section(532, 290, "03", "MASTER GLUE");
 g.setColour(ControlBay::border);
 g.drawVerticalLine(1440, 386, 522);

 g.setFont(FontOptions(9.f, Font::bold));
 g.setColour(ControlBay::text);
 g.drawText("A / EMBER VA", 1310, 207, 112, 17, Justification::centredLeft);
 g.drawText("B / PRISM FLUX", 1310, 275, 112, 17, Justification::centredLeft);
 g.drawText("OUT", 1308, 798, 26, 20, Justification::centredLeft);
 g.setColour(ControlBay::secondary);
 g.setFont(FontOptions(9.f));
 g.drawText("NOTE RANGE + CHANNEL", 1420, 207, 150, 17, Justification::centredRight);
 g.drawText("NOTE RANGE + CHANNEL", 1420, 275, 150, 17, Justification::centredRight);
 g.drawText("CH 0 = OMNI / SWAP REVERSES THE SPLIT", 1310, 343, 259, 12, Justification::centredLeft);
}
void DualEditor::dial(String id,int x,int y,int w,int h){auto label=std::make_unique<Label>();auto* param=p.state.getParameter(id);String title=param->name;
 if(id.endsWith(".channel"))title="Channel";else if(id.endsWith(".low")&&id!="verb.lowcut")title="Low note";else if(id.endsWith(".high"))title="High note";else if(id=="room")title="Size";else if(id=="damp")title="Damping";else if(id=="width")title="Width";
 else if(id=="a.pan"||id=="b.pan")title=id.startsWithChar('a')?"A pan":"B pan";else if(id=="a.gain"||id=="b.gain")title=id.startsWithChar('a')?"A level dB":"B level dB";
 else if(id=="fet.input")title="Input";else if(id=="fet.output")title="Output";else if(id=="fet.attack")title="Attack us";else if(id=="fet.release")title="Release ms";else if(id=="fet.mix")title="Mix";
 else if(id=="verb.predelay")title="Pre-delay ms";else if(id=="verb.lowcut")title="Low cut Hz";else if(id=="low")title="Low shelf dB";else if(id=="air")title="Air shelf dB";else if(id=="wet")title="Mix";
 label->setText(title,dontSendNotification);label->setFont(FontOptions(9.f));label->setJustificationType(Justification::centred);label->setBounds(x,y,w,16);if(x>=1290)label->setColour(Label::textColourId,ControlBay::text);surface.addAndMakeVisible(*label);grouped.push_back({label.get(),currentGroup});controls.push_back(std::move(label));
 auto slider=std::make_unique<Slider>();slider->setSliderStyle(h<60?Slider::LinearHorizontal:Slider::RotaryHorizontalVerticalDrag);slider->setTextBoxStyle(Slider::TextBoxBelow,false,w,16);slider->setBounds(x,y+16,w,h-16);
 if(x>=1290){
  slider->setColour(Slider::thumbColourId,ControlBay::signal);
  slider->setColour(Slider::trackColourId,ControlBay::signal);
  slider->setColour(Slider::backgroundColourId,ControlBay::ink);
  slider->setColour(Slider::textBoxBackgroundColourId,ControlBay::ink);
  slider->setColour(Slider::textBoxTextColourId,ControlBay::text);
 }surface.addAndMakeVisible(*slider);sliders.push_back(std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.state,id,*slider));
 slider->textFromValueFunction=[integer=param->isDiscrete(),pan=id.endsWith(".pan")](double v){if(pan){const int percent=int(std::lround(std::abs(v)*100));return percent==0?String("C"):String(v<0?"L":"R")+String(percent);}return String(std::abs(v)<.0001?0:v,integer?0:2);};slider->updateText();grouped.push_back({slider.get(),currentGroup});controls.push_back(std::move(slider));}
void DualEditor::toggle(String id,String name,int x,int y,int w){auto button=std::make_unique<ToggleButton>(name);button->setBounds(x,y,w,24);surface.addAndMakeVisible(*button);toggles.push_back(std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.state,id,*button));grouped.push_back({button.get(),currentGroup});controls.push_back(std::move(button));}
void DualEditor::menu(String id,StringArray labels,int x,int y,int w,int height){auto combo=std::make_unique<ComboBox>();combo->addItemList(labels,1);combo->setBounds(x,y,w,height);
 if(x>=1290){
  combo->setColour(ComboBox::backgroundColourId,ControlBay::ink);
  combo->setColour(ComboBox::outlineColourId,ControlBay::border);
  combo->setColour(ComboBox::textColourId,ControlBay::text);
  combo->setColour(ComboBox::arrowColourId,ControlBay::signal);
 }surface.addAndMakeVisible(*combo);choices.push_back(std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,id,*combo));grouped.push_back({combo.get(),currentGroup});controls.push_back(std::move(combo));}
void DualEditor::showMasterTab(int which){masterTab=which;for(size_t i=0;i<tabButtons.size();++i)tabButtons[i].setToggleState(int(i)==which,dontSendNotification);refreshMasterTabs();}
void DualEditor::refreshMasterTabs(){
 const bool fet=int(p.get(DualProcessor::CompMode))==DualProcessor::Fet76;
 for(auto& entry:grouped){bool show=true;switch(entry.second){case Always:show=true;break;case CompShared:show=masterTab==0;break;case CompFet:show=masterTab==0&&fet;break;case CompGlue:show=masterTab==0&&!fet;break;case ReverbTab:show=masterTab==1;break;case ToneTab:show=masterTab==2;break;}
  if(entry.first->isVisible()!=show)entry.first->setVisible(show);}
}
void DualEditor::timerCallback(){if(editorA->isVisible()!=(p.panel.load()==0))select(p.panel.load());
 auto db=[](float x){return x>1.e-7f?String(Decibels::gainToDecibels(x),1):String("-inf");};float out=p.peaks[2].load();meter.setText("A "+db(p.peaks[0].load())+"   B "+db(p.peaks[1].load())+"   OUT "+db(out)+" dB   /   GR "+String(p.gainReduction.load(),1)+" dB",dontSendNotification);meter.setColour(Label::textColourId,out>1?Colours::salmon:aqua());
 barsA.setLevels(p.levels[0].load(),p.levels[1].load());barsB.setLevels(p.levels[2].load(),p.levels[3].load());barsOut.setLevels(p.levels[4].load(),p.levels[5].load());
 reductionMeter.setBypassed(p.value("glue")<.5f);
 reductionMeter.setTitle(int(p.get(DualProcessor::CompMode))==DualProcessor::Fet76?"FET 76":"GLUE");reductionMeter.setReduction(p.gainReduction.load());reductionMeter.tick();
 const float bpm=p.lastTapBpm();tapLabel.setText(bpm>0?String(bpm,1)+" BPM":"tap tempo",dontSendNotification);
 refreshMasterTabs();
 String route=p.value("routing")==0?"LAYER":p.value("routing")==1?"SPLIT at "+MidiMessage::getMidiNoteName(int(p.value("split")),true,true,4)+(p.value("swap")>.5f?" (swapped)":""):p.value("routing")==2?"A ONLY":"B ONLY";
 status.setText(route+"    /    Shared MIDI keyboard    /    Master FX "+String(p.value("glue")>.5f?"ON":"BYPASSED"),dontSendNotification);}
void DualEditor::file(bool saving){chooser=std::make_unique<FileChooser>(saving?"Save dual performance":"Load dual performance",File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("Dual Performance.dualsynth"),"*.dualsynth");Component::SafePointer<DualEditor> safe(this);chooser->launchAsync(saving?FileBrowserComponent::saveMode|FileBrowserComponent::canSelectFiles|FileBrowserComponent::warnAboutOverwriting:FileBrowserComponent::openMode|FileBrowserComponent::canSelectFiles,[safe,saving](const FileChooser& chosen){if(!safe||chosen.getResult()==File())return;auto file=chosen.getResult();if(saving){MemoryBlock block;safe->p.getStateInformation(block);if(!file.withFileExtension("dualsynth").replaceWithData(block.getData(),block.getSize()))AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,"Performance","Could not save performance.");}else{
  auto warn=[](const String& why){AlertWindow::showMessageBoxAsync(MessageBoxIconType::WarningIcon,"Performance not loaded",why+"\nNothing was changed.");};
  if(file.getSize()>DualProcessor::maxPerformanceBytes){warn("The file is larger than 8 MB.");return;}
  MemoryBlock block;if(!file.loadFileAsData(block)){warn("The file could not be read.");return;}
  auto result=safe->p.loadPerformance(block.getData(),int(block.getSize()));if(result.failed())warn(result.getErrorMessage());
 }});}

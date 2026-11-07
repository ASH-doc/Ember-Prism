// sa-editor.h — One native, scalable panel used by both standalone and VST3.
#pragma once
#include "sa-processor.h"
namespace SynthAUI {
class Workbench;
inline juce::Colour amber(){return juce::Colour(0xffffb23e);}inline juce::Colour cream(){return juce::Colour(0xffe9dcc3);}
class HardwareLook final:public juce::LookAndFeel_V4 {
 public:HardwareLook();
 void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&)override;
 void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&)override;
 void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool)override;
 juce::Label* createSliderTextBox(juce::Slider&)override;
 juce::Font getComboBoxFont(juce::ComboBox&)override{return juce::Font(juce::FontOptions(10.f));}
};
class Control final:public juce::Component {
 public:Control(SynthAProcessor&,int,bool fader=false,bool inlineLabel=false,bool rotaryChoice=false);void resized()override;void paint(juce::Graphics&)override;
 juce::Slider slider;juce::ComboBox combo;juce::Label label;int parameter;
 private:SynthAProcessor& proc;bool fader,inlineLabel;
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sa;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ca;
 void menu(const juce::MouseEvent&);void mouseDown(const juce::MouseEvent&)override;
};
class Inspector final:public juce::Component {
 public:Inspector(SynthAProcessor&,const juce::String&,HardwareLook&);void resized()override;
 private:SynthAProcessor& proc;juce::TextEditor search;juce::Viewport viewport;juce::Component list;
 std::vector<std::unique_ptr<Control>> controls;void filter();
};
class GateGrid final:public juce::Component {
 public:explicit GateGrid(SynthAProcessor& p):proc(p){}void paint(juce::Graphics&)override;void mouseDown(const juce::MouseEvent&)override;void mouseDrag(const juce::MouseEvent&)override;
 private:SynthAProcessor& proc;int previous=-1;float previousValue=0;bool ties=false;void edit(const juce::MouseEvent&);
};
// The 16-step lanes of the arp and gate board: arp velocity, gate level and gate ties share one column grid.
// Every edit goes through SynthAProcessor::setParam, so automation, undo and preset refresh behave as for any other control.
class StepMatrix final:public juce::Component {
 public:
 explicit StepMatrix(SynthAProcessor& p):proc(p){setWantsKeyboardFocus(true);}
 std::function<void(const juce::String&)> onReadout;
 void paint(juce::Graphics&)override;void mouseDown(const juce::MouseEvent&)override;void mouseDrag(const juce::MouseEvent&)override;
 void mouseUp(const juce::MouseEvent&)override{previous=-1;}void mouseDoubleClick(const juce::MouseEvent&)override;bool keyPressed(const juce::KeyPress&)override;
 void focusGained(FocusChangeType)override{repaint();}void focusLost(FocusChangeType)override{repaint();}
 private:
 enum Lane{Velocity,Gate,Tie};
 SynthAProcessor& proc;int selectedStep=0;Lane selectedLane=Velocity;int previous=-1;float previousValue=0;bool paintTie=false;
 float columnWidth()const{return float(getWidth()-gridLeft-2)/16.f;}
 juce::Rectangle<float> laneRect(Lane)const;int stepAt(float x)const;Lane laneAt(float y)const;
 juce::String idFor(Lane,int step)const;float valueFor(Lane,float y)const;void apply(Lane,int step,float value);void edit(const juce::MouseEvent&,bool first);
 void report(Lane,int step);
 static constexpr int gridLeft=112;
};
class ArpBoard final:public juce::Component,private juce::Timer {
 public:
 ArpBoard(SynthAProcessor&,HardwareLook&);~ArpBoard()override{stopTimer();}
 std::function<void()> onClose;
 void paint(juce::Graphics&)override;void resized()override;bool keyPressed(const juce::KeyPress&)override;
 void visibilityChanged()override{if(isVisible())startTimerHz(15);else stopTimer();}
 void focusMatrix(){matrix.grabKeyboardFocus();}StepMatrix& stepMatrix(){return matrix;}
 private:
 SynthAProcessor& proc;StepMatrix matrix;juce::TextButton closeButton{"CLOSE"};juce::Label readout;
 std::vector<std::unique_ptr<Control>> controls;std::vector<std::unique_ptr<juce::ToggleButton>> switches;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> attachments;
 HardwareLook& look;
 Control& knob(const juce::String&,int,int,int,int);void sw(const juce::String&,const juce::String&,juce::Rectangle<int>);
 void timerCallback()override{matrix.repaint();}
};
class Face final:public juce::Component,private juce::Timer {
 public:explicit Face(SynthAProcessor&);~Face()override;void paint(juce::Graphics&)override;
 // Embedded mode: the host owns the shared keyboard and wheels, so this panel hides its own.
 void setEmbedded(bool embedded){keyboard.setVisible(!embedded);pitch.setVisible(!embedded);mod.setVisible(!embedded);}
 // The arp and gate board is an overlay owned by this panel (opened from the three dots beside the compact arp strip).
 void showWorkbench(bool);
 bool workbenchShown()const;
 void openArpBoard();void closeArpBoard();bool arpBoardOpen()const{return arpBoard!=nullptr&&arpBoard->isVisible();}
 StepMatrix* stepMatrix(){return arpBoard!=nullptr?&arpBoard->stepMatrix():nullptr;}
 private:
 SynthAProcessor& proc;HardwareLook look;juce::TooltipWindow tooltips{this,600};
 std::vector<std::unique_ptr<Control>> controls;std::vector<std::unique_ptr<juce::TextButton>> buttons;
 std::vector<std::unique_ptr<juce::ToggleButton>> switches;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> switchAttachments;
 juce::ComboBox presetBox,categoryBox;juce::TextEditor search;juce::Label status;
 juce::MidiKeyboardComponent keyboard;juce::Slider pitch,mod;
 juce::Label macroLabels[4];std::unique_ptr<juce::FileChooser> chooser;
 std::unique_ptr<Workbench> workbench;
 std::vector<juce::Component*> advancedControls;
 juce::TextButton viewButton{"ADVANCED VIEW"};
 std::unique_ptr<GateGrid> gateGrid;std::unique_ptr<ArpBoard> arpBoard;juce::Component::SafePointer<juce::TextButton> arpMore;
 std::unique_ptr<juce::DocumentWindow> inspectorWindow;int selectedPreset=0;
 juce::String lastName;juce::Image phosphor;
 Control& control(const juce::String&,int,int,int,int,bool fader=false,bool inlineLabel=false);
 void button(const juce::String&,juce::Rectangle<int>,std::function<void()>);
 void toggle(const juce::String&,const juce::String&,juce::Rectangle<int>);
 void section(juce::Graphics&,juce::Rectangle<int>,const juce::String&);
 void showInspector(const juce::String&);void browse();void loadFile();void saveFile();
 void timerCallback()override;
};
}
class SynthAEditor final:public juce::AudioProcessorEditor {
 public:explicit SynthAEditor(SynthAProcessor&);void resized()override;void setEmbedded(bool embedded){face.setEmbedded(embedded);}
 SynthAUI::Face& panel(){return face;}
 private:SynthAUI::Face face;juce::ComponentBoundsConstrainer ratio;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthAEditor)
};

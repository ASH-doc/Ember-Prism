// sa-editor.h — One native, scalable panel used by both standalone and VST3.
#pragma once
#include "sa-processor.h"
namespace SynthAUI {
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
 public:Control(SynthAProcessor&,int,bool fader=false,bool inlineLabel=false);void resized()override;void paint(juce::Graphics&)override;
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
class Face final:public juce::Component,private juce::Timer {
 public:explicit Face(SynthAProcessor&);~Face()override;void paint(juce::Graphics&)override;
 // Embedded mode: the host owns the shared keyboard and wheels, so this panel hides its own.
 void setEmbedded(bool embedded){keyboard.setVisible(!embedded);pitch.setVisible(!embedded);mod.setVisible(!embedded);}
 private:
 SynthAProcessor& proc;HardwareLook look;juce::TooltipWindow tooltips{this,600};
 std::vector<std::unique_ptr<Control>> controls;std::vector<std::unique_ptr<juce::TextButton>> buttons;
 std::vector<std::unique_ptr<juce::ToggleButton>> switches;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> switchAttachments;
 juce::ComboBox presetBox,categoryBox;juce::TextEditor search;juce::Label status;
 juce::MidiKeyboardComponent keyboard;juce::Slider pitch,mod;
 juce::Label macroLabels[4];std::unique_ptr<juce::FileChooser> chooser;
 std::unique_ptr<GateGrid> gateGrid;
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
 private:SynthAUI::Face face;juce::ComponentBoundsConstrainer ratio;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthAEditor)
};

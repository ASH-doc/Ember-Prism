// sb-ui.h — Prism Flux's independent native visual system and editing surfaces.
#pragma once
#include "sb-plugin.h"
namespace PrismUI {
inline juce::Colour ice(){return juce::Colour(0xffeaf2ff);}inline juce::Colour aqua(){return juce::Colour(0xff4fe3f5);}inline juce::Colour violet(){return juce::Colour(0xff8f7bff);}
juce::Colour sourceColour(int);void card(juce::Graphics&,juce::Rectangle<float>,juce::Colour accent=juce::Colours::transparentBlack);
class Style final:public juce::LookAndFeel_V4 {
 public:explicit Style(PrismProcessor&);juce::Label* createSliderTextBox(juce::Slider&)override;
 void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&)override;
 void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&)override;
 void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool)override;
 juce::Font getComboBoxFont(juce::ComboBox&)override{return juce::Font(juce::FontOptions(10.f));}
 private:PrismProcessor& processor;
};
class Dial final:public juce::Component,public juce::DragAndDropTarget {
 public:Dial(PrismProcessor&,Style&,int parameter,bool linear=false,bool row=false);~Dial()override;void resized()override;void paint(juce::Graphics&)override;
 bool isInterestedInDragSource(const SourceDetails&)override;void itemDragEnter(const SourceDetails&)override;void itemDragExit(const SourceDetails&)override;void itemDropped(const SourceDetails&)override;
 juce::Slider slider;juce::ComboBox choice;juce::Label label;int parameter;std::function<void(juce::String)> inspect;
 private:PrismProcessor& processor;bool linear,row,hover=false;std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> binding;std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> menuBinding;
 void mouseDown(const juce::MouseEvent&)override;
};
class SourceChip final:public juce::Component {
 public:SourceChip(int source,juce::String label):source(source),label(std::move(label)){}void paint(juce::Graphics&)override;void mouseDrag(const juce::MouseEvent&)override;
 private:int source;juce::String label;
};
class WaveView final:public juce::Component {
 public:WaveView(PrismProcessor& p,int o):processor(p),oscillator(o){}void paint(juce::Graphics&)override;
 private:PrismProcessor& processor;int oscillator;
};
class FilterView final:public juce::Component {
 public:FilterView(PrismProcessor& p,int f):processor(p),filter(f){}void paint(juce::Graphics&)override;void mouseDown(const juce::MouseEvent& e)override{mouseDrag(e);}void mouseDrag(const juce::MouseEvent&)override;
 private:PrismProcessor& processor;int filter;
};
class ShapeCanvas final:public juce::Component {
 public:ShapeCanvas(PrismProcessor& p,int l):processor(p),lfo(l){}void paint(juce::Graphics&)override;void mouseDown(const juce::MouseEvent&)override;void mouseDrag(const juce::MouseEvent&)override;
 private:PrismProcessor& processor;int lfo,lastPoint=-1;float lastValue=0;
};
class SequenceCanvas final:public juce::Component {
 public:SequenceCanvas(PrismProcessor& p):processor(p){}void paint(juce::Graphics&)override;void mouseDown(const juce::MouseEvent&)override;void mouseDrag(const juce::MouseEvent&)override;
 private:PrismProcessor& processor;int editing=0;void change(const juce::MouseEvent&);
};
class Spectrum final:public juce::Component {
 public:Spectrum(PrismProcessor& p):processor(p),bins(1024){}void paint(juce::Graphics&)override;void update();
 private:PrismProcessor& processor;std::vector<std::complex<float>> bins;std::array<float,256> levels{};
};
class DetailView final:public juce::Component {
 public:DetailView(PrismProcessor&,Style&,juce::String);void resized()override;
 private:juce::TextEditor search;juce::Viewport viewport;juce::Component rows;std::vector<std::unique_ptr<Dial>> controls;void apply();
};
class MotionPanel final:public juce::Component {
 public:MotionPanel(PrismProcessor&,Style&);void paint(juce::Graphics&)override;void choose(int);void refresh();std::function<void(juce::String)> inspect;
 private:PrismProcessor& processor;Style& style;int tab=0;std::array<juce::TextButton,4> tabs;
 juce::Component content;std::vector<std::unique_ptr<Dial>> controls;std::vector<std::unique_ptr<ShapeCanvas>> shapes;
 std::unique_ptr<SequenceCanvas> sequence;juce::Viewport matrixViewport;juce::Component matrixRows;
 std::vector<std::unique_ptr<juce::TextButton>> buttons;
 Dial& dial(int,int,int,int,int,bool linear=false,bool row=false);void rebuild();
};
class RackCell final:public juce::Component,public juce::DragAndDropTarget {
 public:RackCell(PrismProcessor&,Style&,int slot,int effect);void paint(juce::Graphics&)override;void mouseDrag(const juce::MouseEvent&)override;
 bool isInterestedInDragSource(const SourceDetails&)override;void itemDropped(const SourceDetails&)override;
 std::function<void(juce::String)> inspect;
 private:PrismProcessor& processor;int slot,effect;std::unique_ptr<Dial> wet,main;juce::ToggleButton power;juce::TextButton more;std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerBinding;
};
class Surface final:public juce::Component,public juce::DragAndDropContainer,private juce::Timer {
 public:Surface(PrismProcessor&);~Surface()override;void paint(juce::Graphics&)override;
 // Embedded mode: the host owns the shared keyboard and wheels, so this panel hides its own.
 void setEmbedded(bool embedded){keyboard.setVisible(!embedded);wheel.setVisible(!embedded);bend.setVisible(!embedded);}
 private:PrismProcessor& processor;Style style;juce::TooltipWindow tips{this,500};
 std::vector<std::unique_ptr<Dial>> controls;std::vector<std::unique_ptr<SourceChip>> chips;std::vector<std::unique_ptr<juce::TextButton>> buttons;
 std::array<std::unique_ptr<WaveView>,2> waves;std::array<std::unique_ptr<FilterView>,2> filters;std::array<std::unique_ptr<RackCell>,10> rack;
 std::array<juce::Label,8> macroLabels;juce::ComboBox preset,category;juce::TextEditor search;juce::Label status;
 MotionPanel motion;Spectrum spectrum;juce::MidiKeyboardComponent keyboard;juce::Slider wheel,bend;
 std::unique_ptr<juce::DocumentWindow> detail;std::unique_ptr<juce::FileChooser> picker;int selected=0;std::array<int,10> lastOrder{};
 juce::String lastName;
 Dial& dial(juce::String,int,int,int,int);void button(juce::String,juce::Rectangle<int>,std::function<void()>);
 void showDetail(juce::String);void rebuildBrowser();void file(bool save);void import(int oscillator);void timerCallback()override;
};
}
class PrismEditor final:public juce::AudioProcessorEditor {
 public:explicit PrismEditor(PrismProcessor&);void resized()override;void setEmbedded(bool embedded){surface.setEmbedded(embedded);}
 private:PrismUI::Surface surface;juce::ComponentBoundsConstrainer bounds;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrismEditor)
};

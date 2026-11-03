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
// Illustrative filter magnitude (about 0..1.3) at a frequency, shared by the compact and the large graph.
float filterResponse(int type,float cutoff,float resonance,float hz);
// The selected wavetable drawn as stacked frames. Drag or use the arrow keys to scan the table position (osc.pos).
class WaveView final:public juce::Component {
 public:WaveView(PrismProcessor& p,int o):processor(p),oscillator(o){setWantsKeyboardFocus(true);setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);setTitle("Wavetable position");}
 void paint(juce::Graphics&)override;void mouseDown(const juce::MouseEvent&)override;void mouseDrag(const juce::MouseEvent&)override;void mouseUp(const juce::MouseEvent&)override;bool keyPressed(const juce::KeyPress&)override;
 private:PrismProcessor& processor;int oscillator;bool gesture=false;
};
// Large response graph for both filters: draw one at a time (the selector only changes which is edited).
// X is cutoff (20 Hz to 20 kHz, logarithmic), Y is resonance. Arrow keys do the same for keyboard use.
class FilterGraph final:public juce::Component {
 public:explicit FilterGraph(PrismProcessor& p):processor(p){setWantsKeyboardFocus(true);setTitle("Filter response");}
 void select(int f){selected=f;repaint();}
 void paint(juce::Graphics&)override;void mouseDown(const juce::MouseEvent&)override;void mouseDrag(const juce::MouseEvent&)override;void mouseUp(const juce::MouseEvent&)override;bool keyPressed(const juce::KeyPress&)override;
 private:PrismProcessor& processor;int selected=0;bool dragging=false;
 void drag(const juce::MouseEvent&);void begin();void end();
};
// One motion lane's 16 steps (bipolar). Drag to draw, double-click to zero a step, right-click for shapes.
class MotionSteps final:public juce::Component {
 public:MotionSteps(PrismProcessor& p,int l):processor(p),lane(l){setTitle("Motion lane "+juce::String(l+1)+" steps");}
 void paint(juce::Graphics&)override;void mouseDown(const juce::MouseEvent&)override;void mouseDrag(const juce::MouseEvent&)override;void mouseDoubleClick(const juce::MouseEvent&)override;
 private:PrismProcessor& processor;int lane,previous=-1;float previousValue=0;void edit(const juce::MouseEvent&);void shapes(const juce::MouseEvent&);
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
// The Signal view: two large oscillator cards, one large filter workbench and the Motion lanes. It sits over the
// macro, source-chip, motion and spectrum area; every control in it is an ordinary parameter of this engine.
class SignalView final:public juce::Component {
 public:SignalView(PrismProcessor&,Style&,std::function<void(juce::String)> inspect,std::function<void(int)> importWave);
 void paint(juce::Graphics&)override;void refresh();void chooseFilter(int);int chosenFilter()const{return selectedFilter;}
 private:
 PrismProcessor& processor;Style& style;
 std::vector<std::unique_ptr<Dial>> dials;std::vector<std::unique_ptr<juce::TextButton>> buttons;
 std::array<std::unique_ptr<WaveView>,2> waves;std::unique_ptr<FilterGraph> graph;std::array<std::unique_ptr<MotionSteps>,3> steps;
 std::array<juce::TextButton,2> filterTabs;std::array<std::vector<juce::Component*>,2> perFilter;
 std::vector<std::unique_ptr<juce::ToggleButton>> toggles;std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> toggleBindings;
 std::vector<std::unique_ptr<juce::Slider>> sliders;std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderBindings;
 int selectedFilter=0;std::function<void(juce::String)> inspect;
 Dial& dial(const juce::String&,juce::Rectangle<int>,bool menuOnly=false);
};
class Surface final:public juce::Component,public juce::DragAndDropContainer,private juce::Timer {
 public:Surface(PrismProcessor&);~Surface()override;void paint(juce::Graphics&)override;
 // Signal view (large oscillators, filter graph, motion lanes) or the classic compact layout.
 void showSignalView(bool show);bool signalViewShown()const{return signal!=nullptr&&signal->isVisible();}SignalView* signalView(){return signal.get();}
 // Embedded mode: the host owns the shared keyboard and wheels, so this panel hides its own.
 void setEmbedded(bool embedded){keyboard.setVisible(!embedded);wheel.setVisible(!embedded);bend.setVisible(!embedded);}
 private:PrismProcessor& processor;Style style;juce::TooltipWindow tips{this,500};
 std::vector<std::unique_ptr<Dial>> controls;std::vector<std::unique_ptr<SourceChip>> chips;std::vector<std::unique_ptr<juce::TextButton>> buttons;
 std::array<std::unique_ptr<WaveView>,2> waves;std::array<std::unique_ptr<FilterView>,2> filters;std::array<std::unique_ptr<RackCell>,10> rack;
 std::array<juce::Label,8> macroLabels;juce::ComboBox preset,category;juce::TextEditor search;juce::Label status;
 MotionPanel motion;Spectrum spectrum;juce::MidiKeyboardComponent keyboard;juce::Slider wheel,bend;
 std::unique_ptr<SignalView> signal;juce::TextButton viewButton{"CLASSIC VIEW"};
 std::unique_ptr<juce::DocumentWindow> detail;std::unique_ptr<juce::FileChooser> picker;int selected=0;std::array<int,10> lastOrder{};
 juce::String lastName;
 Dial& dial(juce::String,int,int,int,int);void button(juce::String,juce::Rectangle<int>,std::function<void()>);
 void showDetail(juce::String);void rebuildBrowser();void file(bool save);void import(int oscillator);void timerCallback()override;
};
}
class PrismEditor final:public juce::AudioProcessorEditor {
 public:explicit PrismEditor(PrismProcessor&);void resized()override;void setEmbedded(bool embedded){surface.setEmbedded(embedded);}
 PrismUI::Surface& panel(){return surface;}
 private:PrismUI::Surface surface;juce::ComponentBoundsConstrainer bounds;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrismEditor)
};

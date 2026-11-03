// sb-plugin.cpp — Stereo VST3/standalone processing and sample-offset MIDI delivery.
#include "sb-plugin.h"
#include "sb-ui.h"
#include <BinaryData.h>
juce::AudioProcessorValueTreeState::ParameterLayout PrismProcessor::makeLayout(){
 juce::AudioProcessorValueTreeState::ParameterLayout layout;
 for(const auto& spec:SynthB::Schema::get().specs){juce::ParameterID id(spec.key,1);juce::String name=spec.section+" / "+spec.title;
  if(!spec.options.empty()){juce::StringArray choices;for(const auto& item:spec.options)choices.add(item);layout.add(std::make_unique<juce::AudioParameterChoice>(id,name,choices,int(spec.initial)));}
  else if(spec.integer)layout.add(std::make_unique<juce::AudioParameterInt>(id,name,int(spec.low),int(spec.high),int(spec.initial)));
  else layout.add(std::make_unique<juce::AudioParameterFloat>(id,name,juce::NormalisableRange<float>(spec.low,spec.high,0,spec.power),spec.initial,juce::AudioParameterFloatAttributes().withLabel(spec.unit)));
 }return layout;
}
PrismProcessor::PrismProcessor():AudioProcessor(BusesProperties().withOutput("Prism stereo",juce::AudioChannelSet::stereo(),true)),parameters(*this,&undo,"SynthB",makeLayout()){
 const auto& specs=SynthB::Schema::get().specs;for(size_t i=0;i<specs.size();++i)atomics[i]=parameters.getRawParameterValue(specs[i].key);for(auto& a:ccAssignments)a.store(-1);for(auto& p:ccPending)p.store(-1);
 auto bank=juce::JSON::parse(juce::String::fromUTF8(BinaryData::factory_json,BinaryData::factory_jsonSize));if(auto* array=bank.getArray())factory=*array;
 setLatencySamples(12);scene=patch();
}
PrismProcessor::~PrismProcessor(){cancelPendingUpdate();}
bool PrismProcessor::isBusesLayoutSupported(const BusesLayout& layout)const{return layout.getMainInputChannelSet().isDisabled()&&layout.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
void PrismProcessor::prepareToPlay(double rate,int){SynthB::State state{};for(size_t i=0;i<SynthB::Schema::get().specs.size();++i)state[i]=atomics[i]->load();engine.parameters(state);engine.prepare(rate);keyboard.reset();}
void PrismProcessor::processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals denormals;double start=juce::Time::getMillisecondCounterHiRes();audio.clear();if(audio.getNumChannels()!=2)return;
 SynthB::State state{};for(size_t i=0;i<SynthB::Schema::get().specs.size();++i)state[i]=atomics[i]->load(std::memory_order_relaxed);engine.parameters(state);
 if(silence.exchange(false)){engine.reset();keyboard.reset();}
 keyboard.processNextMidiBuffer(midi,0,audio.getNumSamples(),true);int length=0;float wheel=wheelRequest.exchange(-1),bend=bendRequest.exchange(-2);
 if(wheel>=0)events[size_t(length++)]={SynthB::Event::Controller,0,1,1,wheel};if(bend>=-1)events[size_t(length++)]={SynthB::Event::Bend,0,0,1,bend};
 bool overflow=false;for(const auto item:midi){auto message=item.getMessage();SynthB::Event event;event.offset=item.samplePosition;event.channel=message.getChannel();bool relevant=true;
  if(message.isNoteOn()){event.kind=SynthB::Event::Down;event.key=message.getNoteNumber();event.value=message.getFloatVelocity();}
  else if(message.isNoteOff()){event.kind=SynthB::Event::Up;event.key=message.getNoteNumber();}
  else if(message.isPitchWheel()){event.kind=SynthB::Event::Bend;event.value=float(message.getPitchWheelValue()-8192)/8192;}
  else if(message.isChannelPressure()||message.isAftertouch()){event.kind=SynthB::Event::Pressure;event.value=float(message.isChannelPressure()?message.getChannelPressureValue():message.getAfterTouchValue())/127;}
  else if(message.isController()){event.kind=SynthB::Event::Controller;event.key=message.getControllerNumber();event.value=float(message.getControllerValue());int learn=learnTarget.load();if(learn>=0&&event.key!=64&&event.key!=120&&event.key!=123){ccAssignments[size_t(event.key)].store(learn);learnTarget.store(-1);}int index=ccAssignments[size_t(event.key)].load();if(index>=0){ccPending[size_t(index)].store(event.value/127);triggerAsyncUpdate();}}
  else relevant=false;
  if(relevant){if(length<int(events.size()))events[size_t(length++)]=event;else overflow=true;}
 }
 if(overflow){engine.reset();length=0;}double bpm=0,beat=-1;if(auto* playhead=getPlayHead())if(auto info=playhead->getPosition()){if(auto t=info->getBpm())bpm=*t;if(info->getIsPlaying())if(auto p=info->getPpqPosition())beat=*p;}
 engine.render(audio.getWritePointer(0),audio.getWritePointer(1),audio.getNumSamples(),events.data(),length,bpm,beat,importedTable());midi.clear();voices.store(engine.active());
 float fraction=float((juce::Time::getMillisecondCounterHiRes()-start)*getSampleRate()/(1000*std::max(1,audio.getNumSamples())));cpuLoad.store(cpuLoad.load()*.94f+fraction*.06f);
}
float PrismProcessor::read(int index)const{return index>=0&&index<int(SynthB::Schema::get().specs.size())?atomics[size_t(index)]->load():0;}
float PrismProcessor::read(const juce::String& key)const{return read(SynthB::Schema::get().find(key.toStdString()));}
void PrismProcessor::write(int index,float value){if(index<0||index>=int(SynthB::Schema::get().specs.size()))return;const auto& def=SynthB::Schema::get().specs[size_t(index)];auto* p=parameters.getParameter(def.key);p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(def.valid(value)));p->endChangeGesture();}
void PrismProcessor::write(const juce::String& key,float value){write(SynthB::Schema::get().find(key.toStdString()),value);}
void PrismProcessor::handleAsyncUpdate(){for(size_t i=0;i<SynthB::Schema::get().specs.size();++i){float value=ccPending[i].exchange(-1);if(value>=0){auto* parameter=parameters.getParameter(SynthB::Schema::get().specs[i].key);parameter->beginChangeGesture();parameter->setValueNotifyingHost(value);parameter->endChangeGesture();}}}
const SynthB::WaveSet* PrismProcessor::importedTable()const{const auto* current=currentWave.load(std::memory_order_acquire);return current?current->table.get():nullptr;}
juce::AudioProcessorEditor* PrismProcessor::createEditor(){return new PrismEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new PrismProcessor;}

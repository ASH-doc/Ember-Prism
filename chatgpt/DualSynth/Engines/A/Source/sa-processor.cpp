// sa-processor.cpp — Sample-accurate host MIDI, APVTS automation and standalone keyboard.
#include "sa-processor.h"
#include "sa-editor.h"
#include <EmberBankData.h>
#include <EmberClaudeBankData.h>
juce::AudioProcessorValueTreeState::ParameterLayout SynthAProcessor::layout(){
 juce::AudioProcessorValueTreeState::ParameterLayout result;
 for(const auto& p:SynthA::parameters()){
   juce::ParameterID id(p.id,1);
   if(!p.choices.empty()){juce::StringArray items;for(auto& c:p.choices)items.add(c);result.add(std::make_unique<juce::AudioParameterChoice>(id,p.group+" / "+p.label,items,int(p.initial)));}
   else if(p.discrete)result.add(std::make_unique<juce::AudioParameterInt>(id,p.group+" / "+p.label,int(p.min),int(p.max),int(p.initial)));
   else result.add(std::make_unique<juce::AudioParameterFloat>(id,p.group+" / "+p.label,juce::NormalisableRange<float>(p.min,p.max,0,p.skew),p.initial,juce::AudioParameterFloatAttributes().withLabel(p.unit)));
 }
 return result;
}
SynthAProcessor::SynthAProcessor():AudioProcessor(BusesProperties().withOutput("Stereo",juce::AudioChannelSet::stereo(),true)),state(*this,&undo,"SynthA",layout()){
 for(size_t i=0;i<SynthA::parameters().size();++i)raw[i]=state.getRawParameterValue(SynthA::parameters()[i].id);
 for(auto& x:pendingCC)x.store(-1);for(auto& x:ccMap)x.store(-1);for(auto& x:heldNotes)x.store(0);
 auto parsed=juce::JSON::parse(juce::String::fromUTF8(EmberBank::factory_json,EmberBank::factory_jsonSize));
 if(auto* array=parsed.getArray())bank=*array;
 // Presets imported from the Claude baseline are appended after the factory bank (before Signal Studies, which stays last), so every factory index is unchanged.
 auto imported=juce::JSON::parse(juce::String::fromUTF8(EmberClaudeBank::claudeimport_json,EmberClaudeBank::claudeimport_jsonSize));
 if(auto* array=imported.getArray())bank.addArray(*array);
 auto curated = juce::JSON::parse(juce::String::fromUTF8(EmberBank::signal_studies_json, EmberBank::signal_studies_jsonSize));
 if (auto* array = curated.getArray())
  for (const auto& preset : *array) bank.add(preset);
 setLatencySamples(16);comparePatch=getPatch();
}
SynthAProcessor::~SynthAProcessor(){cancelPendingUpdate();}
void SynthAProcessor::prepareToPlay(double sr,int){
 SynthA::Values p{};for(size_t i=0;i<SynthA::parameters().size();++i)p[i]=raw[i]->load();engine.setParameters(p);engine.prepare(sr);keyboard.reset();
}
bool SynthAProcessor::isBusesLayoutSupported(const BusesLayout& l)const{return l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()&&l.getMainInputChannelSet().isDisabled();}
void SynthAProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals guard;const double start=juce::Time::getMillisecondCounterHiRes();
 buffer.clear();if(buffer.getNumChannels()<2)return;
 SynthA::Values p{};for(size_t i=0;i<SynthA::parameters().size();++i)p[i]=raw[i]->load(std::memory_order_relaxed);engine.setParameters(p);
 if(panicRequested.exchange(false)){engine.panic();keyboard.reset();for(auto& n:heldNotes)n.store(0);}
 keyboard.processNextMidiBuffer(midi,0,buffer.getNumSamples(),true);
 int count=0;bool overflow=false;
 const float pitch=uiPitch.exchange(-2),mod=uiMod.exchange(-1);
 if(pitch>=-1)midiEvents[size_t(count++)]={SynthA::MidiEvent::PitchBend,0,0,1,pitch};
 if(mod>=0)midiEvents[size_t(count++)]={SynthA::MidiEvent::ControlChange,0,1,1,mod};
 for(const auto metadata:midi){
   const auto m=metadata.getMessage();SynthA::MidiEvent e;e.sample=metadata.samplePosition;e.channel=m.getChannel();bool use=true;
   if(m.isNoteOn()){e.type=SynthA::MidiEvent::NoteOn;e.note=m.getNoteNumber();e.value=m.getFloatVelocity();heldNotes[size_t(e.note)].store(e.value);}
   else if(m.isNoteOff()){e.type=SynthA::MidiEvent::NoteOff;e.note=m.getNoteNumber();heldNotes[size_t(e.note)].store(0);}
   else if(m.isPitchWheel()){e.type=SynthA::MidiEvent::PitchBend;e.value=float(m.getPitchWheelValue()-8192)/8192;}
   else if(m.isChannelPressure()||m.isAftertouch()){e.type=SynthA::MidiEvent::Aftertouch;e.value=float(m.isChannelPressure()?m.getChannelPressureValue():m.getAfterTouchValue())/127;}
   else if(m.isController()){
     e.type=SynthA::MidiEvent::ControlChange;e.note=m.getControllerNumber();e.value=float(m.getControllerValue());
     const int learning=learnParameter.load();if(learning>=0&&e.note!=64&&e.note!=120&&e.note!=123){ccMap[size_t(e.note)].store(learning);learnParameter.store(-1);lastLearnCC.store(e.note);}
     const int mapped=ccMap[size_t(e.note)].load();if(mapped>=0&&mapped<int(SynthA::parameters().size())){pendingCC[size_t(mapped)].store(e.value/127);triggerAsyncUpdate();}
     if(e.note==120||e.note==123)for(auto& n:heldNotes)n.store(0);
   }else use=false;
   if(use){if(count<int(midiEvents.size()))midiEvents[size_t(count++)]=e;else overflow=true;}
 }
 if(overflow){engine.panic();count=0;}
 double bpm=0,ppq=-1;
 if(auto* playhead=getPlayHead())if(auto position=playhead->getPosition()){
   if(auto hostBpm=position->getBpm())bpm=*hostBpm;
   if(position->getIsPlaying())if(auto hostPpq=position->getPpqPosition())ppq=*hostPpq;
 }
 engine.render(buffer.getWritePointer(0),buffer.getWritePointer(1),buffer.getNumSamples(),midiEvents.data(),count,bpm,ppq);
 midi.clear();voicesPlaying.store(engine.activeVoices());
 const float cost=float((juce::Time::getMillisecondCounterHiRes()-start)*getSampleRate()/(1000*std::max(1,buffer.getNumSamples())));
 cpu.store(cpu.load()*.95f+cost*.05f);
}
void SynthAProcessor::handleAsyncUpdate(){for(size_t i=0;i<SynthA::parameters().size();++i){float value=pendingCC[i].exchange(-1);if(value>=0){auto* param=state.getParameter(SynthA::parameters()[i].id);param->beginChangeGesture();param->setValueNotifyingHost(value);param->endChangeGesture();}}}
float SynthAProcessor::getParam(const juce::String& id)const{if(auto* p=state.getRawParameterValue(id))return p->load();return 0;}
void SynthAProcessor::setParam(const juce::String& id,float value){if(auto* p=state.getParameter(id)){undo.beginNewTransaction();p->beginChangeGesture();p->setValueNotifyingHost(p->convertTo0to1(value));p->endChangeGesture();}}
juce::AudioProcessorEditor* SynthAProcessor::createEditor(){return new SynthAEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createEmbeddedA(){return new SynthAProcessor();}

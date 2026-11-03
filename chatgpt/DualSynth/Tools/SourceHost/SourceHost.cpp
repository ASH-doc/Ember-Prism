// SourceHost.cpp — Hosts an installed third-party VST3 (Serum, Spire) so its own parameter text and audio can be read.
// Used only on the machine that owns the licence. Output (parameter names, texts, reference renders) stays local.
//   DualSourceHost <plugin.vst3>                       probe: instantiate, count parameters, render one note
//   DualSourceHost <plugin.vst3> --params [max]        list parameters with the plug-in's own text at 0, 0.25, 0.5, 0.75, 1
//   DualSourceHost <plugin.vst3> --state <file>        write the plug-in's current state chunk to a file
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI gui;
 if(argc<2){std::cerr<<"Supply the complete VST3 path\n";return 2;}
 juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> found;format.findAllTypesForFile(found,argv[1]);
 if(found.isEmpty()){std::cout<<"DISCOVERY FAILED\n";return 1;}
 for(auto* description:found)std::cout<<"found: "<<description->name<<" / "<<description->manufacturerName<<" / "<<description->version<<" / instrument="<<int(description->isInstrument)<<" / inputs "<<description->numInputChannels<<" outputs "<<description->numOutputChannels<<"\n";
 juce::String problem;auto instance=format.createInstanceFromDescription(*found[0],48000,256,problem);
 if(!instance){std::cout<<"INSTANTIATION FAILED: "<<problem<<"\n";return 1;}
 std::cout<<"instantiated: "<<instance->getName()<<", parameters: "<<instance->getParameters().size()<<"\n";
 juce::AudioBuffer<float> audio(juce::jmax(2,instance->getTotalNumOutputChannels()),256);instance->setPlayConfigDetails(0,2,48000,256);instance->prepareToPlay(48000,256);
 if(argc>=3&&juce::String(argv[2])=="--params"){
  const int limit=argc>=4?juce::String(argv[3]).getIntValue():100000;int shown=0;
  for(auto* parameter:instance->getParameters()){if(shown++>=limit)break;
   std::cout<<parameter->getParameterIndex()<<"\t"<<parameter->getName(60)<<"\tdefault "<<parameter->getDefaultValue()<<"\tsteps "<<parameter->getNumSteps()<<"\tdiscrete "<<int(parameter->isDiscrete())<<"\t";
   for(float value:{0.f,.25f,.5f,.75f,1.f})std::cout<<"["<<value<<" => "<<parameter->getText(value,40)<<"] ";std::cout<<"\n";}
  return 0;
 }
 if(argc>=4&&juce::String(argv[2])=="--state"){juce::MemoryBlock state;instance->getStateInformation(state);juce::File(argv[3]).replaceWithData(state.getData(),state.getSize());std::cout<<"state bytes: "<<state.getSize()<<"\n";
  // Also describe what the plug-in keeps inside its component state.
  {const auto text=juce::String::fromUTF8(static_cast<const char*>(state.getData())+8,int(state.getSize())-8);if(auto xml=juce::parseXML(text))if(auto* component=xml->getChildByName("IComponent")){juce::MemoryBlock inner;if(inner.fromBase64Encoding(component->getAllSubText())){std::cout<<"IComponent bytes: "<<inner.getSize()<<"\nhead: ";
    for(size_t i=0;i<std::min<size_t>(48,inner.getSize());++i){const auto c=static_cast<const unsigned char*>(inner.getData())[i];std::cout<<(c>=32&&c<127?char(c):'.');}std::cout<<"\nhex: ";for(size_t i=0;i<std::min<size_t>(32,inner.getSize());++i)std::cout<<juce::String::toHexString(static_cast<const unsigned char*>(inner.getData())[i])<<' ';std::cout<<"\n";}}}
  return 0;}
 // --fxp <file>: give the plug-in the chunk of a Serum 1 .fxp as its state, then print every parameter with its own text.
 // The real plug-in acts as an oracle: this is how a decoder's output can be checked against what Serum itself understood.
 if(argc>=4&&juce::String(argv[2])=="--fxp"){
  juce::MemoryBlock file;juce::File(argv[3]).loadFileAsData(file);const auto* bytes=static_cast<const unsigned char*>(file.getData());
  if(file.getSize()<60||bytes[0]!='C'||bytes[1]!='c'||bytes[2]!='n'||bytes[3]!='K'){std::cout<<"not an fxp\n";return 1;}
  const auto be=[&](size_t at){return (unsigned(bytes[at])<<24)|(unsigned(bytes[at+1])<<16)|(unsigned(bytes[at+2])<<8)|unsigned(bytes[at+3]);};
  const unsigned chunk=be(56);std::cout<<"fxp chunk size "<<chunk<<" of file "<<file.getSize()<<"\n";
  // JUCE's VST3 host wrapper keeps the plug-in's component state as "VC2!" + length + an XML document whose IComponent
  // element holds the state in JUCE's own base64 text. Build that wrapper around the fxp chunk.
  {juce::MemoryBlock chunkBlock(bytes+60,chunk);juce::String xml="<?xml version=\"1.0\" encoding=\"UTF-8\"?> <VST3PluginState><IComponent>"+chunkBlock.toBase64Encoding()+"</IComponent></VST3PluginState>";
   juce::MemoryOutputStream wrapped;wrapped.writeInt(0x21324356);wrapped.writeInt(int(xml.getNumBytesAsUTF8()));wrapped.write(xml.toRawUTF8(),xml.getNumBytesAsUTF8());wrapped.writeByte(0);
   instance->setStateInformation(wrapped.getData(),int(wrapped.getDataSize()));}
  for(int i=0;i<40;++i){juce::MidiBuffer idle;instance->processBlock(audio,idle);juce::Thread::sleep(25);}
  {juce::MemoryBlock after;instance->getStateInformation(after);std::cout<<"state after load: "<<after.getSize()<<" bytes, hash "<<juce::String::toHexString(after.toBase64Encoding().hashCode64())<<"\n";}
  {double energy=0;for(int block=0;block<60;++block){juce::MidiBuffer midi;if(block==0)midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);instance->processBlock(audio,midi);for(int c=0;c<2;++c)for(int n=0;n<256;++n)energy+=audio.getSample(c,n)*audio.getSample(c,n);}std::cout<<"energy of the loaded preset note: "<<energy<<"\n";}
  for(auto* parameter:instance->getParameters()){const float value=parameter->getValue();if(std::abs(value-parameter->getDefaultValue())<1.e-6f)continue;
   std::cout<<parameter->getParameterIndex()<<"\t"<<parameter->getName(60)<<"\t"<<value<<"\t"<<parameter->getText(value,40)<<"\n";}
  return 0;
 }
 double energy=0;float peak=0;
 for(int block=0;block<250;++block){audio.clear();juce::MidiBuffer midi;if(block==0)midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);if(block==100)midi.addEvent(juce::MidiMessage::noteOff(1,60),0);instance->processBlock(audio,midi);
  for(int c=0;c<2;++c)for(int n=0;n<256;++n){const float sample=audio.getSample(c,n);if(!std::isfinite(sample)){std::cout<<"non-finite output\n";return 1;}energy+=sample*sample;peak=std::max(peak,std::abs(sample));}}
 std::cout<<"rendered one note: energy "<<energy<<", peak "<<peak<<"\n";
 {std::unique_ptr<juce::AudioProcessorEditor> editor(instance->createEditorIfNeeded());std::cout<<"editor: "<<(editor?"created":"none")<<"\n";}
 juce::MemoryBlock state;instance->getStateInformation(state);std::cout<<"state bytes: "<<state.getSize()<<"\n";
 instance->releaseResources();return 0;
}

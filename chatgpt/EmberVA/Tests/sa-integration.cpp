// sa-integration.cpp — Actual JUCE processor/editor checks, bank audition renders and screenshot.
#include "sa-processor.h"
#include "sa-editor.h"
#include <iostream>
using namespace juce;
static void check(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
int main(int argc,char** argv){
 ScopedJuceInitialiser_GUI init;SynthAProcessor processor;processor.prepareToPlay(48000,256);
 check(processor.presetCount()==115,"114 factory recipes plus init");
 AudioBuffer<float> buffer(2,256);MidiBuffer midi;double bankEnergy=0;
 for(int p=0;p<processor.presetCount();++p){
   auto result=processor.setPatch(processor.preset(p));check(result.wasOk(),"factory patch validation");
   auto patch=processor.getPatch();MemoryBlock bytes;processor.getStateInformation(bytes);
   processor.setParam("flt1.cutoff",1234);processor.setStateInformation(bytes.getData(),int(bytes.getSize()));
   auto restored=processor.getPatch();
   for(const auto& def:SynthA::parameters()){float original=float(patch["params"][Identifier(String(def.id))]),actual=float(restored["params"][Identifier(String(def.id))]);if(std::abs(def.normalise(original)-def.normalise(actual))>1.e-5f){std::cerr<<"Roundtrip "<<def.id<<": "<<original<<" -> "<<actual<<"\n";check(false,"parameter state round trip");}}
   check(patch["name"]==restored["name"]&&patch["macros"]==restored["macros"],"patch metadata round trip");
   for(int b=0;b<80;++b){midi.clear();if(b==0){midi.addEvent(MidiMessage::noteOn(1,48,.8f),0);midi.addEvent(MidiMessage::noteOn(1,55,.8f),0);midi.addEvent(MidiMessage::noteOn(1,60,.8f),0);}if(b==55)midi.addEvent(MidiMessage::allNotesOff(1),0);
     processor.processBlock(buffer,midi);for(int ch=0;ch<2;++ch)for(int s=0;s<256;++s){float v=buffer.getSample(ch,s);check(std::isfinite(v)&&std::abs(v)<.65f,"factory output safety");bankEnergy+=v*v;}
   }
 }
 check(bankEnergy>1,"bank produces audio");
 auto before=JSON::toString(processor.getPatch(),true);auto invalid=processor.getPatch();invalid["params"].getDynamicObject()->setProperty("flt1.cutoff",-1);check(processor.setPatch(invalid).failed(),"invalid patch rejected");check(before==JSON::toString(processor.getPatch(),true),"invalid patch leaves state intact");
 processor.loadPreset(1);
 if(argc>1){File output=File::getCurrentWorkingDirectory().getChildFile(argv[1]);output.createDirectory();
   for(int width:{1000,1280,1600}){std::unique_ptr<AudioProcessorEditor> editor(processor.createEditor());editor->setSize(width,width*9/16);auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true);File file=output.getChildFile("Ember-VA-"+String(width)+".png");auto stream=file.createOutputStream();check(bool(stream),"screenshot stream");stream->setPosition(0);stream->truncate();PNGImageFormat format;check(format.writeImageToStream(image,*stream),"editor screenshot");}
   File wav=output.getChildFile("Ember-VA-audio-demo.wav");auto stream=wav.createOutputStream();stream->setPosition(0);stream->truncate();WavAudioFormat format;
   std::unique_ptr<AudioFormatWriter> writer(format.createWriterFor(stream.release(),48000,2,24,{},0));check(bool(writer),"WAV writer");
   for(int preset:{1,15,29,41,57,71}){processor.loadPreset(preset);for(int b=0;b<1125;++b){midi.clear();const bool shortPatch=preset==15||preset==41;
   if((!shortPatch&&b==0)||(shortPatch&&b<750&&b%94==0))for(int n:{48,55,60,63})midi.addEvent(MidiMessage::noteOn(1,n,.8f),0);
   if(b==750||(shortPatch&&b%94==47))midi.addEvent(MidiMessage::allNotesOff(1),0);processor.processBlock(buffer,midi);writer->writeFromAudioSampleBuffer(buffer,0,256);}}
 }
 std::cout<<"PASS: 115 factory patches, host-state round trips, invalid input, actual processor render, editor lifecycle.\n";
}

#include "Host.h"
#include "Catalog.h"
#include <iostream>
#include <map>
#include <thread>
using namespace juce;
void require(bool ok,const char* message){if(!ok){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}

// ---- Phase 1 tests: allocation counting, MIDI ownership, slicing, atomic state --------------------------------
// Counts heap allocations made by the thread that switches counting on (used to prove the host's own paths are clean).
namespace {
std::atomic<long> allocationCount{0};thread_local bool countAllocations=false;
}
void* operator new(std::size_t size){if(countAllocations)++allocationCount;if(void* p=std::malloc(size?size:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}

namespace {
std::unique_ptr<DualProcessor> makeHost(){return std::make_unique<DualProcessor>();}
int firstNonZero(const AudioBuffer<float>& buffer){for(int i=0;i<buffer.getNumSamples();++i)if(buffer.getSample(0,i)!=0.f||buffer.getSample(1,i)!=0.f)return i;return -1;}
struct FixedTransport final:AudioPlayHead{
 Optional<PositionInfo> getPosition()const override{PositionInfo info;info.setBpm(120.0);info.setPpqPosition(100.0);info.setIsPlaying(true);info.setTimeInSamples(juce::int64(1000));info.setTimeInSeconds(1000/48000.0);return info;}
};
MemoryBlock withProperty(const MemoryBlock& state,const Identifier& name,const var& value){auto tree=ValueTree::readFromData(state.getData(),state.getSize());tree.setProperty(name,value,nullptr);MemoryBlock out;MemoryOutputStream stream(out,false);tree.writeToStream(stream);return out;}
String base64Of(const String& text){MemoryBlock block(text.toRawUTF8(),text.getNumBytesAsUTF8());return block.toBase64Encoding();}

void midiOwnershipTests(){
 // Repeated physical note-on of the same channel and pitch releases the earlier note first, at the same sample.
 {auto host=makeHost();MidiBuffer in,ma,mb;in.addEvent(MidiMessage::noteOn(1,60,.8f),10);host->routeMidi(in,ma,mb);
  in.clear();in.addEvent(MidiMessage::noteOn(1,60,.8f),20);host->routeMidi(in,ma,mb);
  require(ma.getNumEvents()==2&&mb.getNumEvents()==2,"repeated note-on: earlier note released on both owners before the retrigger");
  for(auto* lane:{&ma,&mb}){auto it=lane->begin();auto first=(*it).getMessage();auto second=(*++it).getMessage();require(first.isNoteOff()&&second.isNoteOn()&&(*lane->begin()).samplePosition==20,"repeated note-on: note-off precedes note-on at the same sample");}
  in.clear();in.addEvent(MidiMessage::noteOff(1,60),30);host->routeMidi(in,ma,mb);require(ma.getNumEvents()==1&&mb.getNumEvents()==1,"one note-off releases the retriggered note");
  in.clear();in.addEvent(MidiMessage::noteOff(1,60),31);host->routeMidi(in,ma,mb);require(ma.isEmpty()&&mb.isEmpty(),"second note-off for an already released note is dropped");}
 // Same pitch on two channels is two different notes.
 {auto host=makeHost();MidiBuffer in,ma,mb;in.addEvent(MidiMessage::noteOn(1,60,.8f),0);in.addEvent(MidiMessage::noteOn(2,60,.8f),1);host->routeMidi(in,ma,mb);
  require(ma.getNumEvents()==2&&mb.getNumEvents()==2,"same pitch on two channels does not release the other channel");
  in.clear();in.addEvent(MidiMessage::noteOff(2,60),5);host->routeMidi(in,ma,mb);require(ma.getNumEvents()==1&&(*ma.begin()).getMessage().getChannel()==2,"note-off keeps its channel");
  in.clear();in.addEvent(MidiMessage::noteOff(1,60),6);host->routeMidi(in,ma,mb);require(ma.getNumEvents()==1&&(*ma.begin()).getMessage().getChannel()==1,"other channel still owned");}
 // CC123 / CC120 forget only their own channel; routing changes do not strand held notes.
 {auto host=makeHost();MidiBuffer in,ma,mb;in.addEvent(MidiMessage::noteOn(1,60,.8f),0);in.addEvent(MidiMessage::noteOn(2,62,.8f),0);host->routeMidi(in,ma,mb);
  in.clear();in.addEvent(MidiMessage::allNotesOff(1),0);host->routeMidi(in,ma,mb);require(ma.getNumEvents()==1&&mb.getNumEvents()==1,"CC123 reaches both engines");
  in.clear();in.addEvent(MidiMessage::noteOff(2,62),0);host->routeMidi(in,ma,mb);require(ma.getNumEvents()==1&&mb.getNumEvents()==1,"CC123 on channel 1 leaves channel 2 notes owned");
  in.clear();in.addEvent(MidiMessage::noteOff(1,60),0);host->routeMidi(in,ma,mb);require(ma.isEmpty()&&mb.isEmpty(),"CC123 on channel 1 cleared channel 1 ownership");
  in.clear();in.addEvent(MidiMessage::noteOn(3,64,.8f),0);host->routeMidi(in,ma,mb);in.clear();in.addEvent(MidiMessage::allSoundOff(3),0);host->routeMidi(in,ma,mb);in.clear();in.addEvent(MidiMessage::noteOff(3,64),0);host->routeMidi(in,ma,mb);require(ma.isEmpty()&&mb.isEmpty(),"CC120 clears its own channel");}
 {auto host=makeHost();MidiBuffer in,ma,mb;host->write("routing",1);host->write("split",60);in.addEvent(MidiMessage::noteOn(1,72,.8f),0);host->routeMidi(in,ma,mb);require(ma.isEmpty()&&mb.getNumEvents()==1,"split sends high note to B");
  host->write("routing",2);in.clear();in.addEvent(MidiMessage::noteOff(1,72),0);host->routeMidi(in,ma,mb);require(ma.isEmpty()&&mb.getNumEvents()==1,"routing change: note-off still goes to the engine that started the note");}
}

void engineChannelTests() {
 for (int controller : {120, 123}) {
  auto host = makeHost();
  host->prepareToPlay(48000, 256);
  AudioBuffer<float> audio(2, 256);
  MidiBuffer midi;
  midi.addEvent(MidiMessage::noteOn(1, 60, .8f), 0);
  midi.addEvent(MidiMessage::noteOn(2, 60, .8f), 1);
  host->processBlock(audio, midi);
  require(host->a.engine.activeVoices() == 2 && host->b.engine.active() == 2,
          "both engines retain the same pitch on two channels");
  midi.addEvent(MidiMessage::controllerEvent(1, controller, 0), 0);
  for (int block = 0; block < 500; ++block)
   host->processBlock(audio, midi);
  require(host->a.engine.activeVoices() == 1 && host->b.engine.active() == 1,
          "channel-local CC120/123 leaves the other channel sounding in both engines");
  host->panic.store(true);
  host->processBlock(audio, midi);
  require(host->a.engine.activeVoices() == 0 && host->b.engine.active() == 0,
          "global panic clears both engines");
 }
}

void recallBoundaryTest() {
 auto source = makeHost();
 auto target = makeHost();
 source->write("a.gain", -12);
 source->b.write("osc1.pos", .8f);
 target->prepareToPlay(48000, 256);
 MemoryBlock saved;
 source->getStateInformation(saved);

 // Probe the callback while Prism is publishing and the host mixer still has its old values.
 struct DuringPublication final : AudioProcessorParameter::Listener {
  explicit DuringPublication(DualProcessor& processor) : host(processor) {}
  void parameterValueChanged(int, float) override {
   if (observed) return;
   observed = true;
   std::thread audioThread([this] {
    AudioBuffer<float> audio(2, 256);
    MidiBuffer midi;
    midi.addEvent(MidiMessage::noteOn(1, 60, .8f), 0);
    host.processBlock(audio, midi);
    silent = audio.getMagnitude(0, 256) == 0 && audio.getMagnitude(1, 256) == 0
             && host.a.engine.activeVoices() == 0 && host.b.engine.active() == 0;
   });
   audioThread.join();
  }
  void parameterGestureChanged(int, bool) override {}
  DualProcessor& host;
  bool observed = false, silent = false;
 } probe(*target);

 auto* parameter = target->b.parameters.getParameter("osc1.pos");
 parameter->addListener(&probe);
 auto result = target->loadPerformance(saved.getData(), int(saved.getSize()));
 parameter->removeListener(&probe);
 require(result.wasOk() && probe.observed && probe.silent,
         "audio callback never renders a partially published performance");

 AudioBuffer<float> audio(2, 256);
 MidiBuffer midi;
 midi.addEvent(MidiMessage::noteOn(1, 60, .8f), 0);
 target->processBlock(audio, midi);
 require(std::abs(target->value("a.gain") + 12) < 1.e-4f
         && std::abs(target->b.read("osc1.pos") - .8f) < 1.e-4f
         && target->a.engine.activeVoices() == 1 && target->b.engine.active() == 1,
         "next audio callback runs with the complete recalled performance");
}

void routeMidiAllocationTest(){
 auto host=makeHost();host->prepareToPlay(48000,256);MidiBuffer in,ma,mb;
 for(int i=0;i<20;++i){in.addEvent(MidiMessage::noteOn(1,40+i,.7f),i);in.addEvent(MidiMessage::noteOff(1,40+i),100+i);}
 host->routeMidi(in,ma,mb);
 allocationCount=0;countAllocations=true;{void* probe = ::operator new(sizeof(int)); ::operator delete(probe);}countAllocations=false;require(allocationCount.load()==1,"allocation counter sees a deliberate allocation");
 allocationCount=0;countAllocations=true;for(int i=0;i<200;++i)host->routeMidi(in,ma,mb);countAllocations=false;
 std::cout<<"INFO: heap allocations in 200 routeMidi calls with 40 events each: "<<allocationCount.load()<<"\n";
 require(allocationCount.load()==0,"host MIDI routing allocates nothing");
}

void slicingTests(){
 // Transport shift: PPQ, samples and seconds advance by the samples already rendered.
 {FixedTransport fixed;SliceTransport slice;slice.set(&fixed,0,48000);auto same=slice.getPosition();require(same&&*same->getPpqPosition()==100.0&&*same->getTimeInSamples()==1000,"slice at offset 0 reports the host position unchanged");
  slice.set(&fixed,48000,48000);auto later=slice.getPosition();require(later&&std::abs(*later->getPpqPosition()-102.0)<1.e-9&&*later->getTimeInSamples()==49000&&std::abs(*later->getTimeInSeconds()-(1000/48000.0+1.0))<1.e-9&&*later->getBpm()==120&&later->getIsPlaying(),"slice transport advances PPQ by one second at 120 BPM and keeps tempo and play state");
  slice.set(nullptr,0,48000);require(!slice.getPosition().hasValue(),"slice transport without a host reports no position");}
 // A host block longer than the prepared storage: MIDI offsets survive the slice boundary and audio starts on the same sample.
 constexpr int length=40000,noteAt=35000;
 for(int engine=0;engine<2;++engine){
  auto host=makeHost();std::unique_ptr<AudioProcessor> reference=engine==0?std::unique_ptr<AudioProcessor>(new SynthAProcessor):std::unique_ptr<AudioProcessor>(new PrismProcessor);
  host->write("routing",float(engine+2));host->write("a.gain",0);host->write("b.gain",0);host->write("master",0);host->write("glue",0);
  host->prepareToPlay(48000,256);reference->setRateAndBufferSizeDetails(48000,256);reference->prepareToPlay(48000,256);
  require(length>host->sliceCapacity(),"test block is longer than one slice");
  MidiBuffer midi,copy;midi.addEvent(MidiMessage::noteOn(1,60,.8f),noteAt);copy=midi;
  AudioBuffer<float> out(2,length),raw(2,length);host->processBlock(out,midi);reference->processBlock(raw,copy);
  const int hostStart=firstNonZero(out),referenceStart=firstNonZero(raw)+(engine==1?4:0);
  float worst=0;for(int c=0;c<2;++c)for(int i=0;i<length;++i){const int source=engine==1?i-4:i;if(source<0)continue;worst=std::max(worst,std::abs(out.getSample(c,i)-raw.getSample(c,source)));}
  std::cout<<"INFO: engine "<<engine<<" sliced "<<length<<"-sample block: first audio at "<<hostStart<<" (single-block reference "<<referenceStart<<"), largest difference "<<worst<<"\n";
  require(hostStart>=noteAt&&hostStart==referenceStart,"MIDI offset and audio onset survive slicing exactly");
  require(worst==0.f,"sliced render is sample-identical to one single block");
 }
}

void stateTests(){
 auto good=makeHost();good->write("split",65);good->write("room",.77f);good->a.loadPreset(3);good->b.selectFactory(4);MemoryBlock saved;good->getStateInformation(saved);
 auto target=makeHost();target->write("split",70);target->a.loadPreset(5);target->b.selectFactory(6);MemoryBlock before;target->getStateInformation(before);
 auto expectRejected=[&](const MemoryBlock& bytes,const char* why){
  auto result=target->loadPerformance(bytes.getData(),int(bytes.getSize()));require(result.failed()&&result.getErrorMessage().isNotEmpty(),why);
  MemoryBlock after;target->getStateInformation(after);require(after==before,why);
  target->setStateInformation(bytes.getData(),int(bytes.getSize()));require(target->lastLoadError().isNotEmpty(),"DAW recall of a bad state records the reason");
 };
 MemoryBlock truncated(saved.getData(),saved.getSize()/2),garbage;for(int i=0;i<4096;++i)garbage.append(&i,1);MemoryBlock empty;
 expectRejected(truncated,"truncated performance rejected without changing anything");
 expectRejected(garbage,"garbage rejected without changing anything");
 {auto result=target->loadPerformance(empty.getData(),0);require(result.failed(),"empty performance rejected");}
 const String validA=base64Of(JSON::toString(good->a.getPatch(),true));
 expectRejected(withProperty(saved,"engineB",base64Of("{\"format\":\"dualsynth-patch\",\"engine\":\"B\",\"version\":1,\"params\":{\"not.a.parameter\":1}}")),"valid Synth A + invalid Synth B patch: nothing is applied");
 expectRejected(withProperty(saved,"engineB","!!! not base64 !!!"),"damaged Synth B payload rejected");
 expectRejected(withProperty(saved,"engineA",base64Of("{\"format\":\"dualsynth-patch\",\"engine\":\"A\",\"version\":1,\"params\":{\"osc1.level\":99}}")),"out-of-range value in Synth A rejected");
 expectRejected(withProperty(withProperty(saved,"engineA",validA),"engineB",String()),"missing Synth B payload rejected");
 {MemoryBlock huge;huge.setSize(size_t(DualProcessor::maxPerformanceBytes)+1,true);expectRejected(huge,"oversized performance rejected");}
 // Valid data still loads, and loading is complete.
 {auto result=target->loadPerformance(saved.getData(),int(saved.getSize()));require(result.wasOk(),"valid performance loads");require(target->value("split")==65&&std::abs(target->value("room")-.77f)<1.e-6&&target->a.patchName()==good->a.patchName()&&target->b.title()==good->b.title(),"valid performance applied completely");
  target->setStateInformation(saved.getData(),int(saved.getSize()));require(target->lastLoadError().isEmpty(),"a good recall clears the recorded error");}
 // Size budget: the largest valid Prism patch (64-frame embedded table) must fit the host and per-engine limits.
 {auto host=makeHost();var patch=host->b.patch();auto* params=patch["params"].getDynamicObject();require(params!=nullptr,"Prism patch has parameters");params->setProperty("osc1.table",30);
  auto* wave=new DynamicObject;wave->setProperty("name","Budget test table");wave->setProperty("frames",64);wave->setProperty("encoding","float32-le-base64");
  std::vector<float> samples(size_t(64*2048));for(size_t i=0;i<samples.size();++i)samples[i]=std::sin(float(i%2048)*MathConstants<float>::twoPi/2048.f*float(1+(i/2048)%16))*.9f;
  MemoryOutputStream encoded;Base64::convertToBase64(encoded,samples.data(),samples.size()*sizeof(float));wave->setProperty("data",encoded.toString());patch.getDynamicObject()->setProperty("wavetable",var(wave));
  auto restored=host->b.restore(patch);if(restored.failed())std::cerr<<"restore said: "<<restored.getErrorMessage()<<"\n";require(restored.wasOk(),"largest embedded wavetable patch restores");
  MemoryBlock engineB;host->b.getStateInformation(engineB);MemoryBlock big;host->getStateInformation(big);
  std::cout<<"INFO: largest supported patch: Synth B state "<<engineB.getSize()<<" bytes, whole performance "<<big.getSize()<<" bytes (limits: 2097152 per engine, "<<DualProcessor::maxPerformanceBytes<<" total)\n";
  require(engineB.getSize()<=size_t(2*1024*1024)&&big.getSize()<=size_t(DualProcessor::maxPerformanceBytes),"largest supported patch fits the state limits");
  auto other=makeHost();auto result=other->loadPerformance(big.getData(),int(big.getSize()));require(result.wasOk(),"largest supported performance round-trips");}
}

// Loads complete performances from this thread while another thread renders audio. This cannot prove the absence of
// races (no race detector is available on this toolchain); it proves there is no crash, hang or non-finite output.
void loadDuringPlaybackTest(){
 auto host=makeHost();host->prepareToPlay(48000,256);
 auto first=makeHost();first->a.loadPreset(2);first->b.selectFactory(3);MemoryBlock stateOne;first->getStateInformation(stateOne);
 auto second=makeHost();second->a.loadPreset(7);second->b.selectFactory(9);second->write("routing",1);MemoryBlock stateTwo;second->getStateInformation(stateTwo);
 std::atomic<bool> running{true},finite{true};std::atomic<int> blocks{0};
 std::thread audio([&]{AudioBuffer<float> out(2,256);while(running){MidiBuffer m;if(blocks%25==0)for(int note:{48,60,67})m.addEvent(MidiMessage::noteOn(1,note,.8f),0);host->processBlock(out,m);for(int c=0;c<2;++c)for(int i=0;i<256;++i)if(!std::isfinite(out.getSample(c,i)))finite=false;++blocks;}});
 int loads=0;for(int i=0;i<2000&&(i<60||blocks.load()<600);++i){const auto& bytes=i%2?stateOne:stateTwo;auto result=host->loadPerformance(bytes.getData(),int(bytes.getSize()));require(result.wasOk(),"performance loads while audio is running");++loads;}
 running=false;audio.join();
 std::cout<<"INFO: "<<loads<<" performance loads while "<<blocks.load()<<" audio blocks rendered: no crash, finite="<<(finite.load()?"yes":"NO")<<"\n";
 require(finite.load(),"output stays finite while performances load during playback");
}

// Every parameter the unmodified baseline published (id, name, steps, default, range and choice texts) must still exist, unchanged.
void parameterCatalogTest(){
 const auto baselineFile=File(__FILE__).getParentDirectory().getChildFile("baseline-parameter-catalog.txt");
 require(baselineFile.existsAsFile(),"baseline parameter catalog file is present next to the tests");
 StringArray baseline;baseline.addLines(baselineFile.loadFileAsString());baseline.removeEmptyStrings();
 auto host=makeHost();const auto current=parameterCatalog(*host);
 std::map<String,String> now;for(const auto& line:current)now[line.upToFirstOccurrenceOf("|",false,false)]=line;
 int missing=0,changed=0;
 for(const auto& line:baseline){auto id=line.upToFirstOccurrenceOf("|",false,false);auto found=now.find(id);if(found==now.end()){++missing;std::cerr<<"catalog: missing "<<id<<"\n";}else if(found->second!=line){++changed;std::cerr<<"catalog: changed "<<id<<"\n  was "<<line<<"\n  now "<<found->second<<"\n";}}
 std::cout<<"INFO: parameter catalog: "<<baseline.size()<<" baseline parameters, "<<current.size()<<" now ("<<(current.size()-baseline.size())<<" added), missing "<<missing<<", changed "<<changed<<"\n";
 require(missing==0&&changed==0,"every baseline parameter is still published with the same name, steps, default, range and choices");
}

// Master effects with settings a baseline recall would have produced. The golden file was rendered by the host
// BEFORE Phase 2 (commit 240f29e). Phase 2 must leave this audio unchanged.
std::vector<float> goldenMasterRender(){
 auto host=makeHost();
 for(auto id:{"glue","comp","verb","eq"})host->write(id,1);
 if(host->state.getParameter("comp.mode")!=nullptr)host->write("comp.mode",0);   // Phase 2+: the baseline compressor is the "Glue" type
 host->write("threshold",-24);host->write("ratio",4);host->write("attack",5);host->write("release",120);host->write("room",.7f);host->write("damp",.35f);host->write("wet",.3f);host->write("width",1.3f);host->write("low",4);host->write("air",3);
 host->write("a.gain",-3);host->write("b.gain",-9);host->prepareToPlay(48000,256);
 AudioBuffer<float> out(2,256);std::vector<float> samples;
 for(int block=0;block<160;++block){
  MidiBuffer midi;if(block==0){int offset=0;for(int note:{48,55,60,67,72})midi.addEvent(MidiMessage::noteOn(1,note,.8f),offset++*7);}if(block==90)midi.addEvent(MidiMessage::allNotesOff(1),3);
  host->processBlock(out,midi);for(int c=0;c<2;++c)for(int i=0;i<256;++i)samples.push_back(out.getSample(c,i));
 }
 return samples;
}
void goldenMasterTest(){
 const auto golden=File(__FILE__).getParentDirectory().getChildFile("golden-master-fx.bin");
 require(golden.existsAsFile(),"golden master-effects render is present next to the tests");
 MemoryBlock stored;golden.loadFileAsData(stored);const auto now=goldenMasterRender();
 require(stored.getSize()==now.size()*sizeof(float),"golden render has the expected length");
 const float* before=static_cast<const float*>(stored.getData());float worst=0;double energy=0;for(size_t i=0;i<now.size();++i){worst=std::max(worst,std::abs(before[i]-now[i]));energy+=now[i]*now[i];}
 std::cout<<"INFO: legacy master-effects render vs pre-Phase-2 golden: largest sample difference "<<worst<<" (signal energy "<<energy<<")\n";
 require(energy>1.e-3,"golden render has signal");
 require(worst<=1.e-5f,"legacy master effects sound unchanged (difference below -100 dB)");
}

void processBlockAllocationInfo(){
 auto host=makeHost();host->prepareToPlay(48000,256);AudioBuffer<float> out(2,256);MidiBuffer midi;
 for(int block=0;block<100;++block){MidiBuffer m;if(block==0)for(int note:{48,55,60,67})m.addEvent(MidiMessage::noteOn(1,note,.8f),0);host->processBlock(out,m);}
 allocationCount=0;countAllocations=true;for(int block=0;block<200;++block){MidiBuffer m;host->processBlock(out,m);}countAllocations=false;
 std::cout<<"INFO: heap allocations in 200 steady-state processBlock calls (both engines held notes, includes engine code): "<<allocationCount.load()<<"\n";
}
}

int main(int argc,char** argv){ScopedJuceInitialiser_GUI gui;
 if(argc==3&&String(argv[1])=="--golden-write"){const auto samples=goldenMasterRender();require(File::getCurrentWorkingDirectory().getChildFile(argv[2]).replaceWithData(samples.data(),samples.size()*sizeof(float)),"golden file written");std::cout<<"golden written: "<<samples.size()<<" samples\n";return 0;}
 midiOwnershipTests();parameterCatalogTest();goldenMasterTest();engineChannelTests();recallBoundaryTest();routeMidiAllocationTest();slicingTests();stateTests();loadDuringPlaybackTest();processBlockAllocationInfo();
 {auto heapP=std::make_unique<DualProcessor>();DualProcessor& p=*heapP;MidiBuffer input,ma,mb;input.addEvent(MidiMessage::noteOn(1,60,.8f),17);p.routeMidi(input,ma,mb);require(ma.getNumEvents()==1&&mb.getNumEvents()==1,"layer fans out to both");p.write("routing",1);p.write("split",72);input.clear();input.addEvent(MidiMessage::noteOff(1,60),21);p.routeMidi(input,ma,mb);require(ma.getNumEvents()==1&&mb.getNumEvents()==1,"held note ownership survives changed split");input.clear();input.addEvent(MidiMessage::noteOn(1,71,.8f),3);input.addEvent(MidiMessage::noteOn(1,72,.8f),4);p.routeMidi(input,ma,mb);require(ma.getNumEvents()==1&&mb.getNumEvents()==1,"split boundary routes A below and B at split");require((*ma.begin()).samplePosition==3&&(*mb.begin()).samplePosition==4,"MIDI sample offsets preserved");p.write("a.high",60);input.clear();input.addEvent(MidiMessage::noteOn(1,61,.7f),0);p.routeMidi(input,ma,mb);require(ma.isEmpty()&&mb.isEmpty(),"split intersects per-engine note range");p.write("a.channel",2);input.clear();input.addEvent(MidiMessage::noteOn(1,40,.7f),0);p.routeMidi(input,ma,mb);require(ma.isEmpty()&&mb.isEmpty(),"channel filter");input.clear();input.addEvent(MidiMessage::controllerEvent(1,64,127),0);p.routeMidi(input,ma,mb);require(ma.getNumEvents()==1&&mb.getNumEvents()==1,"controllers reach both to release tails");}
 for(int engine=0;engine<2;++engine){auto heapHost=std::make_unique<DualProcessor>();DualProcessor& host=*heapHost;std::unique_ptr<AudioProcessor> reference=engine==0?std::unique_ptr<AudioProcessor>(new SynthAProcessor):std::unique_ptr<AudioProcessor>(new PrismProcessor);host.write("routing",float(engine+2));host.write("a.gain",0);host.write("b.gain",0);host.write("master",0);host.write("glue",0);host.prepareToPlay(48000,256);reference->setRateAndBufferSizeDetails(48000,256);reference->prepareToPlay(48000,256);auto visible=std::unique_ptr<AudioProcessorEditor>(host.createEditor());AudioBuffer<float> output(2,256),raw(2,256);std::array<std::array<float,4>,2> delay{};int head=0;double energy=0;for(int block=0;block<200;++block){if(block%40==0)dynamic_cast<DualEditor*>(visible.get())->select((block/40)%2);MidiBuffer midi,copy;if(block==0){midi.addEvent(MidiMessage::noteOn(1,60,.8f),17);midi.addEvent(MidiMessage::noteOn(1,67,.7f),80);}if(block==120)midi.addEvent(MidiMessage::allNotesOff(1),9);copy=midi;host.processBlock(output,midi);reference->processBlock(raw,copy);for(int n=0;n<256;++n){for(int c=0;c<2;++c){float expected=raw.getSample(c,n);if(engine==1){float aligned=delay[size_t(c)][size_t(head)];delay[size_t(c)][size_t(head)]=expected;expected=aligned;}if(output.getSample(c,n)!=expected){std::cerr<<"engine "<<engine<<" block "<<block<<" sample "<<n<<" got "<<output.getSample(c,n)<<" expected "<<expected<<" delta "<<(output.getSample(c,n)-expected)<<" gains "<<host.value("a.gain")<<" "<<host.value("b.gain")<<" "<<host.value("master")<<"\n";require(false,"unity/bypassed output exactly equals original engine (B aligned by4samples)");}energy+=expected*expected;}head=(head+1)%4;}}require(energy>1.e-6,"identity comparison has signal");}
 auto heapMain=std::make_unique<DualProcessor>();DualProcessor& p=*heapMain;p.a.loadPreset(3);p.b.selectFactory(4);p.write("split",65);p.write("a.gain",-3);p.write("b.mute",1);p.write("room",.83f);p.panel.store(1);MemoryBlock state;p.getStateInformation(state);auto heapRecalled=std::make_unique<DualProcessor>();DualProcessor& recalled=*heapRecalled;recalled.setStateInformation(state.getData(),int(state.getSize()));require(recalled.value("split")==65&&recalled.value("b.mute")==1&&std::abs(recalled.value("room")-.83f)<1.e-6,"host state recall");require(recalled.a.patchName()==p.a.patchName()&&recalled.b.title()==p.b.title()&&recalled.panel.load()==1,"both complete engine states and view recall");
 auto* child=p.b.parameters.getParameter("osc1.pos");AudioProcessorParameter* proxy=nullptr;for(auto* parameter:p.getParameters())if(auto* id=dynamic_cast<AudioProcessorParameterWithID*>(parameter))if(id->paramID=="B.osc1.pos")proxy=parameter;require(proxy!=nullptr,"prefixed automation exists");proxy->setValue(.7f);require(std::abs(child->getValue()-.7f)<1.e-6,"host automation forwards to original parameter");p.b.write("osc1.pos",.2f);require(std::abs(proxy->getValue()-child->getValue())<1.e-6,"original UI forwards parameter value");
 p.prepareToPlay(48000,256);p.write("b.mute",0);p.write("glue",1);p.write("eq",1);p.write("width",1.5f);p.write("low",12);p.write("air",12);AudioBuffer<float> output(2,256);double energy=0;for(int block=0;block<600;++block){MidiBuffer midi;if(block==0)for(int note:{48,55,60,67})midi.addEvent(MidiMessage::noteOn(1,note,.8f),0);if(block==180)midi.addEvent(MidiMessage::allNotesOff(1),0);p.processBlock(output,midi);for(int c=0;c<2;++c)for(int n=0;n<256;++n){float v=output.getSample(c,n);require(std::isfinite(v),"master effects finite");energy+=v*v;}}require(energy>1.e-6,"layer and glue render");
 {auto heapMuted=std::make_unique<DualProcessor>();DualProcessor& muted=*heapMuted;muted.prepareToPlay(48000,256);muted.write("a.mute",1);muted.write("b.mute",1);AudioBuffer<float> test(2,256);for(int i=0;i<16;++i){MidiBuffer midi;if(i==0)midi.addEvent(MidiMessage::noteOn(1,60,.8f),0);muted.processBlock(test,midi);}require(test.getMagnitude(0,256)==0&&muted.a.voicesPlaying.load()>0&&muted.b.voices.load()>0,"mute fades output while both engines keep running");muted.write("b.mute",0);double signal=0;for(int i=0;i<16;++i){MidiBuffer midi;muted.processBlock(test,midi);signal+=test.getMagnitude(0,256);}require(signal>1.e-5&&muted.peaks[0].load()==0,"unmuting reveals held B without retriggering A or B");}
 if(argc>1){p.write("low",0);p.write("air",0);p.write("width",1);p.write("eq",0);p.write("glue",0);File folder=File::getCurrentWorkingDirectory().getChildFile(argv[1]);folder.createDirectory();auto editor=std::unique_ptr<AudioProcessorEditor>(p.createEditor());for(int panel=0;panel<2;++panel){dynamic_cast<DualEditor*>(editor.get())->select(panel);for(int width:{1280,1600}){editor->setSize(width,width*840/1600);auto snapshot=editor->createComponentSnapshot(editor->getLocalBounds(),true);auto stream=folder.getChildFile("Dual-Synth-"+String(panel==0?"A":"B")+"-"+String(width)+".png").createOutputStream();stream->setPosition(0);stream->truncate();PNGImageFormat png;require(png.writeImageToStream(snapshot,*stream),"actual host editor snapshot");}}}
 std::cout<<"PASS: layer/split/ranges/channels, note ownership, sample offsets, output mutes preserve voices, both engines exact unity output (B four-sample alignment), complete host+engine recall, prefixed automation, master effects finite, editor lifecycle.\n";
}

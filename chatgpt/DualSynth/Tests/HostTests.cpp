#include "Host.h"
#include "PresetBrowser.h"
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

// ---- Phase 2 tests -----------------------------------------------------------------------------------------------
std::vector<float> sineWave(int count,float amplitude,float hertz,double rate=48000){std::vector<float> v(static_cast<size_t>(count),0.f);for(int i=0;i<count;++i)v[size_t(i)]=amplitude*std::sin(MathConstants<float>::twoPi*hertz*float(i)/float(rate));return v;}
float steadyReduction(const masterdsp::FetParams& p,float peakDb,float hertz=1000,double rate=48000){
 masterdsp::FetCompressor f;f.prepare(rate);auto x=sineWave(48000,masterdsp::dbToGain(peakDb),hertz,rate);float last=0;
 for(int at=0;at<48000;at+=480){last=f.process(x.data()+at,x.data()+at,480,p);}   // process in place; same buffer for both channels is fine for a mono-like test
 return last;
}
void fetDspTests(){
 using namespace masterdsp;
 // Below threshold the compressor is bit-transparent.
 {FetCompressor f;f.prepare(48000);FetParams p;p.inputDb=0;p.outputDb=0;auto x=sineWave(4800,.01f,1000);auto left=x,right=x;const float gr=f.process(left.data(),right.data(),4800,p);require(gr==0&&left==x&&right==x,"FET: below threshold the signal passes unchanged");}
 // Gain reduction follows the ratio buttons: -6 dBFS peak plus 6 dB input is 10 dB over the fixed -10 dB threshold.
 const float expected[5]={7.5f,8.75f,9.17f,9.5f,12.5f};float previous=-1;
 for(int ratio=0;ratio<5;++ratio){FetParams p;p.ratio=ratio;p.inputDb=6;p.sidechainHighpass=true;const float gr=steadyReduction(p,-6);
  std::cout<<"INFO: FET ratio index "<<ratio<<": steady gain reduction "<<gr<<" dB (expected about "<<expected[ratio]<<")\n";
  require(std::abs(gr-expected[ratio])<1.0f,"FET gain reduction matches the ratio button");require(gr>previous,"FET: higher ratio gives more gain reduction (all buttons in is the most)");previous=gr;}
 // More input drives harder.
 {FetParams low,high;low.inputDb=6;high.inputDb=18;require(steadyReduction(high,-6)>steadyReduction(low,-6)+4.f,"FET: more input gives more gain reduction");}
 // Attack: 0.5 ms after a loud burst starts, a fast attack has pulled the gain down much further than a slow one.
 {auto reductionAfterHalfMs=[&](float attackUs){FetCompressor f;f.prepare(48000);FetParams p;p.attackUs=attackUs;p.inputDb=20;p.ratio=3;auto x=sineWave(24,.9f,3000);auto r=x;f.process(x.data(),r.data(),24,p);return f.currentGr();};
  const float fast=reductionAfterHalfMs(20),slow=reductionAfterHalfMs(800);std::cout<<"INFO: FET gain reduction 0.5 ms into a burst: attack 20 us "<<fast<<" dB, attack 800 us "<<slow<<" dB\n";require(fast>slow+1.f,"FET: fast attack reacts sooner than slow attack");}
 // Release: 100 ms after the signal stops, a short release has recovered much more than a long one.
 {auto reductionAfterSilence=[&](float releaseMs){FetCompressor f;f.prepare(48000);FetParams p;p.releaseMs=releaseMs;p.inputDb=20;p.ratio=3;auto loud=sineWave(9600,.9f,200);auto r=loud;f.process(loud.data(),r.data(),9600,p);std::vector<float> silence(4800,0.f),s2=silence;f.process(silence.data(),s2.data(),4800,p);return f.currentGr();};
  const float quick=reductionAfterSilence(50),slow=reductionAfterSilence(1100);std::cout<<"INFO: FET gain reduction 100 ms after the signal stops: release 50 ms "<<quick<<" dB, release 1100 ms "<<slow<<" dB\n";require(quick<slow-3.f,"FET: short release recovers faster than long release");}
 // Parallel mix 0 returns the dry signal exactly even while compressing hard; output control trims level.
 {FetCompressor f;f.prepare(48000);FetParams p;p.inputDb=20;p.ratio=4;p.mix=0;auto x=sineWave(4800,.7f,500);auto left=x,right=x;const float gr=f.process(left.data(),right.data(),4800,p);require(gr>3.f&&left==x&&right==x,"FET: parallel mix 0 is exactly the dry signal while the detector still works");}
 {FetCompressor f;f.prepare(48000);FetParams p;p.inputDb=0;p.outputDb=-6;auto x=sineWave(480,.01f,1000);auto left=x,right=x;f.process(left.data(),right.data(),480,p);float worst=0;for(size_t i=0;i<x.size();++i)worst=std::max(worst,std::abs(left[i]-x[i]*dbToGain(-6)));require(worst<1.e-7f,"FET: output control trims level by the stated amount");}
 // The 90 Hz detector filter keeps bass from pumping: a 40 Hz tone is compressed much less with it than without.
 {FetParams on,off;on.inputDb=off.inputDb=6;on.sidechainHighpass=true;off.sidechainHighpass=false;const float withFilter=steadyReduction(on,-6,40),without=steadyReduction(off,-6,40);std::cout<<"INFO: FET 40 Hz tone: gain reduction with the 90 Hz sidechain filter "<<withFilter<<" dB, without "<<without<<" dB\n";require(withFilter<without-1.f,"FET: sidechain high-pass reduces bass-driven compression");}
 // Extreme input stays finite and bounded.
 {FetCompressor f;f.prepare(48000);FetParams p;p.inputDb=40;p.outputDb=12;p.ratio=4;p.attackUs=20;std::vector<float> square(9600);for(size_t i=0;i<square.size();++i)square[i]=((i/48)%2)?1.f:-1.f;auto left=square,right=square;f.process(left.data(),right.data(),9600,p);bool ok=true;for(float v:left)ok=ok&&std::isfinite(v)&&std::abs(v)<16.f;require(ok,"FET: full-scale square wave at maximum input stays finite and bounded");}
 // Reverb input stage: pre-delay is sample exact, defaults are bit-transparent, low cut removes DC.
 {ReverbInputStage stage;stage.prepare(48000);std::vector<float> out(1000);for(int i=0;i<1000;++i){float l=i==0?1.f:0.f,r=l;stage.process(l,r,10.f,20.f);out[size_t(i)]=l;}int first=-1;for(int i=0;i<1000;++i)if(out[size_t(i)]!=0.f){first=i;break;}require(first==480&&out[480]==1.f,"reverb pre-delay of 10 ms delays by exactly 480 samples at 48 kHz");}
 {ReverbInputStage stage;stage.prepare(48000);bool same=true;for(int i=0;i<2000;++i){float l=std::sin(float(i)*.01f),r=std::cos(float(i)*.013f);const float l0=l,r0=r;stage.process(l,r,0.f,20.f);same=same&&l==l0&&r==r0;}require(same,"reverb input stage at its default settings is bit-transparent");}
 {ReverbInputStage stage;stage.prepare(48000);float l=0,r=0;for(int i=0;i<2400;++i){l=r=1.f;stage.process(l,r,0.f,300.f);}require(std::abs(l)<1.e-3f,"reverb low cut removes a constant level");}
 // Automating pre-delay and low cut does not click: the largest sample-to-sample step stays near that of the plain sine.
 {auto largestStepAfterChange=[&](float delayFrom,float delayTo,float cutFrom,float cutTo){ReverbInputStage stage;stage.prepare(48000);float previous=0,largest=0;
   for(int i=0;i<96000;++i){float l=std::sin(MathConstants<float>::twoPi*200.f*float(i)/48000.f),r=l;const bool after=i>=48000;stage.process(l,r,after?delayTo:delayFrom,after?cutTo:cutFrom);if(i>48000)largest=std::max(largest,std::abs(l-previous));previous=l;}return largest;};
  const float plain=2.f*MathConstants<float>::pi*200.f/48000.f;   // largest step of a unit 200 Hz sine
  const float delayJump=largestStepAfterChange(0,60,20,20),cutJump=largestStepAfterChange(0,0,20,500);
  std::cout<<"INFO: reverb input stage, largest sample step after a sudden change: pre-delay 0->60 ms "<<delayJump<<", low cut 20->500 Hz "<<cutJump<<" (a plain sine steps "<<plain<<")\n";
  require(delayJump<plain*3.f&&cutJump<plain*3.f,"pre-delay and low-cut changes are smoothed (no click)");}
}

MemoryBlock withoutPhase2Controls(const MemoryBlock& state){
 auto tree=ValueTree::readFromData(state.getData(),state.getSize());
 for(int i=tree.getNumChildren();--i>=0;){const auto id=tree.getChild(i)["id"].toString();for(auto added:{"a.pan","b.pan","a.solo","b.solo","swap","tap.target","comp.mode","fet.input","fet.output","fet.attack","fet.release","fet.ratio","fet.mix","fet.hpf","verb.predelay","verb.lowcut"})if(id==added){tree.removeChild(i,nullptr);break;}}
 MemoryBlock out;MemoryOutputStream stream(out,false);tree.writeToStream(stream);return out;
}

void phase2HostTests(){
 // New instances use the FET 76 compressor; an old (baseline) recall gets the baseline Glue compressor and neutral new controls.
 {auto fresh=makeHost();require(int(fresh->get(DualProcessor::CompMode))==DualProcessor::Fet76&&int(fresh->get(DualProcessor::FetRatio))==2,"a new instance defaults to FET 76 at 12:1");
  auto source=makeHost();source->write("a.pan",.5f);source->write("swap",1);source->write("comp.mode",1);source->write("fet.input",30);source->write("verb.predelay",40);source->write("a.solo",1);source->write("room",.8f);
  MemoryBlock saved;source->getStateInformation(saved);const auto old=withoutPhase2Controls(saved);
  require(ValueTree::readFromData(old.getData(),old.getSize()).getChildWithProperty("id","comp.mode").isValid()==false,"test fixture really lacks the Phase 2 controls");
  auto target=makeHost();target->write("a.pan",-.7f);target->write("b.solo",1);target->write("fet.input",10);target->write("comp.mode",1);target->write("tap.target",2);
  auto result=target->loadPerformance(old.getData(),int(old.getSize()));require(result.wasOk(),"a baseline-format performance (no Phase 2 controls) still loads");
  require(int(target->get(DualProcessor::CompMode))==DualProcessor::GlueCompressor,"baseline recall selects the baseline Glue compressor");
  require(target->get(DualProcessor::APan)==0&&target->get(DualProcessor::BSolo)<.5f&&target->get(DualProcessor::FetInput)==6&&int(target->get(DualProcessor::TapTarget))==0&&target->get(DualProcessor::Swap)<.5f,"baseline recall resets every new control to its neutral value");
  require(std::abs(target->value("room")-.8f)<1.e-6,"baseline recall still restores the old controls");
  auto full=makeHost();result=full->loadPerformance(saved.getData(),int(saved.getSize()));require(result.wasOk()&&int(full->get(DualProcessor::CompMode))==DualProcessor::Fet76&&std::abs(full->get(DualProcessor::APan)-.5f)<1.e-6&&full->get(DualProcessor::Swap)>.5f&&std::abs(full->get(DualProcessor::FetInput)-30.f)<1.e-4,"a Phase 2 performance recalls its new controls");}
 // Split swap puts A on the high keys.
 {auto host=makeHost();MidiBuffer in,ma,mb;host->write("routing",1);host->write("split",60);in.addEvent(MidiMessage::noteOn(1,72,.8f),0);in.addEvent(MidiMessage::noteOn(1,48,.8f),1);host->routeMidi(in,ma,mb);
  require(ma.getNumEvents()==1&&(*ma.begin()).getMessage().getNoteNumber()==48&&mb.getNumEvents()==1&&(*mb.begin()).getMessage().getNoteNumber()==72,"split: A plays low, B plays high");
  host->write("swap",1);in.clear();in.addEvent(MidiMessage::noteOn(2,72,.8f),0);in.addEvent(MidiMessage::noteOn(2,48,.8f),1);host->routeMidi(in,ma,mb);
  require(ma.getNumEvents()==1&&(*ma.begin()).getMessage().getNoteNumber()==72&&mb.getNumEvents()==1&&(*mb.begin()).getMessage().getNoteNumber()==48,"split swap: A plays high, B plays low");}
 // Pan: hard right silences that strip's left side and leaves its right side exactly as the centred render.
 for(int engine=0;engine<2;++engine){
  auto centred=makeHost(),panned=makeHost();
  for(auto* host:{centred.get(),panned.get()}){host->write("routing",float(engine+2));host->write("a.gain",0);host->write("b.gain",0);host->write("master",0);}
  panned->write(engine==0?"a.pan":"b.pan",1.f);centred->prepareToPlay(48000,256);panned->prepareToPlay(48000,256);
  AudioBuffer<float> straight(2,256),turned(2,256);float leftPeak=0,rightPeak=0;bool rightIdentical=true;
  for(int block=0;block<60;++block){MidiBuffer m1,m2;if(block==0){m1.addEvent(MidiMessage::noteOn(1,60,.8f),5);m2.addEvent(MidiMessage::noteOn(1,60,.8f),5);}centred->processBlock(straight,m1);panned->processBlock(turned,m2);
   leftPeak=std::max(leftPeak,turned.getMagnitude(0,0,256));rightPeak=std::max(rightPeak,turned.getMagnitude(1,0,256));for(int i=0;i<256;++i)rightIdentical=rightIdentical&&turned.getSample(1,i)==straight.getSample(1,i);}
  std::cout<<"INFO: pan hard right, engine "<<engine<<": left peak "<<leftPeak<<", right peak "<<rightPeak<<"\n";
  require(rightPeak>.01f,"pan test has signal");require(leftPeak<1.e-6f,"pan hard right silences the left side of that strip");require(rightIdentical,"pan hard right leaves the right side exactly as the centred render");
 }
 // Solo: with A soloed in Layer mode the output equals A playing alone, while B keeps running underneath.
 {auto soloed=makeHost(),alone=makeHost();soloed->write("routing",0);soloed->write("a.solo",1);alone->write("routing",2);
  for(auto* host:{soloed.get(),alone.get()}){host->write("a.gain",0);host->write("b.gain",0);host->write("master",0);host->prepareToPlay(48000,256);}
  AudioBuffer<float> one(2,256),two(2,256);bool identical=true;double energy=0;
  for(int block=0;block<80;++block){MidiBuffer m1,m2;if(block==0)for(int note:{48,60,67}){m1.addEvent(MidiMessage::noteOn(1,note,.8f),3);m2.addEvent(MidiMessage::noteOn(1,note,.8f),3);}soloed->processBlock(one,m1);alone->processBlock(two,m2);
   for(int c=0;c<2;++c)for(int i=0;i<256;++i){identical=identical&&one.getSample(c,i)==two.getSample(c,i);energy+=one.getSample(c,i)*one.getSample(c,i);}}
  require(energy>1.e-6&&identical,"solo A in Layer mode sounds exactly like A alone");require(soloed->b.voices.load()>0,"the soloed-out engine keeps running");}
 // Muted and soloed together is silent; solo on both strips lets both through.
 {auto host=makeHost();host->write("routing",0);host->write("b.solo",1);host->write("b.mute",1);host->write("master",0);host->prepareToPlay(48000,256);AudioBuffer<float> out(2,256);float peak=0;
  for(int block=0;block<40;++block){MidiBuffer m;if(block==0)m.addEvent(MidiMessage::noteOn(1,60,.8f),0);host->processBlock(out,m);peak=std::max(peak,out.getMagnitude(0,256));}
  require(peak==0.f,"a muted strip stays silent even when soloed, and the other strip is excluded by the solo");}
 {auto both=makeHost(),plain=makeHost();both->write("a.solo",1);both->write("b.solo",1);for(auto* host:{both.get(),plain.get()}){host->write("routing",0);host->write("master",0);host->prepareToPlay(48000,256);}
  AudioBuffer<float> one(2,256),two(2,256);bool identical=true;for(int block=0;block<40;++block){MidiBuffer m1,m2;if(block==0){m1.addEvent(MidiMessage::noteOn(1,60,.8f),0);m2.addEvent(MidiMessage::noteOn(1,60,.8f),0);}both->processBlock(one,m1);plain->processBlock(two,m2);for(int c=0;c<2;++c)for(int i=0;i<256;++i)identical=identical&&one.getSample(c,i)==two.getSample(c,i);}
  require(identical,"solo on both strips is the same as no solo");}
 // Meters and the gain-reduction readout.
 {auto host=makeHost();host->write("routing",0);host->write("glue",1);host->write("fet.input",24);host->write("fet.ratio",3);host->prepareToPlay(48000,256);AudioBuffer<float> out(2,256);float reduction=0;
  for(int block=0;block<120;++block){MidiBuffer m;if(block==0)for(int note:{48,55,60,67})m.addEvent(MidiMessage::noteOn(1,note,.9f),0);host->processBlock(out,m);if(block==100)reduction=host->gainReduction.load();}
  std::cout<<"INFO: FET gain reduction readout with +24 dB input, 20:1 on a four-note chord: "<<reduction<<" dB; A left "<<host->levels[0].load()<<", output left "<<host->levels[4].load()<<"\n";
  require(reduction>1.f,"the gain-reduction readout moves when the FET compressor works");require(host->levels[0].load()>0&&host->levels[1].load()>0&&host->levels[4].load()>0&&host->levels[5].load()>0,"stereo meters read signal");
  host->write("comp",0);for(int block=0;block<4;++block){MidiBuffer m;host->processBlock(out,m);}require(host->gainReduction.load()==0.f,"gain reduction reads zero with the compressor off");
  // With Master FX bypassed the compressor's work is inaudible, so the needle must not move.
  host->write("comp",1);host->write("glue",0);for(int block=0;block<60;++block){MidiBuffer m;host->processBlock(out,m);}
  std::cout<<"INFO: gain-reduction readout with Master FX bypassed: "<<host->gainReduction.load()<<" dB\n";require(host->gainReduction.load()<.05f,"the gain-reduction needle reads zero while Master FX is bypassed");
  host->write("glue",1);for(int block=0;block<60;++block){MidiBuffer m;host->processBlock(out,m);}require(host->gainReduction.load()>1.f,"the needle moves again when Master FX is switched on");}
 // Switching compressor type starts the new type from a clean state: after loud material in FET, a trip through Glue and back
 // must not leave the FET detector holding old gain reduction.
 {auto host=makeHost();host->write("routing",0);host->write("glue",1);host->write("fet.input",24);host->write("fet.ratio",3);host->prepareToPlay(48000,256);AudioBuffer<float> out(2,256);
  for(int block=0;block<100;++block){MidiBuffer m;if(block==0)for(int note:{48,55,60,67})m.addEvent(MidiMessage::noteOn(1,note,.9f),0);host->processBlock(out,m);}
  require(host->gainReduction.load()>3.f,"switch test: FET is compressing before the switch");
  host->write("comp.mode",0);for(int block=0;block<20;++block){MidiBuffer m;host->processBlock(out,m);}
  host->write("a.mute",1);host->write("b.mute",1);for(int block=0;block<40;++block){MidiBuffer m;host->processBlock(out,m);}   // strips muted: the master path sees silence
  host->write("comp.mode",1);{MidiBuffer m;host->processBlock(out,m);}
  std::cout<<"INFO: FET gain reduction in the first block after switching back from Glue with silent input: "<<host->gainReduction.load()<<" dB\n";
  require(host->gainReduction.load()<.01f,"switching compressor type does not leave stale gain reduction behind");}
 // Tap tempo: three taps half a second apart set about 120 BPM on the chosen engine(s) and turn off their host-sync.
 {auto host=makeHost();host->tapTempo();Thread::sleep(500);host->tapTempo();Thread::sleep(500);const float bpm=host->tapTempo();
  std::cout<<"INFO: tap tempo, three taps 500 ms apart: "<<bpm<<" BPM\n";
  require(bpm>112.f&&bpm<128.f,"tap tempo averages the tap intervals");require(std::abs(host->a.getParam("clock.bpm")-bpm)<.5f&&std::abs(host->b.read("clock.bpm")-bpm)<.5f,"tap tempo sets both engines");require(host->a.getParam("clock.host")<.5f&&host->b.read("clock.host")<.5f,"tap tempo turns off Follow host on both engines");}
 {auto host=makeHost();host->write("tap.target",1);host->tapTempo();Thread::sleep(500);const float bpm=host->tapTempo();require(bpm>0&&std::abs(host->a.getParam("clock.bpm")-bpm)<.5f,"tap target A sets Synth A");require(std::abs(host->b.read("clock.bpm")-138.f)<.01f&&host->b.read("clock.host")>.5f,"tap target A leaves Synth B alone");}
}

// Every parameter the unmodified baseline published (id, name, steps, default, range and choice texts) must still exist, unchanged.
void claudePrismBankTest() {
 auto host = makeHost();
 auto& prism = host->b;
 const auto& schema = SynthB::Schema::get();
 constexpr int originalCount = 117, importedCount = 63;
 require(prism.factoryCount() >= originalCount + importedCount, "Claude bank is appended to the original bank");
 require(prism.factoryPatch(originalCount - 1)["category"].toString() != "Claude Compatibility",
         "the last original preset retains its index");

 const auto& tables = SynthB::Tables::get();
 require(SynthB::tableNames().size() == 31 && SynthB::tableNames()[30] == "Imported",
         "original and imported table choices are unchanged");
 for (int selection = 1; selection <= 30; ++selection) {
  const auto& table = tables.at(0, nullptr, selection);
  require(table.frames == 32 && table.title == SynthB::claudeTableNames()[size_t(selection)],
          "Claude table selection resolves by identity");
  double energy = 0;
  for (int i = 0; i < 128; ++i) {
   const auto value = table.sample(float(i) / 128.f, .37f, 0.f);
   require(std::isfinite(value), "Claude wavetable is finite");
   energy += value * value;
  }
  require(energy > 1.e-5, "Claude wavetable contains signal");
 }
 host->write("routing", 3);
 host->prepareToPlay(48000, 256);
 AudioBuffer<float> audio(2, 256);
 for (int index = originalCount; index < originalCount + importedCount; ++index) {
  const auto preset = prism.factoryPatch(index);
  const auto restored = prism.restore(preset);
  if (restored.failed())
   std::cerr << preset["name"].toString() << ": " << restored.getErrorMessage() << "\n";
  require(restored.wasOk(), "each Claude compatibility preset validates");
  require(prism.read(schema.loc.claudeTable[0]) >= 1 && prism.read(schema.loc.claudeTable[1]) >= 1,
          "mapped oscillators use the Claude table selectors");
  double energy = 0;
  for (int block = 0; block < 110; ++block) {
   MidiBuffer midi;
   if (block == 0) midi.addEvent(MidiMessage::noteOn(1, 60, .8f), 0);
   host->processBlock(audio, midi);
   for (int c = 0; c < 2; ++c)
    for (int n = 0; n < 256; ++n) {
     const float value = audio.getSample(c, n);
     require(std::isfinite(value), "Claude compatibility preset renders finite audio");
     energy += double(value) * value;
    }
  }
  if (energy <= 1.e-9) std::cerr << "silent mapped preset: " << preset["name"].toString() << "\n";
  require(energy > 1.e-9, "Claude compatibility preset produces audio");
 }
 MemoryBlock saved;
 host->getStateInformation(saved);
 auto recalled = makeHost();
 require(recalled->loadPerformance(saved.getData(), int(saved.getSize())).wasOk(),
         "Claude performance round-trips");
 for (int lane = 0; lane < 2; ++lane)
  require(recalled->b.read(schema.loc.claudeTable[lane]) == prism.read(schema.loc.claudeTable[lane]),
          "Claude table selection survives session recall");
 prism.selectFactory(0);
 require(prism.read(schema.loc.claudeTable[0]) == 0 && prism.read(schema.loc.claudeTable[1]) == 0,
         "original presets return to original table selection");
}

void signalStudiesTest(const File& auditionFolder = {}) {
 constexpr int blockSize = 256, blocks = 750;
 for (int engine = 0; engine < 2; ++engine) {
  auto host = makeHost();
  auto count = engine == 0 ? host->a.presetCount() : host->b.factoryCount();
  auto read = [&](int index) { return engine == 0 ? host->a.preset(index) : host->b.factoryPatch(index); };

  ComboBox category, browser;
  PresetBrowser::categories(category, count, read);
  PresetBrowser::rebuild(browser, category, {}, count, read);
  require(browser.getNumItems() == count && browser.getRootMenu()->getNumItems() < count,
          "all sounds are grouped without losing any presets");
  category.setText("Signal Studies", dontSendNotification);
  PresetBrowser::rebuild(browser, category, {}, count, read);
  require(browser.getNumItems() == 8, "Signal Studies filter shows its eight presets");
  PresetBrowser::rebuild(browser, category, "glide", count, read);
  require(browser.getNumItems() == 1 && browser.getItemId(0) == count - 7,
          "tag search returns the original preset ID in a filtered submenu");
  browser.setSelectedId(count - 7, dontSendNotification);
  require(browser.getSelectedId() == count - 7, "nested preset item is selectable");

  std::unique_ptr<AudioFormatWriter> writer;
  if (auditionFolder.isDirectory()) {
   const auto path = auditionFolder.getChildFile(engine == 0 ? "Ember-Signal-Studies.wav" : "Prism-Signal-Studies.wav");
   auto stream = path.createOutputStream();
   require(stream != nullptr, "create audition file");
   stream->setPosition(0);
   stream->truncate();
   WavAudioFormat format;
   writer.reset(format.createWriterFor(stream.release(), 48000, 2, 24, {}, 0));
   require(writer != nullptr, "open audition WAV writer");
  }
  host->write("routing", engine == 0 ? 2 : 3);
  AudioBuffer<float> audio(2, blockSize);
  AudioBuffer<float> example(2, blockSize * blocks);
  std::vector<std::vector<float>> previous;
  for (int index = count - 8; index < count; ++index) {
   const auto patch = read(index);
   const auto restored = engine == 0 ? host->a.setPatch(patch) : host->b.restore(patch);
   if (restored.failed()) std::cerr << patch["name"].toString() << ": " << restored.getErrorMessage() << "\n";
   require(restored.wasOk(), "Signal Studies patch validates");
   host->prepareToPlay(48000, blockSize);
   host->panic.store(true);
   for (int block = 0; block < blocks; ++block) {
    MidiBuffer midi;
    if (block == 0) midi.addEvent(MidiMessage::noteOn(1, 48, .84f), 0);
    if (block == 94) midi.addEvent(MidiMessage::noteOn(1, 55, .66f), 0);
    if (block == 188) midi.addEvent(MidiMessage::noteOn(1, 60, .91f), 0);
    if (block == 469) for (int note : {48, 55, 60}) midi.addEvent(MidiMessage::noteOff(1, note), 0);
    host->processBlock(audio, midi);
    for (int c = 0; c < 2; ++c)
     example.copyFrom(c, block * blockSize, audio, c, 0, blockSize);
   }
   std::vector<float> signature;
   double energy = 0;
   for (int i = 0; i < example.getNumSamples(); ++i) {
    const float left = example.getSample(0, i), right = example.getSample(1, i);
    require(std::isfinite(left) && std::isfinite(right), "curated patch output is finite");
    energy += double(left) * left + double(right) * right;
    if (i % 8 == 0) { signature.push_back(left); signature.push_back(right); }
   }
   require(energy > 1.e-5, "curated patch produces audible signal");
   double norm = 0;
   for (float value : signature) norm += double(value) * value;
   require(norm > 0, "curated signature contains audio");
   for (float& value : signature) value /= float(std::sqrt(norm));
   double largest = 0;
   for (const auto& other : previous) {
    double dot = 0;
    for (size_t i = 0; i < signature.size(); ++i) dot += double(signature[i]) * other[i];
    largest = std::max(largest, std::abs(dot));
   }
   require(largest < .97, "no gain-matched duplicate render in the curated bank");
   previous.push_back(std::move(signature));
   std::cout << "INFO: Signal Studies " << patch["name"].toString()
             << ", max correlation with preceding patches " << largest << "\n";
   if (writer) {
    example.applyGain(.55f / example.getMagnitude(0, example.getNumSamples()));
    require(writer->writeFromAudioSampleBuffer(example, 0, example.getNumSamples()), "write audition clip");
   }
  }
 }
}

void pitchBendModulationTest() {
 auto host = makeHost();
 auto& prism = host->b;
 const auto& schema = SynthB::Schema::get();
 const int destination = schema.find("osc1.pos");
 const int route = schema.loc.route[0];

 prism.write(destination, .5f);
 prism.route(SynthB::pitchBendSource, destination);
 prism.write(route + SynthB::depth, .25f);
 require(prism.read(route) == SynthB::pitchBendSource, "Prism accepts Pitch Bend from the drag source");
 host->prepareToPlay(48000, 256);

 AudioBuffer<float> audio(2, 256);
 MidiBuffer midi;
 midi.addEvent(MidiMessage::noteOn(2, 60, .8f), 0);
 auto render = [&](int channel, int bend) {
  midi.addEvent(MidiMessage::pitchWheel(channel, bend), 0);
  for (int block = 0; block < 48; ++block)
   host->processBlock(audio, midi);
  return prism.engine.displayed[size_t(destination)].load();
 };
 require(std::abs(render(1, 16383) - .5f) < .002f, "other-channel bend leaves Prism modulation unchanged");
 require(std::abs(render(2, 16383) - .75f) < .002f, "positive pitch bend raises the routed destination");
 require(std::abs(render(2, 0) - .25f) < .002f, "negative pitch bend lowers the routed destination");
 prism.write(route + SynthB::bipolar, 0.f);
 require(std::abs(render(2, 0) - .5f) < .002f, "unipolar pitch bend maps the negative endpoint to zero");
 require(std::abs(render(2, 8192) - .625f) < .002f, "unipolar pitch bend maps centre to halfway");

 MemoryBlock saved;
 host->getStateInformation(saved);
 auto recalled = makeHost();
 require(recalled->loadPerformance(saved.getData(), int(saved.getSize())).wasOk()
         && recalled->b.read(route) == SynthB::pitchBendSource,
         "Pitch Bend survives full performance recall");
}

void parameterCatalogTest(){
 const auto baselineFile=File(__FILE__).getParentDirectory().getChildFile("baseline-parameter-catalog.txt");
 require(baselineFile.existsAsFile(),"baseline parameter catalog file is present next to the tests");
 StringArray baseline;baseline.addLines(baselineFile.loadFileAsString());baseline.removeEmptyStrings();
 auto host=makeHost();const auto current=parameterCatalog(*host);
 std::map<String,String> now;for(const auto& line:current)now[line.upToFirstOccurrenceOf("|",false,false)]=line;
 auto equivalent = [](const String& expected, const String& actual) {
  if (expected == actual) return true;
  const auto oldFields = StringArray::fromTokens(expected, "|", "");
  const auto newFields = StringArray::fromTokens(actual, "|", "");
  // The only allowed choice extension is Pitch Bend, appended to Prism's existing routes.
  if (oldFields.size() == 10 && newFields.size() == 10 && oldFields[0].startsWith("B.mod")
      && oldFields[0].endsWith(".source") && oldFields[2] == "24" && oldFields[8] == "Space") {
   auto extended = oldFields;
   extended.set(2, "25");
   extended.set(8, "Pitch Bend");
   extended.set(9, oldFields[9] + ";Pitch Bend");
   return extended == newFields;
  }
  if (oldFields.size() < 9 || oldFields.size() != newFields.size() || oldFields[3] != "0")
   return false;
  for (int field = 0; field < oldFields.size(); ++field) {
   if (oldFields[field] == newFields[field]) continue;
   // Continuous defaults and midpoints are computed through JUCE's float normalization.
   // Allow MSVC/Clang rounding differences while requiring exact metadata and endpoints.
   if (field != 5 && field != 7) return false;
   const double oldValue = oldFields[field].getDoubleValue();
   const double newValue = newFields[field].getDoubleValue();
   const double tolerance = 3.e-6 + 2.e-7 * std::abs(oldValue);
   if (!std::isfinite(newValue) || std::abs(oldValue - newValue) > tolerance) return false;
  }
  return true;
 };
 int missing=0,changed=0;
 for(const auto& line:baseline){auto id=line.upToFirstOccurrenceOf("|",false,false);auto found=now.find(id);if(found==now.end()){++missing;std::cerr<<"catalog: missing "<<id<<"\n";}else if(!equivalent(line,found->second)){++changed;std::cerr<<"catalog: changed "<<id<<"\n  was "<<line<<"\n  now "<<found->second<<"\n";}}
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
 // Same-machine difference is exactly 0 (checked on Windows). The tolerance only absorbs last-bit float differences between
 // compilers and CPUs (the golden file was rendered on Windows); any real change to the legacy path is far above -80 dB.
 require(worst<=1.e-4f,"legacy master effects sound unchanged (difference below -80 dB)");
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
 if(argc==3&&String(argv[1])=="--audition"){auto folder=File::getCurrentWorkingDirectory().getChildFile(argv[2]);folder.createDirectory();signalStudiesTest(folder);return 0;}
 signalStudiesTest();midiOwnershipTests();parameterCatalogTest();pitchBendModulationTest();claudePrismBankTest();goldenMasterTest();fetDspTests();phase2HostTests();engineChannelTests();recallBoundaryTest();routeMidiAllocationTest();slicingTests();stateTests();loadDuringPlaybackTest();processBlockAllocationInfo();
 {auto heapP=std::make_unique<DualProcessor>();DualProcessor& p=*heapP;MidiBuffer input,ma,mb;input.addEvent(MidiMessage::noteOn(1,60,.8f),17);p.routeMidi(input,ma,mb);require(ma.getNumEvents()==1&&mb.getNumEvents()==1,"layer fans out to both");p.write("routing",1);p.write("split",72);input.clear();input.addEvent(MidiMessage::noteOff(1,60),21);p.routeMidi(input,ma,mb);require(ma.getNumEvents()==1&&mb.getNumEvents()==1,"held note ownership survives changed split");input.clear();input.addEvent(MidiMessage::noteOn(1,71,.8f),3);input.addEvent(MidiMessage::noteOn(1,72,.8f),4);p.routeMidi(input,ma,mb);require(ma.getNumEvents()==1&&mb.getNumEvents()==1,"split boundary routes A below and B at split");require((*ma.begin()).samplePosition==3&&(*mb.begin()).samplePosition==4,"MIDI sample offsets preserved");p.write("a.high",60);input.clear();input.addEvent(MidiMessage::noteOn(1,61,.7f),0);p.routeMidi(input,ma,mb);require(ma.isEmpty()&&mb.isEmpty(),"split intersects per-engine note range");p.write("a.channel",2);input.clear();input.addEvent(MidiMessage::noteOn(1,40,.7f),0);p.routeMidi(input,ma,mb);require(ma.isEmpty()&&mb.isEmpty(),"channel filter");input.clear();input.addEvent(MidiMessage::controllerEvent(1,64,127),0);p.routeMidi(input,ma,mb);require(ma.getNumEvents()==1&&mb.getNumEvents()==1,"controllers reach both to release tails");}
 for(int engine=0;engine<2;++engine){auto heapHost=std::make_unique<DualProcessor>();DualProcessor& host=*heapHost;std::unique_ptr<AudioProcessor> reference=engine==0?std::unique_ptr<AudioProcessor>(new SynthAProcessor):std::unique_ptr<AudioProcessor>(new PrismProcessor);host.write("routing",float(engine+2));host.write("a.gain",0);host.write("b.gain",0);host.write("master",0);host.write("glue",0);host.prepareToPlay(48000,256);reference->setRateAndBufferSizeDetails(48000,256);reference->prepareToPlay(48000,256);auto visible=std::unique_ptr<AudioProcessorEditor>(host.createEditor());AudioBuffer<float> output(2,256),raw(2,256);std::array<std::array<float,4>,2> delay{};int head=0;double energy=0;for(int block=0;block<200;++block){if(block%40==0)dynamic_cast<DualEditor*>(visible.get())->select((block/40)%2);MidiBuffer midi,copy;if(block==0){midi.addEvent(MidiMessage::noteOn(1,60,.8f),17);midi.addEvent(MidiMessage::noteOn(1,67,.7f),80);}if(block==120)midi.addEvent(MidiMessage::allNotesOff(1),9);copy=midi;host.processBlock(output,midi);reference->processBlock(raw,copy);for(int n=0;n<256;++n){for(int c=0;c<2;++c){float expected=raw.getSample(c,n);if(engine==1){float aligned=delay[size_t(c)][size_t(head)];delay[size_t(c)][size_t(head)]=expected;expected=aligned;}if(output.getSample(c,n)!=expected){std::cerr<<"engine "<<engine<<" block "<<block<<" sample "<<n<<" got "<<output.getSample(c,n)<<" expected "<<expected<<" delta "<<(output.getSample(c,n)-expected)<<" gains "<<host.value("a.gain")<<" "<<host.value("b.gain")<<" "<<host.value("master")<<"\n";require(false,"unity/bypassed output exactly equals original engine (B aligned by4samples)");}energy+=expected*expected;}head=(head+1)%4;}}require(energy>1.e-6,"identity comparison has signal");}
 auto heapMain=std::make_unique<DualProcessor>();DualProcessor& p=*heapMain;p.a.loadPreset(3);p.b.selectFactory(4);p.write("split",65);p.write("a.gain",-3);p.write("b.mute",1);p.write("room",.83f);p.panel.store(1);MemoryBlock state;p.getStateInformation(state);auto heapRecalled=std::make_unique<DualProcessor>();DualProcessor& recalled=*heapRecalled;recalled.setStateInformation(state.getData(),int(state.getSize()));require(recalled.value("split")==65&&recalled.value("b.mute")==1&&std::abs(recalled.value("room")-.83f)<1.e-6,"host state recall");require(recalled.a.patchName()==p.a.patchName()&&recalled.b.title()==p.b.title()&&recalled.panel.load()==1,"both complete engine states and view recall");
 auto* child=p.b.parameters.getParameter("osc1.pos");AudioProcessorParameter* proxy=nullptr;for(auto* parameter:p.getParameters())if(auto* id=dynamic_cast<AudioProcessorParameterWithID*>(parameter))if(id->paramID=="B.osc1.pos")proxy=parameter;require(proxy!=nullptr,"prefixed automation exists");proxy->setValue(.7f);require(std::abs(child->getValue()-.7f)<1.e-6,"host automation forwards to original parameter");p.b.write("osc1.pos",.2f);require(std::abs(proxy->getValue()-child->getValue())<1.e-6,"original UI forwards parameter value");
 p.prepareToPlay(48000,256);p.write("b.mute",0);p.write("glue",1);p.write("eq",1);p.write("width",1.5f);p.write("low",12);p.write("air",12);AudioBuffer<float> output(2,256);double energy=0;for(int block=0;block<600;++block){MidiBuffer midi;if(block==0)for(int note:{48,55,60,67})midi.addEvent(MidiMessage::noteOn(1,note,.8f),0);if(block==180)midi.addEvent(MidiMessage::allNotesOff(1),0);p.processBlock(output,midi);for(int c=0;c<2;++c)for(int n=0;n<256;++n){float v=output.getSample(c,n);require(std::isfinite(v),"master effects finite");energy+=v*v;}}require(energy>1.e-6,"layer and glue render");
 {auto heapMuted=std::make_unique<DualProcessor>();DualProcessor& muted=*heapMuted;muted.prepareToPlay(48000,256);muted.write("a.mute",1);muted.write("b.mute",1);AudioBuffer<float> test(2,256);for(int i=0;i<16;++i){MidiBuffer midi;if(i==0)midi.addEvent(MidiMessage::noteOn(1,60,.8f),0);muted.processBlock(test,midi);}require(test.getMagnitude(0,256)==0&&muted.a.voicesPlaying.load()>0&&muted.b.voices.load()>0,"mute fades output while both engines keep running");muted.write("b.mute",0);double signal=0;for(int i=0;i<16;++i){MidiBuffer midi;muted.processBlock(test,midi);signal+=test.getMagnitude(0,256);}require(signal>1.e-5&&muted.peaks[0].load()==0,"unmuting reveals held B without retriggering A or B");}
 if(argc>1){p.write("low",0);p.write("air",0);p.write("width",1);p.write("eq",0);p.write("glue",0);File folder=File::getCurrentWorkingDirectory().getChildFile(argv[1]);folder.createDirectory();auto editor=std::unique_ptr<AudioProcessorEditor>(p.createEditor());for(int panel=0;panel<2;++panel){dynamic_cast<DualEditor*>(editor.get())->select(panel);for(int width:{1280,1600}){editor->setSize(width,width*840/1600);auto snapshot=editor->createComponentSnapshot(editor->getLocalBounds(),true);auto stream=folder.getChildFile("Dual-Synth-"+String(panel==0?"A":"B")+"-"+String(width)+".png").createOutputStream();stream->setPosition(0);stream->truncate();PNGImageFormat png;require(png.writeImageToStream(snapshot,*stream),"actual host editor snapshot");}}}
 if(argc>1){ // gain-reduction needle at several readings, for visual review
  const auto folder=File::getCurrentWorkingDirectory().getChildFile(argv[1]);
  {auto host=makeHost();auto editor=std::unique_ptr<AudioProcessorEditor>(host->createEditor());auto* dual=dynamic_cast<DualEditor*>(editor.get());editor->setSize(1600,840);
   struct View{const char* name;int tab;bool glue;};
   for(auto view:{View{"reverb",1,false},View{"tone",2,false},View{"glue",0,true}}){host->write("comp.mode",view.glue?0.f:1.f);dual->showMasterTab(view.tab);
    auto snapshot=editor->createComponentSnapshot(editor->getLocalBounds(),true);auto stream=folder.getChildFile(String("Dual-Synth-master-")+view.name+".png").createOutputStream();stream->setPosition(0);stream->truncate();PNGImageFormat png;require(png.writeImageToStream(snapshot,*stream),"master tab snapshot");}}
  for(float reading:{0.f,4.f,9.f,16.f,24.f}){GainReductionMeter meter;meter.setSize(264,80);meter.setReduction(reading);for(int i=0;i<80;++i)meter.tick();
   auto snapshot=meter.createComponentSnapshot(meter.getLocalBounds(),true,2.f);auto stream=folder.getChildFile("GR-meter-"+String(int(reading))+"dB.png").createOutputStream();stream->setPosition(0);stream->truncate();PNGImageFormat png;require(png.writeImageToStream(snapshot,*stream),"meter snapshot");}
 }
 std::cout<<"PASS: layer/split/ranges/channels, note ownership, sample offsets, output mutes preserve voices, both engines exact unity output (B four-sample alignment), complete host+engine recall, prefixed automation, master effects finite, editor lifecycle.\n";
}

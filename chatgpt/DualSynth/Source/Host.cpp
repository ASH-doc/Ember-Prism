#include "Host.h"
using namespace juce;
namespace {
float mixerGain(float db){return std::abs(db)<1.e-5f?1.f:Decibels::decibelsToGain(db);}
// The original parameter remains the authority. This bridge only publishes it through the host.
class Bridge final:public AudioProcessorParameterWithID,public AudioProcessorParameter::Listener {
 RangedAudioParameter& child;bool attached=true;
public:
 Bridge(String prefix,RangedAudioParameter& source):AudioProcessorParameterWithID(ParameterID(prefix+source.paramID,1),prefix+source.name,source.label),child(source){child.addListener(this);}
 ~Bridge()override{detach();}void detach(){if(attached){child.removeListener(this);attached=false;}}
 float getValue()const override{return child.getValue();}void setValue(float v)override{child.setValueNotifyingHost(v);}
 float getDefaultValue()const override{return child.getDefaultValue();}int getNumSteps()const override{return child.getNumSteps();}
 bool isDiscrete()const override{return child.isDiscrete();}bool isBoolean()const override{return child.isBoolean();}
 String getText(float v,int n)const override{return child.getText(v,n);}float getValueForText(const String& s)const override{return child.getValueForText(s);}
 void parameterValueChanged(int,float v)override{sendValueChangedMessageToListeners(v);}
 void parameterGestureChanged(int,bool begin)override{if(begin)beginChangeGesture();else endChangeGesture();}
};
}

Optional<AudioPlayHead::PositionInfo> SliceTransport::getPosition()const{
 if(source==nullptr)return {};
 auto position=source->getPosition();
 if(!position||offset==0)return position;
 const double seconds=double(offset)/sampleRate;
 if(position->getIsPlaying())if(auto bpm=position->getBpm())if(auto ppq=position->getPpqPosition())position->setPpqPosition(*ppq+seconds*(*bpm)/60.0);
 if(auto samples=position->getTimeInSamples())position->setTimeInSamples(*samples+offset);
 if(auto time=position->getTimeInSeconds())position->setTimeInSeconds(*time+seconds);
 return position;
}

AudioProcessorValueTreeState::ParameterLayout DualProcessor::layout(){
 AudioProcessorValueTreeState::ParameterLayout l;
 auto number=[&](String id,String title,float lo,float hi,float initial){l.add(std::make_unique<AudioParameterFloat>(ParameterID(id,1),title,NormalisableRange<float>(lo,hi),initial));};
 auto flag=[&](String id,String title,bool initial){l.add(std::make_unique<AudioParameterBool>(ParameterID(id,1),title,initial));};
 l.add(std::make_unique<AudioParameterChoice>(ParameterID("routing",1),"Keyboard routing",StringArray{"Layer","Split","A only","B only"},0));
 l.add(std::make_unique<AudioParameterInt>(ParameterID("split",1),"Split note: B starts here",0,127,60));
 for(String engine:{"a","b"}){for(String edge:{"low","high"})l.add(std::make_unique<AudioParameterInt>(ParameterID(engine+"."+edge,1),engine.toUpperCase()+" key "+edge,0,127,edge=="low"?0:127));l.add(std::make_unique<AudioParameterInt>(ParameterID(engine+".channel",1),engine.toUpperCase()+" MIDI channel (0=all)",0,16,0));number(engine+".gain",engine.toUpperCase()+" output dB",-60,6,-6);flag(engine+".mute",engine.toUpperCase()+" mute",false);}
 number("master","Master dB",-60,6,0);flag("glue","Master effects",false);flag("comp","Compressor",true);flag("verb","Reverb",true);flag("eq","Tone EQ",false);
 number("threshold","Threshold dB",-48,0,-18);number("ratio","Ratio",1,10,2);number("attack","Attack ms",1,100,20);number("release","Release ms",20,1000,180);
 number("room","Reverb size",0,1,.65f);number("damp","Reverb damping",0,1,.4f);number("wet","Reverb mix",0,.6f,.15f);
 number("width","Stereo width",0,1.5f,1);number("low","Low EQ dB",-12,12,0);number("air","Air EQ dB",-12,12,0);
 return l;
}
DualProcessor::DualProcessor():AudioProcessor(BusesProperties().withOutput("Dual stereo",AudioChannelSet::stereo(),true)),state(*this,nullptr,"DualHost",layout()){
 for(auto* child:{static_cast<AudioProcessor*>(&a),static_cast<AudioProcessor*>(&b)})for(auto* parameter:child->getParameters())if(auto* ranged=dynamic_cast<RangedAudioParameter*>(parameter))addParameter(new Bridge(child==&a?"A.":"B.",*ranged));
 // Resolve every host parameter once. The audio thread never looks parameters up by name.
 static const char* const ids[IdCount]={"routing","split","a.low","a.high","a.channel","a.gain","a.mute","b.low","b.high","b.channel","b.gain","b.mute","master","glue","comp","verb","eq","threshold","ratio","attack","release","room","damp","wet","width","low","air"};
 for(int i=0;i<IdCount;++i)cached[size_t(i)]=state.getRawParameterValue(ids[i]);
 // Storage is sized up front so the audio callback never allocates, even before the first prepareToPlay.
 bufferA.setSize(2,32768);bufferB.setSize(2,32768);
 a.setPlayHead(&transport);b.setPlayHead(&transport);
 setLatencySamples(16);
}
DualProcessor::~DualProcessor(){for(auto* param:getParameters())if(auto* bridge=dynamic_cast<Bridge*>(param))bridge->detach();}

float DualProcessor::value(const String& id)const{return state.getRawParameterValue(id)->load();}
void DualProcessor::write(const String& id,float v){auto* param=state.getParameter(id);param->beginChangeGesture();param->setValueNotifyingHost(param->convertTo0to1(v));param->endChangeGesture();}
bool DualProcessor::isBusesLayoutSupported(const BusesLayout& l)const{return l.getMainInputChannelSet().isDisabled()&&l.getMainOutputChannelSet()==AudioChannelSet::stereo();}
void DualProcessor::prepareToPlay(double sr,int block){
 sampleRate=sr;a.setRateAndBufferSizeDetails(sr,block);b.setRateAndBufferSizeDetails(sr,block);a.prepareToPlay(sr,block);b.prepareToPlay(sr,block);
 const int capacity=std::max(32768,block);
 bufferA.setSize(2,capacity);bufferB.setSize(2,capacity);midiA.ensureSize(65536);midiB.ensureSize(65536);sliceMidi.ensureSize(65536);
 owners={};alignB={};alignHead=0;keyboard.reset();reverb.setSampleRate(sr);reverb.reset();compressor.prepare({sr,uint32(capacity),2});toneLow[0]=toneLow[1]=toneHigh[0]=toneHigh[1]=0;
 for(auto* smooth:{&gainA,&gainB,&master,&width,&fxBlend,&verbWet})smooth->reset(sr,.02);
 gainA.setCurrentAndTargetValue(get(AMute)>.5f?0:mixerGain(get(AGain)));gainB.setCurrentAndTargetValue(get(BMute)>.5f?0:mixerGain(get(BGain)));master.setCurrentAndTargetValue(mixerGain(get(Master)));width.setCurrentAndTargetValue(get(Width));fxBlend.setCurrentAndTargetValue(get(Glue));verbWet.setCurrentAndTargetValue(get(Verb)>.5f?get(Wet):0);
}
void DualProcessor::releaseResources(){a.releaseResources();b.releaseResources();}

void DualProcessor::routeMidi(const MidiBuffer& input,MidiBuffer& ma,MidiBuffer& mb){
 ma.clear();mb.clear();
 // One snapshot of the routing controls for the whole buffer.
 const int mode=int(get(Routing)),split=int(get(Split));
 const int low[2]={int(get(ALow)),int(get(BLow))},high[2]={int(get(AHigh)),int(get(BHigh))},channel[2]={int(get(AChannel)),int(get(BChannel))};
 auto accepts=[&](int engine,int note,int midiChannel){return note>=low[engine]&&note<=high[engine]&&(channel[engine]==0||midiChannel==channel[engine]);};
 MidiBuffer* const lane[2]={&ma,&mb};
 for(const auto event:input){
  auto m=event.getMessage();int mask=3;
  const int midiChannel=m.getChannel(),c=jlimit(0,15,midiChannel-1);
  if(m.isNoteOn()||m.isNoteOff()){
   const int n=m.getNoteNumber();auto& owner=owners[size_t(c)][size_t(n)];
   if(m.isNoteOn()){
    // The same channel and pitch struck again before its note-off: release the earlier note on whichever
    // engine(s) own it, at this sample position and before the new note, then take fresh ownership.
    for(int engine=0;engine<2;++engine)if(owner&(1<<engine))lane[engine]->addEvent(MidiMessage::noteOff(midiChannel,n),event.samplePosition);
    const bool toA=mode!=3&&(mode!=1||n<split),toB=mode!=2&&(mode!=1||n>=split);
    mask=0;if(toA&&accepts(0,n,midiChannel))mask|=1;if(toB&&accepts(1,n,midiChannel))mask|=2;
    owner=static_cast<unsigned char>(mask);
   }else{mask=owner;owner=0;}
  }
  // CC120 / CC123 are per-channel messages: they forget only that channel's held notes. A host panic clears everything.
  if(m.isAllNotesOff()||m.isAllSoundOff())owners[size_t(c)].fill(0);
  if(mask&1)ma.addEvent(m,event.samplePosition);if(mask&2)mb.addEvent(m,event.samplePosition);
 }
}

void DualProcessor::renderSlice(AudioBuffer<float>& audio,int start,int length,const MidiBuffer& midi,Peaks& peak){
 routeMidi(midi,midiA,midiB);
 // Use sub-buffers without changing either child's block length or processing algorithm.
 AudioBuffer<float> outA(bufferA.getArrayOfWritePointers(),2,length),outB(bufferB.getArrayOfWritePointers(),2,length);
 a.processBlock(outA,midiA);b.processBlock(outB,midiB);
 // One snapshot of the mixer and master controls for the slice.
 gainA.setTargetValue(get(AMute)>.5f?0:mixerGain(get(AGain)));gainB.setTargetValue(get(BMute)>.5f?0:mixerGain(get(BGain)));master.setTargetValue(mixerGain(get(Master)));width.setTargetValue(get(Width));fxBlend.setTargetValue(get(Glue));verbWet.setTargetValue(get(Verb)>.5f?get(Wet):0);
 compressor.setThreshold(get(Threshold));compressor.setRatio(get(Ratio));compressor.setAttack(get(Attack));compressor.setRelease(get(Release));
 Reverb::Parameters rv;rv.roomSize=get(Room);rv.damping=get(Damp);rv.wetLevel=1;rv.dryLevel=0;rv.width=1;reverb.setParameters(rv);
 const float lowCoefficient=1-std::exp(-MathConstants<float>::twoPi*200/float(sampleRate)),highCoefficient=1-std::exp(-MathConstants<float>::twoPi*6500/float(sampleRate));
 const float lowGain=Decibels::decibelsToGain(get(Low)),airGain=Decibels::decibelsToGain(get(Air));
 const bool eqOn=get(Eq)>.5f,compOn=get(Comp)>.5f;
 for(int i=0;i<length;++i){
  const float ga=gainA.getNextValue(),gb=gainB.getNextValue();float dry[2],wet[2];
  for(int c=0;c<2;++c){
   const float sa=outA.getSample(c,i),sb=alignB[size_t(c)][size_t(alignHead)];
   alignB[size_t(c)][size_t(alignHead)]=outB.getSample(c,i);
   dry[c]=sa*ga+sb*gb;wet[c]=dry[c];
   peak.a=std::max(peak.a,std::abs(sa*ga));peak.b=std::max(peak.b,std::abs(sb*gb));
  }
  alignHead=(alignHead+1)%4;
  for(int c=0;c<2;++c){toneLow[c]+=lowCoefficient*(dry[c]-toneLow[c]);toneHigh[c]+=highCoefficient*(dry[c]-toneHigh[c]);if(eqOn)wet[c]+=toneLow[c]*(lowGain-1)+(dry[c]-toneHigh[c])*(airGain-1);}
  const float detector=std::max(std::abs(wet[0]),std::abs(wet[1]));const float compressed=compressor.processSample(0,detector);
  if(compOn&&detector>1.e-12f){const float reduction=compressed/detector;wet[0]*=reduction;wet[1]*=reduction;}
  float reverbL=wet[0],reverbR=wet[1];reverb.processStereo(&reverbL,&reverbR,1);
  const float vw=verbWet.getNextValue();wet[0]+=vw*(reverbL-wet[0]);wet[1]+=vw*(reverbR-wet[1]);
  const float mid=(wet[0]+wet[1])*.5f,side=(wet[0]-wet[1])*.5f*width.getNextValue();wet[0]=mid+side;wet[1]=mid-side;
  const float blend=fxBlend.getNextValue(),mg=master.getNextValue();
  for(int c=0;c<2;++c){const float result=(dry[c]+blend*(wet[c]-dry[c]))*mg;audio.setSample(c,start+i,result);peak.master=std::max(peak.master,std::abs(result));}
 }
}
void DualProcessor::processBlock(AudioBuffer<float>& audio,MidiBuffer& midi){
 const ScopedTryLock publication(publicationLock, !publicationPending.load());
 if (!publication.isLocked() || publicationPending.load()) {
  audio.clear();
  midi.clear();
  for (auto& peak : peaks) peak.store(0);
  return;
 }
 ScopedNoDenormals guard;const int n=audio.getNumSamples();
 if(panic.exchange(false)){a.panicRequested.store(true);b.silence.store(true);owners={};keyboard.reset();alignB={};reverb.reset();compressor.reset();toneLow[0]=toneLow[1]=toneHigh[0]=toneHigh[1]=0;}
 keyboard.processNextMidiBuffer(midi,0,n,true);float pb=bend.exchange(-2),mw=wheel.exchange(-1);if(pb>=-1)midi.addEvent(MidiMessage::pitchWheel(1,jlimit(0,16383,int(pb*8192+8192))),0);if(mw>=0)midi.addEvent(MidiMessage::controllerEvent(1,1,int(mw)),0);
 audio.clear();
 Peaks peak;const int capacity=sliceCapacity();
 if(n<=capacity){
  transport.set(getPlayHead(),0,sampleRate);renderSlice(audio,0,n,midi,peak);
 }else{
  // A host block longer than the prepared storage is rendered in bounded slices. Each slice gets its own
  // block-relative MIDI offsets and a transport position advanced by the samples already rendered.
  for(int start=0;start<n;start+=capacity){
   const int length=std::min(capacity,n-start);sliceMidi.clear();
   for(const auto event:midi)if(event.samplePosition>=start&&event.samplePosition<start+length)sliceMidi.addEvent(event.getMessage(),event.samplePosition-start);
   transport.set(getPlayHead(),start,sampleRate);renderSlice(audio,start,length,sliceMidi,peak);
  }
 }
 peaks[0].store(peak.a);peaks[1].store(peak.b);peaks[2].store(peak.master);midi.clear();
}
void DualProcessor::getStateInformation(MemoryBlock& block){const ScopedLock recall(recallLock);ValueTree tree=state.copyState();MemoryBlock sa,sb;a.getStateInformation(sa);b.getStateInformation(sb);tree.setProperty("engineA",sa.toBase64Encoding(),nullptr);tree.setProperty("engineB",sb.toBase64Encoding(),nullptr);tree.setProperty("panel",panel.load(),nullptr);MemoryOutputStream output(block,false);tree.writeToStream(output);}

Result DualProcessor::loadPerformance(const void* data,int length){
 const ScopedLock recall(recallLock);
 // Stage and validate everything first. Nothing below the "publish" line runs unless every check passed.
 if(data==nullptr||length<=0)return Result::fail("The performance file is empty.");
 if(length>maxPerformanceBytes)return Result::fail("The performance is larger than "+String(maxPerformanceBytes/(1024*1024))+" MB.");
 auto tree=ValueTree::readFromData(data,size_t(length));
 if(!tree.isValid()||!tree.hasType("DualHost"))return Result::fail("This is not a Dual Synth performance.");
 var patch[2];
 for(int engine=0;engine<2;++engine){
  const String name=engine==0?"Synth A":"Synth B";MemoryBlock payload;
  if(!payload.fromBase64Encoding(tree[engine==0?"engineA":"engineB"].toString())||payload.getSize()==0)return Result::fail(name+" data is missing or damaged.");
  if(payload.getSize()>size_t(2*1024*1024))return Result::fail(name+" data is larger than 2 MB.");
  if(JSON::parse(String::fromUTF8(static_cast<const char*>(payload.getData()),int(payload.getSize())),patch[engine]).failed())return Result::fail(name+" data is not a valid patch.");
 }
 if(auto checked=a.setPatch(patch[0],false);checked.failed())return Result::fail("Synth A: "+checked.getErrorMessage());
 // Prism validates and prepares its embedded asset before taking the publication lock.
 // A validated Ember patch has no resource-dependent apply failures.
 const ScopeGuard clearPending{[&] { publicationPending.store(false); }};
 std::optional<ScopedLock> publication;
 auto beginPublication = [&] {
  publicationPending.store(true);
  publication.emplace(publicationLock);
 };
 if (auto applied = b.restore(patch[1], true, beginPublication); applied.failed())
  return Result::fail("Synth B: " + applied.getErrorMessage());
 if (auto applied = a.setPatch(patch[0]); applied.failed())
  return applied;
 panel.store(jlimit(0,1,int(tree["panel"])));state.replaceState(tree);panic.store(true);
 return Result::ok();
}
void DualProcessor::setStateInformation(const void* data,int length){
 const auto result=loadPerformance(data,length);
 const ScopedLock lock(loadErrorLock);loadError=result.failed()?result.getErrorMessage():String();
}
String DualProcessor::lastLoadError()const{const ScopedLock lock(loadErrorLock);return loadError;}
AudioProcessorEditor* DualProcessor::createEditor(){return new DualEditor(*this);}
AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new DualProcessor;}

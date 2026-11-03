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
 setLatencySamples(16);
}
DualProcessor::~DualProcessor(){for(auto* param:getParameters())if(auto* bridge=dynamic_cast<Bridge*>(param))bridge->detach();}

float DualProcessor::value(const String& id)const{return state.getRawParameterValue(id)->load();}
void DualProcessor::write(const String& id,float v){auto* param=state.getParameter(id);param->beginChangeGesture();param->setValueNotifyingHost(param->convertTo0to1(v));param->endChangeGesture();}
bool DualProcessor::isBusesLayoutSupported(const BusesLayout& l)const{return l.getMainInputChannelSet().isDisabled()&&l.getMainOutputChannelSet()==AudioChannelSet::stereo();}
void DualProcessor::prepareToPlay(double sr,int block){sampleRate=sr;a.setRateAndBufferSizeDetails(sr,block);b.setRateAndBufferSizeDetails(sr,block);a.prepareToPlay(sr,block);b.prepareToPlay(sr,block);bufferA.setSize(2,std::max(32768,block));bufferB.setSize(2,std::max(32768,block));midiA.ensureSize(65536);midiB.ensureSize(65536);owners={};alignB={};alignHead=0;keyboard.reset();reverb.setSampleRate(sr);reverb.reset();compressor.prepare({sr,uint32(std::max(block,32768)),2});toneLow[0]=toneLow[1]=toneHigh[0]=toneHigh[1]=0;
 for(auto* smooth:{&gainA,&gainB,&master,&width,&fxBlend,&verbWet})smooth->reset(sr,.02);
 gainA.setCurrentAndTargetValue(value("a.mute")>.5f?0:mixerGain(value("a.gain")));gainB.setCurrentAndTargetValue(value("b.mute")>.5f?0:mixerGain(value("b.gain")));master.setCurrentAndTargetValue(mixerGain(value("master")));width.setCurrentAndTargetValue(value("width"));fxBlend.setCurrentAndTargetValue(value("glue"));verbWet.setCurrentAndTargetValue(value("verb")>.5f?value("wet"):0);
}
void DualProcessor::releaseResources(){a.releaseResources();b.releaseResources();}
void DualProcessor::routeMidi(const MidiBuffer& input,MidiBuffer& ma,MidiBuffer& mb){
 ma.clear();mb.clear();int mode=int(value("routing")),split=int(value("split"));
 for(const auto event:input){auto m=event.getMessage();int mask=3;if(m.isNoteOn()||m.isNoteOff()){int n=m.getNoteNumber(),c=jlimit(0,15,m.getChannel()-1);auto& owner=owners[size_t(c)][size_t(n)];if(m.isNoteOn()){mask=0;bool ra=mode!=3&&(mode!=1||n<split),rb=mode!=2&&(mode!=1||n>=split);if(ra&&n>=value("a.low")&&n<=value("a.high")&&(value("a.channel")==0||m.getChannel()==int(value("a.channel"))))mask|=1;if(rb&&n>=value("b.low")&&n<=value("b.high")&&(value("b.channel")==0||m.getChannel()==int(value("b.channel"))))mask|=2;owner=static_cast<unsigned char>(owner|mask);}else{mask=owner;owner=0;}}
 if(m.isAllNotesOff()||m.isAllSoundOff())owners={};if(mask&1)ma.addEvent(m,event.samplePosition);if(mask&2)mb.addEvent(m,event.samplePosition);
 }
}
void DualProcessor::processBlock(AudioBuffer<float>& audio,MidiBuffer& midi){
 ScopedNoDenormals guard;int n=audio.getNumSamples();if(n>bufferA.getNumSamples()) {bufferA.setSize(2,n,false,false,true);bufferB.setSize(2,n,false,false,true);} // normal callbacks reuse prepared storage
 if(panic.exchange(false)){a.panicRequested.store(true);b.silence.store(true);owners={};keyboard.reset();alignB={};reverb.reset();compressor.reset();toneLow[0]=toneLow[1]=toneHigh[0]=toneHigh[1]=0;}
 keyboard.processNextMidiBuffer(midi,0,n,true);float pb=bend.exchange(-2),mw=wheel.exchange(-1);if(pb>=-1)midi.addEvent(MidiMessage::pitchWheel(1,jlimit(0,16383,int(pb*8192+8192))),0);if(mw>=0)midi.addEvent(MidiMessage::controllerEvent(1,1,int(mw)),0);
 routeMidi(midi,midiA,midiB);a.setPlayHead(getPlayHead());b.setPlayHead(getPlayHead());
 // Use sub-buffers without changing either child's block length or processing algorithm.
 AudioBuffer<float> outA(bufferA.getArrayOfWritePointers(),2,n),outB(bufferB.getArrayOfWritePointers(),2,n);a.processBlock(outA,midiA);b.processBlock(outB,midiB);audio.clear();
 gainA.setTargetValue(value("a.mute")>.5f?0:mixerGain(value("a.gain")));gainB.setTargetValue(value("b.mute")>.5f?0:mixerGain(value("b.gain")));master.setTargetValue(mixerGain(value("master")));width.setTargetValue(value("width"));fxBlend.setTargetValue(value("glue"));verbWet.setTargetValue(value("verb")>.5f?value("wet"):0);
 compressor.setThreshold(value("threshold"));compressor.setRatio(value("ratio"));compressor.setAttack(value("attack"));compressor.setRelease(value("release"));Reverb::Parameters rv;rv.roomSize=value("room");rv.damping=value("damp");rv.wetLevel=1;rv.dryLevel=0;rv.width=1;reverb.setParameters(rv);
 float pa=0,pbPeak=0,pm=0,lowCoefficient=1-std::exp(-MathConstants<float>::twoPi*200/float(sampleRate)),highCoefficient=1-std::exp(-MathConstants<float>::twoPi*6500/float(sampleRate));float lowGain=Decibels::decibelsToGain(value("low")),airGain=Decibels::decibelsToGain(value("air"));
 const bool eqOn=value("eq")>.5f,compOn=value("comp")>.5f;
 for(int i=0;i<n;++i){float ga=gainA.getNextValue(),gb=gainB.getNextValue();float dry[2],wet[2];for(int c=0;c<2;++c){float sa=outA.getSample(c,i),sb=alignB[size_t(c)][size_t(alignHead)];alignB[size_t(c)][size_t(alignHead)]=outB.getSample(c,i);dry[c]=sa*ga+sb*gb;wet[c]=dry[c];pa=std::max(pa,std::abs(sa*ga));pbPeak=std::max(pbPeak,std::abs(sb*gb));}alignHead=(alignHead+1)%4;
 for(int c=0;c<2;++c){toneLow[c]+=lowCoefficient*(dry[c]-toneLow[c]);toneHigh[c]+=highCoefficient*(dry[c]-toneHigh[c]);if(eqOn)wet[c]+=toneLow[c]*(lowGain-1)+(dry[c]-toneHigh[c])*(airGain-1);}
 const float detector=std::max(std::abs(wet[0]),std::abs(wet[1]));const float compressed=compressor.processSample(0,detector);if(compOn&&detector>1.e-12f){const float reduction=compressed/detector;wet[0]*=reduction;wet[1]*=reduction;}
 float reverbL=wet[0],reverbR=wet[1];reverb.processStereo(&reverbL,&reverbR,1);float vw=verbWet.getNextValue();wet[0]+=vw*(reverbL-wet[0]);wet[1]+=vw*(reverbR-wet[1]);float mid=(wet[0]+wet[1])*.5f,side=(wet[0]-wet[1])*.5f*width.getNextValue();wet[0]=mid+side;wet[1]=mid-side;float blend=fxBlend.getNextValue(),mg=master.getNextValue();for(int c=0;c<2;++c){float result=(dry[c]+blend*(wet[c]-dry[c]))*mg;audio.setSample(c,i,result);pm=std::max(pm,std::abs(result));}}
 peaks[0].store(pa);peaks[1].store(pbPeak);peaks[2].store(pm);midi.clear();
}
void DualProcessor::getStateInformation(MemoryBlock& block){ValueTree tree=state.copyState();MemoryBlock sa,sb;a.getStateInformation(sa);b.getStateInformation(sb);tree.setProperty("engineA",sa.toBase64Encoding(),nullptr);tree.setProperty("engineB",sb.toBase64Encoding(),nullptr);tree.setProperty("panel",panel.load(),nullptr);MemoryOutputStream output(block,false);tree.writeToStream(output);}
void DualProcessor::setStateInformation(const void* data,int length){if(length<=0||length>8*1024*1024)return;auto tree=ValueTree::readFromData(data,size_t(length));if(!tree.hasType("DualHost"))return;MemoryBlock sa,sb;if(!sa.fromBase64Encoding(tree["engineA"].toString())||!sb.fromBase64Encoding(tree["engineB"].toString()))return;a.setStateInformation(sa.getData(),int(sa.getSize()));b.setStateInformation(sb.getData(),int(sb.getSize()));panel.store(jlimit(0,1,int(tree["panel"])));state.replaceState(tree);panic.store(true);}
AudioProcessorEditor* DualProcessor::createEditor(){return new DualEditor(*this);}
AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new DualProcessor;}

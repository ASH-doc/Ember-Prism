// sa-patches.cpp — Validated JSON patches, metadata, CC mappings, A/B, mutate and chord capture.
#include "sa-processor.h"
juce::String SynthAProcessor::patchName()const{const juce::ScopedLock lock(metaLock);return name;}
juce::String SynthAProcessor::macroName(int i)const{const juce::ScopedLock lock(metaLock);return macroNames[i];}
void SynthAProcessor::renameMacro(int i,const juce::String& s){if(i<0||i>3)return;const juce::ScopedLock lock(metaLock);macroNames.set(i,s.substring(0,18));}
juce::var SynthAProcessor::getPatch()const{
 auto* root=new juce::DynamicObject;root->setProperty("format","dualsynth-patch");root->setProperty("version",1);root->setProperty("engine","A");
 {const juce::ScopedLock lock(metaLock);root->setProperty("name",name);root->setProperty("category",category);juce::Array<juce::var> macros;for(auto& m:macroNames)macros.add(m);root->setProperty("macros",macros);}
 auto* params=new juce::DynamicObject;for(auto& p:SynthA::parameters())params->setProperty(juce::Identifier(juce::String(p.id)),double(getParam(p.id)));root->setProperty("params",juce::var(params));
 juce::Array<juce::var> mods;for(int n=1;n<=12;++n){auto id="mod."+juce::String(n)+".";int source=int(getParam(id+"source")),dest=int(getParam(id+"dest"))-1;if(source==0||dest<0)continue;auto* row=new juce::DynamicObject;row->setProperty("src",juce::String(SynthA::modSources()[size_t(source)]));row->setProperty("dst",juce::String(SynthA::parameters()[size_t(dest)].id));row->setProperty("amount",getParam(id+"amount"));mods.add(juce::var(row));}root->setProperty("mod",mods);
 juce::Array<juce::var> fx;for(auto type:{"drive","chorus","flanger","phaser","delay","reverb","eq","pump"}){auto prefix=std::string("fx.")+type+".";auto* obj=new juce::DynamicObject;auto* values=new juce::DynamicObject;obj->setProperty("type",type);obj->setProperty("on",getParam(prefix+"on")>.5f);for(const auto& p:SynthA::parameters())if(p.id.rfind(prefix,0)==0&&p.id!=prefix+"on")values->setProperty(juce::Identifier(juce::String(p.id.substr(prefix.size()))),getParam(p.id));obj->setProperty("params",juce::var(values));fx.add(juce::var(obj));}root->setProperty("fx",fx);
 auto* mappings=new juce::DynamicObject;for(int cc=0;cc<128;++cc){int i=ccMap[size_t(cc)].load();if(i>=0)mappings->setProperty(juce::String(cc),juce::String(SynthA::parameters()[size_t(i)].id));}root->setProperty("midiLearn",juce::var(mappings));
 return juce::var(root);
}
juce::Result SynthAProcessor::setPatch(const juce::var& patch,bool commit){
 if(!patch.isObject()||patch["format"].toString()!="dualsynth-patch"||int(patch["version"])!=1||patch["engine"].toString()!="A")return juce::Result::fail("Not a version 1 Synth A patch.");
 auto* params=patch["params"].getDynamicObject();if(!params)return juce::Result::fail("Patch has no parameter object.");
 SynthA::Values values=SynthA::defaults();const auto& defs=SynthA::parameters();
 for(const auto& property:params->getProperties()){
   const int index=SynthA::indexOf(property.name.toString().toStdString());if(index<0)return juce::Result::fail("Unknown parameter: "+property.name.toString());
   const auto& def=defs[size_t(index)];float value=0;
   if(property.value.isString()&&!def.choices.empty()){int choice=-1;for(size_t i=0;i<def.choices.size();++i)if(juce::String(def.choices[i]).equalsIgnoreCase(property.value.toString()))choice=int(i);if(choice<0)return juce::Result::fail("Unknown choice: "+property.value.toString());value=float(choice);}
   else {if(!(property.value.isDouble()||property.value.isInt()||property.value.isInt64()||property.value.isBool()))return juce::Result::fail("Non-numeric parameter: "+property.name.toString());value=float(property.value);}
   if(!std::isfinite(value)||value<def.min||value>def.max)return juce::Result::fail("Out-of-range parameter: "+property.name.toString());values[size_t(index)]=def.limit(value);
 }
 // Full canonical params take precedence. Sparse legacy patches may carry separate mod / fx records.
 if(!params->hasProperty("mod.1.source"))if(auto* mods=patch["mod"].getArray()){
   if(mods->size()>12)return juce::Result::fail("Only 12 modulation slots are supported.");
   for(int n=0;n<mods->size();++n){const auto& row=mods->getReference(n);int src=-1;auto s=row["src"].toString();for(size_t i=0;i<SynthA::modSources().size();++i)if(juce::String(SynthA::modSources()[i]).removeCharacters(" ").equalsIgnoreCase(s.removeCharacters(" ")))src=int(i);
     int dst=SynthA::indexOf(row["dst"].toString().toStdString());float amount=float(row["amount"]);if(src<0||dst<0||!std::isfinite(amount)||std::abs(amount)>1)return juce::Result::fail("Invalid modulation assignment.");
     auto id="mod."+std::to_string(n+1)+".";values[size_t(SynthA::indexOf(id+"source"))]=float(src);values[size_t(SynthA::indexOf(id+"dest"))]=float(dst+1);values[size_t(SynthA::indexOf(id+"amount"))]=amount;
   }
 }
 if(auto* effects=patch["fx"].getArray())for(auto& effect:*effects){auto prefix="fx."+effect["type"].toString()+".";const int enabled=SynthA::indexOf((prefix+"on").toStdString());if(enabled<0)return juce::Result::fail("Unknown effect.");if(!params->hasProperty(prefix+"on")){values[size_t(enabled)]=bool(effect["on"])?1.f:0.f;if(auto* props=effect["params"].getDynamicObject())for(const auto& prop:props->getProperties()){auto id=prefix+prop.name.toString();int index=SynthA::indexOf(id.toStdString());float value=float(prop.value);if(index<0||!std::isfinite(value)||value<defs[size_t(index)].min||value>defs[size_t(index)].max)return juce::Result::fail("Invalid effect parameter.");values[size_t(index)]=value;}}}
 std::array<int,128> mappings;mappings.fill(-1);if(auto* map=patch["midiLearn"].getDynamicObject())for(auto& pair:map->getProperties()){auto key=pair.name.toString();int cc=key.getIntValue();int index=SynthA::indexOf(pair.value.toString().toStdString());if(key!=juce::String(cc)||cc<0||cc>127||index<0)return juce::Result::fail("Invalid MIDI mapping.");mappings[size_t(cc)]=index;}
 if(!commit)return juce::Result::ok(); // validation only: nothing above touched any state
 panicRequested.store(true);undo.beginNewTransaction("Load patch");
 for(size_t i=0;i<defs.size();++i){auto* parameter=state.getParameter(defs[i].id);parameter->setValueNotifyingHost(parameter->convertTo0to1(values[i]));}
 for(int cc=0;cc<128;++cc)ccMap[size_t(cc)].store(mappings[size_t(cc)]);
 {const juce::ScopedLock lock(metaLock);name=patch["name"].toString().substring(0,80);category=patch["category"].toString();if(auto* macros=patch["macros"].getArray())for(int i=0;i<std::min(4,macros->size());++i)macroNames.set(i,macros->getReference(i).toString().substring(0,18));}
 return juce::Result::ok();
}
void SynthAProcessor::loadPreset(int n){if(n>=0&&n<bank.size())setPatch(bank[n]);}
juce::Result SynthAProcessor::loadPatchFile(const juce::File& f){if(f.getSize()>2*1024*1024)return juce::Result::fail("Patch is larger than 2 MB.");juce::var patch;auto result=juce::JSON::parse(f.loadFileAsString(),patch);return result.failed()?result:setPatch(patch);}
juce::Result SynthAProcessor::savePatchFile(const juce::File& f){auto patch=getPatch();patch.getDynamicObject()->setProperty("name",f.getFileNameWithoutExtension());if(!f.replaceWithText(juce::JSON::toString(patch,true)))return juce::Result::fail("Could not write patch.");{const juce::ScopedLock lock(metaLock);name=f.getFileNameWithoutExtension();}return juce::Result::ok();}
void SynthAProcessor::getStateInformation(juce::MemoryBlock& data){auto text=juce::JSON::toString(getPatch(),true);data.replaceAll(text.toRawUTF8(),size_t(text.getNumBytesAsUTF8()));}
void SynthAProcessor::setStateInformation(const void* data,int size){if(size<=0||size>2*1024*1024)return;juce::var patch;if(juce::JSON::parse(juce::String::fromUTF8(static_cast<const char*>(data),size),patch).wasOk())setPatch(patch);}
void SynthAProcessor::mutate(){juce::Random r;undo.beginNewTransaction("Warm mutation");for(const auto& p:SynthA::parameters()){if(p.discrete||p.id.rfind("mod.",0)==0||p.id.rfind("master.",0)==0||p.id.rfind("clock.",0)==0||p.id.rfind("voice.",0)==0||p.id.rfind("chord.",0)==0)continue;auto* param=state.getParameter(p.id);param->setValueNotifyingHost(SynthA::clamp(param->getValue()+(r.nextFloat()-.5f)*.12f,0.f,1.f));}}
void SynthAProcessor::compare(){auto current=getPatch();if(!comparePatch.isVoid())setPatch(comparePatch);comparePatch=current;}
void SynthAProcessor::learnChord(){std::array<int,4> notes{};int count=0;for(int n=0;n<128&&count<4;++n)if(heldNotes[size_t(n)].load()>0)notes[size_t(count++)]=n;if(!count)return;for(int i=0;i<4;++i)setParam("chord."+juce::String(i+1),float(SynthA::clamp(notes[size_t(i%count)]-notes[0],-24,24)));setParam("chord.on",1);}

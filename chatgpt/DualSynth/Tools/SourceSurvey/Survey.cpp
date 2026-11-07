// Survey.cpp — Aggregate statistics over a local preset library (Spire .spf2, Serum 1 .fxp).
// Reads files in place and prints counts only: no preset content is written anywhere, nothing is copied into the repo.
//   DualSurvey spire <folder> [<folder>...]   per-parameter value statistics over all .spf2 files
//   DualSurvey serum1 <folder>                container statistics over all .fxp files (header checks, inflated sizes)
#include <juce_core/juce_core.h>
#include "Import/SpireDecoder.h"
#include <iostream>
#include <functional>
#include <map>
#include <random>
#include <set>
using namespace juce;
namespace {
struct Histogram{std::map<double,int> values;int other=0;int total=0;void add(double v){++total;auto it=values.find(v);if(it!=values.end())++it->second;else if(values.size()<48)values[v]=1;else ++other;}};
void spire(const StringArray& folders){
 std::map<String,Histogram> parameters;int files=0,unreadable=0,noParameters=0;std::map<String,int> vendors;
 for(const auto& folder:folders)for(const auto& entry:RangedDirectoryIterator(File(folder),true,"*.spf2",File::findFiles)){
  ++files;var parsed;if(JSON::parse(entry.getFile().loadFileAsString(),parsed).failed()){++unreadable;continue;}
  auto* object=parsed.getDynamicObject();auto* values=object!=nullptr?parsed["parameters"].getDynamicObject():nullptr;if(values==nullptr){++noParameters;continue;}
  vendors[parsed["vendor"].toString()]++;
  for(const auto& property:values->getProperties())parameters[property.name.toString()].add(double(property.value));
 }
 std::cout<<"files "<<files<<" unreadable "<<unreadable<<" without parameter block "<<noParameters<<" distinct vendors "<<vendors.size()<<" distinct parameters "<<parameters.size()<<"\n";
 for(const auto& [name,histogram]:parameters){
  std::vector<std::pair<int,double>> ranked;for(const auto& [value,count]:histogram.values)ranked.push_back({count,value});std::sort(ranked.rbegin(),ranked.rend());
  std::cout<<name<<"\tn="<<histogram.total<<"\tdistinct>="<<histogram.values.size()+(histogram.other>0?1:0)<<"\ttop:";for(size_t i=0;i<std::min<size_t>(4,ranked.size());++i)std::cout<<" "<<ranked[i].second<<"x"<<ranked[i].first;std::cout<<"\n";
 }
}
void serum1(const String& folder){
 int files=0,notFxp=0,notSerum=0,inflateFailed=0,ok=0;std::map<int,int> sizes;
 for(const auto& entry:RangedDirectoryIterator(File(folder),true,"*.fxp",File::findFiles)){
  ++files;MemoryBlock data;if(!entry.getFile().loadFileAsData(data)||data.getSize()<64){++notFxp;continue;}const auto* b=static_cast<const unsigned char*>(data.getData());
  if(std::memcmp(b,"CcnK",4)!=0||std::memcmp(b+8,"FPCh",4)!=0){++notFxp;continue;}if(std::memcmp(b+16,"XfsX",4)!=0){++notSerum;continue;}
  const size_t chunk=(size_t(b[56])<<24)|(size_t(b[57])<<16)|(size_t(b[58])<<8)|size_t(b[59]);if(chunk+60>data.getSize()){++inflateFailed;continue;}
  MemoryInputStream compressed(b+60,chunk,false);GZIPDecompressorInputStream inflater(&compressed,false,GZIPDecompressorInputStream::zlibFormat);MemoryOutputStream out;out.writeFromInputStream(inflater,-1);
  if(out.getDataSize()==0){++inflateFailed;continue;}++ok;sizes[int(out.getDataSize())]++;
 }
 std::cout<<"files "<<files<<" not fxp/FPCh "<<notFxp<<" other plugin id "<<notSerum<<" inflate failed "<<inflateFailed<<" readable "<<ok<<" distinct inflated sizes "<<sizes.size()<<"\n";
 std::vector<std::pair<int,int>> ranked;for(const auto& [size,count]:sizes)ranked.push_back({count,size});std::sort(ranked.rbegin(),ranked.rend());
 for(size_t i=0;i<std::min<size_t>(12,ranked.size());++i)std::cout<<"inflated size "<<ranked[i].second<<" : "<<ranked[i].first<<" presets\n";
 int smallest=sizes.empty()?0:sizes.begin()->first,largest=sizes.empty()?0:sizes.rbegin()->first;std::cout<<"smallest "<<smallest<<" largest "<<largest<<"\n";
}
}
// Runs the product decoder over a local library and counts outcomes (parsing coverage only, not playable translation).
void spireDecode(const StringArray& folders){
 int files=0,ok=0,blocked=0,withWarnings=0;std::map<String,int> reasons;
 for(const auto& folder:folders)for(const auto& entry:RangedDirectoryIterator(File(folder),true,"*.spf2",File::findFiles)){
  ++files;MemoryBlock data;if(!entry.getFile().loadFileAsData(data)){++blocked;reasons["unreadable file"]++;continue;}
  const auto result=aether::import::decodeSpire(data.getData(),data.getSize(),entry.getFile().getFileNameWithoutExtension().toStdString());
  if(result.ok){++ok;if(!result.patch.warnings.empty())++withWarnings;}else{++blocked;reasons[String(result.blocker)]++;}
 }
 std::cout<<"files "<<files<<" decoded "<<ok<<" blocked "<<blocked<<" (decoded files with warnings: "<<withWarnings<<", all of them the ignored icon unless listed below)\n";
 for(const auto& [reason,count]:reasons)std::cout<<"blocked: "<<reason<<" x"<<count<<"\n";
}
// Tallies which list indexes the matrix sources and targets and the envelope / LFO targets use (aggregate counts only).
void spireTargets(const StringArray& folders){
 std::map<int,int> sources,targets,envLfoTargets;int files=0,slotsUsed=0;std::map<int,int> routesPerFile;
 auto index=[](const var& v){return int(double(v)*999.0+0.5);};
 for(const auto& folder:folders)for(const auto& entry:RangedDirectoryIterator(File(folder),true,"*.spf2",File::findFiles)){
  var parsed;if(JSON::parse(entry.getFile().loadFileAsString(),parsed).failed())continue;auto* values=parsed["parameters"].getDynamicObject();if(values==nullptr)continue;++files;int routes=0;
  for(int slot=1;slot<=15;++slot){const String prefix="slot"+String(slot)+"_";const int src1=index(values->getProperty(prefix+"Src1")),src2=index(values->getProperty(prefix+"Src2"));if(src1==0&&src2==0)continue;++slotsUsed;
   if(src1>0)sources[src1]++;if(src2>0)sources[src2]++;
   for(int t=1;t<=4;++t){const int target=index(values->getProperty(prefix+"Trg"+String(t)));if(target>0){targets[target]++;++routes;}}}
  routesPerFile[std::min(routes,40)]++;
  for(int e=1;e<=4;++e)for(int t=1;t<=2;++t){const int target=index(values->getProperty("env"+String(e)+"_target"+String(t)));if(target>0)envLfoTargets[target]++;}
  for(int l=1;l<=4;++l)for(int t=1;t<=2;++t){const int target=index(values->getProperty("lfo"+String(l)+"_target"+String(t)));if(target>0)envLfoTargets[1000+target]++;}
 }
 auto top=[&](const char* title,const std::map<int,int>& m,int n){std::vector<std::pair<int,int>> r;for(const auto& [k,c]:m)r.push_back({c,k});std::sort(r.rbegin(),r.rend());std::cout<<title<<" (files "<<files<<")\n";for(size_t i=0;i<std::min<size_t>(size_t(n),r.size());++i)std::cout<<"  index "<<r[i].second<<" : "<<r[i].first<<"\n";};
 top("matrix sources",sources,20);top("matrix targets",targets,40);top("env (index) and lfo (1000+index) targets",envLfoTargets,30);
 std::cout<<"matrix routes per file (target fields set): ";for(const auto& [k,c]:routesPerFile)std::cout<<k<<"x"<<c<<" ";std::cout<<"\nslots with a source: "<<slotsUsed<<"\n";
}
// Draws the tuning and evaluation sets of the Spire import contract from the factory presets (stratified, seeded, disjoint) and
// copies the chosen files into one folder under short names so they can be loaded and rendered one by one. Counts and names only
// leave this machine; the copies stay local.
//   DualSurvey spire-select <factory folder> <output folder> [seed]
void spireSelect(const File& factory,const File& output,uint64_t seed){
 struct Candidate{File file;String category;bool osc4=false,link=false,multi=false;};
 std::vector<Candidate> all;
 for(const auto& entry:RangedDirectoryIterator(factory,true,"*.spf2",File::findFiles)){
  var parsed;if(JSON::parse(entry.getFile().loadFileAsString(),parsed).failed())continue;auto* values=parsed["parameters"].getDynamicObject();if(values==nullptr)continue;
  Candidate c;c.file=entry.getFile();c.category=entry.getFile().getParentDirectory().getFileName();auto v=[&](const char* k){return double(values->getProperty(k));};
  c.osc4=v("mix_osc4")>.001&&v("osc4_mute")<.5;c.link=v("flt_link")>.5;int routes=0;
  for(int slot=1;slot<=15;++slot){const String p="slot"+String(slot)+"_";if(int(double(values->getProperty(p+"Src1"))*999+.5)==0&&int(double(values->getProperty(p+"Src2"))*999+.5)==0)continue;for(int t=1;t<=4;++t)if(int(double(values->getProperty(p+"Trg"+String(t)))*999+.5)>0)++routes;}
  c.multi=routes>=3;all.push_back(c);
 }
 std::mt19937_64 rng(seed);std::vector<size_t> order(all.size());for(size_t i=0;i<order.size();++i)order[i]=i;std::shuffle(order.begin(),order.end(),rng);
 std::set<size_t> used;auto take=[&](auto accept)->int{for(size_t i:order)if(!used.count(i)&&accept(all[i])){used.insert(i);return int(i);}return -1;};
 struct Slot{const char* label;std::function<bool(const Candidate&)> accept;int count;};
 const Slot plan[]={{"bass",[](const Candidate& c){return c.category=="Basses";},5},{"lead",[](const Candidate& c){return c.category=="Leads";},5},{"pluck",[](const Candidate& c){return c.category=="Plucks";},4},{"pad",[](const Candidate& c){return c.category=="Pads"||c.category=="Keys";},2},
  {"hard:osc4",[](const Candidate& c){return c.osc4;},1},{"hard:filter-link",[](const Candidate& c){return c.link;},1},{"hard:multi-target",[](const Candidate& c){return c.multi;},1},{"hard:osc4+link",[](const Candidate& c){return c.osc4&&c.link;},1}};
 output.createDirectory();String listing;
 for(const char* setName:{"T","E"}){int n=0;listing<<"== "<<setName<<(String(setName)=="T"?" (tuning set)":" (evaluation set, never used for mapping)")<<"\n";
  for(const auto& slot:plan)for(int k=0;k<slot.count;++k){const int pick=take(slot.accept);if(pick<0){listing<<"  no candidate for "<<slot.label<<"\n";continue;}
   ++n;const auto& c=all[size_t(pick)];const String name=String(setName)+String(n).paddedLeft('0',2)+"-"+String(slot.label).replaceCharacter(':','-').replaceCharacter('+','-')+".spf2";
   c.file.copyFileTo(output.getChildFile(name));listing<<"  "<<name<<"  <- "<<c.category<<"/"<<c.file.getFileName()<<"  osc4="<<int(c.osc4)<<" link="<<int(c.link)<<" multi="<<int(c.multi)<<"\n";}}
 output.getChildFile("selection.txt").replaceWithText(listing);std::cout<<listing<<"candidates: "<<all.size()<<", seed "<<seed<<"\n";
}
int main(int argc,char** argv){
 if(argc<3){std::cerr<<"usage: DualSurvey spire|serum1 <folder> ...\n";return 2;}
 const String mode(argv[1]);if(mode=="spire"){StringArray folders;for(int i=2;i<argc;++i)folders.add(argv[i]);spire(folders);return 0;}
 if(mode=="spire-select"&&argc>=4){spireSelect(File(argv[2]),File(argv[3]),argc>=5?uint64_t(String(argv[4]).getLargeIntValue()):20261107ull);return 0;}
 if(mode=="spire-targets"){StringArray folders;for(int i=2;i<argc;++i)folders.add(argv[i]);spireTargets(folders);return 0;}
 if(mode=="spire-decode"){StringArray folders;for(int i=2;i<argc;++i)folders.add(argv[i]);spireDecode(folders);return 0;}
 if(mode=="serum1"){serum1(argv[2]);return 0;}return 2;
}

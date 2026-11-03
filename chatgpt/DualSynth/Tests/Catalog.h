#pragma once
#include "Host.h"
// Describes every parameter the plugin publishes to a DAW, one line per parameter:
//   id | name | steps | discrete | boolean | default text | text at 0 | text at 0.5 | text at 1 | every choice text (choice parameters)
// The same function produced Tests/baseline-parameter-catalog.txt from the unmodified ChatGPT baseline (commit f143191).
inline juce::StringArray parameterCatalog(DualProcessor& processor){
 using namespace juce;
 StringArray lines;
 for(auto* parameter:processor.getParameters()){
  auto* withId=dynamic_cast<AudioProcessorParameterWithID*>(parameter);if(withId==nullptr)continue;
  String line=withId->paramID+"|"+withId->name+"|"+String(withId->getNumSteps())+"|"+String(withId->isDiscrete()?1:0)+"|"+String(withId->isBoolean()?1:0)
   +"|"+withId->getText(withId->getDefaultValue(),64)+"|"+withId->getText(0.f,64)+"|"+withId->getText(.5f,64)+"|"+withId->getText(1.f,64);
  if(withId->isDiscrete()&&withId->getNumSteps()>1&&withId->getNumSteps()<=64){
   line+="|";for(int i=0;i<withId->getNumSteps();++i)line+=(i?";":"")+withId->getText(float(i)/float(withId->getNumSteps()-1),64);
  }
  lines.add(line);
 }
 return lines;
}

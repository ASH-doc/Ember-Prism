// sb-vst3-check.cpp — Independent actual-binary host check; no synth implementation linked.
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI gui;if(argc<2){std::cerr<<"Supply the complete VST3 path\n";return 2;}juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> found;format.findAllTypesForFile(found,argv[1]);if(found.isEmpty()){std::cerr<<"VST3 discovery failed\n";return 1;}
 juce::String problem;auto instance=format.createInstanceFromDescription(*found[0],48000,256,problem);if(!instance){std::cerr<<problem<<'\n';return 1;}instance->prepareToPlay(48000,256);juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;double energy=0;
 for(int block=0;block<250;++block){audio.clear();midi.clear();if(block==0)midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);if(block==100)midi.addEvent(juce::MidiMessage::noteOff(1,60),0);instance->processBlock(audio,midi);for(int c=0;c<2;++c)for(int n=0;n<256;++n){float sample=audio.getSample(c,n);if(!std::isfinite(sample)||std::abs(sample)>.65f)return 1;energy+=sample*sample;}}
 if(energy<1.e-6)return 1;juce::MemoryBlock state;instance->getStateInformation(state);instance->setStateInformation(state.getData(),int(state.getSize()));{std::unique_ptr<juce::AudioProcessorEditor> editor(instance->createEditorIfNeeded());if(!editor||!editor->createComponentSnapshot(editor->getLocalBounds()).isValid())return 1;}instance->releaseResources();std::cout<<"PASS: real VST3 discovery, instantiation, MIDI/audio, finite output, state, and editor.\n";return 0;
}

// sa-plugin-probe.cpp — Loads the actual VST3 binary in an independent JUCE host.
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI init;if(argc<2){std::cerr<<"Pass the built .vst3 bundle path.\n";return 2;}
 juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> types;
 format.findAllTypesForFile(types,argv[1]);if(types.isEmpty()){std::cerr<<"VST3 discovery failed\n";return 1;}
 juce::String error;auto plugin=format.createInstanceFromDescription(*types[0],48000,256,error);
 if(!plugin){std::cerr<<error<<'\n';return 1;}plugin->prepareToPlay(48000,256);
 juce::AudioBuffer<float> buffer(2,256);juce::MidiBuffer midi;double energy=0;
 for(int block=0;block<200;++block){buffer.clear();midi.clear();if(block==0)midi.addEvent(juce::MidiMessage::noteOn(1,60,.8f),0);if(block==100)midi.addEvent(juce::MidiMessage::noteOff(1,60),0);plugin->processBlock(buffer,midi);for(int c=0;c<2;++c)for(int n=0;n<256;++n){float x=buffer.getSample(c,n);if(!std::isfinite(x)||std::abs(x)>.65f)return 1;energy+=x*x;}}
 if(energy<1.e-5){std::cerr<<"VST3 produced no sound\n";return 1;}
 juce::MemoryBlock state;plugin->getStateInformation(state);plugin->setStateInformation(state.getData(),int(state.getSize()));
 {std::unique_ptr<juce::AudioProcessorEditor> editor(plugin->createEditorIfNeeded());if(!editor){std::cerr<<"VST3 editor missing\n";return 1;}auto snapshot=editor->createComponentSnapshot(editor->getLocalBounds());if(!snapshot.isValid())return 1;}
 plugin->releaseResources();std::cout<<"PASS: actual VST3 discovery, load, MIDI audio, finite output, host state and editor.\n";return 0;
}

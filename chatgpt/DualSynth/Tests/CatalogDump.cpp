// Writes the parameter catalog of whatever DualProcessor this is linked against.  Usage: DualCatalog <output file>
#include "Catalog.h"
#include <iostream>
int main(int argc,char** argv){
 juce::ScopedJuceInitialiser_GUI gui;
 if(argc<2){std::cerr<<"usage: DualCatalog <output file>\n";return 2;}
 auto processor=std::make_unique<DualProcessor>();
 auto lines=parameterCatalog(*processor);
 if(!juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]).replaceWithText(lines.joinIntoString("\n")+"\n")){std::cerr<<"could not write "<<argv[1]<<"\n";return 1;}
 std::cout<<lines.size()<<" parameters written\n";
 return 0;
}

#include "Options.h"
#include <iostream>

int main(int argc,char* argv[])
{
	if (argc<2) {
		std::cerr<<"USAGE is "<<argv[0]<<" comma,separated,list,of,options\n";
		return 1;
	}
	std::vector<std::string> registerOpts;
	registerOpts.push_back("fast");
	registerOpts.push_back("verbose");
	registerOpts.push_back("hasthreads");
	PsimagLite::OptionsWriteable optWriteable(registerOpts);
	
	std::string myoptions(argv[1]);
	PsimagLite::OptionsWriteable::OptionsReadable optsReadable(optWriteable,myoptions);
}


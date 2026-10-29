#include "tools/XeenDosExport.h"
#include "games/xeen/XeenInstallationDetector.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#include <iostream>

int main() {
	int count=0; auto *raw=CommandLineToArgvW(GetCommandLineW(),&count);
	if(!raw) return 1;
	const std::vector<std::wstring> args(raw,raw+count); LocalFree(raw);
	try {
		const auto usage=[] {throw std::invalid_argument("Usage: mmodern_dos_export <game-dir> <save.mmsave> <out-folder> --slot N --name TEXT [--overwrite]");};
		if(args.size()!=8 && args.size()!=9) usage();
		if(args[4]!=L"--slot" || args[6]!=L"--name" || (args.size()==9 && args[8]!=L"--overwrite")) usage();
		unsigned slot=0;
		if(args[5].empty()) usage();
		for(auto c:args[5]) {if(c<L'0' || c>L'9' || slot>10) usage(); slot=slot*10+c-L'0';}
		std::string name;
		for(auto c:args[7]) {if(c<32 || c>126) throw std::invalid_argument("DOS name must use printable ASCII"); name.push_back(static_cast<char>(c));}
		const auto installation=mmodern::XeenInstallationDetector().detect(std::filesystem::path(args[1]));
		if(!installation) throw std::runtime_error("Game installation not found");
		mmodern::developer::exportDosSave(*installation,args[2],args[3],slot,name,args.size()==9);
		std::cout<<"Exported XEEN/DARK slot "<<slot<<".\n"<<mmodern::developer::dosProjectionSummary();
		return 0;
	} catch(const std::exception &e) {std::cerr<<e.what()<<'\n'; return 1;}
}

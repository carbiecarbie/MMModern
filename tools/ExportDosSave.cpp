#include "tools/XeenDosExport.h"
#include "tools/XeenDosDiff.h"
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
		if(args.size()>=2 && args[1]==L"--diff") {
			if(args.size()!=6 && !(args.size()==7 && args[6]==L"--resave-check"))
				throw std::invalid_argument("Usage: mmodern_dos_export --diff <folderA> <slotA> <folderB> <slotB> [--resave-check]");
			auto slot=[](const std::wstring &s) {std::string r; for(auto c:s) {if(c>127) throw std::invalid_argument("Invalid slot"); r.push_back(char(c));} return r;};
			const auto a=mmodern::developer::readDosPair(args[2],slot(args[3]));
			const auto b=mmodern::developer::readDosPair(args[4],slot(args[5]));
			return mmodern::developer::diffDosPairs(a,b,args.size()==7,std::cout)?0:1;
		}
		const auto usage=[] {throw std::invalid_argument("Usage: mmodern_dos_export <game-dir> <save.mmsave> <out-folder> --slot N --name TEXT [--overwrite] [--place clouds|darkside map x y north|east|south|west] [--dosbox exe [--cfg file]]");};
		if(args.size()<8) usage();
		if(args[4]!=L"--slot" || args[6]!=L"--name") usage();
		unsigned slot=0;
		if(args[5].empty()) usage();
		for(auto c:args[5]) {if(c<L'0' || c>L'9' || slot>10) usage(); slot=slot*10+c-L'0';}
		std::string name;
		for(auto c:args[7]) {if(c<32 || c>126) throw std::invalid_argument("DOS name must use printable ASCII"); name.push_back(static_cast<char>(c));}
		bool overwrite=false;std::optional<mmodern::developer::DosPlace> place;
		std::optional<std::filesystem::path> dosbox,cfg;
		const auto number=[&](const std::wstring &s) {unsigned n=0;if(s.empty())usage();for(auto c:s) {if(c<L'0'||c>L'9'||n>255)usage();n=n*10+c-L'0';}return n;};
		for(std::size_t i=8;i<args.size();) {
			if(args[i]==L"--overwrite" && !overwrite) {overwrite=true;++i;continue;}
			if(args[i]==L"--dosbox" && !dosbox && i+1<args.size()) {dosbox=args[i+1];i+=2;continue;}
			if(args[i]==L"--cfg" && !cfg && i+1<args.size()) {cfg=args[i+1];i+=2;continue;}
			if(args[i]==L"--place" && !place && i+5<args.size()) {
				const auto side=args[i+1]==L"clouds"?mmodern::XeenSide::Clouds:args[i+1]==L"darkside"?mmodern::XeenSide::Darkside:throw std::invalid_argument("Invalid destination side");
				const auto &d=args[i+5];
				const auto facing=d==L"north"?mmodern::XeenDirection::North:d==L"east"?mmodern::XeenDirection::East:d==L"south"?mmodern::XeenDirection::South:d==L"west"?mmodern::XeenDirection::West:throw std::invalid_argument("Invalid destination facing");
				place=mmodern::developer::DosPlace{side,number(args[i+2]),number(args[i+3]),number(args[i+4]),facing};i+=6;continue;
			}
			usage();
		}
		if(cfg && !dosbox)usage();
		const auto installation=mmodern::XeenInstallationDetector({},cfg?*cfg:std::filesystem::path{}).detect(std::filesystem::path(args[1]));
		if(!installation) throw std::runtime_error("Game installation not found");
		if(dosbox)mmodern::developer::exportDosBoxSave(*installation,args[2],args[3],slot,name,overwrite,place,*dosbox,cfg);
		else mmodern::developer::exportDosSave(*installation,args[2],args[3],slot,name,overwrite,place);
		std::cout<<"Exported XEEN/DARK slot "<<slot<<".\n"<<mmodern::developer::dosProjectionSummary();
		return 0;
	} catch(const std::exception &e) {std::cerr<<e.what()<<'\n'; return 1;}
}

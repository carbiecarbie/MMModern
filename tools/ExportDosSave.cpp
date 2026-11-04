#include "core/PathText.h"
#include "tools/XeenDosExport.h"
#include "tools/XeenDosDiff.h"
#include "games/xeen/XeenInstallationDetector.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif
#include <iostream>
#ifdef _WIN32
#define NATIVE(x) L##x
#else
#define NATIVE(x) x
#endif

int main(int argc,char **argv) {
#ifdef _WIN32
	int count=0; auto *raw=CommandLineToArgvW(GetCommandLineW(),&count);
	if(!raw) return 1;
	const std::vector<std::wstring> args(raw,raw+count); LocalFree(raw);
#else
    const std::vector<std::string> args(argv,argv+argc);
#endif

	try {
		if(args.size()>=2 && args[1]==NATIVE("--diff")) {
			if(args.size()!=6 && !(args.size()==7 && args[6]==NATIVE("--resave-check")))
				throw std::invalid_argument("Usage: mmodern_dos_export --diff <folderA> <slotA> <folderB> <slotB> [--resave-check]");
			auto slot=[](const decltype(args)::value_type &s) {std::string r; for(auto c:s) {if(c>127 || static_cast<unsigned char>(c)>127) throw std::invalid_argument("Invalid slot"); r.push_back(char(c));} return r;};
			const auto a=mmodern::developer::readDosPair(args[2],slot(args[3]));
			const auto b=mmodern::developer::readDosPair(args[4],slot(args[5]));
			return mmodern::developer::diffDosPairs(a,b,args.size()==7,std::cout)?0:1;
		}
		const auto usage=[] {throw std::invalid_argument("Usage: mmodern_dos_export <game-dir> <save.mmsave> <out-folder> --slot N --name TEXT [--overwrite] [--place clouds|darkside map x y north|east|south|west] [--dosbox exe [--cfg file]]");};
		if(args.size()<8) usage();
		if(args[4]!=NATIVE("--slot") || args[6]!=NATIVE("--name")) usage();
		unsigned slot=0;
		if(args[5].empty()) usage();
		for(auto c:args[5]) {if(c<NATIVE('0') || c>NATIVE('9') || slot>10) usage(); slot=slot*10+c-NATIVE('0');}
		std::string name;
		for(auto c:args[7]) {if(c<32 || c>126) throw std::invalid_argument("DOS name must use printable ASCII"); name.push_back(static_cast<char>(c));}
		bool overwrite=false;std::optional<mmodern::developer::DosPlace> place;
		std::optional<std::filesystem::path> dosbox,cfg;
		const auto number=[&](const decltype(args)::value_type &s) {unsigned n=0;if(s.empty())usage();for(auto c:s) {if(c<NATIVE('0')||c>NATIVE('9')||n>255)usage();n=n*10+c-NATIVE('0');}return n;};
		for(std::size_t i=8;i<args.size();) {
			if(args[i]==NATIVE("--overwrite") && !overwrite) {overwrite=true;++i;continue;}
			if(args[i]==NATIVE("--dosbox") && !dosbox && i+1<args.size()) {dosbox=args[i+1];i+=2;continue;}
			if(args[i]==NATIVE("--cfg") && !cfg && i+1<args.size()) {cfg=args[i+1];i+=2;continue;}
			if(args[i]==NATIVE("--place") && !place && i+5<args.size()) {
				const auto side=args[i+1]==NATIVE("clouds")?mmodern::XeenSide::Clouds:args[i+1]==NATIVE("darkside")?mmodern::XeenSide::Darkside:throw std::invalid_argument("Invalid destination side");
				const auto &d=args[i+5];
				const auto facing=d==NATIVE("north")?mmodern::XeenDirection::North:d==NATIVE("east")?mmodern::XeenDirection::East:d==NATIVE("south")?mmodern::XeenDirection::South:d==NATIVE("west")?mmodern::XeenDirection::West:throw std::invalid_argument("Invalid destination facing");
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
	} catch(const std::exception &e) {std::cerr<<mmodern::xeenDiagnosticText(e.what())<<'\n'; return 1;}
}

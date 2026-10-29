#include "XeenDosExportTestSupport.h"
#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "TestTreeRemoval.h"
#include "tools/XeenTestSave.h"
#include "formats/xeen/XeenSaveFormat.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
namespace {
using namespace mmodern;
using namespace mmodern::developer;
namespace fs=std::filesystem;
using dos_export_test::check;
using dos_export_test::bytes;
template<class F> void rejects(F f,const char *fragment) {
	try {f();}catch(const std::exception &e) {check(std::string(e.what()).find(fragment)!=std::string::npos,std::string("Wrong refusal: ")+e.what());return;}
	throw std::runtime_error(std::string("Expected refusal: ")+fragment);
}
DosBytes read(const fs::path &p) {std::ifstream f(p,std::ios::binary);check(bool(f),"Read test output"); return {std::istreambuf_iterator<char>(f),{}};}
void raw(const fs::path &p,const DosBytes &b) {std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char *>(b.data()),b.size());check(bool(f),"Write test input");}
void units() {
	const DosMembers members{{"first.bin",dosMemberId("first.bin"),{0,0xff,0x35}},{"second.bin",dosMemberId("second.bin"),{1,2,3,4}}};
	const auto wire=writeDosArchive(members); const auto decoded=readDosArchive(wire);
	check(decoded.size()==2 && decoded[0].bytes==members[0].bytes && decoded[1].bytes==members[1].bytes,"Synthetic archive round trip");
	check(writeDosArchive(decoded)==wire,"Synthetic index/offset determinism");
	check(dosMemberId("mazex004.dat")==dosMemberId("mazex100.dat"),"Known hash alias");
	DosBytes dat(892);dat[768]=100;
	DosMembers alias{{{},dosMemberId("mazex004.dat"),dat}};nameDosMembers(alias);
	check(alias[0].name=="mazex100.dat","DAT embedded ID resolves alias");
	for(const auto &name:std::vector<std::string>{"", " bad",std::string(16,'a'),"bad\n",std::string(1,char(127)),std::string(1,char(0x80))})
		rejects([&]{dosExportNam(name);},"name");
	for(const auto &name:{std::string("A"),std::string(15,'X'),std::string("MMModern Export")}) {
		const auto nam=dosExportNam(name);check(nam.size()==31 && nam[29]==0 && nam[30]==1,"NAM side and native marker");
		check(std::equal(name.begin(),name.end(),nam.begin()) && std::all_of(nam.begin()+name.size(),nam.begin()+30,[](auto b){return b==0;}),"NAM padding follows passing native pair");
	}
	XeenSaveSnapshot s;s.journey.emplace();s.journey->context.emplace();s.journey->purse.emplace();s.journey->serviceEconomy.emplace();s.journey->worldFlags.emplace();
	s.activeRosterIds={4};s.camera={28,18,4,XeenDirection::West};s.gameFlags[0]=s.gameFlags[7]=s.gameFlags[8]=true;
	s.questFlags[29]=true;s.journey->worldFlags->values[127]=true;
	s.journey->serviceEconomy->bank.gold=0x12345678;s.journey->serviceEconomy->bank.gems=0x87654321;
	DosBytes pty(1530,0);pty[742]=3;pty[743]=0x6a;XeenCharacterFormat::writeDosParty(pty,s);
	check(pty[659]==0x81 && pty[660]==0x80 && pty[742]==7 && pty[743]==0x6a && pty[738]==1,"DOS MSB flag order and mixed quest byte");
	check(XeenCharacterFormat::parseBankBalances(DosBytes(pty.begin(),pty.begin()+812))==s.journey->serviceEconomy->bank,"Distinct nonzero bank gold and gems");
	XeenSaveMapState state;state.id=100;state.explored.emplace();(*state.explored)[0]=1;(*state.explored)[31]=0x80;state.tavernTips=7;
	state.cells.push_back({255,0x1234,0xab,0,0});dat[828]=0x69;
	XeenMapFormat::writeDosDat(dat,state);
	check(dat[860]==0x80 && dat[891]==1 && dat[828]==0x69 && dat[827]==7 && dat[510]==0x34 && dat[511]==0x12 && dat[767]==0xab,"DAT physical cell, tip, seen preservation and reversal");
	DosBytes mob(48,0xff);for(auto v:{1,2,0,3,255,255,255,255,4,5,0,2,255,255,255,255,6,7,0,1,255,255,255,255})mob.push_back(v);
	state.objects={{0,-128,-128,true,4}};state.wallItems={{0,10,11}};
	state.monsters=XeenSaveMonsterTable{1,{{-128,-128,XeenActorLifecycle::Defeated},{20,21,XeenActorLifecycle::Present}}};
	const auto oldSize=mob.size();XeenMapFormat::writeDosMob(mob,state);
	const auto entities=XeenMapFormat::parseMob(mob);
	check(mob.size()==oldSize && entities.objects[0].tableIndex==4 && entities.objects[0].x==-128 && entities.wallItems[0].x==10 && entities.monsters[0].x==-128,"MOB original count, open picture, removed monster, wall item; extra Spawn omitted");
	DosBytes evt{6,1,2,4,0,0x12,99};state.events={{0,0}};XeenEventFormat::writeDos(evt,state);
	check(evt==DosBytes({6,1,2,4,0,0,99}),"EVT writes opcode only");
	std::cout<<"Synthetic archive, aliases, NAM, flags, DAT/MOB/EVT passed\n";
}
XeenSaveSnapshot newGame(training_test::Inputs &in) {
	auto party=XeenPartyLoader().loadFromResources(in.chr,in.pty);auto camera=XeenCharacterFormat::parsePartyLocation(in.pty);XeenGameFlags flags;
	XeenWorld world(in.mapLoader(),in.objectLoader());
	XeenJourneySetup setup{in.chr,XeenGameplayContextFormat::parse(in.pty),in.statistics,in.city,7,in.regional()};
	setup.prepared=false;setup.purse=XeenCharacterFormat::parseMonsterPurse(in.pty);
	setup.bank=XeenCharacterFormat::parseBankBalances(in.pty);setup.worldFlags=XeenQuestFlagFormat::parseWorldFlags(in.pty);
	setup.regionalText=in.texts.load(23);setup.learnedNames=in.names;setup.learnedNamesProvider=[&in]{return in.names;};
	setup.cityEventsProvider=[&in]{return in.city;};setup.mainlandEventsProvider=[&in]{return in.mainland;};
	XeenEncounterFlow flow(in.programState,world,party,camera,flags,[]{return 0;},setup);
	check(flow.prepareJourneyFrame(flow.ticket(),[]{}) && flow.presentJourney(flow.ticket()),"New Game boundary");
	return XeenSaveState::capture(in.signature,party,camera,flags,world);
}
void list(const DosMembers &members,bool dark) {
	std::vector<std::string> expected{"maze.nam","maze.chr","maze.pty"};
	if(dark) {
		for(unsigned i=1;i<=129;++i)for(const char *ext:{"dat","evt","mob"})expected.push_back(dosMapName(i,ext));
		for(unsigned i=130;i<=183;++i)expected.push_back(dosMapName(i,"dat"));
	} else {
		for(const char *ext:{"mob","evt","dat"})for(unsigned i=1;i<=85;++i)expected.push_back(dosMapName(i,ext));
		for(unsigned i=100;i<=142;++i)expected.push_back(dosMapName(i,"dat"));
	}
	check(members.size()==expected.size(),"Canonical member count");
	for(unsigned i=0;i<members.size();++i)check(members[i].name==expected[i],"DOS member order at "+std::to_string(i)+": "+members[i].name+" vs "+expected[i]);
}
void newGameEquality(const DosExportPair &out,const DosExportPair &base,const XeenSaveSnapshot &s) {
	dos_export_test::compare(out,base,s);
	for(unsigned side=0;side<2;++side)for(const auto &m:side?base.darkside:base.clouds) {
		const auto &now=bytes(side?out.darkside:out.clouds,m.name);
		for(unsigned i=0;i<now.size();++i)if(now[i]!=m.bytes[i]) {
			// Every exception below is parsed and checked against s by compare().
			const bool exception=m.name=="maze.nam" ||
				// The live party is initialized from Clouds, including all thirty CHRs.
				(side && m.name=="maze.chr" && now[i]==bytes(base.clouds,"maze.chr")[i]) ||
				(side && m.name=="maze.pty" && i<782 && now[i]==bytes(base.clouds,"maze.pty")[i]) ||
				(side && m.name=="maze.pty" && ((i>=28 && i<604) || (i>=832 && i<1408))) ||
				(!side && m.name==dosMapName(109,"dat") && i>=860 && i<892);
			check(exception,"Unlisted New Game exception: "+std::to_string(side)+"/"+m.name+":"+std::to_string(i)+" "+std::to_string(m.bytes[i])+" -> "+std::to_string(now[i]));
		}
	}
	std::cout<<"New Game exceptions: NAM export metadata; live Clouds New Game roster/party instead of DARK's defaults; generated merchant wares on both sides; initial explored cell. Every byte checked against its source and every modeled value parsed back.\n";
}
struct Junction {
	fs::path path;
	Junction(fs::path p,const fs::path &target):path(std::move(p)) {
		const auto cmd=L"cmd.exe /d /c mklink /J \""+path.wstring()+L"\" \""+target.wstring()+L"\" >NUL";
		check(_wsystem(cmd.c_str())==0,"Create real junction for path guard");
	}
	~Junction(){RemoveDirectoryW(path.c_str());}
};
void original(const GameInstallation &installation) {
	const auto dir=fs::temp_directory_path()/("mmodern-m97-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
	fs::create_directory(dir);
	training_test::Inputs in(installation);auto s=newGame(in);const auto base=dosNewGame(installation);
	list(base.clouds,false);list(base.darkside,true);
	const auto out=projectDosSave(installation,s,"MMModern Export");
	newGameEquality(out,base,s);
	check(bytes(out.clouds,"maze.nam")==bytes(out.darkside,"maze.nam") && bytes(out.darkside,"maze.nam")[30]==1,"Pair marker 1/1");
	const auto second=projectDosSave(installation,s,"MMModern Export");
	check(writeDosArchive(out.clouds)==writeDosArchive(second.clouds) && writeDosArchive(out.darkside)==writeDosArchive(second.darkside),"Projection determinism");
	const auto input=dir/"input.mmsave";raw(input,XeenSaveFormat::encode(s));
	const auto absent=dir/"absent";
	for(unsigned slot:{0u,11u,999u})rejects([&]{exportDosSave(installation,input,absent,slot,"Good");},"1..10");
	rejects([&]{exportDosSave(installation,input,absent,1," bad");},"name");
	auto bad=s;bad.camera.mapId.side=XeenSide::Darkside;rejects([&]{projectDosSave(installation,bad,"Good");},"Darkside");
	bad=s;bad.insideInn=true;rejects([&]{projectDosSave(installation,bad,"Good");},"Inn");
	bad=s;bad.camera.mapId.number=0;rejects([&]{projectDosSave(installation,bad,"Good");},"Inn");
	bad=s;++bad.resources.clouds.crc32;raw(input,XeenSaveFormat::encode(bad));
	rejects([&]{exportDosSave(installation,input,absent,1,"Good");},"archive");
	bad=s;bad.camera={86,1,1,XeenDirection::North};raw(input,XeenSaveFormat::encode(bad));
	rejects([&]{exportDosSave(installation,input,absent,1,"Good");},"");
	bad=s;bad.characters[29].intellect.permanent=256;raw(input,XeenSaveFormat::encode(bad));
	rejects([&]{exportDosSave(installation,input,absent,1,"Good");},"CHR byte overflow");
	bad=s;bad.questItems[0]=256;raw(input,XeenSaveFormat::encode(bad));
	rejects([&]{exportDosSave(installation,input,absent,1,"Good");},"quest item count");
	bad=s;bad.characters[29].awards[73]=1;raw(input,XeenSaveFormat::encode(bad));
	rejects([&]{exportDosSave(installation,input,absent,1,"Good");},"award slot 73");
	auto old=XeenSaveFormat::encode(s);old[8]=21;raw(input,old);
	rejects([&]{exportDosSave(installation,input,absent,1,"Good");},"older");
	check(!fs::exists(absent),"Refusal created partial output");raw(input,XeenSaveFormat::encode(s));
	for(const auto &p:{installation.root,installation.root/"cloud_saves"/"m97-must-not-exist"})
		rejects([&]{exportDosSave(installation,input,p,1,"Good");},"installation");
	{Junction alias(dir/"game-link",installation.root);
		rejects([&]{exportDosSave(installation,input,alias.path/"m97-must-not-exist",1,"Good");},"installation");}
	std::wstring lower=installation.root.wstring();std::transform(lower.begin(),lower.end(),lower.begin(),::towlower);
	rejects([&]{exportDosSave(installation,input,fs::path(lower)/"m97-must-not-exist",1,"Good");},"installation");
	wchar_t shortName[32768];const DWORD shortLength=GetShortPathNameW(installation.root.c_str(),shortName,32768);
	check(shortLength && shortLength<32768,"Resolve short installation path");
	rejects([&]{exportDosSave(installation,input,fs::path(shortName)/"m97-must-not-exist",1,"Good");},"installation");
	std::cout<<"Input and resolved-path refusals passed\n";
	const auto target=dir/"pair";fs::create_directory(target);raw(target/"DARK01.WOX",{7,8,9});
	rejects([&]{exportDosSave(installation,input,target,1,"Good");},"exists");
	check(!fs::exists(target/"XEEN01.WOX") && read(target/"DARK01.WOX")==DosBytes({7,8,9}),"Existing one-sided pair unchanged");
	exportDosSave(installation,input,target,1,"Good",true);
	const auto firstX=read(target/"XEEN01.WOX"),firstD=read(target/"DARK01.WOX");
	exportDosSave(installation,input,target,1,"Good",true);
	check(firstX==read(target/"XEEN01.WOX") && firstD==read(target/"DARK01.WOX"),"CLI-support determinism and overwrite");
	const HANDLE locked=CreateFileW((target/"DARK01.WOX").c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
	check(locked!=INVALID_HANDLE_VALUE,"Lock second target for pair rollback");
	try {rejects([&]{exportDosSave(installation,input,target,1,"Good",true);},"");}
	catch(...) {CloseHandle(locked);throw;}
	CloseHandle(locked);
	check(firstX==read(target/"XEEN01.WOX") && firstD==read(target/"DARK01.WOX"),"Failed second backup rolls back the first without changing either slot file");
	fs::create_directory(target/"XEEN02.WOX");
	rejects([&]{exportDosSave(installation,input,target,2,"Good",true);},"ordinary file");
	check(!fs::exists(target/"DARK02.WOX"),"Non-file target left a partial pair");
	rejects([&]{exportDosSave(installation,input,target,1,"Good");},"exists");
	generateTestSave(installation,"m96-chest",dir/"chest.mmsave");const auto chest=XeenSaveFile::read(dir/"chest.mmsave");
	dos_export_test::compare(projectDosSave(installation,chest,"Chest"),base,chest);
	check(!chest.maps.empty(),"Chest preset has no overlays");
	std::cout<<"Original New Game, canonical DOS order, DARK reader, chest, restore/path refusals, atomic pair and determinism passed\n";
	check(removeTestTree(dir),"M97 test working tree cleanup");
}
}
int main(int argc,char **argv) try {
	units();
	if(argc==2) {const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Installation absent");original(*installation);}
	return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}

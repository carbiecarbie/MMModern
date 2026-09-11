#include "XeenEncounterTestSupport.h"
#include "SyntheticXeenArchive.h"
#include "formats/xeen/XeenAssetSource.h"
#include <chrono>
#include <iostream>

using namespace encounter_test;
namespace fs=std::filesystem;
int main() {
	try {
		for(auto n:{0,1,59,61,119,65535,65536}) rejects([&]{XeenMonsterFormat::parse(Bytes(n));});
		check(XeenMonsterFormat::parse(Bytes(65520)).size()==1092,"maximum complete record bound");
		Bytes bytes(120); for(unsigned i=0;i<120;++i)bytes[i]=static_cast<std::uint8_t>(i);
		auto records=XeenMonsterFormat::parse(bytes);
		check(records.size()==2 && records[1].raw[0]==60 && records[1].raw[59]==119,"record boundary/EOF drift");
		check(records[0].experience()==0x13121110 && records[0].baseHp()==0x1514 &&
			records[0].strikes()==0x1b1a && records[0].gold()==0x2b2a,"little endian fields");
		records[0].raw.fill('Z'); check(records[0].name()==std::string(15,'Z'),"bounded name");
		records[0].raw[2]=0; check(records[0].name()=="ZZ","NUL bounded name");
		check(!records[0].supportsApproach(),"unsupported metadata normalized");
		// Compact table holes: physical record 2 -> local slot 1 -> type 8 -> image 42.
		Bytes mobBytes(48,255);mobBytes[16]=3;mobBytes[19]=8;
		Bytes tail{255,255,255,255,255,255,255,255,
			128,255,0,7, 4,5,3,2, 13,2,1,0, 255,255,255,255, 255,255,255,255};
		mobBytes.insert(mobBytes.end(),tail.begin(),tail.end());
		XeenObjectFile file{20,"synthetic",true,XeenMapFormat::parseMob(mobBytes)};
		auto actors=XeenActorApproach::actorsFromResources(file,stats());
		check(actors.size()==3 && actors[0].x==-128 && actors[0].y==-1 &&
			actors[1].lifecycle==XeenActorLifecycle::Unresolved && actors[2].id.recordIndex==2 &&
			actors[2].original.tableIndex==1 && actors[2].original.resourceId==8 && actors[2].statistics->image()==42,
			"physical/local/type/image identity drift");
		file.entities.monsters.resize(108);rejects([&]{XeenActorApproach::actorsFromResources(file,stats());});
		check(file.entities.monsters.size()==108,"capacity truncated immutable records");
		Bytes largeMob(48,255);largeMob[16]=8;
		largeMob.insert(largeMob.end(),{255,255,255,255,255,255,255,255});
		for(int i=0;i<108;++i)largeMob.insert(largeMob.end(),{13,2,0,0});
		largeMob.insert(largeMob.end(),{255,255,255,255,255,255,255,255});
		check(XeenMapFormat::parseMob(largeMob).monsters.size()==108,"structural MOB parser imposed simulation capacity");
		for(std::size_t n:{0,27,610,617,658})rejects([&]{XeenGameplayContextFormat::parse(Bytes(n));});
		auto b=pty();auto c=XeenGameplayContextFormat::parse(b);
		check(c.day==1 && c.year==610 && c.minutes==480 && c.ctr24==0 && !c.newDay,"PTY offsets");
		b[620]=0x34;b[621]=0x12;b[630]=0x78;b[631]=0x56;b[18]=7;
		c=XeenGameplayContextFormat::parse(b);
		check(c.effects[0]==7 && c.lightAndResistances[0]==0x1234 && c.lightAndResistances[5]==0x5678,
			"PTY context fields normalized or skipped incorrectly");
		for(auto offset:{27,658,610}) {b=pty();b[offset]=255;rejects([&]{XeenGameplayContextFormat::parse(b);});}
		check(!party().encounterContext,"ordinary party load acquired encounter context");
		const auto dir=fs::temp_directory_path()/("mmodern-monster-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		fs::create_directories(dir);
		struct Cleanup {fs::path dir;~Cleanup(){std::error_code e;fs::remove_all(dir,e);}} cleanup{dir};
		GameInstallation installation;installation.root=dir;installation.xeenArchive=dir/"xeen.cc";
		installation.darkArchive=dir/"dark.cc";installation.edition=GameEdition::WorldOfXeen;
		sprite_test::archive(installation.xeenArchive,{{"dummy",{1}}});
		sprite_test::archive(installation.darkArchive,{{"xeen.mon",bytes},{"mae.xen",{1,0}}});
		{XeenAssetSource a(installation);check(a.readCloudsMonsterStatisticsFromDarkArchive()==std::optional<Bytes>(bytes),"checked exact statistics read");
		 check(a.readItemMaterialNamesFromDarkArchive()==std::optional<Bytes>(Bytes{1,0}),"material read changed");}
		sprite_test::archive(installation.darkArchive,{{"xeen.mon",{1}}});
		{XeenAssetSource a(installation);auto data=a.readCloudsMonsterStatisticsFromDarkArchive();
		 check(bool(data),"malformed member reported missing");rejects([&]{XeenMonsterFormat::parse(*data);},"malformed");}
		sprite_test::archive(installation.darkArchive,{{"xeen.mon",bytes}});
		fs::resize_file(installation.darkArchive,fs::file_size(installation.darkArchive)-1);
		{XeenAssetSource a(installation);rejects([&]{a.readCloudsMonsterStatisticsFromDarkArchive();},"truncated");}
		// Valid decoded index with a malicious payload offset into the index itself.
		sprite_test::archive(installation.darkArchive,{{"xeen.mon",bytes}});
		{
			std::fstream stream(installation.darkArchive,std::ios::binary|std::ios::in|std::ios::out);
			for(unsigned i=2;i<=4;++i) {
				const unsigned b=(0-(0xac+i*0x67))&255;
				const char encoded=static_cast<char>((b>>2)|(b<<6));
				stream.seekp(2+i);stream.write(&encoded,1);
			}
		}
		{XeenAssetSource a(installation);rejects([&]{a.readCloudsMonsterStatisticsFromDarkArchive();},"overlaps");}
		sprite_test::archive(installation.darkArchive,{{"other",{1}}});
		{XeenAssetSource a(installation);check(!a.readCloudsMonsterStatisticsFromDarkArchive(),"missing member not distinct");}
		installation.darkArchive.clear();installation.edition=GameEdition::CloudsOfXeen;
		{XeenAssetSource a(installation);check(!a.readCloudsMonsterStatisticsFromDarkArchive(),"missing Dark archive not distinct");}
		std::cout<<"Monster/context parsing and checked asset boundaries passed\n";
		return 0;
	} catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

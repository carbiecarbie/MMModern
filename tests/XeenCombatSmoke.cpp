// Optional commercial-resource checks, never registered as ordinary CTest.
#include "XeenCombatTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include <iostream>
using namespace combat_test;
int main(int argc,char **argv) {try {
	if(argc!=2)throw std::invalid_argument("usage: mmodern_combat_smoke <original-installation>");
	const auto installation=XeenInstallationDetector().detect(argv[1]);check(installation&&installation->hasDarkside(),"World of Xeen required");
	XeenAssetSource assets(*installation);XeenMapLoader maps;
	XeenEventLoader eventsLoader([&](const std::string &name)->std::optional<Bytes>{if(!assets.hasInitialResource(name))return {};return assets.readInitialResource(name);});
	const auto chrBytes=assets.readInitialResource("maze.chr"),ptyBytes=assets.readInitialResource("maze.pty");
	const auto mon=assets.readCloudsMonsterStatisticsFromDarkArchive();check(bool(mon),"monster statistics missing");
	const auto stats=XeenMonsterFormat::parse(*mon);const auto evt=eventsLoader.load(20);
	for(unsigned trace=0;trace<5;++trace) {
		auto party=XeenPartyLoader().loadInitialCloudsParty(assets);const auto initial=party;
		check(!party.roster.combatMarked(),"ordinary initial loading attached combat inputs");
		XeenWorld world([&](XeenMapIdentity id){return maps.loadGeometryMap(assets,id);},[&](XeenMapIdentity id){return maps.loadObjects(assets,id);});
		auto camera=XeenActorApproach::kEntry;XeenCombatBoundary boundary(world,party,camera);
		XeenCombat fight(world,party,camera,boundary,chrBytes,XeenGameplayContextFormat::parse(ptyBytes),stats,evt,
			trace==0?XeenCombatRandom(mixedTape()):trace==1?XeenCombatRandom(defeatTape()):XeenCombatRandom(trace==3?19:1));
		if(trace==3)for(unsigned i=0;i<6;++i)for(unsigned slot=0;slot<9;++slot)if(party.roster.at(kXeenCombatOwners[i]).armor[slot].id)
			check(fight.equipment(fight.ticket(),i,XeenInventoryCategory::Armor,slot,XeenEquipmentOperation::Remove).status==XeenEquipmentStatus::Success,"original armor removal");
		if(trace==4) {
			check(fight.transfer(fight.ticket(),5,0,XeenInventoryCategory::Accessories,1).status==XeenTransferStatus::Success,"original ring transfer");
			check(fight.equipment(fight.ticket(),0,XeenInventoryCategory::Accessories,1,XeenEquipmentOperation::Equip).status==XeenEquipmentStatus::Success,"original transferred ring equip");
		}
		check(fight.beginApproach(fight.ticket()).status==Status::Accepted,"original begin");
		if(trace==4){fight.approachAction(fight.ticket(),XeenEncounterAction::Right);fight.approachPulse(fight.ticket());fight.approachAction(fight.ticket(),XeenEncounterAction::Forward);fight.approachPulse(fight.ticket());fight.approachPulse(fight.ticket());fight.approachPulse(fight.ticket());}
		fight.approachAction(fight.ticket(),XeenEncounterAction::Wait);check(fight.phase()==Phase::Engaged,"original engagement");
		check(party.encounterContext->minutes==(trace==4?500:490),"original entry minutes");
		const auto actors=world.sessionState().actors();check(actors.size()==27,"all original actors");fight.beginCombat(fight.ticket());
		unsigned commands=0,iterations=0;
		while(fight.phase()!=Phase::Victory&&fight.phase()!=Phase::Defeat&&iterations++<300) {
			check(fight.phase()!=Phase::Failed&&fight.phase()!=Phase::SupportStopped,"original fight stopped");
			if(fight.phase()==Phase::PlayerReady) {
				Command action=Command::Block;
				if(trace==0)action=(commands==0||commands==2||commands==3||commands>=6)?Command::Attack:Command::Block;
				if(trace==2||trace==4)action=commands<6?Command::Block:Command::Attack;
				fight.command(fight.ticket(),action);++commands;
			}else fight.service(fight.ticket());
		}
		const bool victory=trace==0||trace==2||trace==4;
		check(fight.phase()==(victory?Phase::Victory:Phase::Defeat),"original full outcome");
		const unsigned minutes[]{492,495,493,500,503};
		check(party.encounterContext->minutes==minutes[trace],"original literal outcome minute including delayed Victory503");
		if(trace==3)check(commands==35,"original seed19 Block count");
		for(unsigned i=0;i<27;++i)if(i!=5)sameActors({actors[i]},{world.sessionState().actors()[i]});
		for(unsigned i=0;i<30;++i)if(std::find(kXeenCombatOwners.begin(),kXeenCombatOwners.end(),i)==kXeenCombatOwners.end())remove_test::checkSameCharacter(initial.roster.at(i),party.roster.at(i));
		world.discardMapCache();world.map(20);world.objectFile(20);
		check(world.sessionState().actors()[5].hp==(victory?0:20),"original cache preserves actor outcome");
		std::cout<<"trace="<<trace<<" commands="<<commands<<" minutes="<<party.encounterContext->minutes<<" outcome="<<(victory?"Victory":"Defeat")<<" actors=27 bystanders=unchanged\n";
	}
	return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

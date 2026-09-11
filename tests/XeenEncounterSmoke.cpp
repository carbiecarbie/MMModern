// Optional read-only domain evidence. No original payloads or production CLI mode.
#include "XeenEncounterTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include <iostream>

using namespace encounter_test;
using Action=XeenEncounterAction;
int main(int argc,char **argv) {
	try {
		if(argc!=2)throw std::invalid_argument("usage: mmodern_encounter_domain_smoke <original-installation>");
		const auto installation=XeenInstallationDetector().detect(argv[1]);
		check(installation && installation->hasDarkside(),"World of Xeen installation required");
		XeenAssetSource assets(*installation);
		XeenMapLoader mapLoader;
		XeenEventLoader eventLoader([&](const std::string &name)->std::optional<Bytes>{
			if(!assets.hasInitialResource(name))return std::nullopt;return assets.readInitialResource(name);});
		const auto evt=eventLoader.load(20);
		const auto bytes=assets.readCloudsMonsterStatisticsFromDarkArchive();check(bytes && bytes->size()==5400,"original statistics extent");
		const auto statistics=XeenMonsterFormat::parse(*bytes);
		check(statistics.size()==90 && statistics[8].baseHp()==20 && statistics[8].image()==8 &&
			statistics[8].experience()==250 && statistics[8].supportsApproach(),"original anchor statistics");
		check(assets.readInitialResource("maze0020.mob").size()==200 &&
			assets.readInitialResource("maze0020.dat").size()==892 &&
			assets.readInitialResource("maze0020.evt").size()==128,"original map resource chain");
		const auto ptyBytes=assets.readInitialResource("maze.pty");check(ptyBytes.size()==812,"PTY extent");
		const auto context=XeenGameplayContextFormat::parse(ptyBytes);
		check(context.minutes==480 && context.ctr24==0 && context.year==610 && context.day==1,"original PTY context");
		const auto mob=mapLoader.loadObjects(assets,20);
		const auto original=XeenActorApproach::actorsFromResources(mob,statistics);
		const int positions[27][2]={{3,3},{3,5},{5,5},{5,3},{3,8},{13,2},{15,8},{10,6},{8,9},{6,14},
			{15,15},{15,15},{15,15},{11,15},{11,15},{11,15},{13,14},{8,15},{8,15},{3,4},{4,5},{5,4},{4,3},{6,9},{6,9},{1,13},{15,8}};
		check(original.size()==27,"original monster count");
		for(std::size_t i=0;i<27;++i)check(original[i].id==XeenMonsterIdentity{20,i} &&
			original[i].x==positions[i][0] && original[i].y==positions[i][1] &&
			original[i].original.resourceId==(i<17?8:9),"original identity/position/type control");
		XeenWorld world([&](XeenMapIdentity id){return mapLoader.loadGeometryMap(assets,id);},
			[&](XeenMapIdentity id){return mapLoader.loadObjects(assets,id);});
		auto party=XeenPartyLoader().loadInitialCloudsParty(assets);check(!party.encounterContext,"ordinary initial loading became encounter");
		const int hp[]{12,16,12,10,7,5},sp[]{2,0,2,0,7,9};
		for(unsigned i=0;i<6;++i){const auto &c=party.party.member(party.roster,i);check(c.currentHp==hp[i] && c.currentSp==sp[i],"original party HP/SP");}
		XeenActorApproach::validateDomain(world,party,context,original,evt);
		const int cells[4][2]={{13,1},{14,1},{13,2},{14,2}};
		const int ns[4][4]={{0,0,0,2},{1,1,3,1},{2,0,2,2},{1,3,3,3}};
		const int ew[4][4]={{0,0,0,1},{1,1,0,1},{2,3,2,2},{2,3,3,3}};
		unsigned controls=0;
		for(unsigned p=0;p<4;++p)for(unsigned d=0;d<4;++d)for(unsigned a=0;a<4;++a) {
			auto actors=original;actors[5].x=cells[a][0];actors[5].y=cells[a][1];
			const XeenCamera camera{20,cells[p][0],cells[p][1],static_cast<XeenDirection>(d)};
			const auto view=XeenActorApproach::classify(actors,camera);
			for(unsigned i=0;i<27;++i)if(i!=5)check(!view.activation[i],"bystander activation");
			// Deliberately force bystanders active: movement must still preserve all 26.
			for(auto &actor:actors)actor.activated=true;
			const auto moved=XeenActorApproach::move(actors,camera,[&](const XeenActor &actor,int x,int y){
				const auto cell=world.sampleCell(actor.id.mapId,x,y);
				return cell && cell->cell->rawWord==0x31 && cell->cell->rawAttributes==0?
					XeenMonsterTerrain::Allowed:XeenMonsterTerrain::Unsupported;});
			for(unsigned i=0;i<27;++i)if(i!=5)check(moved[i].id==original[i].id && moved[i].x==positions[i][0] && moved[i].y==positions[i][1],"active bystander moved");
			const int expected=(d%2?ew:ns)[p][a];
			check(moved[5].x==cells[expected][0] && moved[5].y==cells[expected][1],"literal 64-control approach destination");
			++controls;
		}
		for(int trace=0;trace<4;++trace) {
			XeenWorld w([&](XeenMapIdentity id){return mapLoader.loadGeometryMap(assets,id);},
				[&](XeenMapIdentity id){return mapLoader.loadObjects(assets,id);});
			auto p=XeenPartyLoader().loadInitialCloudsParty(assets);const auto initial=p;
			auto c=XeenActorApproach::kEntry;XeenEncounterState s;
			auto result=XeenActorApproach::initializeFromResources(assets,w,p,c,s);
			check(result.view.slots[3]->recordIndex==5,"original startup slot");
			auto action=[&](Action a){return XeenActorApproach::action(w,p,c,s,a,evt);};
			auto pulse=[&]{return XeenActorApproach::pulse(w,p,c,s,evt);};
			if(trace==0)action(Action::Wait);
			if(trace==1){action(Action::Forward);check(s.pending()==3,"original charge count3");pulse();}
			if(trace>=2) {
				action(Action::Right);pulse();action(Action::Forward);check(s.pending()==3,"original East count3");pulse();
				check(s.pending()==2 && c.x==14 && c.y==1,"original East post-action count2");
				if(trace==2) {
					pulse();pulse();const auto &a=w.sessionState().actors()[5];check(a.x==13 && a.y==1 && p.encounterContext->minutes==490,"original delayed approach");
					action(Action::Left);pulse();action(Action::Left);result=pulse();check(result.view.slots[3]->recordIndex==5,"original reveal slot");
				}
				result=action(Action::Wait);if(trace==3)check(result.movementOpportunities==2,"original rapid opportunities");
			}
			const auto &a=w.sessionState().actors()[5];check(s.phase()==XeenEncounterPhase::Engaged && s.pending()==0 && a.x==c.x && a.y==c.y && a.hp==20,"original terminal engagement");
			check(p.encounterContext->minutes==(trace>=2?500:490),"original final minutes");sameParty(initial,p);
			const auto live=w.sessionState().actors();w.discardMapCache();w.map(20);w.objectFile(20);sameActors(live,w.sessionState().actors());
			std::cout<<"trace="<<trace<<" party="<<c.x<<','<<c.y<<" actor="<<a.x<<','<<a.y<<" minutes="<<p.encounterContext->minutes<<" Engaged\n";
		}
		std::cout<<"Original resource/domain evidence: 90 statistics, 27 identities, "<<controls<<" isolation controls, 4 transition traces passed. Visual admission not tested.\n";
		return 0;
	}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

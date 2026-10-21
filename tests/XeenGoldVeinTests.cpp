// Original-data Gold Vein scripts: tiered amounts, cave-ins, exhaustion and the
// vein wall item. maze0034.evt (14,14) steps 250 -> 100 -> last tier, as in the
// original recording; maze0035.evt (11,6) is a one-tier vein whose wall item faces
// West although its Event fires facing North; maze0035.evt (12,2) holds two
// copies of its script and runs the first match of every line.
#include "XeenRegionalSaveGameplayTestSupport.h"
#include "XeenCombatGameplayTestSupport.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "games/xeen/XeenIndoorScene.h"
#include <iostream>
using namespace mmodern;
using regional_test::check;
namespace {
enum class Outcome { Gold, CaveIn, Nothing };
struct Attempt { Outcome outcome; std::uint32_t gold; };
struct Vein {
	XeenCamera start;
	std::size_t wallItem;
	XeenDirection drawn; // the wall item's facing
};
struct Run {
	std::vector<Attempt> attempts;
	bool drawnBefore=false,drawnAfter=true,drawnAtStart=false,exhausted=false;
	std::uint64_t draws=0;
};
bool veinDrawn(combat_gameplay_test::Harness &h,const Vein &vein,const XeenCamera &camera) {
	const auto commands=XeenIndoorScene().build(*h.world,camera,nullptr,nullptr,{},{},false,
		[&](const std::string &name){return h.assets->spriteFrameCount(name,xeenPictureArchive(camera.mapId));});
	return std::any_of(commands.begin(),commands.end(),[&](const auto &c) {
		const auto *wall=c.wallItem();return wall && wall->recordIndex==vein.wallItem;});
}
Run mine(const char *directory,const Vein &vein,std::uint32_t seed,unsigned attempts) {
	Run run;
	combat_gameplay_test::Harness h{std::filesystem::path(directory)};auto s=h.services();
	XeenEventTextLoader texts([&](const std::string &name)->std::optional<regional_test::Bytes> {
		if(!h.assets->hasArchiveResource(name))return {};return h.assets->readArchiveResource(name);
	});
	s.texts=[&](auto id){return texts.load(id);};
	s.resources.loadInitialPurse=[&]{return XeenCharacterFormat::parseMonsterPurse(h.assets->readInitialResource("maze.pty"));};
	s.resources.loadInitialBankBalances=[&]{return XeenCharacterFormat::parseBankBalances(h.assets->readInitialResource("maze.pty"));};
	s.resources.loadInitialWorldFlags=[&]{return XeenQuestFlagFormat::parseWorldFlags(h.assets->readInitialResource("maze.pty"));};
	s.resources.loadLearnedSpellNames=[&]{return XeenLearnedSpellNames::parse(*h.assets->readLearnedSpellNamesFromDarkArchive());};
	s.resources.regionalManifest=[&](const auto &m,const auto &o,const auto &e,const auto &statistics) {
		xeenValidateRegionalManifest(m,o,e,statistics,h.assets->readInitialResource("maze0023.dat"),
			h.assets->readInitialResource("maze0023.mob"),h.assets->readInitialResource("maze0023.evt"));
	};
	s.eventFixture=vein.start;s.saveUnavailable="disabled in the Event developer session";bool shown=false;
	XeenCamera view=vein.start;view.direction=vein.drawn;
	s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &) {
		shown=true;h.flow->prepareJourneySprites();
		run.drawnBefore=veinDrawn(h,vein,view);run.drawnAtStart=veinDrawn(h,vein,vein.start);
		const auto random=h.world->sessionState().journeyRandom();
		std::optional<XeenPresentationRequest> request;unsigned failures=0;
		h.flow->reportManual=[&](const auto &result) {
			request.reset();
			if(const auto *p=std::get_if<XeenEventExecutionSuspended>(&result))request=p->request;
			if(const auto *e=std::get_if<XeenEventExecutionError>(&result)) {
				++failures;std::cerr<<"Gold Vein map "<<vein.start.mapId.number<<": "<<e->message<<'\n';
			}
		};
		const auto press=[&](const PlayerAction &action) {
			handler.framePresented(h.flow->frame().presentation());handler.completeInputHandoff(h.flow->frame().presentation());
			handler.beginCycle(++h.cycle);handler.withDisplayedInput(action,*handler.displayedInput());
			handler.framePresented(h.flow->frame().presentation());handler.completeInputHandoff(h.flow->frame().presentation());
		};
		const auto hp=[&] {int total=0;for(auto id:h.party->party.activeRosterIds())total+=h.party->roster.at(id).currentHp;return total;};
		for(unsigned attempt=0;attempt<attempts;++attempt) {
			const auto gold=std::uint32_t(h.party->purse->gold);const int health=hp();bool selected=false;
			press(InteractionAction{});
			for(unsigned n=0;n<1000 && h.flow->blocksGameplay();++n) {
				h.now+=125;handler.beginCycle(++h.cycle);idle();
				handler.framePresented(h.flow->frame().presentation());handler.completeInputHandoff(h.flow->frame().presentation());
				if(request && request->response!=XeenPresentationResponseRequirement::Presented) {
					if(request->kind==XeenPresentationKind::CharacterSelection) {selected=true;press(SelectMemberAction{0});}
					else if(request->response==XeenPresentationResponseRequirement::YesNo)press(YesAction{});
					else press(AcknowledgeAction{});
				}
			}
			check(!failures && !h.flow->blocksGameplay(),"Gold Vein Event did not return session input");
			const auto gained=std::uint32_t(h.party->purse->gold)-gold;
			run.attempts.push_back({gained?Outcome::Gold:hp()<health?Outcome::CaveIn:Outcome::Nothing,gained});
			if(!selected) {run.exhausted=true;break;}
		}
		run.drawnAfter=veinDrawn(h,vein,view);
		run.draws=h.world->sessionState().journeyRandom()->count-random->count;
		return true;
	};
	check(Application(s.program).playGameplay(s,{}, {},false,XeenEncounterEntry::Journey,seed)==0 && shown,
		"Original-data Gold Vein session startup failed");
	return run;
}
std::string describe(const Run &run) {
	std::string text;
	for(const auto &a:run.attempts)
		text+=a.outcome==Outcome::Gold?std::to_string(a.gold)+" ":a.outcome==Outcome::CaveIn?"cave-in ":"nothing ";
	return text;
}
} // namespace
// Gold amounts by tier: GiveMulti 100 draws 1..100, plus 250 or 100 from mode 34.
bool inTier(const Attempt &a,std::uint32_t base) {return a.outcome==Outcome::Gold && a.gold>base && a.gold<=base+100;}
std::vector<Attempt> golds(const Run &run) {
	std::vector<Attempt> result;
	for(const auto &a:run.attempts) {
		check(a.outcome!=Outcome::Nothing || &a==&run.attempts.back(),"A vein attempt did nothing before exhaustion");
		if(a.outcome==Outcome::Gold)result.push_back(a);
	}
	return result;
}
int main(int argc,char **argv) {
	std::ostringstream output;auto *stream=std::cout.rdbuf(output.rdbuf());
	try {
		check(argc==2,"usage: gold-vein <installation>");
		// Tiered vein: every seed steps 250 -> 100 -> last and then stops; cave-ins
		// (JumpRnd 1..10 == 10) never change the tier. Seed 10 gives the recording's
		// order: two tiers, a cave-in, the last tier, then a plain wall.
		const Vein tiered{{34,14,14,XeenDirection::North},1,XeenDirection::North};
		for(const std::uint32_t seed:{10u,30u,33u}) {
			const auto run=mine(argv[1],tiered,seed,12);const auto gold=golds(run);
			check(run.exhausted && gold.size()==3 && inTier(gold[0],250) && inTier(gold[1],100) && inTier(gold[2],0),
				("Tiered vein did not step 250 -> 100 -> last: "+describe(run)).c_str());
			check(run.drawnBefore && run.drawnAtStart && !run.drawnAfter,"Tiered vein wall item did not show, or stayed after exhaustion");
			if(seed==10)check(run.attempts.size()==5 && run.attempts[2].outcome==Outcome::CaveIn,
				("Seed 10 no longer reproduces the recording's order: "+describe(run)).c_str());
			if(seed==30)check(run.attempts[1].outcome==Outcome::CaveIn && run.attempts[2].outcome==Outcome::CaveIn,
				"Consecutive cave-ins changed the tier");
		}
		// One-tier vein: 1..100 once, then exhausted. Its wall item (MOB item 3) faces
		// West, so the North view that runs the Event does not show it (original data).
		const Vein west{{35,11,6,XeenDirection::North},3,XeenDirection::West};
		for(const std::uint32_t seed:{1u,33u}) {
			const auto run=mine(argv[1],west,seed,12);const auto gold=golds(run);
			check(run.exhausted && gold.size()==1 && inTier(gold[0],0),("One-tier vein differs: "+describe(run)).c_str());
			check(run.drawnBefore && !run.drawnAtStart && !run.drawnAfter,"Map-35 vein wall item placement or removal differs");
		}
		// Two script copies at one cell: the first copy's lines run, then copy two's
		// line 23 disables line 0, so the vein pays 101..200 once.
		const Vein twin{{35,12,2,XeenDirection::West},12,XeenDirection::West};
		const auto run=mine(argv[1],twin,33,12);const auto gold=golds(run);
		check(run.exhausted && gold.size()==1 && inTier(gold[0],100) && run.drawnBefore && !run.drawnAfter,
			("Duplicated vein script did not run the first matching lines: "+describe(run)).c_str());
		std::cout.rdbuf(stream);std::cout<<"Gold Vein tiers, cave-ins, exhaustion and wall item passed\n";return 0;
	}catch(const std::exception &error){std::cout.rdbuf(stream);std::cerr<<error.what()<<'\n';return 1;}
}

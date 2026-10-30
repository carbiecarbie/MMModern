// M68 gameplay integration. Part A: full consequence publication of monster
// secondary effects in exploration (mixed ranged volley) and in combat through
// the production Application/EventFlow path, exact save round trips after each
// effect, and the developer fixture provider (synthetic MOB; the original-data
// variant runs with a game directory).
#include "XeenRegionalSaveGameplayTestSupport.h"
#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "games/xeen/XeenMonsterFixture.h"
#include "formats/xeen/XeenMapFormat.h"
#include "games/xeen/XeenRegionalRules.h"
#include <iostream>
#include <set>
#include <array>
using namespace mmodern;
using gameplay_test::check;
namespace mmodern {
struct XeenMonsterAbilityTestAccess {
	static std::shared_ptr<const XeenRegionalObservation> ranged(const XeenEncounterFlow &f) { return f._rangedObservation; }
};
// The Rest continuation is private; this test reads its phase like XeenRestOriginalTests.
struct XeenRestTestAccess {
	static bool active(const XeenEventFlow &f) { return bool(f._encounter->_rest); }
	static bool confirm(const XeenEventFlow &f) { return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Confirm; }
	// The ten sleeping opportunities, where monsters move; later phases have none.
	static bool charging(const XeenEventFlow &f) { return f._encounter->_rest && f._encounter->_rest->phase==XeenEncounterFlow::RestContinuation::Phase::Charges; }
};
}
namespace {
using Bytes=std::vector<std::uint8_t>;
template<class F> void rejects(F f,const char *m) { bool refused=false;try { f(); } catch (const std::exception &) { refused=true; } check(refused,m); }
// Luck 0 (-5) at the prepared levels 3-4 makes every Physical save fail (value
// -2 or -1, draws from 1); resistances are zero, so every element save fails;
// dice of 1 make the damage exact. The Paladins get spell points (byte 163) so
// Drain SP is visible; weapon ID 6 is equipped in slot 0.
// `knightLeader` makes member 0 (roster 0) the only Knight, so a hatred-0
// monster strikes exactly that member and the other five survive untouched.
Bytes bytes(bool knightLeader=false) {
	auto b=regional_test::characterBytes();
	for (unsigned owner=0;owner<30;++owner) { const auto n=owner*354;b[n+32]=0;b[n+163]=1; }
	if (knightLeader) b[kXeenCombatOwners[0]*354+19]=0;
	return b;
}
enum Species : unsigned { Physical=7, Melee=8, DrainSp=9, Eradicate=10, Aging=11, CurseItem=12, BreakWeapon=13,
	KnightEradicate=14, KnightDeath=15, KnightStone=16 };
std::vector<XeenMonsterRecord> statistics(bool meleeDrains=false) {
	auto out=regional_test::statistics();out.resize(17);
	const auto ranged=[&](unsigned i,unsigned type,unsigned special,unsigned hatred=15) {
		auto &r=out[i];r=out[Melee];r.raw[0]='R';r.raw[24]=1;r.raw[25]=std::uint8_t(hatred);r.raw[26]=1;r.raw[28]=1;r.raw[29]=std::uint8_t(type);r.raw[30]=std::uint8_t(special);r.raw[31]=5;r.raw[32]=1;
	};
	ranged(Physical,0,0);ranged(DrainSp,2,12);ranged(Eradicate,4,19);ranged(Aging,3,20);ranged(CurseItem,6,10);ranged(BreakWeapon,1,17);
	ranged(KnightEradicate,4,19,0);ranged(KnightDeath,3,21,0);ranged(KnightStone,2,22,0);
	if (meleeDrains) { auto &m=out[Melee];m.raw[25]=15;m.raw[26]=1;m.raw[28]=1;m.raw[29]=2;m.raw[30]=12; }
	return out;
}
// Contact melee at (8,11) plus two ranged sources one row off the party's line:
// record 1 physical at (7,12), record 2 the chosen species at (7,10). Both are
// in view (activated) but not in line, so the approach Wait fires no shot; water
// at (8,10) and (8,12) blocks their primary step, and the fallback step puts
// both on (7,11), in line two cells from the party, for the first combat round.
XeenObjectFile combatVolleyObjects(unsigned species) {
	XeenObjectFile m{23,"synthetic.mob",true,{}};
	m.entities.monsters.push_back({8,11,0,0,int(Melee)});
	m.entities.monsters.push_back({7,12,0,0,int(Physical)});
	m.entities.monsters.push_back({7,10,0,0,int(species)});
	m.entities.objects.push_back({9,11,0,0,7});
	return m;
}
XeenMap volleyWaterMap(XeenMapIdentity id) {
	auto m=regional_test::map(id);
	if (id==XeenMapIdentity(23)) for (const auto cell:{std::pair<int,int>{8,10},{8,12}}) m.geometry.cells[cell.second*16+cell.first].surfaceIndex=0;
	return m;
}
// Two ranged sources west of the party at (9,11): record 0 physical at (7,11),
// record 1 the chosen species at (6,11). Water around them keeps them in place.
XeenObjectFile objects(unsigned species,bool physical) {
	XeenObjectFile m{23,"synthetic.mob",true,{}};
	if (physical) m.entities.monsters.push_back({7,11,0,0,int(Physical)});
	m.entities.monsters.push_back({6,11,0,0,int(species)});
	m.entities.objects.push_back({9,11,0,0,7});
	return m;
}
XeenObjectFile contactObjects() {
	XeenObjectFile m{23,"synthetic.mob",true,{}};
	m.entities.monsters.push_back({8,11,0,0,int(Melee)});m.entities.objects.push_back({9,11,0,0,7});return m;
}
XeenMap waterMap(XeenMapIdentity id) {
	auto m=regional_test::map(id);
	if (id==XeenMapIdentity(23)) for (const auto cell:{std::pair<int,int>{5,11},{6,10},{6,12},{7,10},{7,12},{8,11}}) m.geometry.cells[cell.second*16+cell.first].surfaceIndex=0;
	return m;
}
struct Scenario {
	std::vector<XeenMonsterRecord> monsters;
	XeenObjectFile mob;
	std::function<XeenMap(XeenMapIdentity)> maps;
	Bytes chr=bytes();
	XeenSaveState::Resources resources() const {
		auto r=regional_test::resources();
		r.loadMonsterStatistics=[m=monsters]{return m;};r.loadInitialCharacters=[c=chr]{return c;};
		r.loadInitialParty=[c=chr]{return XeenPartyLoader().loadFromResources(c,regional_test::partyBytes());};
		return r;
	}
	XeenWorld::ObjectLoader objectLoader() const { return [m=mob](XeenMapIdentity id){return id==XeenMapIdentity(23) ? m : regional_test::objects(id);}; }
};
// The production entry: Application::playGameplay with synthetic providers.
struct Harness {
    mmodern::XeenProgramState programState{mmodern::XeenWallClock::fixed(0)};
	Scenario scenario;
	XeenFontFormat font{gameplay_test::fontBytes()};
	XeenItemCatalog catalog;
	XeenEventFlow *flow=nullptr;XeenWorld *world=nullptr;const XeenPartyState *party=nullptr;XeenCamera *camera=nullptr;const XeenGameFlags *flags=nullptr;
	std::uint64_t cycle=0,now=0;
	explicit Harness(Scenario s):scenario(std::move(s)) {}
	XeenGameplayServices services() {
		XeenGameplayServices s{programState,scenario.resources(),[]{return XeenGameFlags{};},scenario.maps,scenario.objectLoader(),regional_test::texts,font,
			[](auto &,const auto &,const auto &,auto){return XeenEventFlow::Composition{IndexedFrame{320,200,Bytes(64000)},true};}, {},
			[&](auto &f,const auto &){flow=&f;f.dialogText=&dos_test::text();
				f.loadRestDream=[]{return IndexedFrame{320,200,Bytes(64000)};};
				f.loadSampleDuration=[](const char *,XeenSceneArchive){return std::uint64_t(1000000);};}, {},
			[&](auto &w,auto &,const auto &p,auto &c,const auto &g){world=&w;party=&p;camera=&c;flags=&g;}};
		s.composeEncounter=[&](auto &,const auto &,const auto &,auto,auto){return XeenEventFlow::Composition{IndexedFrame{320,200,Bytes(64000)},true};};
		s.clock=[&]{return now;};s.catalog=&catalog;s.validateEncounterSprite=[](auto){};s.validateCombatSprite=[](auto){};
		return s;
	}
	void present(const SdlWindow::FrameUpdateHandler &h) { h.framePresented(flow->frame().presentation()); }
	void press(const SdlWindow::FrameUpdateHandler &h,const PlayerAction &a) {
		present(h);h.beginCycle(++cycle);const auto token=h.displayedInput();check(token.has_value(),"displayed input");
		h.withDisplayedInput(a,*token);check(h.frameCurrent(),"current frame after input");present(h);
	}
	void tick(const SdlWindow::FrameUpdateHandler &h,const SdlWindow::IdleFrameHandler &idle) {
		now+=110;h.beginCycle(++cycle);idle();check(h.frameCurrent(),"current idle frame");present(h);
	}
	const XeenCharacter &member(unsigned i) const { return party->roster.at(kXeenCombatOwners[i]); }
	XeenSaveSnapshot capture() { return XeenSaveState::capture(regional_test::signature(),*party,*camera,*flags,*world); }
	bool quiet() const { return flow->canSave() && !flow->encounter()->state().pending(); }
	int run(const std::function<void(const SdlWindow::FrameUpdateHandler &,const SdlWindow::IdleFrameHandler &)> &drive) {
		auto s=services();
		s.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){present(handler);drive(handler,idle);return true;};
		return Application(s.program).playGameplay(s,xeenJourneyContent().entry,{},false,XeenEncounterEntry::Journey,56);
	}
};
// A restored copy through the same providers, for exact round trips.
struct Restored {
    mmodern::XeenProgramState programState{mmodern::XeenWallClock::fixed(0)};
	Scenario scenario;
	XeenPartyState p;XeenCamera camera;XeenGameFlags flags;
	XeenWorld w;
	XeenEventPresenter::Clock clock=[]{return 0;};
	std::unique_ptr<XeenEncounterFlow> flow;
	Restored(const Scenario &s,const XeenSaveSnapshot &saved):scenario(s),w(scenario.maps,scenario.objectLoader()) {
		XeenSaveState::restoreBeforeGameplay(saved,scenario.resources(),p,camera,flags,w,[](auto &,const auto &,const auto &,const auto &){});
		flow=std::make_unique<XeenEncounterFlow>(programState,w,p,camera,flags,clock,XeenJourneyRestoreTag{});
		check(flow->prepareJourneyFrame(flow->ticket(),[]{}) && flow->presentJourney(flow->ticket()),"restored frame");
	}
	XeenSaveSnapshot capture() { return XeenSaveState::capture(regional_test::signature(),p,camera,flags,w); }
	const XeenCharacter &member(unsigned i) const { return p.roster.at(kXeenCombatOwners[i]); }
};
void roundTrip(Harness &h,const char *what) {
	check(h.quiet(),"round trip needs a quiet boundary");
	const auto saved=h.capture();
	Restored g(h.scenario,saved);
	check(XeenSaveFormat::encode(g.capture())==XeenSaveFormat::encode(saved),what);
	check(*g.w.sessionState().journeyRandom()==*saved.journey->random,"Restore replays no draw");
	for (unsigned i=0;i<6;++i) check(xeen_state::sameCharacter(g.member(i),h.member(i)),"Restored members equal the published ones");
}
struct Volley { std::shared_ptr<const XeenRegionalObservation> first;std::set<unsigned> pows;unsigned opportunities=0; };
// Wait once, then idle until the Journey is quiet (or stops), collecting the
// published volley observations and the projectile resources shown meanwhile.
Volley waitAndSettle(Harness &h,const SdlWindow::FrameUpdateHandler &handler,const SdlWindow::IdleFrameHandler &idle) {
	Volley v;h.press(handler,WaitAction{});
	// One idle can run the completing pulse and the owed follow-up pulse, so the
	// completed volley's observation is replaced before the test can read it.
	// The travel observation (queued shots with their sources) survives for the
	// presented ticks; the attack results are checked in the live roster.
	std::shared_ptr<const XeenRegionalObservation> last;
	const auto observe=[&] {
		const auto *e=h.flow->encounter();
		if (const auto a=e->appearance();a.projectile) v.pows.insert(a.projectile->pow);
		const auto r=XeenMonsterAbilityTestAccess::ranged(*e);
		if (r && r!=last && r->count) { last=r;++v.opportunities;if (!v.first) v.first=r; }
	};
	observe();
	for (unsigned n=0;n<600 && !h.quiet() && h.flow->encounter()->state().phase()==XeenEncounterPhase::Exploring && !h.flow->encounter()->combat();++n) { h.tick(handler,idle);observe(); }
	return v;
}
void mixedVolleyDrainsSp() {
	Harness h({statistics(),objects(DrainSp,true),waterMap});
	check(h.run([&](const auto &handler,const auto &idle) {
		std::array<int,6> sp{},hp{};
		for (unsigned i=0;i<6;++i) { sp[i]=h.member(i).currentSp;hp[i]=h.member(i).currentHp;check(sp[i]>0 && hp[i]>0,"fixture start"); }
		const auto v=waitAndSettle(h,handler,idle);
		if (!(h.quiet() && v.first && v.first->count==2)) {
			const auto *e=h.flow->encounter();
			throw std::runtime_error("One mixed volley observed, then quiet: canSave="+std::to_string(h.flow->canSave())+" pending="+std::to_string(e->state().pending())+
				" phase="+std::to_string(unsigned(e->state().phase()))+" activity="+std::to_string(unsigned(h.world->sessionState().journeyActivity()))+
				" opportunities="+std::to_string(v.opportunities)+" first="+std::to_string(v.first ? v.first->count : 99u)+" revision="+std::to_string(e->result().revision)+
				" combat="+std::to_string(bool(e->combat()))+" now="+std::to_string(h.now)+" pows="+std::to_string(v.pows.size())+
				" rangedStage="+std::to_string(XeenMonsterAbilityTestAccess::ranged(*e) ? int(XeenMonsterAbilityTestAccess::ranged(*e)->stage) : -1)+
				" rangedCount="+std::to_string(XeenMonsterAbilityTestAccess::ranged(*e) ? XeenMonsterAbilityTestAccess::ranged(*e)->count : 99u));
		}
		const unsigned fireShot=v.first->shots[0].source.recordIndex==1 ? 0 : 1;
		check(v.first->shots[fireShot].source.recordIndex==1 && v.first->shots[1-fireShot].source.recordIndex==0 &&
			v.first->shots[fireShot].distance==3 && v.first->shots[1-fireShot].distance==2,"Both sources shoot in the volley");
		// MONSTER_SHOOT_POW: physical 12 (the arrow), Fire 0. The volley picks the
		// first non-arrow resource in queue order and draws it for every lane.
		check(v.pows.count(0) && !v.pows.count(12) && v.pows.size()==1,"The volley draws the first non-arrow projectile (Fire, pow 0) for every lane");
		// The Fire source drains every member (party hatred, no hit roll); the
		// physical source only wounds. Per-source draw order is in the rules test.
		for (unsigned i=0;i<6;++i) check(h.member(i).currentSp==0 && h.member(i).currentHp<hp[i],"Published SP and HP in the live roster");
		(void)sp;
		roundTrip(h,"Exact round trip after Drain SP");
	})==0,"Drain SP session");
}
void publishedEffects() {
	{
		Harness h({statistics(),objects(Aging,false),waterMap});
		check(h.run([&](const auto &handler,const auto &idle) {
			const auto v=waitAndSettle(h,handler,idle);
			check(h.quiet() && v.opportunities>=1,"Aging volley settled");
			for (unsigned i=0;i<6;++i) check(h.member(i).temporaryAge>=1 && h.member(i).temporaryAge==h.member(0).temporaryAge,"Temporary age published for every member");
			roundTrip(h,"Exact round trip after Aging");
		})==0,"Aging session");
	}
	{
		Harness h({statistics(),objects(CurseItem,false),waterMap});
		check(h.run([&](const auto &handler,const auto &idle) {
			waitAndSettle(h,handler,idle);check(h.quiet(),"Curse Item volley settled");
			for (unsigned i=0;i<6;++i) {
				const auto &c=h.member(i);
				check(c.weapons[0].id==6 && c.weapons[0].state==0x40 && c.weapons[0].frame==1 && c.weapons[8].state==0x40 && c.armor[0].state==0x40 && c.accessories[3].state==0x40 && c.miscellaneous[8].state==0x40,"Every inventory slot cursed in the live roster");
			}
			roundTrip(h,"Exact round trip after Curse Item");
		})==0,"Curse Item session");
	}
	{
		Harness h({statistics(),objects(BreakWeapon,false),waterMap});
		check(h.run([&](const auto &handler,const auto &idle) {
			waitAndSettle(h,handler,idle);check(h.quiet(),"Break Weapon volley settled");
			for (unsigned i=0;i<6;++i) check(h.member(i).weapons[0].id==6 && h.member(i).weapons[0].state==0x80 && h.member(i).weapons[0].frame==0,"Broken weapon unequipped in the live roster");
			roundTrip(h,"Exact round trip after Break Weapon");
		})==0,"Break Weapon session");
	}
	{
		Harness h({statistics(),objects(Eradicate,false),waterMap});
		h.run([&](const auto &handler,const auto &idle) {
			waitAndSettle(h,handler,idle);
			check(h.flow->encounter()->state().phase()==XeenEncounterPhase::SupportStopped && h.flow->encounter()->state().reason()==XeenEncounterStop::Defeat,"Eradicating the whole party ends the Journey");
			for (unsigned i=0;i<6;++i) {
				const auto &c=h.member(i);
				check(c.conditions[15]==1 && c.currentHp<=0 && c.weapons[0].state==0x80 && c.weapons[0].frame==0 && c.weapons[5].state==0x80 && c.armor[0].state==0x80 && c.accessories[0].state==0x80 && c.miscellaneous[0].state==0x80,"Eradication published to every inventory byte");
			}
		});
	}
}
void combatPublication() {
	Harness h({statistics(true),contactObjects(),[](auto id){return regional_test::map(id);}});
	check(h.run([&](const auto &handler,const auto &idle) {
		std::array<int,6> sp{},hp{};
		for (unsigned i=0;i<6;++i) { sp[i]=h.member(i).currentSp;hp[i]=h.member(i).currentHp;check(sp[i]>0,"combat fixture start"); }
		h.press(handler,WaitAction{});
		for (unsigned n=0;n<200 && !h.flow->encounter()->combat();++n) h.tick(handler,idle);
		const auto *combat=h.flow->encounter()->combat();check(combat!=nullptr,"Contact engagement attached combat");
		unsigned turns=0;
		while (turns<60 && combat->phase()==XeenCombatPhase::PlayerReady) { h.press(handler,BlockAction{});++turns;for (unsigned n=0;n<20 && combat->pending()==XeenCombatWork::Action;++n) h.tick(handler,idle); }
		for (unsigned n=0;n<200 && combat->pending()==XeenCombatWork::Enemy;++n) h.tick(handler,idle);
		const auto r=h.flow->encounter()->combatResult();
		check(r.operation==XeenCombatOperation::EnemyAttack && r.injuryCount==6 && r.damage==6,"Fire contact attack hits all six with one strike each");
		for (unsigned n=0;n<6;++n) check(r.injury(n).beforeSp==sp[n] && r.injury(n).afterSp==0,"Combat injury observes the SP drain");
		for (unsigned i=0;i<6;++i) check(h.member(i).currentSp==0 && h.member(i).currentHp==hp[i]-1,"Combat published SP and HP to the live roster");
		for (unsigned n=0;n<60 && combat->pending()!=XeenCombatWork::None;++n) h.tick(handler,idle);
		check(combat->phase()!=XeenCombatPhase::Failed && combat->phase()!=XeenCombatPhase::SupportStopped,"The published state matches the retained expectation");
	})==0,"combat session");
}
// Member predicates shared by the table-driven scenarios below.
using Member=const XeenCharacter &;
bool drained(Member c) { return c.currentSp==0; }
bool weaponBroken(Member c) { return c.weapons[0].id==6 && c.weapons[0].state==0x80 && c.weapons[0].frame==0; }
// A mixed ranged volley during combat: the melee contact fights while the
// physical and the secondary-effect sources shoot from the back row.
void combatVolley() {
	struct Case { unsigned species;unsigned pow;bool (*effect)(Member);const char *name; };
	const Case cases[]{{DrainSp,0,drained,"Drain SP"},{BreakWeapon,14,weaponBroken,"Break Weapon"}};
	for (const auto &t:cases) {
		Harness h({statistics(),combatVolleyObjects(t.species),volleyWaterMap});
		check(h.run([&](const auto &handler,const auto &idle) {
			std::array<int,6> hp{};for (unsigned i=0;i<6;++i) { hp[i]=h.member(i).currentHp;check(!t.effect(h.member(i)),"combat volley start"); }
			h.press(handler,WaitAction{});
			for (unsigned n=0;n<200 && !h.flow->encounter()->combat();++n) h.tick(handler,idle);
			const auto *combat=h.flow->encounter()->combat();check(combat!=nullptr,"Contact engagement attached combat");
			for (unsigned i=0;i<6;++i) check(!t.effect(h.member(i)),"No shot before combat: the sources were off the line during the approach");
			std::set<unsigned> pows;bool publishedDuringRound=false;
			const auto round=[&] {
				unsigned turns=0;
				while (turns<60 && combat->phase()==XeenCombatPhase::PlayerReady) { h.press(handler,BlockAction{});++turns;for (unsigned n=0;n<20 && combat->pending()==XeenCombatWork::Action;++n) h.tick(handler,idle); }
				for (unsigned n=0;n<400 && combat->pending()!=XeenCombatWork::None && combat->phase()!=XeenCombatPhase::PlayerReady;++n) {
					h.tick(handler,idle);
					if (const auto a=h.flow->encounter()->appearance();a.projectile) pows.insert(a.projectile->pow);
					// The effect reaches the live roster while the round's movement work is still owed.
					if (combat->pending()==XeenCombatWork::Round && t.effect(h.member(0))) publishedDuringRound=true;
				}
			};
			round();
			const auto observation=XeenMonsterAbilityTestAccess::ranged(*h.flow->encounter());
			check(observation && observation->count==2 && observation->shots[0].source.recordIndex!=observation->shots[1].source.recordIndex,"Both back-row sources shot during the combat round");
			for (unsigned n=0;n<2;++n) {
				const auto &shot=observation->shots[n];
				check(shot.source.recordIndex==1 || shot.source.recordIndex==2,"Volley sources are the two ranged records");
				for (unsigned i=0;i<shot.attack.injuryCount;++i) {
					const auto &injury=shot.attack.injury(i);
					if (shot.source.recordIndex==2 && t.species==DrainSp) check(injury.afterSp==0 && injury.beforeSp>0,"The secondary source drains SP at its injury");
					if (shot.source.recordIndex==1) check(injury.beforeSp==injury.afterSp,"The physical source applies only its own type");
				}
			}
			check(pows.count(t.pow) && !pows.count(12) && pows.size()==1,"The combat volley shares the first non-arrow projectile for every lane");
			for (unsigned i=0;i<6;++i) check(t.effect(h.member(i)) && h.member(i).currentHp<hp[i],"Secondary effect published to the live roster in combat");
			check(publishedDuringRound,"Live state carried the effect at the injury publication, before the round settled");
			// Continuation: the published state matches the retained expectation and the fight goes on.
			check(combat->phase()!=XeenCombatPhase::Failed && combat->phase()!=XeenCombatPhase::SupportStopped,"Retained-state authority after the volley");
			round();
			check(combat->phase()!=XeenCombatPhase::Failed && combat->phase()!=XeenCombatPhase::SupportStopped,"Combat continues past the authority check");
		})==0,t.name);
	}
}
// Rest interrupted by a ranged secondary effect: every member is hit, so the
// whole party wakes, the Rest stops without recovery, and the state survives a
// save round trip.
void restInterruption() {
	struct Case { unsigned species;bool (*effect)(Member);const char *name; };
	const Case cases[]{{DrainSp,drained,"Rest: Drain SP"},{BreakWeapon,weaponBroken,"Rest: Break Weapon"}};
	for (const auto &t:cases) {
		Harness h({statistics(),objects(t.species,false),waterMap});
		check(h.run([&](const auto &handler,const auto &idle) {
			std::array<int,6> hp{};for (unsigned i=0;i<6;++i) { hp[i]=h.member(i).currentHp;check(!t.effect(h.member(i)),"rest start"); }
			const auto minutes=h.party->encounterContext->minutes;
			h.press(handler,RestAction{});
			bool publishedWhileResting=false;
			for (unsigned n=0;n<600 && XeenRestTestAccess::active(*h.flow);++n) {
				if (XeenRestTestAccess::confirm(*h.flow)) h.press(handler,DialogKeyAction{'y'});
				else h.tick(handler,idle);
				if (XeenRestTestAccess::active(*h.flow) && t.effect(h.member(0))) publishedWhileResting=true;
			}
			check(!XeenRestTestAccess::active(*h.flow),"The ranged hit interrupted the Rest");
			check(publishedWhileResting,"The effect reached the live roster at the injury publication, before the Rest settled");
			for (unsigned n=0;n<200 && !h.quiet() && h.flow->encounter()->state().phase()==XeenEncounterPhase::Exploring;++n) h.tick(handler,idle);
			check(h.quiet(),"Interrupted Rest settles to a quiet boundary");
			// The source keeps shooting in the opportunities that follow the interruption, so the wound is at least one point.
			for (unsigned i=0;i<6;++i) check(t.effect(h.member(i)) && h.member(i).currentHp<hp[i] && !h.member(i).conditions[8],"Effect, wound and wake published after settlement");
			check(h.party->encounterContext->minutes>minutes && h.party->encounterContext->minutes<minutes+60,"No recovery: only the elapsed charge minutes passed");
			roundTrip(h,"Exact round trip after an interrupted Rest");
		})==0,t.name);
	}
}
// Eradicate, Death and Stone on one member while the others survive: the
// terminal condition, the zeroed HP and the inventory writes persist exactly.
void survivingRoundTrips() {
	struct Case { unsigned species;unsigned condition;const char *name; };
	const Case cases[]{{KnightEradicate,15,"Eradicate"},{KnightDeath,13,"Death"},{KnightStone,14,"Stone"}};
	for (const auto &t:cases) {
		Scenario scenario{statistics(),objects(t.species,false),waterMap};scenario.chr=bytes(true);
		Harness h(scenario);
		check(h.run([&](const auto &handler,const auto &idle) {
			std::array<XeenCharacter,6> before;for (unsigned i=0;i<6;++i) before[i]=h.member(i);
			check(h.member(0).characterClass==XeenCharacterClass::Knight && h.member(1).characterClass==XeenCharacterClass::Paladin,"one Knight leads");
			waitAndSettle(h,handler,idle);
			check(h.quiet(),"The surviving party settles");
			const auto &victim=h.member(0);
			check(victim.conditions[t.condition]>=1 && victim.currentHp<=0,"The hated Knight carries the terminal condition with zeroed HP");
			if (t.species==KnightEradicate) check(victim.weapons[0].state==0x80 && victim.weapons[0].frame==0 && victim.armor[0].state==0x80 && victim.miscellaneous[0].state==0x80,"Eradicate inventory writes persist");
			for (unsigned i=1;i<6;++i) check(xeen_state::sameCharacter(h.member(i),before[i]),"Other members untouched");
			roundTrip(h,t.name);
		})==0,t.name);
	}
}
// M68 Part B: flyers, through the production gameplay path. A moat of Water
// (surface index 0) at (7,11), (8,11), (10,11) and (11,11) surrounds the party
// at (9,11) facing West: a monster at (6,11) is in view across the water, one
// at (12,11) is behind the party across the water.
enum MovementSpecies : unsigned { Flyer=17 };
std::vector<XeenMonsterRecord> movementStatistics() {
	auto out=statistics();out.resize(18);
	out[Flyer]=out[Melee];out[Flyer].raw[0]='F';out[Flyer].raw[46]=1;
	return out;
}
XeenObjectFile single(unsigned species,int x,int y) {
	XeenObjectFile m{23,"synthetic.mob",true,{}};
	m.entities.monsters.push_back({x,y,0,0,int(species)});m.entities.objects.push_back({9,11,0,0,7});return m;
}
XeenMap moatMap(XeenMapIdentity id) {
	auto m=regional_test::map(id);
	if (id==XeenMapIdentity(23)) for (const int x:{7,8,10,11}) {
		auto &c=m.geometry.cells[11*16+x];c.rawWord=0;c.surfaceIndex=0;c.geometry=XeenOutdoorLayers{0,0,0,0};
	}
	return m;
}
// The flyer crosses the water one cell per Wait and enters contact, and combat
// admits it; the walker stays on its bank. Movement makes no random draw: after
// the first Wait both sessions hold the same Journey random state.
void flightToContact() {
	std::vector<std::decay_t<decltype(*std::declval<XeenWorld &>().sessionState().journeyRandom())>> afterFirstWait;
	for (const unsigned species:{unsigned(Melee),unsigned(Flyer)}) {
		Harness h({movementStatistics(),single(species,6,11),moatMap});
		check(h.run([&](const auto &handler,const auto &idle) {
			const auto actor=[&]() -> const XeenActor & { return h.world->sessionState().monsters(23).at(0); };
			waitAndSettle(h,handler,idle);
			check(species==Flyer ? actor().x==7 && actor().y==11 : actor().x==6 && actor().y==11,
				species==Flyer ? "The flyer's first step is onto the water" : "The walker is held on its bank by the water");
			afterFirstWait.push_back(*h.world->sessionState().journeyRandom());
			for (unsigned wait=0;wait<4 && !h.flow->encounter()->combat();++wait) waitAndSettle(h,handler,idle);
			if (species==Flyer) {
				const auto *combat=h.flow->encounter()->combat();check(combat!=nullptr,"The flyer entered contact and combat attached");
				for (unsigned n=0;n<40 && combat->phase()!=XeenCombatPhase::PlayerReady;++n) h.tick(handler,idle);
				check(combat->phase()==XeenCombatPhase::PlayerReady,"Combat admits the flying contact actor");
			} else {
				check(!h.flow->encounter()->combat() && actor().x==6 && h.quiet(),"The walker never reaches contact");
				roundTrip(h,"Exact round trip beside a held walker");
			}
		})==0,species==Flyer ? "flyer session" : "walker session");
	}
	check(afterFirstWait.size()==2 && afterFirstWait[0]==afterFirstWait[1],"Flight adds no random draw");
}
// Rest moves every present actor, activated or not: a flyer behind the party
// crosses the water and stops the Rest during its sleeping opportunities; the
// walker behind it never moves through all ten of them.
void restActivation() {
	for (const unsigned species:{unsigned(Melee),unsigned(Flyer)}) {
		Harness h({movementStatistics(),single(species,12,11),moatMap});
		check(h.run([&](const auto &handler,const auto &idle) {
			const auto actor=[&]() -> const XeenActor & { return h.world->sessionState().monsters(23).at(0); };
			check(!actor().activated,"The monster behind the party is not activated");
			h.press(handler,RestAction{});
			bool moved=false,charged=false;
			for (unsigned n=0;n<6000 && XeenRestTestAccess::active(*h.flow) && (!charged || XeenRestTestAccess::charging(*h.flow));++n) {
				charged=charged || XeenRestTestAccess::charging(*h.flow);
				if (XeenRestTestAccess::confirm(*h.flow)) h.press(handler,DialogKeyAction{'y'});
				else h.tick(handler,idle);
				moved=moved || actor().x!=12;
			}
			check(charged,"The Rest reached its sleeping opportunities");
			if (species==Flyer) check(moved && actor().x<=10 && !XeenRestTestAccess::active(*h.flow),"Rest moved the unactivated flyer across the water and it stopped the Rest");
			else check(!moved && XeenRestTestAccess::active(*h.flow) && !XeenRestTestAccess::charging(*h.flow) && actor().x==12 && actor().y==11,
				"The walker behind the water never moved through the ten sleeping opportunities");
		})==0,species==Flyer ? "flyer Rest" : "walker Rest");
	}
}
void originalFixture(const std::filesystem::path &game) {
    mmodern::XeenProgramState programState{mmodern::XeenWallClock::fixed(0)};
	const auto installation=xeenTestInstallationDetector().detect(game);
	check(installation && installation->hasDarkside(),"World of Xeen installation");
	training_test::Inputs in(*installation);
	const auto original=in.assets.readInitialResource("maze0023.mob");
	const auto originalEntities=XeenMapFormat::parseMob(original);
	const auto map23=in.maps.loadGeometryMap(in.assets,23);
	// The default placement: a legal start with the monster 2-3 cells straight ahead on a clear line.
	const auto placement=xeenMonsterFixturePlacement(map23,originalEntities,{17,{},{}});
	{
		constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};const auto d=unsigned(placement.start.direction);
		check(!xeenMonsterFixtureStartRefusal(map23,originalEntities,placement.start) && placement.inView && placement.distance>=2 && placement.distance<=3 &&
			placement.monsterX==originalEntities.monsters[placement.record].x && placement.monsterY==originalEntities.monsters[placement.record].y &&
			placement.monsterX==placement.start.x+dx[d]*int(placement.distance) && placement.monsterY==placement.start.y+dy[d]*int(placement.distance) &&
			xeenPlayerRayRows(map23,placement.start)>placement.distance,"Default F3 placement is legal, straight ahead and unobstructed");
		check(xeenMonsterFixtureStartRefusal(map23,originalEntities,{23,0,6,XeenDirection::North}).has_value(),"--start on water refused");
		check(xeenMonsterFixtureStartRefusal(map23,originalEntities,{23,originalEntities.monsters[0].x,originalEntities.monsters[0].y,XeenDirection::North}).has_value(),"--start on a spawn refused");
		check(!xeenMonsterFixtureStartRefusal(map23,originalEntities,xeenJourneyContent().entry),"the Journey entry is a legal --start");
		rejects([&]{xeenMonsterFixturePlacement(map23,originalEntities,{17,{},XeenCamera{23,0,6,XeenDirection::North}});},"placement with a refused --start");
		const auto entry=xeenMonsterFixturePlacement(map23,originalEntities,{17,{},xeenJourneyContent().entry});
		check(entry.start.x==9 && entry.start.y==11 && entry.start.direction==XeenDirection::West,"--start overrides the chosen start");
	}
	// The Giant Bat at a water edge (Part B): no original spawn faces a reachable
	// shore across water, so the chosen record moves to the far bank.
	const auto edge=xeenMonsterFixturePlacement(map23,originalEntities,{1,{},{},true});
	const auto fixtureFor=[&](unsigned species,const XeenMonsterFixturePlacement &at) {
		XeenMonsterFixture fixture{species,at.record,{}};
		if (at.relocated) fixture.spawn=std::make_pair(at.monsterX,at.monsterY);
		return fixture;
	};
	// Cleric of Yak (17): Electrical ranged, no special; Lava Golem (65): Fire
	// with Break Weapon; Giant Bat (1) flying, moved to the water edge.
	const auto fixtureJourney=[&](unsigned species,const XeenMonsterFixturePlacement &placement) {
		XeenMonsterFixtureDescription d;
		const unsigned record=placement.record;
		const auto bytes=xeenMonsterFixtureMob(original,fixtureFor(species,placement),in.statistics,&d);
		const auto entities=XeenMapFormat::parseMob(bytes);
		check(entities.monsters.size()==originalEntities.monsters.size() && entities.monsters[record].resourceId==int(species) &&
			d.name==in.statistics[species].name() && d.x==placement.monsterX && d.y==placement.monsterY &&
			entities.monsters[record].x==placement.monsterX && entities.monsters[record].y==placement.monsterY,"Fixture retargets the chosen record");
		for (unsigned n=0;n<entities.monsters.size();++n) if (n!=record) check(entities.monsters[n].resourceId==originalEntities.monsters[n].resourceId &&
			entities.monsters[n].x==originalEntities.monsters[n].x && entities.monsters[n].y==originalEntities.monsters[n].y,"Other placements unchanged");
		const auto objectLoader=[&](XeenMapIdentity id) {
			return in.maps.loadObjects([&](const std::string &name)->std::optional<Bytes>{
				if (name=="maze0023.mob") return bytes;
				if (!in.assets.hasInitialResource(name)) return std::nullopt;return in.assets.readInitialResource(name);},id);
		};
		const XeenRegionalManifest manifest=[&](const auto &m,const auto &o,const auto &e,const auto &s) {
			xeenValidateRegionalManifest(m,o,e,s,in.assets.readInitialResource("maze0023.dat"),bytes,in.assets.readInitialResource("maze0023.evt"));
		};
		// Parsing, the manifest, a fresh Journey and reconstruction all use the fixture bytes.
		XeenWorld w(in.mapLoader(),objectLoader);
		manifest(w.map(23),w.objectFile(23),in.mainland,in.statistics);
		auto p=XeenPartyLoader().loadInitialParty(in.assets);auto c=placement.start;XeenGameFlags f;
		auto setup=in.setup();setup.regionalManifest=manifest;setup.start=placement.start;
		XeenEncounterFlow flow(programState,w,p,c,f,[]{return 0;},setup);
		check(flow.prepareJourneyFrame(flow.ticket(),[]{}) && flow.presentJourney(flow.ticket()),"fixture Journey frame");
		const auto &actors=w.sessionState().monsters(23);
		check(actors.size()==originalEntities.monsters.size() && actors[record].original.resourceId==int(species) && actors[record].statistics->name()==in.statistics[species].name(),"Fixture species is the live actor at the chosen record");
		check(c.x==placement.start.x && c.y==placement.start.y && c.direction==placement.start.direction,"The Journey starts at the chosen cell");
		const auto view=XeenActorApproach::classify(actors,c);bool seen=false;
		for (unsigned slot=3*placement.distance;slot<3*placement.distance+3;++slot) seen=seen || view.slots[slot]==XeenMonsterIdentity{23,record};
		check(seen,"The chosen monster is in view straight ahead at the start");
		const auto saved=XeenSaveState::capture(in.signature,p,c,f,w);
		XeenWorld restored(in.mapLoader(),objectLoader);XeenPartyState rp;XeenCamera rc;XeenGameFlags rf;
		auto resources=in.resources();resources.regionalManifest=manifest;
		XeenSaveState::restoreBeforeGameplay(saved,resources,rp,rc,rf,restored,[](auto &,const auto &,const auto &,const auto &){});
		XeenEncounterFlow again(programState,restored,rp,rc,rf,[]{return 0;},XeenJourneyRestoreTag{});
		check(again.prepareJourneyFrame(again.ticket(),[]{}) && again.presentJourney(again.ticket()),"restored fixture frame");
		check(XeenSaveFormat::encode(XeenSaveState::capture(in.signature,rp,rc,rf,restored))==XeenSaveFormat::encode(saved),"Fixture round trip through the same provider");
		// The original resource no longer matches a fixture world.
		rejects([&]{xeenValidateRegionalManifest(w.map(23),w.objectFile(23),in.mainland,in.statistics,in.assets.readInitialResource("maze0023.dat"),original,in.assets.readInitialResource("maze0023.evt"));},"fixture world differs from the original manifest");
	};
	fixtureJourney(17,placement);fixtureJourney(65,placement);fixtureJourney(1,edge);
	rejects([&]{xeenMonsterFixtureMob(original,{0,0,{}},in.statistics);},"Slime's animation loop is refused by the mainland admission");
	rejects([&]{xeenMonsterFixtureMob(original,{17,255,{}},in.statistics);},"record outside the placements");
	// Part B: flyers and species 59 are admitted.
	check(in.statistics[1].flying() && xeenMonsterMovementSupported(in.statistics[1],1) && xeenMonsterMovementSupported(in.statistics[59],59),"Giant Bat and Water Dragon move");
	(void)xeenMonsterFixtureMob(original,{59,placement.record,{}},in.statistics);
	rejects([&]{xeenMonsterFixturePlacement(map23,originalEntities,{1,{},xeenJourneyContent().entry,true});},"--water-edge with --start");
	{
		// The cell right ahead of the water-edge start is water a walker cannot
		// enter; the bat crosses it into contact over the original geometry while
		// an Orc (6) from the same spawn is held.
		constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};const auto d=unsigned(edge.start.direction);
		const int ax=edge.start.x+dx[d],ay=edge.start.y+dy[d];
		const auto &ahead=xeenGet<XeenOutdoorLayers>(map23.geometry.cells[ay*16+ax].geometry);
		const auto surface=map23.geometry.surfaceTypes[ahead.surface];
		check(edge.relocated && edge.inView && edge.distance>=2 && !xeenMonsterFixtureStartRefusal(map23,originalEntities,edge.start) && (surface==0 || surface==8) &&
			edge.monsterX==edge.start.x+dx[d]*int(edge.distance) && edge.monsterY==edge.start.y+dy[d]*int(edge.distance),
			"The water-edge start faces water with the monster in view");
		const auto approach=[&](unsigned species) {
			const auto bytes=xeenMonsterFixtureMob(original,fixtureFor(species,edge),in.statistics);
			auto actors=XeenActorApproach::actorsFromResources({23,"fixture",true,XeenMapFormat::parseMob(bytes)},in.statistics);
			for (auto &other:actors) other.activated=false;
			actors.at(edge.record).activated=true;
			bool overWater=false;
			for (unsigned step=0;step<8 && !(actors[edge.record].x==edge.start.x && actors[edge.record].y==edge.start.y);++step) {
				actors=XeenActorApproach::move(actors,edge.start,[&](const XeenActor &m,int x,int y){return xeenRegionalActorTerrain(map23,m,x,y);},true,
					[](const std::vector<XeenActor> &,std::size_t){});
				const auto &a=actors[edge.record];
				const auto &l=xeenGet<XeenOutdoorLayers>(map23.geometry.cells[a.y*16+a.x].geometry);
				const auto t=map23.geometry.surfaceTypes[l.surface];overWater=overWater || t==0 || t==8;
			}
			return std::make_pair(actors[edge.record],overWater);
		};
		const auto bat=approach(1);
		check(bat.first.x==edge.start.x && bat.first.y==edge.start.y && bat.second,"The Giant Bat crossed the water into contact on the original map");
		const auto orc=approach(6);
		check(!orc.second && !(orc.first.x==edge.start.x && orc.first.y==edge.start.y),"An Orc from the same spawn is held by the water");
		std::cout<<"Water-edge fixture placement: record "<<edge.record<<" at ("<<edge.monsterX<<','<<edge.monsterY<<"), start ("<<edge.start.x<<','<<edge.start.y<<") facing "<<unsigned(edge.start.direction)<<", distance "<<edge.distance<<'\n';
	}
	std::cout<<"Default fixture placement: record "<<placement.record<<" at ("<<placement.monsterX<<','<<placement.monsterY<<"), start ("<<placement.start.x<<','<<placement.start.y<<") facing "<<unsigned(placement.start.direction)<<", distance "<<placement.distance<<'\n';
	std::cout<<"Original-data monster fixture passed\n";
}
}
int main(int argc,char **argv) {
	try {
		if (argc>1) { originalFixture(std::filesystem::u8path(argv[1]));return 0; }
	} catch (const std::exception &e) { std::cerr<<"originalFixture: "<<e.what()<<'\n';return 1; }
	const std::pair<const char *,void(*)()> sections[]{{"mixedVolleyDrainsSp",mixedVolleyDrainsSp},{"publishedEffects",publishedEffects},{"combatPublication",combatPublication},
		{"combatVolley",combatVolley},{"restInterruption",restInterruption},{"survivingRoundTrips",survivingRoundTrips},
		{"flightToContact",flightToContact},{"restActivation",restActivation}};
	for (const auto &[name,run]:sections) {
		try { run(); } catch (const std::exception &e) { std::cerr<<name<<": "<<e.what()<<'\n';return 1; }
	}
	std::cout<<"Monster ability gameplay publication passed\n";return 0;
}

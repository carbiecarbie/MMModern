// Read-only original-resource initialization/restore checks and separately
// identified synthetic callback/ownership faults; no gameplay witness injection.
#include "app/XeenEncounterFlow.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "formats/xeen/XeenMonsterFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenSaveState.h"
#include "platform/XeenSaveFile.h"
#include "XeenSaveTestSupport.h"
#include <iostream>
#include <memory>
#include <cstdlib>
#include <new>

namespace allocation_test {
long countdown=-1;
bool triggered=false;
const mmodern::XeenPartyState *publication=nullptr;
}
void *operator new(std::size_t size) {
	if(allocation_test::publication && allocation_test::publication->serviceEconomy)allocation_test::countdown=-1;
	if(allocation_test::countdown>=0 && allocation_test::countdown--==0) {
		allocation_test::countdown=-1;allocation_test::triggered=true;throw std::bad_alloc();
	}
	if(auto *value=std::malloc(size?size:1))return value;throw std::bad_alloc();
}
void *operator new[](std::size_t size) {return ::operator new(size);}
void operator delete(void *value) noexcept {std::free(value);}
void operator delete[](void *value) noexcept {std::free(value);}
void operator delete(void *value,std::size_t) noexcept {std::free(value);}
void operator delete[](void *value,std::size_t) noexcept {std::free(value);}

using namespace mmodern;
using save_test::check;
using save_test::rejects;
namespace {
struct Inputs {
	XeenAssetSource assets;
	XeenMapLoader maps;
	XeenEventLoader events;
	XeenEventTextLoader texts;
	std::vector<std::uint8_t> chr,pty;
	std::vector<XeenMonsterRecord> statistics;
	XeenEventFile mainlandEvents;
	XeenLearnedSpellNames names;
	XeenSaveResourceSignature signature;
	explicit Inputs(const GameInstallation &installation) : assets(installation),
		events([&](const auto &name)->std::optional<std::vector<std::uint8_t>> {
			if(!assets.hasInitialResource(name))return {};return assets.readInitialResource(name);}),
		texts([&](const auto &name)->std::optional<std::vector<std::uint8_t>> {
			if(!assets.hasArchiveResource(name))return {};return assets.readArchiveResource(name);}) {
		chr=assets.readInitialResource("maze.chr");pty=assets.readInitialResource("maze.pty");
		const auto mon=assets.readCloudsMonsterStatisticsFromDarkArchive();check(bool(mon),"original MON absent");
		statistics=XeenMonsterFormat::parse(*mon);mainlandEvents=events.load(23);
		const auto spellNames=assets.readLearnedSpellNamesFromDarkArchive();check(bool(spellNames),"original spell names absent");
		names=XeenLearnedSpellNames::parse(*spellNames);
		signature=XeenSaveFile::fingerprint(installation);
	}
	XeenPartyState ordinary() { return XeenPartyLoader().loadInitialCloudsParty(assets); }
	XeenRegionalManifest regional() {
		return [&](const auto &m,const auto &o,const auto &e,const auto &s) {
			xeenValidateRegionalManifest(m,o,e,s,assets.readInitialResource("maze0023.dat"),
				assets.readInitialResource("maze0023.mob"),assets.readInitialResource("maze0023.evt"));};
	}
	XeenVertigoManifest vertigo() {
		return [&](auto &w,const auto &e,const auto &s) {xeenValidateVertigoManifest(w,e,s,[&](const auto &name) {
			return name.rfind("maze",0)==0?assets.readInitialResource(name):assets.readArchiveResource(name);});};
	}
	XeenJourneySetup setup() {
		XeenJourneySetup out{chr,XeenGameplayContextFormat::parse(pty),statistics,mainlandEvents,3626689381u,regional()};
		out.purse=XeenCharacterFormat::parseMonsterPurse(pty);
		out.regionalRecovery=XeenQuestFlagFormat::parseRegionalRecovery(pty);out.regionalText=texts.load(23);
		out.learnedNames=names;out.learnedNamesProvider=[&]{return names;};out.vertigoManifest=vertigo();
		out.bank=XeenCharacterFormat::parseBankBalances(pty);
		out.cityEventsProvider=[&]{return events.load(28);};
		return out;
	}
	XeenSaveState::Resources restoreResources(unsigned &freshCalls) {
		XeenSaveState::Resources r;
		r.signature=signature;r.loadEvents=[&](auto id){return events.load(id);};
		r.loadMonsterStatistics=[&]{return statistics;};r.regionalManifest=regional();r.vertigoManifest=vertigo();
		r.loadRegionalText=[&](auto id){return texts.load(id);};r.loadLearnedSpellNames=[&]{return names;};
		// A restored graph may read immutable resources, never any fresh inputs.
		r.loadInitialParty=[&]{++freshCalls;throw std::runtime_error("restore called fresh party");return XeenPartyState{};};
		r.loadInitialCharacters=[&]{return chr;};
		r.loadInitialContext=[&]{++freshCalls;throw std::runtime_error("restore called fresh context");return XeenGameplayContext{};};
		r.loadInitialPurse=[&]{++freshCalls;throw std::runtime_error("restore called fresh purse");return XeenMonsterTreasure{};};
		r.loadInitialRegionalRecovery=[&]{++freshCalls;throw std::runtime_error("restore called fresh recovery");return XeenRegionalRecoveryState{};};
		r.loadInitialBankBalances=[&]{++freshCalls;throw std::runtime_error("restore called fresh bank");return XeenBankBalances{};};
		return r;
	}
};

struct Graph {
	XeenPartyState p;
	XeenCamera c{23,9,11,XeenDirection::West};XeenGameFlags flags;
	std::function<void()> mapFault,objectFault;
	unsigned objectCalls=0;
	XeenWorld world;
	std::unique_ptr<XeenEncounterFlow> flow;
	explicit Graph(Inputs &i,bool initial=true) : world([&](auto id){if(mapFault)mapFault();return i.maps.loadGeometryMap(i.assets,id);},
		[&](auto id){++objectCalls;if(objectFault)objectFault();return i.maps.loadObjects(i.assets,id);}) {
		if(initial)p=i.ordinary();
	}
	void present() {check(flow && flow->prepareJourneyFrame(flow->ticket(),[]{}) && flow->presentJourney(flow->ticket()),"headless concrete Journey frame");}
	void fresh(const XeenJourneySetup &setup) {flow=std::make_unique<XeenEncounterFlow>(world,p,c,flags,[]{return 0;},setup);present();}
	XeenSaveSnapshot capture(Inputs &i) {return XeenSaveState::capture(i.signature,p,c,flags,world);}
	void restore(Inputs &i,const XeenSaveSnapshot &s,unsigned &freshCalls,const XeenSaveState::Preflight &extra={}) {
		XeenSaveState::restoreBeforeGameplay(s,i.restoreResources(freshCalls),p,c,flags,world,
			[&](auto &w,const auto &p,const auto &c,const auto &f) {if(extra)extra(w,p,c,f);});
		flow=std::make_unique<XeenEncounterFlow>(world,p,c,flags,[]{return 0;},XeenJourneyRestoreTag{});present();
	}
};

void parserAndFresh(Inputs &i) {
	const auto ordinary=i.ordinary();check(!ordinary.serviceEconomy,"ordinary PartyLoader initializes merchant economy");
	const auto bank=XeenCharacterFormat::parseBankBalances(i.pty);check(!bank.gold&&!bank.gems,"original bank is not zero");
	for(unsigned size:{0u,645u,646u,649u,650u,653u,654u,811u,813u}) {
		auto shortInput=i.pty;shortInput.resize(size);rejects([&]{XeenCharacterFormat::parseBankBalances(shortInput);});
	}
	for(unsigned offset:{0u,28u,603u,638u,642u,646u,650u,811u}) {
		auto changed=i.pty;changed[offset]^=1;rejects([&]{XeenCharacterFormat::parseBankBalances(changed);});
	}
	Graph source(i);source.fresh(i.setup());const auto s=source.capture(i);
	check(s.journey->schema==9 && s.journey->content==14 && s.journey->serviceEconomy &&
		s.journey->random==std::optional<XeenJourneyRandomState>{{1,7,886}},"fresh post-generation world cursor differs from independent vector");
	check(s.journey->context->year==610 && s.journey->context->day==8 && s.journey->context->minutes==480 &&
		!s.journey->context->ctr24 && !s.journey->serviceEconomy->bank.gold && !s.journey->serviceEconomy->bank.gems,"fresh bank/calendar mismatch");
	check(s.activeRosterIds==std::vector<std::uint8_t>{0,18,14,11,1,6},"fresh ordered membership");
	const std::array<std::array<unsigned,4>,8> counts{{{{8,5,1,2}},{{8,8,2,1}},{{5,8,3,1}},{{8,6,3,1}},
		{{7,8,1,3}},{{5,8,2,2}},{{7,8,1,2}},{{8,4,3,1}}}};
	for(unsigned shop=0;shop<8;++shop)for(unsigned category=0;category<4;++category) {
		unsigned occupied=0;for(const auto &item:s.journey->serviceEconomy->wares[shop/4][shop%4][category])occupied+=item.id!=0;
		check(occupied==counts[shop][category],"fresh complete eight-shop independent count vector");
	}
	for(unsigned owner=0;owner<30;++owner) {
		const auto &c=source.p.roster.at(owner);const auto &initial=ordinary.roster.at(owner);
		check(xeen_state::sameItemCategory(c.weapons,initial.weapons) && xeen_state::sameItemCategory(c.armor,initial.armor) &&
			xeen_state::sameItemCategory(c.accessories,initial.accessories) && xeen_state::sameItemCategory(c.miscellaneous,initial.miscellaneous),"fresh initialization changed original carried items");
		check(c.learnedSpells==XeenCharacterFormat::parseLearnedSpells(i.chr,owner),"fresh thirty raw learned books");
		if(std::find(s.activeRosterIds.begin(),s.activeRosterIds.end(),owner)!=s.activeRosterIds.end())
			check(c.currentHp==XeenCharacterRules::maxHp(c,{610}) && c.currentSp==XeenCharacterRules::maxSp(c,{610}),"fresh prepared HP/SP mismatch");
		else check(c.currentHp==initial.currentHp && c.currentSp==initial.currentSp,"fresh inactive HP/SP changed");
	}
	check(s.journey->treasure->gold==800 && s.journey->treasure->gems==10,"fresh carried purse changed");
	std::cout<<"M40 original PTY/ordinary-loader, complete fresh post-generation cursor/stock, thirty original owners initialization checks passed\n";
}

void freshFailures(Inputs &i) {
	for(unsigned mode=0;mode<7;++mode) {
		Graph g(i);auto setup=i.setup();
		if(mode==0)setup.bank.reset();
		if(mode==1)setup.bank->gold=1;
		if(mode==2)setup.bank->gems=1;
		if(mode==3)g.objectFault=[] {throw std::runtime_error("synthetic original MOB I/O");};
		if(mode==4)setup.regionalManifest=[](const auto &,const auto &,const auto &,const auto &) {throw std::bad_alloc();};
		if(mode==5 || mode==6) {
			const auto manifest=setup.regionalManifest;
			setup.regionalManifest=[&,manifest,mode](const auto &m,const auto &o,const auto &e,const auto &s) {
				manifest(m,o,e,s);
				if(mode==5) {g.p.serviceEconomy.emplace();auto &v=g.p.serviceEconomy->wares[1][3][3][8].state;++v;--v;g.p.serviceEconomy.reset();}
				else {g.world.discardMapCache();g.objectFault=[] {throw std::runtime_error("synthetic post-stock MOB I/O");};}
			};
		}
		rejects([&]{g.fresh(setup);});
		check(!g.p.serviceEconomy && !g.p.encounterContext && !g.p.roster.combatMarked() &&
			!g.world.sessionState().journeyRandom() && g.world.sessionState().actors().empty() &&
			!XeenSaveState::canCapture(g.p,g.c,g.world),"fresh failure published usable partial stock/cursor/owner graph");
		if(mode==6)check(g.objectCalls==2,"post-stock failure did not reach reconstructed MOB provider");
	}
	// Setup input is detached before provider hooks. Parameter mutation cannot
	// become a second source of original bank, seed or stock authority.
	Graph g(i);auto setup=i.setup();const auto manifest=setup.regionalManifest;
	setup.regionalManifest=[&](const auto &m,const auto &o,const auto &e,const auto &s) {
		manifest(m,o,e,s);setup.bank->gold=199;setup.bank->gems=200;setup.seed=1;};
	g.fresh(setup);check(!g.p.serviceEconomy->bank.gold && !g.p.serviceEconomy->bank.gems &&
		g.world.sessionState().journeyRandom()==std::optional<XeenJourneyRandomState>{{1,7,886}},"fresh provider replaced detached initialization inputs");
	std::cout<<"M40 SYNTHETIC fresh absent/nonzero bank, resource/allocation/ABA/post-stock faults and detached-input checks passed\n";
}

void restoreAndFinalOwners(Inputs &i) {
	Graph source(i);source.fresh(i.setup());auto saved=source.capture(i);
	// Nonzero balances test exact persistence independently of production's zeros.
	saved.journey->serviceEconomy->bank.gold=4252442868u;saved.journey->serviceEconomy->bank.gems=0xffffffffu;
	const auto bytes=XeenSaveFormat::encode(saved);
	for(unsigned mutation=0;mutation<8;++mutation)for(bool exceptional:{false,true}) {
		Graph d(i,false);unsigned fresh=0;
		rejects([&]{d.restore(i,saved,fresh,[&](auto &,const auto &p,const auto &,const auto &) {
			check(p.serviceEconomy==saved.journey->serviceEconomy,"restore changed saved bank/stock before preflight");
			auto &economy=const_cast<XeenPartyState &>(p).serviceEconomy;
			if(mutation<4) {auto &item=economy->wares[1][3][3][8];auto &v=mutation==0?item.material:mutation==1?item.id:mutation==2?item.state:item.frame;++v;--v;}
			if(mutation==4){++economy->bank.gold;--economy->bank.gold;}
			if(mutation==5){++economy->bank.gems;--economy->bank.gems;}
			if(mutation==6){const auto old=economy;economy.reset();economy=old;}
			if(mutation==7){auto other=*economy;other.bank.gold=0;using std::swap;swap(other,*economy);swap(other,*economy);}
			if(exceptional)throw std::runtime_error("synthetic callback after owner ABA");
		});});
		check(fresh==0 && !d.p.serviceEconomy && !d.world.hasEncounterState(),"failed restore published or called fresh providers");
	}
	for(unsigned mutation=0;mutation<7;++mutation) {
		Graph d(i,false);unsigned fresh=0;d.restore(i,XeenSaveFormat::decode(bytes),fresh);
		save_test::sameSnapshot(saved,d.capture(i));check(bytes==XeenSaveFormat::encode(d.capture(i)) && fresh==0,"restore full bytes/fields/fresh-input isolation");
		auto &economy=d.p.serviceEconomy;
		if(mutation<4){auto &item=economy->wares[1][3][3][8];auto &v=mutation==0?item.material:mutation==1?item.id:mutation==2?item.state:item.frame;++v;--v;}
		if(mutation==4){++economy->bank.gold;--economy->bank.gold;}
		if(mutation==5){++economy->bank.gems;--economy->bank.gems;}
		if(mutation==6){const auto old=economy;economy.reset();economy=old;}
		check(!d.flow->journeyQuiet()&&!XeenSaveState::canCapture(d.p,d.c,d.world),"final-owner merchant/bank ABA revived Quiet capture");
		rejects([&]{d.capture(i);});
	}
	std::cout<<"M40 SYNTHETIC exact nonzero bank restore, no fresh-provider replay, exceptional preflight ABA and final-owner stock/bank guards passed\n";
}
void postPublicationFailure(Inputs &i) {
 for(bool manifest:{false,true}) {
  Graph g(i);auto setup=i.setup();bool fired=false;
  const auto fail=[&]{fired=true;check(g.p.serviceEconomy && g.p.encounterContext && g.p.roster.combatMarked(),"Training validation did not follow fresh publication");throw std::bad_alloc();};
  if(manifest)setup.vertigoManifest=[&](auto &,const auto &,const auto &){fail();};
  else setup.cityEventsProvider=[&]()->XeenEventFile{fail();return {};};
  rejects([&]{g.fresh(setup);});
  check(fired && g.p.serviceEconomy && g.p.encounterContext && !XeenSaveState::canCapture(g.p,g.c,g.world),"failed Training admission left save authority");
  check(g.world.sessionState().journeyRandom()==std::optional<XeenJourneyRandomState>{{1,7,886}},"failed Training admission replayed fresh RNG");
 }
}
void freshPublicationAllocationSweep(Inputs &i) {
	// Exercise every allocating preparation from the admitted resource callback
	// through detached stock generation, final-owner guard and capture storage.
	// A failure at any allocation must precede party/economy/RNG publication.
	for(unsigned skipped=0;skipped<1000;++skipped) {
		Graph g(i);auto setup=i.setup();const auto manifest=setup.regionalManifest;
		setup.regionalManifest=[&](const auto &m,const auto &o,const auto &e,const auto &s) {
			manifest(m,o,e,s);allocation_test::triggered=false;allocation_test::countdown=skipped;
		};
		bool failed=false;allocation_test::publication=&g.p;
		try {g.flow=std::make_unique<XeenEncounterFlow>(g.world,g.p,g.c,g.flags,[]{return 0;},setup);}
		catch(const std::bad_alloc &) {failed=true;}
		allocation_test::countdown=-1;allocation_test::publication=nullptr;
		if(!failed) {
			check(!allocation_test::triggered && skipped>0,"allocation sweep did not cover failing preparation");
			g.present();check(g.p.serviceEconomy && g.world.sessionState().journeyRandom()==std::optional<XeenJourneyRandomState>{{1,7,886}},"allocation-sweep final fresh state");
			std::cout<<"M40 SYNTHETIC fresh publication allocation sweep passed "<<skipped<<" prepublication failure positions\n";return;
		}
		check(allocation_test::triggered && !g.p.serviceEconomy && !g.p.encounterContext &&
			!g.p.roster.combatMarked() && !g.world.sessionState().journeyRandom() && g.world.sessionState().actors().empty() &&
			!XeenSaveState::canCapture(g.p,g.c,g.world),"fallible final-owner preparation happened after fresh publication");
	}
	throw std::runtime_error("fresh publication allocation sweep bound exhausted");
}
}
int main(int argc,char **argv) {
	try {
		if(argc!=2)throw std::invalid_argument("usage: mmodern_service_day_initialization_tests <original-installation>");
		const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"original installation absent");
		Inputs inputs(*installation);parserAndFresh(inputs);freshFailures(inputs);restoreAndFinalOwners(inputs);freshPublicationAllocationSweep(inputs);postPublicationFailure(inputs);
		return 0;
	}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}

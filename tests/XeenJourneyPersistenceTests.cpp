#include "games/xeen/XeenJourneyRules.h"
#include "XeenJourneyTestSupport.h"
#include "XeenRestoreReplayProbe.h"
#include "XeenSaveGameplayTestSupport.h"
#include "games/xeen/XeenOutdoorScene.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenStateEquality.h"
#include <fstream>
#include <iostream>
#include <new>
#include "XeenChildProcessTestSupport.h"
using namespace combat_test;
namespace {
using journey_test::Fixture;
XeenSaveSnapshot capture(Fixture &f) { return XeenSaveState::capture(save_test::sample().resources,f.p,f.camera,f.flags,f.w); }
XeenSaveSnapshot moved() {
	Fixture f; f.action(XeenEncounterAction::Right); f.action(XeenEncounterAction::Forward);
	f.pulse(); f.pulse(); f.pulse();
	check(f.camera.x == 14 && f.camera.y == 1 && f.camera.direction == XeenDirection::East &&
		f.w.sessionState().actors()[5].x == 13 && f.w.sessionState().actors()[5].y == 1 &&
		f.p.encounterContext->minutes == 490 && f.p.encounterContext->ctr24 == 2, "literal moved source");
	return capture(f);
}
XeenSaveSnapshot ended() {
	Fixture f;
	f.flow->journeyTransfer(f.flow->ticket(),5,0,XeenInventoryCategory::Accessories,1); f.present();
	f.flow->journeyEquipment(f.flow->ticket(),0,XeenInventoryCategory::Accessories,1,XeenEquipmentOperation::Equip); f.present();
	f.engage(); for (unsigned i=0;i<6;++i) f.command(Command::Block);
	auto *combat=f.flow->combat();
	while (combat->pending()==Work::Enemy) combat->service(combat->ticket());
	combat->service(combat->ticket()); f.command(Command::Attack); f.command(Command::Attack);
	check(combat->phase()==Phase::VictoryAwaitingEnd, "literal lethal source");
	rejects([&]{capture(f);});
	check(combat->service(combat->ticket()).status==Status::Victory,"genuine End");
	rejects([&]{capture(f);});
	check(f.flow->retireJourney(f.flow->ticket()),"genuine retirement"); rejects([&]{capture(f);}); f.present();
	check(f.p.encounterContext->minutes==492 && f.p.roster.at(1).currentHp==-17 &&
		f.p.roster.at(1).conditions[12]==1 && f.p.roster.at(1).conditions[13]==1,"literal retired injuries");
	f.flow->journeyTransfer(f.flow->ticket(),0,5,XeenInventoryCategory::Accessories,1); f.present();
	return capture(f);
}
struct Destination {
	XeenPartyState p;
	XeenCamera c{20,14,2,XeenDirection::West};
	XeenGameFlags f;
	std::function<void(unsigned)> observer;
	unsigned calls=0, legacy=0;
	void observe(unsigned seam) { ++calls; if(observer) observer(seam); }
	XeenWorld w{[&](XeenMapIdentity id) { observe(0); auto m=map(); m.geometry.id=id.number; return m; },
		[&](XeenMapIdentity id) { observe(1); auto m=objects(); m.mapId=id; return m; }};
	XeenEventPresenter::Clock clock=[] { return 0; };
	std::unique_ptr<XeenEncounterFlow> flow;
	XeenSaveState::Resources resources() {
		return {save_test::sample().resources,[&] { ++legacy; return XeenPartyLoader().loadFromResources(chr(),pty()); },
			[&](XeenMapIdentity id) { observe(2); auto e=events(); e.mapId=id; e.records.emplace_back(); return e; },
			[&] { ++legacy; return chr(); },[&] { ++legacy; return XeenGameplayContextFormat::parse(pty()); },
			[&] { observe(3); return statistics(); }};
	}
	static void compose(XeenWorld &w,const XeenPartyState &p,const XeenCamera &c,const XeenGameFlags &) {
		check(w.sessionState().journey() && !XeenSaveState::canCapture(p,c,w),"preflight has no capture authority");
		check(CloudsUiComposer::buildPortraitPlacements(p).size()==6,"Journey portraits");
		check(!XeenOutdoorScene().build(w,c,nullptr,nullptr,0,XeenMonsterAppearance{0}).empty(),"Journey saved scene with actors");
		const auto view=XeenActorApproach::classify(w.sessionState().actors(),c);
		check(!view.engaged(),"quiet restored view");
	}
	void restore(const XeenSaveSnapshot &s, XeenSaveState::Preflight extra={}) {
		replay_test::Scope scope;
		XeenSaveState::restoreBeforeGameplay(s,resources(),p,c,f,w,[&](auto &w,const auto &p,const auto &c,const auto &f) {
			observe(4); compose(w,p,c,f); if(extra) extra(w,p,c,f);
		});
		check(!XeenSaveState::canCapture(p,c,w),"publication is unbound");
	}
	void bind() {
		replay_test::Scope scope;
		flow=std::make_unique<XeenEncounterFlow>(w,p,c,f,clock,XeenJourneyRestoreTag{});
		check(!XeenSaveState::canCapture(p,c,w),"restored binding awaits frame");
		check(flow->prepareJourneyFrame(flow->ticket(),[] {}) && flow->presentJourney(flow->ticket()),"restored frame handoff");
	}
	XeenSaveSnapshot capture() { return XeenSaveState::capture(save_test::sample().resources,p,c,f,w); }
};
void same(const XeenSaveSnapshot &s, Destination &d) {
	save_test::sameSnapshot(s,d.capture());
	check(d.p.firstSerializedCount==6 && d.p.effectiveSerializedCount==6 && d.legacy==0,"fixed counts, no CHR/PTY reinitialization");
	for(unsigned i=0;i<30;++i) check(xeen_state::sameInputs(s.journey->supplements[i].inputs,*d.p.roster.combatInputs(i)),"all owner supplements");
	check(d.p.encounterContext==s.journey->context && d.w.sessionState().skeletonSeed()==s.journey->skeletonSeed,"context/seed exact");
	const auto mob=objects(); const auto mon=statistics();
	for(unsigned i=0;i<27;++i) {
		XeenActor expected; expected.id={20,i}; expected.original=mob.entities.monsters[i];
		expected.x=expected.original.x; expected.y=expected.original.y;
		if(expected.original.hasResource()) { expected.statistics=mon.at(expected.original.resourceId); expected.hp=expected.statistics->baseHp(); expected.lifecycle=XeenActorLifecycle::Present; }
		if(expected.original.isDisabled()) expected.lifecycle=XeenActorLifecycle::Disabled;
		if(i==5) { const auto &a=s.journey->actors[0]; expected.x=a.x;expected.y=a.y;expected.hp=a.hp;expected.activated=a.activated;expected.lifecycle=a.lifecycle;expected.status=a.status; }
		check(xeen_state::sameActor(expected,d.w.sessionState().actors()[i]),"independent complete actor oracle");
	}
	check(XeenSaveFormat::encode(s)==XeenSaveFormat::encode(d.capture()),"all exact detached fields");
}
void roundtrips() {
	for(auto s:{moved(),ended()}) {
		for(unsigned i=0;i<30;++i) {
			s.journey->supplements[i].inputs.experience=i==0?0:i==29?0xfffffff0u:1000+i;
			s.characters[i].miscellaneous[3]={255,37,129,254}; s.characters[i].miscellaneous[6]={86,0,7,12};
			s.characters[i].currentSp=-100-static_cast<int>(i);
		}
		s.characters[29].intellect.permanent=-12345; s.characters[29].conditions[0]=255;
		s.characters[0].currentHp=-300; s.characters[0].conditions[12]=1;
		s.characters[0].accessories[7]={105,1,0,8}; // Historical injury, Journey-valid but melee-unready.
		s.questItems[17]=7; s.questFlags[2]=true; s.gameFlags[7]=true;
		s.disabledObjects={{20,0},{21,0}}; s.disabledEvents={{21,0}};
		for(unsigned pass=0;pass<3;++pass) {
			Destination d; d.restore(XeenSaveFormat::decode(XeenSaveFormat::encode(s))); d.bind(); same(s,d);
			rejects([&]{xeenValidateJourneyMelee(d.p);});
			rejects([&]{XeenPartyState copy(d.p);});
			s=d.capture();
		}
	}
	Destination movedOwner; auto s=moved(); movedOwner.restore(s);
	{
		replay_test::Scope scope;
		XeenPartyState otherParty; XeenCamera otherCamera=movedOwner.c; XeenGameFlags otherFlags=movedOwner.f;
		rejects([&]{XeenEncounterFlow wrong(movedOwner.w,otherParty,movedOwner.c,movedOwner.f,movedOwner.clock,XeenJourneyRestoreTag{});});
		rejects([&]{XeenEncounterFlow wrong(movedOwner.w,movedOwner.p,otherCamera,movedOwner.f,movedOwner.clock,XeenJourneyRestoreTag{});});
		rejects([&]{XeenEncounterFlow wrong(movedOwner.w,movedOwner.p,movedOwner.c,otherFlags,movedOwner.clock,XeenJourneyRestoreTag{});});
		check(!XeenSaveState::canCapture(movedOwner.p,movedOwner.c,movedOwner.w),"wrong owners cannot consume restoration handoff");
	}
	movedOwner.bind(); same(s,movedOwner);
	check(movedOwner.flow->journeyAction(movedOwner.flow->ticket(),XeenEncounterAction::Wait).outcome==XeenEncounterOutcome::Engaged &&
		movedOwner.p.encounterContext->minutes==500 && movedOwner.p.encounterContext->ctr24==3,"explicit Wait after restore uses moved state");
}
void repair(Bytes &b) {
	const auto put=[&](unsigned at,std::uint32_t v) {for(unsigned i=0;i<4;++i)b[at+i]=static_cast<std::uint8_t>(v>>(8*i));};
	put(12,static_cast<std::uint32_t>(b.size()-20));
	put(16,static_cast<std::uint32_t>(crc32(0,b.data()+20,static_cast<uInt>(b.size()-20))));
}
void wire() {
	auto s=moved(); auto ordinary=s; ordinary.journey.reset();
	const auto base=XeenSaveFormat::encode(ordinary); const auto bytes=XeenSaveFormat::encode(s); const auto B=base.size();
	check(bytes[8]==4 && bytes[9]==0 && base[8]==2 && bytes.size()==B+1060,"literal v4 size/version/EOF");
	check(std::equal(base.begin()+20,base.end(),bytes.begin()+20),"unchanged v2 base");
	Bytes expected(1060,0);
	const auto put=[&](unsigned at,std::uint32_t value,unsigned width) {for(unsigned i=0;i<width;++i)expected[at+i]=static_cast<std::uint8_t>(value>>(8*i));};
	put(0,3,1);put(1,1,2);put(3,1,2);put(5,1,1);put(8,2,2);put(10,1,2);put(12,610,2);put(14,490,2);put(39,30,1);
	// Independently read the synthetic CHR physical fields, not the codec/capture.
	const auto raw=chr();
	for(unsigned i=0;i<30;++i) {const unsigned b=40+i*33;put(b,i,1); unsigned k=1;
		for(unsigned offset:{20u,21u,28u,29u,30u,31u,34u}) {put(b+k,raw[i*354+offset],4);k+=4;}
		put(b+29,0,4);
	}
	put(1030,56,4);put(1034,0,1);put(1035,20,2);put(1037,27,2);put(1039,1,2);
	put(1041,0,1);put(1042,20,2);put(1044,5,4);put(1048,13,2);put(1050,1,2);put(1052,20,4);put(1056,1,1);
	check(std::equal(expected.begin(),expected.end(),bytes.begin()+B),"every literal suffix byte/width/offset");
	const auto diskPath=std::filesystem::temp_directory_path()/("mmodern-journey-wire-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64())+".mmsave");
	XeenSaveFile::write(diskPath,s);
	{std::ifstream file(diskPath,std::ios::binary);const Bytes actual(std::istreambuf_iterator<char>(file),{});
		check(actual.size()==B+1060 && actual[8]==4 && std::equal(expected.begin(),expected.end(),actual.begin()+B),"literal independent disk suffix and exact EOF");}
	std::filesystem::remove(diskPath);
	for(unsigned offset:{0u,1u,3u,5u,6u,7u,37u,38u,39u,40u,73u,42u,1034u,1037u,1039u,1041u,1049u,1055u,1056u,1057u,1058u,1059u}) {
		auto bad=bytes; bad[B+offset]=255; repair(bad); rejects([&]{XeenSaveFormat::decode(bad);});
	}
	for(unsigned size=0;size<1060;++size) {auto bad=bytes;bad.resize(B+size);repair(bad);rejects([&]{XeenSaveFormat::decode(bad);},"extension");}
	auto bad=bytes;bad.push_back(0);repair(bad);rejects([&]{XeenSaveFormat::decode(bad);},"extension");
	bad=bytes;for(unsigned n=0;n<4;++n)bad[B+1030+n]=0;repair(bad);rejects([&]{XeenSaveFormat::decode(bad);},"seed");
	bad=bytes;bad[B+1039]=2;bad.insert(bad.end(),bytes.begin()+B+1041,bytes.end());repair(bad);rejects([&]{XeenSaveFormat::decode(bad);},"extension");
	bad=bytes;bad[B+1059]=1; // Valid boolean, incoherent Present accounting: distinct domain rejection.
	repair(bad);const auto incoherent=XeenSaveFormat::decode(bad);Destination domain;rejects([&]{domain.restore(incoherent);},"durable");
	bad=bytes;bad[8]=5;rejects([&]{XeenSaveFormat::decode(bad);},"version");
	s.completedEncounter.emplace(); rejects([&]{XeenSaveFormat::encode(s);},"exclusive");
}
void captureIntegrity() {
	Fixture f;
	const auto ticket = f.flow->ticket();
	const auto hp = f.p.roster.at(0).currentHp;
	f.p.roster.at(0).currentHp = hp - 1;
	rejects([&] { capture(f); });
	f.p.roster.at(0).currentHp = hp;
	// Do not query Flow before this second capture: direct capture must latch the mismatch itself.
	rejects([&] { capture(f); });
	check(!f.flow->current(ticket), "observed capture integrity failure keeps the original ticket stale");
	check(!f.flow->journeyQuiet(), "equal bytes cannot clear Journey integrity failure");
	rejects([&] { capture(f); });

	Fixture valid;
	XeenPartyState unrelated;
	XeenCamera detached = valid.camera;
	XeenGameFlags detachedFlags;
	const auto validTicket = valid.flow->ticket();
	const auto validHp = valid.p.roster.at(0).currentHp;
	valid.p.roster.at(0).currentHp = validHp - 1;
	rejects([&] { XeenSaveState::capture(save_test::sample().resources,unrelated,valid.camera,valid.flags,valid.w); });
	rejects([&] { XeenSaveState::capture(save_test::sample().resources,valid.p,detached,valid.flags,valid.w); });
	rejects([&] { XeenSaveState::capture(save_test::sample().resources,valid.p,valid.camera,detachedFlags,valid.w); });
	valid.p.roster.at(0).currentHp = validHp;
	check(valid.flow->current(validTicket), "wrong-owner requests do not observe the bound graph's integrity");
	check(capture(valid).journey.has_value(), "unrelated capture requests do not poison bound owners");
	valid.flow->journeyAction(valid.flow->ticket(),XeenEncounterAction::Right);
	rejects([&] { capture(valid); });
	valid.present();
	check(capture(valid).journey.has_value(), "presentation refusal clears through legitimate handoff");
	valid.action(XeenEncounterAction::Forward);
	for (unsigned n : {3u,2u,1u}) {
		rejects([&] { capture(valid); });
		check(valid.flow->state().pending() == n, "capture refusal preserves pending work");
		valid.pulse();
	}
	check(capture(valid).journey.has_value(), "pending refusal does not poison completed approach");
	const auto save = valid.flow->beginJourneySave();
	rejects([&] { capture(valid); });
	check(valid.flow->endJourneySave(save), "capture refusal preserves the current save lease");
	check(capture(valid).journey.has_value(), "save lease refusal is nonpoisoning");
}
void refusals() {
	Fixture flagOwner;XeenGameFlags detachedFlags;
	rejects([&]{XeenSaveState::capture(save_test::sample().resources,flagOwner.p,flagOwner.camera,detachedFlags,flagOwner.w);},"bound game-flag");
	Fixture f; f.action(XeenEncounterAction::Right); f.action(XeenEncounterAction::Forward);
	for(unsigned n:{3u,2u,1u}) {const auto time=f.p.encounterContext;rejects([&]{capture(f);});check(f.flow->state().pending()==n&&time==f.p.encounterContext,"refusal drains nothing");f.pulse();}
	const auto old=f.flow->ticket();auto lease=f.flow->boundary().hold(XeenCombatBoundary::Work::Inventory);
	rejects([&]{capture(f);});f.flow->boundary().release(XeenCombatBoundary::Work::Inventory,lease);rejects([&]{capture(f);});
	f.action(XeenEncounterAction::Left); check(f.flow->journeyQuiet(),"fresh action establishes a new boundary");
	check(!f.flow->current(old),"old frame ticket stays stale");
	f.flow.reset();rejects([&]{capture(f);});
	Fixture altered; const_cast<XeenActor &>(altered.w.sessionState().actors()[0]).statistics.reset(); rejects([&]{capture(altered);});
	Fixture failed; failed.engage();failed.lethal();failed.flow->combat()->invalidate();rejects([&]{capture(failed);});
}
void failures() {
	const auto s=moved();
	for(unsigned seam=0;seam<5;++seam) for(bool throws:{false,true}) for(unsigned mutation=0;mutation<5;++mutation) {
		Destination d; d.w.map(20); d.w.objectFile(20); unsigned hits=0;
		d.observer=[&](unsigned at) {if(at!=seam)return;++hits;
			switch(mutation) {
			case 0:d.p.roster.at(29).currentSp=-19;break;
			case 1:d.p=XeenPartyState(d.p);break;
			case 2:d.c.~XeenCamera();new(&d.c)XeenCamera{20,14,2,XeenDirection::West};break;
			case 3:const_cast<XeenMap &>(d.w.map(20)).geometry.cells[0].rawWord^=1;break;
			case 4:d.f.set(7);break;
			}
			if(throws)throw std::runtime_error("after external mutation");
		};
		rejects([&]{d.restore(s);});check(hits==1&&!d.w.hasEncounterState()&&!d.p.encounterContext,"failure publishes no Journey");
		check(d.w.cachedMapCount()==1&&d.w.cachedObjectFileCount()==1,"destination caches not published");
	}
	for(unsigned mutation=0;mutation<7;++mutation) for(bool throws:{false,true}) {
		Destination d; rejects([&]{d.restore(s,[&](auto &w,const auto &party,const auto &camera,const auto &flags) {
			switch(mutation) {
			case 0:const_cast<XeenPartyState &>(party).roster.at(0).currentSp=-99;break;
			case 1:const_cast<XeenPartyState &>(party).encounterContext->minutes++;break;
			case 2:const_cast<XeenCombatInputs &>(*party.roster.combatInputs(29)).experience++;break;
			case 3:const_cast<XeenActor &>(w.sessionState().actors()[5]).activated=false;break;
			case 4:const_cast<XeenGameFlags &>(flags).set(7);break;
			case 5:const_cast<XeenMap &>(w.map(20)).geometry.cells[0].rawWord^=1;break;
			case 6:const_cast<XeenCamera &>(camera).x=13;break;
			}
			if(throws)throw std::runtime_error("candidate mutation exception");
		});});check(!d.w.hasEncounterState()&&!d.p.encounterContext,"candidate failure atomicity");
	}
	Destination stale;stale.restore(s);stale.f.set(7);rejects([&]{stale.bind();});check(!XeenSaveState::canCapture(stale.p,stale.c,stale.w),"binding rejects postpublication mutation");
	for(unsigned kind=0;kind<8;++kind) {
		auto bad=s;switch(kind) {
		case 0:bad.resources.clouds.crc32++;break;
		case 1:bad.journey->actors[0].accounted=true;break;
		case 2:bad.journey->actors[0].hp=19;break;
		case 3:bad.journey->actors[0].activated=false;break;
		case 4:bad.journey->context->minutes=960;break;
		case 5:bad.journey->actors[0].id.recordIndex=6;break;
		case 6:bad.journey->originalActorCount=26;break;
		case 7:bad.characters[0].conditions[1]=1;break;
		}Destination d;rejects([&]{d.restore(bad);});check(!d.w.hasEncounterState(),"invalid domain not published");
	}
	for(bool throws:{false,true})for(unsigned seam:{0u,1u}) {
		Destination d;XeenPartyState *candidate=nullptr;bool hit=false;
		d.observer=[&](unsigned at){if(!candidate||at!=seam)return;hit=true;candidate->roster.at(29).currentSp=-71;
			if(throws)throw std::runtime_error("nested candidate mutation");};
		rejects([&]{d.restore(s,[&](auto &w,const auto &p,const auto &,const auto &){candidate=&const_cast<XeenPartyState &>(p);
			w.discardMapCache();if(seam==0)w.map(20);else w.objectFile(20);});});
		check(hit&&!d.w.hasEncounterState(),"nested candidate provider mutation cannot become an adopted phase");
	}
	for(unsigned owner=0;owner<3;++owner) {
		Destination d;d.observer=[&](unsigned at){if(at!=3)return;
			if(owner==0){d.p.~XeenPartyState();new(&d.p)XeenPartyState;}
			if(owner==1){d.w.~XeenWorld();new(&d.w)XeenWorld([](auto){return map();},[](auto){return objects();});}
			if(owner==2){d.f.~XeenGameFlags();new(&d.f)XeenGameFlags;}
			throw std::runtime_error("destination lifetime ABA");};
		rejects([&]{d.restore(s);});check(!d.w.hasEncounterState(),"same-address destination lifetime rejects publication");
	}
	Destination caches;caches.restore(s,[](auto &w,const auto &,const auto &c,const auto &){w.discardMapCache();w.map(c.mapId);w.objectFile(20);});
	caches.bind();same(s,caches);
	// Retain aliases to movable provider storage. Saved owners must use detached copies.
	XeenPartyState p;XeenCamera c;XeenGameFlags flags;
	XeenMapEntity *alias=nullptr;
	XeenWorld w([](auto){return map();},[&](auto){auto mob=objects();alias=mob.entities.monsters.data();return mob;});
	Destination providers;auto r=providers.resources();
	XeenSaveState::restoreBeforeGameplay(s,r,p,c,flags,w,[&](auto &candidate,const auto &,const auto &,const auto &){
		// The moved return's storage may have been freed after detachment. Never
		// dereference an expired alias; assert it is not the candidate's storage.
		check(alias!=candidate.objectFile(20).entities.monsters.data(),"MOB storage detached from provider-held alias");
	});
}
struct ApplicationFixture {
	gameplay_test::Fixture presentation;
	Bytes bytes=chr(); XeenPartyState p=XeenPartyLoader().loadFromResources(bytes,pty());
	XeenCamera c=XeenActorApproach::kEntry; XeenGameFlags f;
	XeenEventFile evt=events(); std::vector<XeenMonsterRecord> mon=statistics();
	bool armed=false; unsigned calls=0;
	std::function<void(unsigned)> observer;
	void observe(unsigned seam) {if(armed){++calls;if(observer)observer(seam);}}
	XeenWorld w{[&](XeenMapIdentity id){observe(0);auto m=map();m.geometry.id=id.number;return m;},
		[&](XeenMapIdentity id){observe(1);auto m=objects();m.mapId=id;return m;}};
	XeenEventSystem eventsOwner{[&](auto){return XeenEventScript(evt);},[](auto){return XeenEventTextFile{};}};
	std::unique_ptr<XeenEventFlow> flow;
	ApplicationFixture() {
		XeenJourneySetup setup{bytes,XeenGameplayContextFormat::parse(pty()),mon,evt,56};
		flow=std::make_unique<XeenEventFlow>(w,eventsOwner,p,c,f,presentation.font,
			[](auto){return XeenEventFlow::Composition{};},XeenEventPresenter::NpcDraw{},XeenEventPresenter::Clock{[]{return 0;}},
			XeenEventPresenter::RandomFrame{},nullptr,nullptr,[](auto,auto){return XeenEventFlow::Composition{IndexedFrame{320,200,Bytes(64000)},false};},&setup);
		check(!flow->canSave(),"Application initial handoff closed"); flow->framePresented(); check(flow->canSave(),"Application presented Journey");
	}
	XeenGameplayServices services() {
		auto s=presentation.services();
		s.resources.signature=save_test::sample().resources;
		s.resources.loadInitialParty=[]()->XeenPartyState{throw std::runtime_error("unexpected Journey CHR/PTY read");};
		s.resources.loadInitialCharacters=[]()->Bytes{throw std::runtime_error("unexpected Journey CHR read");};
		s.resources.loadInitialContext=[]()->XeenGameplayContext{throw std::runtime_error("unexpected Journey PTY read");};
		s.resources.loadMonsterStatistics=[&]{observe(3);return statistics();};
		s.resources.loadEvents=[&](auto id){observe(2);auto e=events();e.mapId=id;return e;};
		s.maps=[&](auto id){observe(0);auto m=map();m.geometry.id=id.number;return m;};
		s.objects=[&](auto id){observe(1);auto m=objects();m.mapId=id;return m;};
		s.observeSaveStage=[&](auto stage){observe(5+unsigned(stage));};
		return s;
	}
	void save(const std::filesystem::path &path) {
		replay_test::Scope noReplay;
		auto s=services();
		xeenSaveGameplay(s,w,p,c,f,*flow,path,[&](auto &w,const auto &p,const auto &c,const auto &f){observe(4);Destination::compose(w,p,c,f);});
	}
};
Bytes disk(const std::filesystem::path &path) {
	std::ifstream file(path,std::ios::binary);return Bytes(std::istreambuf_iterator<char>(file),{});
}
void application(const std::filesystem::path &directory) {
	std::filesystem::create_directories(directory);
	const auto path=std::filesystem::absolute(directory/"journey.mmsave");
	ApplicationFixture good;good.armed=true;good.save(path);
	check(XeenSaveFile::read(path).journey.has_value()&&good.flow->canSave(),"Application writes v4 and releases own save lease");
	const auto previous=disk(path);
	for(unsigned seam=0;seam<8;++seam)for(bool throws:{false,true}) {
		ApplicationFixture source;source.armed=true;bool injected=false;
		source.observer=[&](unsigned at){if(at!=seam||injected)return;injected=true;
			check(!XeenSaveState::canCapture(source.p,source.c,source.w)&&!source.flow->canSave(),"save lease closes reentrant capture");
			rejects([&]{source.save(path);});
			source.f.set(7);if(throws)throw std::runtime_error("source callback after mutation");
		};
		rejects([&]{source.save(path);});check(injected&&disk(path)==previous,"all source seams preserve previous valid bytes");
	}
	ApplicationFixture preflight;preflight.armed=true;preflight.observer=[](unsigned at){if(at==4)throw std::runtime_error("detached presentation failure");};
	rejects([&]{preflight.save(path);});check(preflight.flow->canSave()&&disk(path)==previous,"preflight failure preserves source eligibility and bytes");
	ApplicationFixture fatal;fatal.armed=true;fatal.flow->closeGameplay();rejects([&]{fatal.save(path);});check(fatal.calls==0,"fatal save refuses before providers");
	for(auto work:{XeenCombatBoundary::Work::Inventory,XeenCombatBoundary::Work::Certificate,XeenCombatBoundary::Work::Event,
		XeenCombatBoundary::Work::Reward,XeenCombatBoundary::Work::PresentationFailure}) {
		ApplicationFixture busy;busy.armed=true;
		const auto lease=const_cast<XeenEncounterFlow *>(busy.flow->encounter())->boundary().hold(work);
		rejects([&]{busy.save(directory/"nonexistent"/"never.mmsave");});check(busy.calls==0,"early refusal calls no provider or observer");
		const_cast<XeenEncounterFlow *>(busy.flow->encounter())->boundary().release(work,lease);rejects([&]{busy.save(path);});check(busy.calls==0,"released boundary ABA refuses capture");
	}
	ApplicationFixture stale;stale.armed=true;std::optional<XeenEventFlow::SaveBoundary> newer;
	stale.observer=[&](unsigned at){if(at==5&&!newer){stale.flow->endSave();newer=stale.flow->beginSave();}};
	rejects([&]{stale.save(path);});check(newer&&stale.flow->saveCurrent(*newer)&&disk(path)==previous,"stale cleanup retains newer save lease");
	stale.flow->endSave(*newer);
	const auto value=moved();
	for(auto fault:{XeenSaveFile::Operation::Open,XeenSaveFile::Operation::Write,XeenSaveFile::Operation::ShortWrite,
		XeenSaveFile::Operation::Flush,XeenSaveFile::Operation::Close,XeenSaveFile::Operation::Replace}) {
		rejects([&]{XeenSaveFile::write(path,value,[&](auto operation){return operation==fault;});});
		check(disk(path)==previous,"v4 file fault preserves prior bytes");
	}
	const auto unknown=directory/"unknown.mmsave";{std::ofstream file(unknown,std::ios::binary);file<<"unknown";}
	const auto unknownBytes=disk(unknown);rejects([&]{good.save(unknown);});check(disk(unknown)==unknownBytes,"unknown target protected");
}
}
int main(int argc,char **argv) {
	try {
		if(argc==4 && std::string(argv[1])=="consume") {
			const auto s=XeenSaveFile::read(std::filesystem::absolute(argv[2]));
			Destination d; d.restore(s); d.bind(); same(s,d);
			check(replay_test::unexpected==0 && replay_test::journeyInitializations==0 && replay_test::journeyConstructions==0,
				"new process restore has no initialization/combat history");
			XeenSaveFile::write(std::filesystem::absolute(argv[3]),d.capture());return 0;
		}
		captureIntegrity(); wire(); roundtrips(); refusals(); failures();
		const auto directory=std::filesystem::temp_directory_path()/("mmodern-journey-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
		application(directory);
		for(auto s:{moved(),ended()}) {
			const auto label=s.journey->actors[0].accounted?"ended":"moved";
			auto input=directory/(std::string(label)+"0.mmsave");XeenSaveFile::write(input,s);
			for(unsigned n=1;n<=2;++n) {
				const auto output=directory/(std::string(label)+std::to_string(n)+".mmsave");
				const auto result=child_test::launch(std::filesystem::absolute(argv[0]),{L"consume",input.wstring(),output.wstring()},
					directory/(std::string(label)+std::to_string(n)+".log"));
				check(result.exit==0,"separate-process Journey consumer");check(disk(input)==disk(output),"separate-process exact disk preservation");input=output;
			}
		}
		check(replay_test::unexpected==0,"restore replay observed gameplay");
		check(replay_test::journeyInitializations && replay_test::journeyConstructions && replay_test::actions && replay_test::pulses &&
			replay_test::retirements && replay_test::commands && replay_test::services,"positive controls reached Journey probes");
		// The active scope must detect genuine calls, not merely expose counters.
		const auto before=replay_test::unexpected;
		{replay_test::Scope scope;journey_test::Fixture positive;positive.action(XeenEncounterAction::Right);positive.pulse();XeenCombatRandom rng(56);rng.draw(1,20);}
		check(replay_test::unexpected>before && replay_test::draws,"probe positive control detects real work in scope");
		std::cout<<"Journey persistence tests passed\n";return 0;
	} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}

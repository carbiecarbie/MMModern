// Test-only typed input adapter around production CLI, resources, Flow and scene.
#include "app/Application.h"
#include "app/XeenGameplayServices.h"
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "platform/XeenSaveFile.h"
#include "XeenSaveTestSupport.h"
#include "XeenRestoreReplayProbe.h"
#include "platform/sdl/SdlWindow.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace mmodern;
namespace fs=std::filesystem;
#define PLAY_SYMBOL "_ZNK7mmodern11Application12playGameplayERKNS_20XeenGameplayServicesENS_10XeenCameraERKSt8optionalINSt10filesystem7__cxx114pathEEbNS_18XeenEncounterEntryES5_IjES5_ItE"
extern "C" int realPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>,std::optional<std::uint16_t>) asm("__real_" PLAY_SYMBOL);
extern "C" int wrappedPlay(const Application *,const XeenGameplayServices &,XeenCamera,const std::optional<fs::path> &,bool,XeenEncounterEntry,std::optional<std::uint32_t>,std::optional<std::uint16_t>) asm("__wrap_" PLAY_SYMBOL);

#define M36_CONFIRM "_ZN7mmodern17XeenEncounterFlow14confirmCastingERKNS0_6TicketEyyyRKSt10shared_ptrIKNS_12IndexedFrameEE"
#define M36_TARGET "_ZN7mmodern17XeenEncounterFlow20respondCastingTargetERKNS0_6TicketESt8optionalIyEyRKSt10shared_ptrIKNS_12IndexedFrameEE"
#define M36_AWAKEN "_ZN7mmodern17XeenEncounterFlow13publishAwakenERKNS0_6TicketE"
#define M36_SERVICE "_ZN7mmodern17XeenEncounterFlow14serviceCastingEv"
namespace m36_probe {unsigned confirms=0,targets=0,awakens=0,services=0;}
extern "C" bool realConfirm(XeenEncounterFlow *,const XeenEncounterFlow::Ticket &,std::size_t,std::size_t,
	std::uint64_t,const IndexedFrame::Presentation &) asm("__real_" M36_CONFIRM);
extern "C" bool wrappedConfirm(XeenEncounterFlow *flow,const XeenEncounterFlow::Ticket &ticket,
	std::size_t caster,std::size_t slot,std::uint64_t input,const IndexedFrame::Presentation &frame) asm("__wrap_" M36_CONFIRM);
extern "C" bool wrappedConfirm(XeenEncounterFlow *flow,const XeenEncounterFlow::Ticket &ticket,
	std::size_t caster,std::size_t slot,std::uint64_t input,const IndexedFrame::Presentation &frame) {
	++m36_probe::confirms;return realConfirm(flow,ticket,caster,slot,input,frame);
}
extern "C" bool realTarget(XeenEncounterFlow *,const XeenEncounterFlow::Ticket &,std::optional<std::size_t>,
	std::uint64_t,const IndexedFrame::Presentation &) asm("__real_" M36_TARGET);
extern "C" bool wrappedTarget(XeenEncounterFlow *flow,const XeenEncounterFlow::Ticket &ticket,
	std::optional<std::size_t> target,std::uint64_t input,const IndexedFrame::Presentation &frame) asm("__wrap_" M36_TARGET);
extern "C" bool wrappedTarget(XeenEncounterFlow *flow,const XeenEncounterFlow::Ticket &ticket,
	std::optional<std::size_t> target,std::uint64_t input,const IndexedFrame::Presentation &frame) {
	++m36_probe::targets;return realTarget(flow,ticket,target,input,frame);
}
extern "C" bool realAwaken(XeenEncounterFlow *,const XeenEncounterFlow::Ticket &) asm("__real_" M36_AWAKEN);
extern "C" bool wrappedAwaken(XeenEncounterFlow *flow,const XeenEncounterFlow::Ticket &ticket) asm("__wrap_" M36_AWAKEN);
extern "C" bool wrappedAwaken(XeenEncounterFlow *flow,const XeenEncounterFlow::Ticket &ticket) {
	++m36_probe::awakens;return realAwaken(flow,ticket);
}
extern "C" bool realService(XeenEncounterFlow *) asm("__real_" M36_SERVICE);
extern "C" bool wrappedService(XeenEncounterFlow *flow) asm("__wrap_" M36_SERVICE);
extern "C" bool wrappedService(XeenEncounterFlow *flow) {
	++m36_probe::services;return realService(flow);
}

namespace {
void check(bool value,const char *message) { if (!value) throw std::runtime_error(message); }
}
extern "C" int wrappedPlay(const Application *application,const XeenGameplayServices &original,XeenCamera camera,
		const std::optional<fs::path> &target,bool resume,XeenEncounterEntry entry,
		std::optional<std::uint32_t> seed,std::optional<std::uint16_t> contract) {
	try {
		replay_test::journeyInitializations=replay_test::journeyConstructions=0;
		replay_test::actions=replay_test::pulses=replay_test::retirements=0;
		replay_test::commands=replay_test::draws=0;
		replay_test::constructions=replay_test::services=replay_test::preparations=0;
		replay_test::timePreparations=replay_test::eventExecutions=0;
		replay_test::transfers=replay_test::equipmentChanges=0;
		m36_probe::confirms=m36_probe::targets=m36_probe::awakens=m36_probe::services=0;
		std::vector<std::string> drawTrace;
		replay_test::observeDraw=[&](auto lo,auto hi,auto value,auto state) {
			drawTrace.push_back(std::to_string(lo)+":"+std::to_string(hi)+":"+
				(value?std::to_string(*value):"rejected")+":"+std::to_string(state.state)+":"+
				std::to_string(state.count));
		};
		struct ClearProbe {~ClearProbe(){replay_test::observeDraw={};}} clearProbe;
		const auto *routeText=std::getenv("MMODERN_M36_ROUTE");
		const std::string route=routeText?routeText:"";
		const std::string stage=std::getenv("MMODERN_M36_STAGE")?std::getenv("MMODERN_M36_STAGE"):"full";
		const bool contactCorrection=stage=="correction-contact" || stage=="correction-contact-multistep";
		check(route=="firstaid" || route=="awaken", "M36 witness route missing");
		check(stage=="full" || stage=="checkpoint" ||
			((stage=="synthetic-tick" || stage=="synthetic-tick-refund" || stage=="synthetic-dusk" ||
			  stage=="synthetic-gems-zero" || stage=="synthetic-gems-max" ||
			  stage=="synthetic-terminal-target" || stage=="synthetic-authority-frames" ||
			  stage=="synthetic-authority-sdl" || stage=="correction-settlement" || stage=="correction-refund" || stage=="correction-sign" ||
			  stage=="correction-well" || contactCorrection || stage=="correction-sdl" ||
			  stage=="synthetic-authority-owner" || stage=="synthetic-authority-reentry" ||
			  stage=="synthetic-authority-book" || stage=="synthetic-authority-class" ||
			  stage=="synthetic-authority-membership" || stage=="synthetic-authority-sp" ||
			  stage=="synthetic-authority-purse" || stage=="synthetic-authority-context" ||
			  stage=="synthetic-authority-cycle" || stage=="synthetic-rng-overflow" ||
			  stage=="synthetic-clock-overflow" || stage=="synthetic-report-reentry") && route=="firstaid" && resume) ||
			((stage=="cancellation" || stage=="fault-before-debit" ||
			  stage=="fault-after-debit" || stage=="fault-after-effect" ||
			  stage=="fault-after-time") && route=="firstaid" && !resume),
			"M36 witness stage invalid");
		check(target &&
			(resume ? !seed && !contract : entry==XeenEncounterEntry::Journey &&
				seed==std::optional<std::uint32_t>{route=="firstaid"?1u:7u} && contract==7),
			"M36 witness production entry/contract mismatch");
		if (stage=="synthetic-tick" || stage=="synthetic-tick-refund" || stage=="synthetic-dusk" ||
			stage=="synthetic-gems-zero" || stage=="synthetic-gems-max" ||
			stage=="synthetic-terminal-target" || stage=="synthetic-rng-overflow" ||
			stage=="synthetic-report-reentry" || stage=="correction-sign" || stage=="correction-well" || contactCorrection) {
			auto fixture=XeenSaveFile::read(*target);
			check(fixture.journey && fixture.journey->schema==7 && fixture.journey->contract==7 &&
				fixture.journey->context && fixture.journey->context->minutes==521,
				"M36 synthetic calendar fixture requires genuine checkpoint");
			if (stage=="synthetic-dusk") fixture.journey->context->minutes=1250;
			else if (stage=="synthetic-tick" || stage=="synthetic-tick-refund" ||
				stage=="synthetic-rng-overflow" || stage=="synthetic-report-reentry") fixture.journey->context->minutes=950;
			if (stage=="synthetic-rng-overflow") fixture.journey->random->count=std::numeric_limits<std::uint64_t>::max();
			if (stage=="synthetic-report-reentry") for(auto owner:fixture.activeRosterIds)
				fixture.characters[owner].conditions[4]=255;
			if (stage=="synthetic-gems-zero" || stage=="synthetic-gems-max") {
				check(bool(fixture.journey->treasure),"M36 synthetic gems fixture missing purse");
				fixture.journey->treasure->gems=stage=="synthetic-gems-zero"?0u:std::numeric_limits<std::uint32_t>::max();
			}
			if (stage=="synthetic-terminal-target") {
				fixture.characters[6].currentHp=0;
				fixture.characters[6].conditions[13]=1;
			}
			if (stage=="correction-sign") { fixture.camera={23,5,9,XeenDirection::North};
				for(auto &actor:fixture.journey->actors) if(actor.lifecycle==XeenActorLifecycle::Present) actor.activated=true;
			}
			if (stage=="correction-well") fixture.camera={23,7,7,XeenDirection::North};
			if (contactCorrection) {
				fixture.camera={23,7,11,XeenDirection::West};
				auto &actor=fixture.journey->actors.at(9);actor.x=6;actor.y=11;actor.activated=true;
				actor.hp=25;actor.lifecycle=XeenActorLifecycle::Present;actor.accounted=false;
				if(stage=="correction-contact-multistep") { auto &second=fixture.journey->actors.at(7);second.x=5;second.y=11;second.activated=true; }
			}
			XeenSaveFile::write(*target,fixture);
		}
		auto services=original;
		XeenEventFlow *flow=nullptr;
		XeenWorld *world=nullptr;
		const XeenPartyState *party=nullptr;
		const XeenCamera *position=nullptr;
		const XeenGameFlags *flags=nullptr;
			std::uint64_t now=0,cycle=0;
			bool continuation=resume;
		unsigned compositions=0,saveStages=0;
		bool faulted=false,armBeforeDebit=false;
		bool reenterProvider=false;
		unsigned providerReentries=0;
		const auto loadNames=services.resources.loadLearnedSpellNames;
		services.resources.loadLearnedSpellNames=[&] {
			if (reenterProvider && flow && party && flow->encounter()->castingActive()) {
				++providerReentries;
				const auto sp=party->roster.at(1).currentSp;
				flow->handle(CastSpellAction{},flow->displayedInput());
				check(party->roster.at(1).currentSp==sp,"M36 reentrant names provider repeated casting cost");
			}
			return loadNames();
		};
		services.clock=[&] {return now;};
		const auto compose=original.composeEncounter;
		services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto actor) {
			if (armBeforeDebit && flow && flow->encounter()->castingActive() &&
				!flow->encounter()->castingCommitted()) {
				check(p.roster.at(1).currentSp==21 && p.encounterContext->minutes==511,
					"M36 pre-debit fault prefix changed");
				armBeforeDebit=false;faulted=true;throw std::bad_alloc();
			}
			if (!faulted && stage=="fault-after-time" && flow && !flow->encounter()->castingActive() &&
				p.encounterContext && p.encounterContext->minutes==521) {
				check(p.roster.at(1).currentSp==20 && p.roster.at(6).currentHp==15,
					"M36 time publication prefix changed");
				faulted=true;throw std::bad_alloc();
			}
			if (!faulted && flow && flow->encounter()->castingCommitted() &&
				((stage=="fault-after-debit" && flow->encounter()->castingResult().empty()) ||
				 (stage=="fault-after-effect" && !flow->encounter()->castingResult().empty()))) {
				check(p.roster.at(1).currentSp==20 && p.encounterContext->minutes==511 &&
					p.roster.at(6).currentHp==(stage=="fault-after-debit"?11:15),
					"M36 injected publication prefix changed");
				faulted=true;throw std::bad_alloc();
			}
			auto result=compose(w,p,c,phase,actor);
			check(result.frame.isValid() && result.frame.width==320 && result.frame.height==200,
				"M36 original scene composition failed");
			++compositions;return result;
		};
		const auto observe=original.observeGameplay;
		services.observeGameplay=[&](auto &w,auto &events,const auto &p,auto &c,const auto &f) {
			if (observe) observe(w,events,p,c,f);
			world=&w;party=&p;position=&c;flags=&f;
		};
		const auto configure=original.configureFlow;
		services.configureFlow=[&](auto &f,const auto &c) {if (configure) configure(f,c);flow=&f;};
		services.observeSaveStage=[&](auto) {++saveStages;};
		services.show=[&](const IndexedFrame &first,const auto &handler,const auto &escape,const auto &idle,const auto &status) {
			check(flow && world && party && position && flags && compositions && first.isValid(),
				"M36 production owners/scene missing");
			const auto names=original.resources.loadLearnedSpellNames();
			check(names.raw.size()==937 && names.names[1]=="Awaken" && names.names[26]=="First Aid" &&
				names.names[76]=="None Ready","M36 original names/admission changed");
			const auto capture=[&] {return XeenSaveState::capture(original.resources.signature,*party,*position,*flags,*world);};
			const auto traceState=[&](const char *phase) {
				const auto &context=*party->encounterContext;
				const auto &random=*world->sessionState().journeyRandom();
				std::cout<<"M36 STATE "<<route<<' '<<phase<<" camera="<<position->mapId.number<<','<<position->x<<','<<position->y<<','<<unsigned(position->direction)
					<<" calendar="<<context.year<<','<<context.day<<','<<context.minutes<<','<<context.ctr24
					<<" rng="<<random.state<<','<<random.count<<" activity="<<unsigned(world->sessionState().journeyActivity())<<'\n';
				for (unsigned owner=0;owner<30;++owner) {
					const auto &c=party->roster.at(owner);
					std::cout<<"M36 OWNER "<<owner<<" hp="<<c.currentHp<<" sp="<<c.currentSp<<" condition=";
					for(auto value:c.conditions)std::cout<<unsigned(value)<<',';
					std::cout<<" book=";
					if(c.learnedSpells)for(auto value:*c.learnedSpells)std::cout<<unsigned(value)<<',';
					std::cout<<'\n';
				}
				for(const auto &actor:world->sessionState().actors())
					std::cout<<"M36 ACTOR "<<actor.id.mapId.number<<','<<actor.id.recordIndex<<" xy="<<actor.x<<','<<actor.y
						<<" hp="<<actor.hp<<" active="<<actor.activated<<" life="<<unsigned(actor.lifecycle)
						<<" status="<<unsigned(actor.status)<<'\n';
				if(party->monsterTreasure) {
					const auto &treasure=*party->monsterTreasure;
					std::cout<<"M36 TREASURE gold="<<treasure.gold<<" gems="<<treasure.gems
						<<" pending="<<treasure.pendingMask<<','<<treasure.pendingGold<<'\n';
					for(unsigned index=0;index<10;++index)for(unsigned category=0;category<2;++category){
						const auto &item=category?treasure.armor[index]:treasure.weapons[index];
						std::cout<<"M36 TREASURE ITEM "<<category<<','<<index<<','<<unsigned(item.source)<<','
							<<unsigned(item.item.material)<<','<<unsigned(item.item.id)<<','
							<<unsigned(item.item.state)<<','<<unsigned(item.item.frame)<<'\n';
					}
				}
			};
			unsigned protectedContactFrames=0;
			const auto present=[&] {
				handler.framePresented(flow->frame().presentation());
				if(flow->encounter()->castingSettlement() && flow->encounter()->combat() && flow->encounter()->appearance().projectile) {
					++protectedContactFrames;
					const auto projectile=*flow->encounter()->appearance().projectile;
					std::cout<<"M36 PROTECTED CONTACT PROJECTILE actor="<<projectile.source->recordIndex<<" row="<<projectile.row<<'\n';
				}
			};
			present();
			if (resume) {
				check(!replay_test::journeyInitializations && !replay_test::journeyConstructions &&
					!replay_test::actions && !replay_test::pulses && !replay_test::commands &&
					!replay_test::draws && !replay_test::retirements && !replay_test::constructions &&
					!replay_test::services && !replay_test::preparations &&
					!replay_test::timePreparations && !replay_test::eventExecutions &&
					!replay_test::transfers && !replay_test::equipmentChanges &&
					!m36_probe::confirms && !m36_probe::targets && !m36_probe::awakens &&
					!m36_probe::services,
					"M36 restore replayed initialization/action/combat/RNG");
				const auto saved=XeenSaveFile::read(*target),live=capture();
				save_test::sameSnapshot(saved,live);
					check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(live),"M36 fresh restore bytes changed");
					traceState("restored-before-input");
				std::cout<<"M36 RESTORE EXACT BEFORE INPUT "<<route<<'\n';
				std::cout<<"M36 RESTORE REPLAY COUNTERS 0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0/0\n";
			}
			const auto input=[&](PlayerAction action) {
				present();handler.beginCycle(++cycle);
				const auto displayed=handler.displayedInput();check(bool(displayed),"M36 displayed input missing");
				handler.withDisplayedInput(action,*displayed);present();
			};
			const auto pulse=[&] {now+=100;handler.beginCycle(++cycle);idle();present();};
			unsigned combatInputs=0;
			bool combatCastRefused=false;
			const auto settle=[&] {
				for (unsigned n=0;n<30000;++n) {
					// PlayerReady does not bypass autonomous residual projectile work.
					if(flow->encounter()->appearance().projectile) {pulse();continue;}
					if (const auto *combat=flow->encounter()->combat()) {
						if (combat->phase()==XeenCombatPhase::Failed || combat->phase()==XeenCombatPhase::Defeat ||
							combat->phase()==XeenCombatPhase::SupportStopped)
							throw std::runtime_error("M36 combat terminal phase="+std::to_string(unsigned(combat->phase()))+
								" notice="+flow->encounter()->notice());
						if (combat->phase()==XeenCombatPhase::PlayerReady) {
							if (!combatCastRefused) {
								const auto sp1=party->roster.at(1).currentSp,sp6=party->roster.at(6).currentSp;
								input(CastSpellAction{});
								check(combat->phase()==XeenCombatPhase::PlayerReady && !flow->encounter()->castingActive() &&
									party->roster.at(1).currentSp==sp1 && party->roster.at(6).currentSp==sp6,
									"M36 combat C changed combat or spell state");
								combatCastRefused=true;
							}
							check(++combatInputs<600,"M36 combat bound");
							const auto rows=combat->contacts();unsigned selected=0;
							for(unsigned i=0;i<rows.size();++i) if(rows[i] && (!rows[selected] || rows[i]->recordIndex<rows[selected]->recordIndex)) selected=i;
							if (route=="awaken" && continuation) input(RunAction{});
							else if (!(rows[selected]==combat->selectedTarget())) input(SelectCombatTargetAction{selected});
							else input(AttackAction{});
						} else pulse();
						continue;
					}
					if (world->sessionState().journeyActivity()==XeenJourneyActivity::Reward) {input(AcknowledgeAction{});continue;}
					if (flow->canSave()) return;
					pulse();
				}
				throw std::runtime_error("M36 settlement bound");
			};
			const auto prefix=[&](const char *keys) {
				for (const char *key=keys;*key;++key) {
					check(flow->canSave(),"M36 prefix input not quiet");
					if (*key=='U') input(NavigationAction::MoveForward);
					else if (*key=='L') input(NavigationAction::TurnLeft);
					else if (*key=='R') input(NavigationAction::TurnRight);
					else if (*key=='F') input(ShootAction{});
					else throw std::logic_error("Unknown M36 prefix");
					settle();
				}
			};
			const auto castFirstAid=[&](std::size_t targetIndex=5) {
				check(flow->canSave(),"M36 First Aid requires quiet");
				input(CastSpellAction{});input(SelectMemberAction{4});
				input(NavigationAction::MoveBackward); // Awaken -> First Aid in learned rows.
				input(AcknowledgeAction{});input(AcknowledgeAction{});
				check(party->roster.at(1).currentSp>=0,"M36 First Aid debit");
				input(SelectMemberAction{targetIndex});
			};
			const auto castAwaken=[&] {
				check(flow->canSave(),"M36 Awaken requires quiet");
				input(CastSpellAction{});input(SelectMemberAction{5});
				input(AcknowledgeAction{});input(AcknowledgeAction{});
			};
			const auto save=[&] {
				check(flow->canSave(),"M36 F9 checkpoint is not quiet");
				const auto before=saveStages;input(SaveGameAction{});
				check(saveStages>before && fs::exists(*target),"M36 production F9 file capture missing");
				const auto saved=XeenSaveFile::read(*target),live=capture();
				save_test::sameSnapshot(saved,live);
				check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(live),"M36 saved bytes mismatch");
				traceState("saved-quiet");
				const auto rng=*world->sessionState().journeyRandom();
				std::cout<<"M36 SAVE "<<route<<" minute="<<party->encounterContext->minutes<<" ctr24="<<party->encounterContext->ctr24
					<<" rng="<<rng.state<<','<<rng.count<<" HP6="<<party->roster.at(6).currentHp
					<<" SP1="<<party->roster.at(1).currentSp<<" SP6="<<party->roster.at(6).currentSp<<'\n';
			};
			const auto beginFirstAidTarget=[&] {
				check(flow->canSave(),"M36 authority fixture needs presented Quiet");
				input(CastSpellAction{});input(SelectMemberAction{4});
				input(NavigationAction::MoveBackward);input(AcknowledgeAction{});
				input(AcknowledgeAction{});
				check(flow->encounter()->castingCommitted() && party->roster.at(1).currentSp==19 &&
					party->encounterContext->minutes==521,"M36 authority fixture did not debit exactly once");
			};


			if (stage=="correction-sdl") {
				const auto sp=party->roster.at(1).currentSp;
				const auto camera=*position;const auto minutes=party->encounterContext->minutes;
				unsigned loops=0,idles=0;
				const auto key=[](SDL_Keycode code,Uint8 repeat=0) {
					SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=code;e.key.repeat=repeat;e.key.timestamp=SDL_GetTicks()+1;
					check(SDL_PushEvent(&e)==1,"SDL correction key");e.type=SDL_KEYUP;e.key.repeat=0;SDL_PushEvent(&e);
				};
				auto windowHandler=handler;windowHandler.closed={};windowHandler.failed={};
				windowHandler.beginCycle=[&](std::uint64_t) {
					handler.beginCycle(++cycle);check(++loops<=11,"SDL correction loop bound");
					switch(loops) {
					case 1:key(SDLK_c);key(SDLK_ESCAPE);break;
					case 2:key(SDLK_F5);break;
					case 3:key(SDLK_DOWN);break;
					case 4:key(SDLK_RETURN);break;
					case 5:key(SDLK_RETURN);key(SDLK_ESCAPE);break;
					case 6:key(SDLK_ESCAPE);key(SDLK_ESCAPE,1);break;
					default:key(SDLK_ESCAPE);if(loops<11){key(SDLK_UP);key(SDLK_PERIOD);key(SDLK_F9);key(SDLK_i);key(SDLK_f);key(SDLK_c);}break;
					}
				};
				const bool ok=SdlWindow().showInteractive(flow->frame(),"M36 settlement Escape fence",windowHandler,escape,
					[&]()->std::optional<IndexedFrame> {
						++idles;check(xeen_state::sameCamera(camera,*position) && saveStages==0,"SDL rejected settlement input leaked gameplay/save");
						if(loops==1)check(flow->encounter()->castingActive(),"Batched C Escape cancelled/exited");
						if(loops==5)check(m36_probe::confirms==1 && !m36_probe::targets && party->roster.at(1).currentSp==sp-1,"Batched target Escape consumed refund");
						if(loops>=6)check(m36_probe::targets==1 && party->roster.at(1).currentSp==sp,"Fresh target refund missing or repeated");
						if(loops>=8)check(flow->encounter()->state().pending()==11-loops && party->encounterContext->minutes==minutes+10,"SDL pending interval accepted gameplay");
						now+=100;return idle();
					},status);
				check(ok && loops==11 && idles==10 && flow->canSave() && m36_probe::confirms==1 && m36_probe::targets==1,
					"Escape exited before casting settlement/final presentation or failed ordinary exit");
				std::cout<<"M36 CORRECTION PASS SDL ESCAPE/BATCH/SETTLEMENT\n";return true;
			}
			if (stage.rfind("correction-",0)==0) {
				const auto sp=party->roster.at(1).currentSp;
				const auto minutes=party->encounterContext->minutes;
				const auto ctr=party->encounterContext->ctr24;
				if (stage=="correction-sign" || stage=="correction-well") {
					// Control pixels are produced by the actual casting renderer at the same
					// address before Event layers exist. Compare the visible selector panel.
					input(CastSpellAction{});const auto caster=flow->frame();input(CancelInteractionAction{});
					const auto executions=replay_test::eventExecutions;
					input(InteractionAction{});
					if (stage=="correction-well") {
						check(flow->presentationGeneration().has_value(),"Well did not enter its response owner");
						input(CastSpellAction{});check(!flow->encounter()->castingActive(),"Active Event admitted C");
						input(SelectMemberAction{4});
						for(unsigned n=0;!flow->canSave() && n<8;++n) input(AcknowledgeAction{});
					}
					check(flow->canSave() && !flow->presentationGeneration() && replay_test::eventExecutions>executions,
						"Event did not finish at presented Quiet");
					const auto completed=replay_test::eventExecutions;
					input(CastSpellAction{});
					const auto &shown=*flow->frame().presentation();
					for (unsigned y=105;y<200;++y) for(unsigned x=1;x<231;++x)
						check(shown.pixels[y*shown.width+x]==caster.pixels[y*caster.width+x],
							"Completed Event hid the actual ChooseCaster pixels");
					check(replay_test::eventExecutions==completed,"Casting cleanup replayed the Event");
				} else input(CastSpellAction{});
				input(SelectMemberAction{4});input(NavigationAction::MoveBackward);input(AcknowledgeAction{});
				const auto confirmation=flow->frame().presentation();
				const auto confirmationInput=*handler.displayedInput();
				handler.beginCycle(++cycle);handler.withDisplayedInput(AcknowledgeAction{},confirmationInput);
				check(escape() && party->roster.at(1).currentSp==sp-1,"Unpresented target handoff became exit");
				handler.withDisplayedInput(CancelInteractionAction{},*handler.displayedInput());
				check(party->roster.at(1).currentSp==sp-1 && !m36_probe::targets,"Unpresented target Escape refunded");
				check(flow->frame().presentation()!=confirmation,"Target reused confirmation authority");
				present();
				handler.withDisplayedInput(CancelInteractionAction{},confirmationInput);
				check(!m36_probe::targets,"Stale confirmation Escape consumed target");
				const bool refund=stage=="correction-refund";
				const auto targetFrame=flow->frame().presentation();
				handler.beginCycle(++cycle);
				handler.withDisplayedInput(refund?PlayerAction{CancelInteractionAction{}}:PlayerAction{SelectMemberAction{4}},*handler.displayedInput());
				check(escape() && !flow->canSave(),"Consumed target lost protected result handoff");
				handler.withDisplayedInput(CancelInteractionAction{},*handler.displayedInput());
				check(m36_probe::targets==1 && party->roster.at(1).currentSp==(refund?sp:sp-1),"Consumed target Escape repeated effect/refund");
				check(flow->frame().presentation()!=targetFrame,"Result reused target authority");present();
				const auto result=flow->encounter()->castingResult();
				check(!result.empty(),"Missing effect/refund result");
				// Observe the publication handoff before its required presentation.
				now+=100;handler.beginCycle(++cycle);idle();
				check(flow->encounter()->state().pending()==3 && flow->encounter()->castingSettlement() && escape(),
					"Time publication did not retain settlement protection");
				handler.withDisplayedInput(CancelInteractionAction{},*handler.displayedInput());present();
				for(unsigned pending=3;pending;--pending) {
					check(flow->encounter()->state().pending()==pending,"Normal countdown stage skipped");
					const auto cameraBefore=*position;const auto contextBefore=*party->encounterContext;
					const auto rng=*world->sessionState().journeyRandom();
					const auto characters=party->roster.characters();const std::vector<XeenActor> actors=world->sessionState().actors();
					const auto saveBefore=saveStages;
					const auto ticket=flow->encounter()->ticket();
					for(const PlayerAction &action:std::vector<PlayerAction>{NavigationAction::MoveForward,NavigationAction::MoveBackward,
						NavigationAction::TurnLeft,NavigationAction::TurnRight,WaitAction{},ShootAction{},InspectInventoryAction{},
						UseItemAction{},InteractionAction{},CastSpellAction{},SaveGameAction{},CancelInteractionAction{},
						AcknowledgeAction{},AttackAction{},RunAction{},SelectMemberAction{4}}) {
						input(action);
						check(xeen_state::sameCamera(cameraBefore,*position) && contextBefore==*party->encounterContext &&
							rng==*world->sessionState().journeyRandom() && flow->encounter()->state().pending()==pending &&
							flow->encounter()->current(ticket) && saveStages==saveBefore && escape() && !flow->canSave(),
							"Rejected settlement input changed camera/time/RNG/pending/authority or reached save");
						for(unsigned owner=0;owner<30;++owner) check(xeen_state::sameCharacter(characters[owner],party->roster.at(owner)),"Settlement input changed roster");
						for(unsigned i=0;i<actors.size();++i) check(xeen_state::sameActor(actors[i],world->sessionState().actors()[i]),"Settlement input changed actor");
						check(flow->encounter()->castingResult()==result,"Refused action retired result");
					}
					if(pending==1) {
						now+=100;handler.beginCycle(++cycle);idle();
						check(flow->encounter()->castingSettlement() && escape() && !flow->canSave(),
							"Final opportunity released protection before successor presentation");
						const auto successor=flow->frame().presentation();
						handler.withDisplayedInput(CancelInteractionAction{},*handler.displayedInput());
						handler.withDisplayedInput(NavigationAction::TurnRight,*handler.displayedInput());
						check(flow->frame().presentation()==successor && !flow->canSave(),"Unpresented successor accepted input");
						present();
					} else pulse();
				}
				check(party->encounterContext->minutes==minutes+10 && party->encounterContext->ctr24==ctr &&
					party->roster.at(1).currentSp==(refund?sp:sp-1),"Settlement repeated cost/time or self-target restored SP");
				check(flow->encounter()->castingResult()==result && flow->encounter()->notice().find("owed")==std::string::npos,
					"Settlement lost feedback or retained owed language");

				if(contactCorrection) {
					check(flow->encounter()->combat(),"Contact fixture did not attach");
					const auto projectile=flow->encounter()->appearance().projectile;
					check(projectile && projectile->enemy && projectile->source==std::optional<XeenMonsterIdentity>{{23,stage=="correction-contact-multistep"?7u:9u}} &&
						projectile->row==(stage=="correction-contact-multistep"?1u:0u),
						"First presented combat frame did not retain the reviewed actor-9 projectile");
					const auto cameraBefore=*position;const auto contextBefore=*party->encounterContext;
					const auto rng=*world->sessionState().journeyRandom();
					const auto characters=party->roster.characters();const std::vector<XeenActor> actors=world->sessionState().actors();
					const auto combat=flow->encounter()->combat()->ticket();
					const auto counts=std::array<unsigned,9>{m36_probe::confirms,m36_probe::targets,m36_probe::awakens,
						m36_probe::services,replay_test::commands,replay_test::services,replay_test::draws,
						replay_test::journeyConstructions,saveStages};
					const auto unchanged=[&] {
						check(xeen_state::sameCamera(cameraBefore,*position) && contextBefore==*party->encounterContext &&
							rng==*world->sessionState().journeyRandom() && flow->encounter()->combat()->current(combat),
							"Residual projectile input changed camera/time/RNG/combat authority");
						for(unsigned owner=0;owner<30;++owner)check(xeen_state::sameCharacter(characters[owner],party->roster.at(owner)),"Residual projectile input changed character");
						for(unsigned i=0;i<actors.size();++i)check(xeen_state::sameActor(actors[i],world->sessionState().actors()[i]),"Residual projectile input changed actor");
						check(counts==std::array<unsigned,9>{m36_probe::confirms,m36_probe::targets,m36_probe::awakens,
							m36_probe::services,replay_test::commands,replay_test::services,replay_test::draws,
							replay_test::journeyConstructions,saveStages},"Residual projectile replayed gameplay publication");
					};
					unsigned loops=0,idles=0,steps=0;bool completionPresented=false;
					IndexedFrame::Presentation completionFrame;
					const auto key=[](SDL_Keycode code,Uint32 type=SDL_KEYDOWN,Uint8 repeat=0) {
						SDL_Event e{};e.type=type;e.key.keysym.sym=code;e.key.repeat=repeat;e.key.timestamp=SDL_GetTicks()+1;
						check(SDL_PushEvent(&e)==1,"Residual projectile SDL key enqueue");
					};
					auto windowHandler=handler;windowHandler.closed={};windowHandler.failed={}; // Outer Application retains ownership.
					windowHandler.beginCycle=[&](std::uint64_t) {
						handler.beginCycle(++cycle);check(++loops<10,"Residual projectile SDL loop bound");
						if(loops==1) key(SDLK_ESCAPE); // Fresh against the actually presented combat frame.
						else if(loops==2) {key(SDLK_ESCAPE,SDL_KEYDOWN,1);key(SDLK_ESCAPE);}
						else {
							key(SDLK_ESCAPE,SDL_KEYUP);key(SDLK_ESCAPE);key(SDLK_ESCAPE,SDL_KEYUP);
							if(!completionPresented)for(auto code:{SDLK_UP,SDLK_PERIOD,SDLK_SPACE,SDLK_b,SDLK_r,SDLK_i,SDLK_f,SDLK_c,SDLK_F9}) {
								key(code);key(code,SDL_KEYUP);
							}
						}
					};
					windowHandler.framePresented=[&](const auto &frame) {
						if(frame==completionFrame)check(flow->encounter()->castingSettlement(),"Projectile completion released before actual presentation");
						handler.framePresented(frame);
						if(frame==completionFrame) {
							check(!flow->encounter()->castingSettlement() && !flow->encounter()->appearance().projectile && !escape(),
								"Presented projectile completion failed to restore normal combat/exit routing");
							completionPresented=true;
						}
					};
					const bool ok=SdlWindow().showInteractive(flow->frame(),"M36 residual contact projectile",windowHandler,escape,
						[&]()->std::optional<IndexedFrame> {
							++idles;unchanged();
							check(flow->encounter()->castingSettlement() && escape() && !flow->canSave(),"Presented combat released residual projectile protection");
							handler.withDisplayedInput(CancelInteractionAction{},confirmationInput);unchanged();
							if(loops<=3) return {}; // No elapsed animation time: isolate fresh/held/repeated input.
							const auto before=flow->encounter()->appearance().projectile;check(bool(before),"Projectile serviced twice");
							now+=100;auto next=idle();++steps;unchanged();
							const auto after=flow->encounter()->appearance().projectile;
							if(stage=="correction-contact-multistep" && steps<3) {
								check(after && after->row==0 && after->source==std::optional<XeenMonsterIdentity>{{23,steps==1?7u:9u}},
									"Multiple projectile sequence skipped or repeated a step/source");
							} else check(!after && before->row==0,"Final projectile did not complete exactly once");
							check(flow->encounter()->castingSettlement(),"Animation completion bypassed presentation fence");
							if(!after) {
								completionFrame=flow->frame().presentation();
								handler.withDisplayedInput(CancelInteractionAction{},*handler.displayedInput());
								check(!idle() && flow->encounter()->castingSettlement(),"Unpresented projectile completion advanced/released work");
							}
							return next;
						},status);
					check(ok && completionPresented && steps==(stage=="correction-contact-multistep"?3u:1u) && idles==3+steps && loops==idles+1,
						"Fresh SDL Escape exited before residual projectile completion and presentation");
					unchanged();std::cout<<"M36 CONTACT PROJECTILE SDL PASS steps="<<steps<<'\n';
				}

				if(flow->encounter()->combat()) {
					check(flow->encounter()->combat() && !flow->canSave(),"Casting opportunity did not transfer directly to combat");
					input(CastSpellAction{});check(flow->encounter()->castingResult()==result,"Combat refusal cleared contact feedback");
					settle();check(flow->encounter()->castingResult().empty(),"Accepted combat action kept old casting result");
				} else {
					settle();check(flow->encounter()->castingResult()==result,"Own settlement retired feedback");
					input(AttackAction{});check(flow->encounter()->castingResult()==result,"Ineligible action retired result");
					handler.withDisplayedInput(NavigationAction::TurnLeft,confirmationInput);
					check(flow->encounter()->castingResult()==result,"Stale action retired result");
					input(NavigationAction::TurnLeft);check(flow->encounter()->castingResult().empty(),"Accepted movement kept old result");settle();
					input(CastSpellAction{});input(CancelInteractionAction{});
					check(flow->encounter()->castingResult().empty(),"Later cast resurrected old feedback");
				}
				check(m36_probe::confirms==1 && m36_probe::targets==1,"Correction controls replayed spell publications");
				std::cout<<"M36 CORRECTION PASS "<<stage<<'\n';return true;
			}
			if (stage=="synthetic-authority-frames") {
				const auto originalSp=party->roster.at(1).currentSp;
				input(CastSpellAction{});
				const auto obsolete=flow->frame().presentation();
				input(CancelInteractionAction{});input(CastSpellAction{});
				const auto validTicket=flow->encounter()->ticket();
				auto wrongTicket=validTicket;
				wrongTicket.generation=std::numeric_limits<std::uint64_t>::max();
				check(!flow->encounter()->current(wrongTicket),"forged casting generation gained authority");
				wrongTicket=validTicket;wrongTicket.boundaryGeneration=std::numeric_limits<std::uint64_t>::max();
				check(!flow->encounter()->current(wrongTicket) && flow->encounter()->current(validTicket),
					"wrong casting lease generation poisoned newer work");
				bool rejected=false;try {handler.framePresented(obsolete);} catch(const std::exception &) {rejected=true;}
				check(rejected && flow->encounter()->castingActive(),"obsolete casting generation/frame handoff accepted");
				const auto copied=std::make_shared<const IndexedFrame>(flow->frame());
				rejected=false;try {handler.framePresented(copied);} catch(const std::exception &) {rejected=true;}
				check(rejected,"copied frame object authorized casting input");
				IndexedFrame equal(flow->frame().width,flow->frame().height,flow->frame().pixels);
				equal.palette=flow->frame().palette;
				rejected=false;try {handler.framePresented(std::make_shared<const IndexedFrame>(std::move(equal)));}
				catch(const std::exception &) {rejected=true;}
				check(rejected,"equal-pixel frame authorized casting input");
				input(SelectMemberAction{1});
				check(!flow->encounter()->castingCommitted() && party->roster.at(1).currentSp==originalSp,
					"wrong caster/phase debited SP");
				input(SelectMemberAction{4});input(NavigationAction::MoveBackward);input(AcknowledgeAction{});
				const auto confirmationInput=*handler.displayedInput();
				handler.beginCycle(++cycle);handler.withDisplayedInput(AcknowledgeAction{},confirmationInput);
				const auto uploaded=flow->frame().presentation();
				check(uploaded && party->roster.at(1).currentSp==originalSp-1 &&
					!flow->canSave(),
					"merely composed/uploaded target frame gained response authority");
				const auto beforeTargets=m36_probe::targets;
				handler.beginCycle(++cycle);handler.withDisplayedInput(SelectMemberAction{5},*handler.displayedInput());
				check(m36_probe::targets==beforeTargets && party->roster.at(6).currentHp==15,
					"unpresented target frame consumed response");
				handler.framePresented(uploaded);
				handler.beginCycle(++cycle);handler.withDisplayedInput(SelectMemberAction{5},confirmationInput);
				check(m36_probe::targets==beforeTargets,"obsolete displayed-input generation consumed response");
				input(SelectMemberAction{5});
				handler.beginCycle(++cycle);handler.withDisplayedInput(SelectMemberAction{5},confirmationInput);
				check(m36_probe::confirms==1 && m36_probe::targets==beforeTargets+1 &&
					party->roster.at(1).currentSp==originalSp-1,"stale/repeated response replayed cost or effect");
				settle();check(party->encounterContext->minutes==531 && party->roster.at(1).currentSp==originalSp-1,
					"frame authority repeated or lost ten-minute charge");
				std::cout<<"M36 CASTING FRAME AUTHORITY PASS\n";return true;
			}
			if (stage=="synthetic-authority-sdl") {
				const auto sp=party->roster.at(1).currentSp;
				present();handler.beginCycle(++cycle);
				handler.withDisplayedInput(CastSpellAction{},*handler.displayedInput());
				const auto initial=flow->frame();
				unsigned innerCycle=0,presentations=0,idles=0;
				const auto key=[](SDL_Keycode code,Uint8 repeat=0) {
					SDL_Event event{};event.type=SDL_KEYDOWN;event.key.keysym.sym=code;
					event.key.timestamp=SDL_GetTicks()+1;event.key.repeat=repeat;
					check(SDL_PushEvent(&event)==1,"M36 SDL key enqueue failed");
					event.type=SDL_KEYUP;event.key.repeat=0;
					check(SDL_PushEvent(&event)==1,"M36 SDL key release failed");
				};
				auto windowHandler=handler;
				windowHandler.closed={};windowHandler.failed={}; // The outer Application still owns this Flow.
				windowHandler.beginCycle=[&](std::uint64_t) {
					handler.beginCycle(++cycle);
					if(++innerCycle==1) key(SDLK_F5);
					else if(innerCycle==2) {
						key(SDLK_c);key(SDLK_F5);key(SDLK_F5,1);
						key(SDLK_RETURN);key(SDLK_RETURN);key(SDLK_F6);
					}
				};
				windowHandler.framePresented=[&](const auto &frame) {
					if(++presentations==1) return; // SDL rendered/uploaded, but handoff was omitted.
					handler.framePresented(frame);
				};
				const bool ok=SdlWindow().showInteractive(initial,"M36 casting authority",windowHandler,escape,
					[&]()->std::optional<IndexedFrame> {
						check(++idles<8,"M36 SDL authority loop bound");
						if(idles==1) check(flow->frame().presentation()==initial.presentation() &&
							party->roster.at(1).currentSp==sp && !m36_probe::confirms,
							"merely uploaded casting frame accepted F5");
						if(idles==2) {
							check(flow->frame().presentation()!=initial.presentation() &&
								party->roster.at(1).currentSp==sp && !m36_probe::confirms && !m36_probe::targets,
								"batched casting keys crossed the presented caster phase");
							SDL_Event quit{};quit.type=SDL_QUIT;check(SDL_PushEvent(&quit)==1,"M36 SDL quit enqueue failed");
						}
						return {};
					},status);
				check(ok && presentations>=2 && idles==2,"M36 SDL uploaded/presented fixture failed");
				input(NavigationAction::MoveBackward);input(AcknowledgeAction{});
				input(AcknowledgeAction{});input(SelectMemberAction{5});settle();
				check(party->roster.at(1).currentSp==sp-1 && party->encounterContext->minutes==531 &&
					m36_probe::confirms==1 && m36_probe::targets==1,
					"SDL delayed handoff duplicated or lost casting work");
				std::cout<<"M36 CASTING SDL UPLOAD/BATCH AUTHORITY PASS\n";return true;
			}
			if (stage=="synthetic-authority-owner") {
				beginFirstAidTarget();
				auto &live=*const_cast<XeenPartyState *>(party);
				XeenPartyState replacement;
				for(unsigned owner=0;owner<30;++owner)replacement.roster.at(owner)=live.roster.at(owner);
				replacement.party=live.party;replacement.encounterContext=live.encounterContext;
				bool refused=false;try {live=replacement;} catch(const std::logic_error &) {refused=true;}
				check(refused,"marked casting party accepted equal-character replacement");
				XeenRoster roster;
				for(unsigned owner=0;owner<30;++owner)roster.at(owner)=live.roster.at(owner);
				refused=false;try {live.roster=roster;} catch(const std::logic_error &) {refused=true;}
				check(refused && flow->encounter()->castingCommitted(),"marked casting roster accepted replacement/ABA");
				input(SelectMemberAction{5});settle();
				check(party->roster.at(1).currentSp==19 && party->encounterContext->minutes==531 &&
					m36_probe::confirms==1 && m36_probe::targets==1,"rejected owner replacement repeated casting work");
				std::cout<<"M36 CASTING OWNER REPLACEMENT PASS\n";return true;
			}
			if (stage.rfind("synthetic-authority-",0)==0 && stage!="synthetic-authority-reentry" &&
				stage!="synthetic-authority-cycle") {
				beginFirstAidTarget();
				auto &live=*const_cast<XeenPartyState *>(party);
				const auto ticket=flow->encounter()->ticket();
				std::function<void()> restore;
				if(stage=="synthetic-authority-book") {auto &value=live.roster.at(29).learnedSpells->at(0);const auto old=value;value^=1;restore=[&value,old]{value=old;};}
				else if(stage=="synthetic-authority-class") {auto &value=live.roster.at(1).characterClass;const auto old=value;value=XeenCharacterClass::Knight;restore=[&value,old]{value=old;};}
				else if(stage=="synthetic-authority-membership") {const auto old=live.party;live.party=XeenParty::fromRosterIds({0,18,14,11,1,1});restore=[&live,old]{live.party=old;};}
				else if(stage=="synthetic-authority-sp") {auto &value=live.roster.at(1).currentSp;const auto old=value;++value;restore=[&value,old]{value=old;};}
				else if(stage=="synthetic-authority-purse") {auto &value=live.monsterTreasure->gems;const auto old=value;++value;restore=[&value,old]{value=old;};}
				else if(stage=="synthetic-authority-context") {auto &value=live.encounterContext->minutes;const auto old=value;++value;restore=[&value,old]{value=old;};}
				check(bool(restore),"unknown casting preimage mutation");
				check(!flow->encounter()->current(ticket),"casting preimage admitted mutated owner");
				restore();
				check(!flow->encounter()->current(ticket) && !flow->canSave() &&
					party->roster.at(1).currentSp==19 && party->encounterContext->minutes==521 &&
					m36_probe::confirms==1 && m36_probe::targets==0,
					"mutation/reversion reopened casting or rolled back published debit");
				std::cout<<"M36 CASTING PREIMAGE PASS stage="<<stage<<'\n';return true;
			}
			if (stage=="synthetic-authority-reentry") {
				unsigned callbackReentries=0;
				reenterProvider=true;
				flow->beforeEncounterFrameCopy=[&] {
					if(!flow->encounter()->castingActive())return;
					++callbackReentries;
					const auto sp=party->roster.at(1).currentSp;
					flow->handle(CastSpellAction{},flow->displayedInput());
					bool rejected=false;try {handler.framePresented(flow->frame().presentation());}
					catch(const std::exception &) {rejected=true;}
					check(rejected && party->roster.at(1).currentSp==sp,
						"reentrant presentation callback repeated cost or authorized handoff");
				};
				castFirstAid();reenterProvider=false;flow->beforeEncounterFrameCopy={};
				check(providerReentries && callbackReentries && m36_probe::confirms==1 &&
					m36_probe::targets==1 && party->roster.at(1).currentSp==19,
					"reentrant provider/presentation duplicated cast");
				settle();check(party->encounterContext->minutes==531 && party->roster.at(1).currentSp==19,
					"reentrant callback duplicated charge");
				std::cout<<"M36 CASTING REENTRY PASS provider="<<providerReentries<<" callback="<<callbackReentries<<'\n';return true;
			}
			if (stage=="synthetic-authority-cycle") {
				beginFirstAidTarget();
				handler.beginCycle(std::numeric_limits<std::uint64_t>::max());
				bool rejected=false;try {handler.beginCycle(1);} catch(const std::exception &) {rejected=true;}
				check(rejected && flow->encounter()->castingCommitted() && !flow->canSave() &&
					party->roster.at(1).currentSp==19 && party->encounterContext->minutes==521 &&
					m36_probe::confirms==1 && m36_probe::targets==0,
					"exhausted casting input cycle reopened Quiet or repeated debit");
				std::cout<<"M36 CASTING CYCLE LIMIT PASS\n";return true;
			}
			if (stage=="synthetic-clock-overflow") {
				castFirstAid();
				now=std::numeric_limits<std::uint64_t>::max();
				bool rejected=false;try {handler.beginCycle(++cycle);idle();} catch(const std::exception &) {rejected=true;}
				check(rejected && !flow->canSave() && party->roster.at(1).currentSp==19 &&
					party->encounterContext->minutes==531 &&
					world->sessionState().journeyActivity()==XeenJourneyActivity::Failed &&
					m36_probe::confirms==1 && m36_probe::targets==1,
					"casting presentation clock overflow reopened Quiet/refund or repeated work");
				std::cout<<"M36 CASTING CLOCK LIMIT PASS\n";return true;
			}
			if (stage=="synthetic-report-reentry") {
				check(party->encounterContext->minutes==950,"reporting fixture calendar missing");
				unsigned reports=0;
				flow->reportText=[&](const std::string &) {
					++reports;const auto sp=party->roster.at(6).currentSp;
					flow->handle(CastSpellAction{},flow->displayedInput());
					check(party->roster.at(6).currentSp==sp && !flow->canSave(),
						"reentrant casting terminal reporter repeated cost or opened Quiet");
				};
				castAwaken();
				for(unsigned n=0;n<30 && world->sessionState().journeyActivity()!=XeenJourneyActivity::SupportStopped;++n)
					pulse();
				check(reports && world->sessionState().journeyActivity()==XeenJourneyActivity::SupportStopped &&
					party->roster.at(6).currentSp==26 && party->encounterContext->minutes==960 &&
					!flow->canSave() && m36_probe::awakens==1,
					"casting terminal report reentry repeated effect/time or recovered Quiet");
				std::cout<<"M36 CASTING REPORT REENTRY PASS reports="<<reports<<'\n';return true;
			}
			if (stage=="synthetic-rng-overflow") {
				check(party->encounterContext->minutes==950 &&
					world->sessionState().journeyRandom()->count==std::numeric_limits<std::uint64_t>::max(),
					"casting overflow fixture missing");
				castFirstAid();
				for(unsigned n=0;n<30 && world->sessionState().journeyActivity()!=XeenJourneyActivity::Failed;++n) {
					now+=100;handler.beginCycle(++cycle);try {idle();} catch(const std::exception &) {}
				}
				check(world->sessionState().journeyActivity()==XeenJourneyActivity::Failed && !flow->canSave() &&
					party->roster.at(1).currentSp==19 && party->encounterContext->minutes==950 &&
					world->sessionState().journeyRandom()->count==std::numeric_limits<std::uint64_t>::max(),
					"casting RNG counter overflow manufactured Quiet/refund/time publication");
				std::cout<<"M36 CASTING COUNTER OVERFLOW PASS\n";return true;
			}
			if (stage=="synthetic-tick" || stage=="synthetic-tick-refund") {
				check(party->encounterContext->minutes==950,"M36 synthetic tick prestate");
				const auto sp=party->roster.at(1).currentSp;
				if (stage=="synthetic-tick") castFirstAid();
				else {
					input(CastSpellAction{});input(SelectMemberAction{4});
					input(NavigationAction::MoveBackward);input(AcknowledgeAction{});input(AcknowledgeAction{});
					check(party->roster.at(1).currentSp==sp-1,"M36 synthetic refund debit");
					input(CancelInteractionAction{});
				}
				check(party->roster.at(1).currentSp==sp-(stage=="synthetic-tick"?1:0) && party->encounterContext->minutes==950,
					"M36 synthetic tick cost/effect order");
				traceState("synthetic-tick-effect-before-time");
				settle();
				check(party->encounterContext->minutes==960 && party->encounterContext->ctr24==2 &&
					party->roster.at(1).currentSp==sp-(stage=="synthetic-tick"?1:0) &&
					world->sessionState().journeyRandom()->state==2771657022u &&
					world->sessionState().journeyRandom()->count==30,
					"M36 synthetic 950 to 960 condition/RNG settlement");
				save();std::cout<<"M36 SYNTHETIC TICK PASS stage="<<stage<<'\n';return true;
			}
			if (stage=="synthetic-dusk") {
				check(party->encounterContext->minutes==1250,"M36 synthetic dusk prestate");
				const auto before=XeenSaveFormat::encode(capture());
				const auto sp=party->roster.at(1).currentSp;
				input(CastSpellAction{});input(SelectMemberAction{4});
				input(NavigationAction::MoveBackward);input(AcknowledgeAction{});input(AcknowledgeAction{});
				check(party->roster.at(1).currentSp==sp && party->encounterContext->minutes==1250 &&
					!flow->canSave(),"M36 1250 charge refusal must precede debit");
				input(CancelInteractionAction{});input(CancelInteractionAction{});input(CancelInteractionAction{});
				check(flow->canSave() && XeenSaveFormat::encode(capture())==before,
					"M36 refused cast changed durable state");
				std::cout<<"M36 SYNTHETIC DUSK REFUSAL PASS\n";return true;
			}
			if (stage=="synthetic-gems-zero" || stage=="synthetic-gems-max") {
				const auto gems=stage=="synthetic-gems-zero"?0u:std::numeric_limits<std::uint32_t>::max();
				check(party->monsterTreasure && party->monsterTreasure->gems==gems,
					"M36 synthetic gem prestate");
				castFirstAid();settle();
				check(party->monsterTreasure->gems==gems && party->roster.at(1).currentSp==19 &&
					party->encounterContext->minutes==531,"M36 zero-gem casting cost changed");
				save();std::cout<<"M36 SYNTHETIC GEMS PASS stage="<<stage<<'\n';return true;
			}
			if (stage=="synthetic-terminal-target") {
				check(party->roster.at(6).currentHp==0 && party->roster.at(6).conditions[13]==1,
					"M36 synthetic terminal prestate");
				castFirstAid();
				check(party->roster.at(1).currentSp==19 && party->roster.at(6).currentHp==0 &&
					party->roster.at(6).conditions[13]==1 &&
					flow->encounter()->castingResult().find("Spell failed")!=std::string::npos,
					"M36 terminal First Aid failure/refund behavior changed");
				settle();
				check(party->encounterContext->minutes==531 && party->roster.at(1).currentSp==19,
					"M36 terminal failure lost time or cost");
				save();std::cout<<"M36 SYNTHETIC TERMINAL PASS\n";return true;
			}
			if (!resume) {
				check(world->sessionState().journeyContract()==7,"M36 fresh contract not 7");
			for (unsigned owner=0;owner<30;++owner) check(bool(party->roster.at(owner).learnedSpells),"M36 missing original book");
			prefix(route=="firstaid"?"UFU":"LUUURUULURUULUUU");
			check(combatCastRefused,"M36 prefix did not exercise combat C refusal");
			if (stage=="fault-before-debit") {
				input(CastSpellAction{});input(SelectMemberAction{4});
				input(NavigationAction::MoveBackward);
				armBeforeDebit=true;input(AcknowledgeAction{});
				check(faulted && party->roster.at(1).currentSp==21 &&
					party->encounterContext->minutes==511 && !flow->canSave(),
					"M36 pre-debit presentation fault charged cost");
				input(AcknowledgeAction{});input(SelectMemberAction{5});settle();
				check(party->roster.at(1).currentSp==20 && party->encounterContext->minutes==521,
					"M36 pre-debit retry did not cast once");
				save();std::cout<<"M36 PRE-DEBIT FAULT PASS\n";return true;
			}
			if (stage=="cancellation") {
				check(party->encounterContext->minutes==511 && party->roster.at(6).currentHp==11,
					"M36 cancellation prefix changed");
				const auto original=XeenSaveFormat::encode(capture());
				const auto sp=party->roster.at(1).currentSp;
				const auto unsafeSave=[&] {
					const auto before=saveStages;
					input(SaveGameAction{});
					check(saveStages==before && !fs::exists(*target),"M36 unsafe F9 reached capture/file");
				};
				const auto oldFrame=flow->frame().presentation();
				input(CastSpellAction{});
				bool staleFrameRejected=false;
				try {handler.framePresented(oldFrame);} catch(const std::exception &) {staleFrameRejected=true;}
				check(staleFrameRejected && !flow->canSave(),"M36 old concrete frame was accepted");
				unsafeSave();input(CancelInteractionAction{});
				check(flow->canSave() && XeenSaveFormat::encode(capture())==original,
					"M36 caster Escape changed durable state");
				input(CastSpellAction{});input(SelectMemberAction{4});unsafeSave();
				input(CancelInteractionAction{});input(CancelInteractionAction{});
				check(flow->canSave() && XeenSaveFormat::encode(capture())==original,
					"M36 browse Escape changed durable state");
				input(CastSpellAction{});input(SelectMemberAction{4});
				input(NavigationAction::MoveBackward);input(AcknowledgeAction{});unsafeSave();
				input(CancelInteractionAction{});input(CancelInteractionAction{});input(CancelInteractionAction{});
				check(flow->canSave() && XeenSaveFormat::encode(capture())==original,
					"M36 confirmation Escape changed durable state");
				input(CastSpellAction{});input(SelectMemberAction{4});
				input(NavigationAction::MoveBackward);input(NavigationAction::MoveBackward);
				input(AcknowledgeAction{});
				check(party->roster.at(1).currentSp==sp && party->encounterContext->minutes==511 &&
					!flow->encounter()->castingCommitted(),"M36 learned Light selection charged SP/time");
				input(CancelInteractionAction{});input(CancelInteractionAction{});
				check(flow->canSave() && XeenSaveFormat::encode(capture())==original,
					"M36 unsupported Light changed durable state");
				input(CastSpellAction{});input(SelectMemberAction{4});
				input(NavigationAction::MoveBackward);input(AcknowledgeAction{});input(AcknowledgeAction{});
				check(party->roster.at(1).currentSp==sp-1 && !flow->canSave(),"M36 debit missing");
				unsafeSave();input(SelectMemberAction{99});
				check(party->roster.at(1).currentSp==sp-1 && !flow->canSave(),"M36 invalid target consumed cast");
				input(CancelInteractionAction{});
				check(party->roster.at(1).currentSp==sp && party->roster.at(6).currentHp==11 &&
					party->encounterContext->minutes==511 && !flow->canSave(),
					"M36 target Escape refund/owed time changed");
				unsafeSave();input(CancelInteractionAction{});
				check(party->roster.at(1).currentSp==sp,"M36 target refund repeated");
				settle();
				check(party->encounterContext->minutes==521 && party->encounterContext->ctr24==2 &&
					party->roster.at(1).currentSp==sp,"M36 refunded target settlement changed");
				save();
				std::cout<<"M36 CANCELLATION PASS firstaid\n";
				return true;
			}
			if (route=="firstaid") {
					check(position->x==7 && position->y==11 && party->encounterContext->minutes==511 &&
						party->roster.at(6).currentHp==11 && party->roster.at(1).currentSp==21,
						"M36 First Aid prefix changed");
					castFirstAid();
					check(party->roster.at(1).currentSp==20 && party->roster.at(6).currentHp==15 &&
						party->monsterTreasure->gems==10,"M36 First Aid cost/effect changed");
					traceState("effect-before-time");
					if (stage=="fault-after-debit" || stage=="fault-after-effect")
						check(faulted,"M36 presentation fault was not exercised");
				} else {
					check(position->x==5 && position->y==4 && party->encounterContext->minutes==592 &&
						party->roster.at(1).conditions[8]==1 && party->roster.at(14).conditions[8]==1,
						"M36 Awaken prefix changed");
					const auto hp1=party->roster.at(1).currentHp,hp14=party->roster.at(14).currentHp;
					castAwaken();
					check(party->roster.at(6).currentSp==26 && !party->roster.at(1).conditions[8] &&
						!party->roster.at(14).conditions[8] && party->roster.at(1).currentHp==hp1 &&
						party->roster.at(14).currentHp==hp14,"M36 Awaken cost/party effect changed");
					traceState("effect-before-time");
				}
				settle();
				if (stage=="fault-after-time") check(faulted,"M36 time presentation fault was not exercised");
				check(party->encounterContext->minutes==(route=="firstaid"?521:602),"M36 charge minute changed");
				check(party->encounterContext->ctr24==(route=="firstaid"?2:16),"M36 charge ctr24 changed");
				save();
				std::cout<<"M36 TRACE "<<route<<" checkpoint draws="<<drawTrace.size()<<'\n';
				if (stage=="checkpoint" || stage=="fault-after-debit" || stage=="fault-after-effect" || stage=="fault-after-time") {
					std::cout<<"M36 CHECKPOINT PASS "<<route<<" stage="<<stage<<'\n';return true;
				}
				continuation=true;
			}
			if (route=="firstaid") {
				castFirstAid();settle();
			} else {
				if (party->roster.at(1).canAct() && party->roster.at(1).currentSp>=1) {
					const auto sp=party->roster.at(1).currentSp;
					castFirstAid(4);
					check(party->roster.at(1).currentSp==sp-1,
						"M36 self-target First Aid restored pre-debit caster SP");
				} else castAwaken();
				settle();
			}
			if (route=="firstaid") {
				input(NavigationAction::TurnLeft);settle();
				input(NavigationAction::MoveForward);settle();
				input(NavigationAction::MoveBackward);settle();
			}
			save();
			std::cout<<"M36 TRACE "<<route<<" continuation draws="<<drawTrace.size()<<'\n';
			for (const auto &draw:drawTrace) std::cout<<"M36 DRAW "<<draw<<'\n';
			std::cout<<"M36 PROTECTED CONTACT PRESENTATIONS "<<protectedContactFrames<<'\n';
			std::cout<<"M36 CLI ROUTE PASS "<<route<<(resume?" restored":" uninterrupted")<<'\n';
			return true;
		};
		return realPlay(application,services,camera,target,resume,entry,seed,contract);
	} catch (const std::exception &error) {
		std::cerr<<"M36 CLI witness: "<<error.what()<<'\n';return 8;
	}
}

// Test-only typed input driver around the real CLI and original scene composition.
#include "XeenM35CliWitness.h"
#include "XeenSaveTestSupport.h"
#include "app/XeenEventFlow.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenStateEquality.h"
#include "platform/XeenSaveFile.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace mmodern {
struct XeenInventoryTestAccess {
	static void discardRegionalCaches(XeenEventFlow &flow) {
		flow._events.discardScriptCache();
		flow._events.discardTextCache();
	}
};
namespace {
void check(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
}
int runM35CliWitness(const XeenGameplayServices &original,
		const std::optional<std::filesystem::path> &target,bool resume,
		std::optional<std::uint32_t> seed,std::optional<std::uint16_t> contract,
		const std::function<int(const XeenGameplayServices &)> &launch) {
	try {
		const std::string stage=std::getenv("MMODERN_M35_STAGE")?std::getenv("MMODERN_M35_STAGE"):"";
		check(stage=="entry" || stage=="request" || stage=="before-phirna" || stage=="collected" || stage=="return" || stage=="exchange" ||
			stage=="well" || stage=="item" || stage=="continue" || stage=="post" || stage=="full",
			"Unknown M35 production CLI stage");
		check(bool(target) && (resume ? !seed && !contract : seed==7 && contract==6),
			"M35 production CLI seed/contract or restore entry changed");
		auto services=original;
		XeenEventFlow *flow=nullptr;
		XeenWorld *world=nullptr;
		const XeenPartyState *party=nullptr;
		const XeenCamera *camera=nullptr;
		const XeenGameFlags *flags=nullptr;
		std::optional<XeenPresentationRequest> pending;
		std::uint64_t now=0,cycle=0;
		unsigned compositions=0,saves=0;
		services.clock=[&]{return now;};
		const auto compose=original.composeEncounter;
		check(bool(compose),"Production scene composer is missing");
		services.composeEncounter=[&](XeenWorld &w,const XeenPartyState &p,const XeenCamera &c,
				std::uint64_t phase,XeenMonsterAppearance appearance) {
			auto result=compose(w,p,c,phase,appearance);
			check(result.frame.isValid() && result.frame.width==320 && result.frame.height==200,
				"Original production scene composition failed");
			++compositions;
			return result;
		};
		const auto observe=original.observeGameplay;
		services.observeGameplay=[&](auto &w,auto &e,const auto &p,auto &c,const auto &f) {
			if(observe)observe(w,e,p,c,f);
			world=&w;party=&p;camera=&c;flags=&f;
		};
		const auto configure=original.configureFlow;
		services.configureFlow=[&](auto &f,const auto &position) {
			if(configure)configure(f,position);
			flow=&f;
			const auto report=f.reportManual;
			f.reportManual=[&,report](const auto &result) {
				if(const auto *s=std::get_if<XeenEventExecutionSuspended>(&result))pending=s->request;
				else pending.reset();
				if(report)report(result);
			};
		};
		services.observeSaveStage=[&](auto){++saves;};
		services.show=[&](const IndexedFrame &first,const auto &handler,const auto &,const auto &idle,const auto &) {
			check(flow && world && party && camera && flags && compositions>0,
				"Production CLI did not construct gameplay owners and original scene");
			check(first.isValid() && first.width==320 && first.height==200 &&
				std::any_of(first.pixels.begin(),first.pixels.end(),[&](auto pixel){return pixel!=first.pixels.front();}),
				"Production entry scene is blank");
			const auto snapshot=[&]{return XeenSaveState::capture(original.resources.signature,*party,*camera,*flags,*world);};
			const auto present=[&]{handler.framePresented(flow->frame().presentation());};
			present();
			if(resume) {
				const auto saved=XeenSaveFile::read(*target);
				const auto live=snapshot();save_test::sameSnapshot(saved,live);
				check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(live),
					"Production CLI restored initial state differs in exact bytes");
				std::cout<<"M35 CLI RESTORE EXACT STATE AND BYTES PASS "<<stage<<'\n';
			}
			const auto input=[&](PlayerAction action) {
				present();handler.beginCycle(++cycle);
				check(handler.displayedInput().has_value(),"Production CLI displayed input missing");
				handler.withDisplayedInput(action,*handler.displayedInput());present();
			};
			const auto pulse=[&]{now+=100;handler.beginCycle(++cycle);idle();present();};
			unsigned combatInputs=0;
			const auto settle=[&](bool yes=false) {
				for(unsigned n=0;n<30000;++n) {
					if(const auto *combat=flow->encounter()->combat()) {
						check(combat->phase()!=XeenCombatPhase::Failed && combat->phase()!=XeenCombatPhase::Defeat &&
							combat->phase()!=XeenCombatPhase::SupportStopped,"M35 production CLI combat terminal");
						if(combat->phase()==XeenCombatPhase::PlayerReady) {
							check(++combatInputs<=600,"M35 production CLI combat input bound");
							const auto rows=combat->contacts();unsigned selected=0;
							for(unsigned i=0;i<rows.size();++i)if(rows[i] && (!rows[selected] ||
								rows[i]->recordIndex<rows[selected]->recordIndex))selected=i;
							if(!(rows[selected]==combat->selectedTarget()))input(SelectCombatTargetAction{selected});
							else input(AttackAction{});
						}else pulse();
						continue;
					}
					const auto activity=world->sessionState().journeyActivity();
					if(activity==XeenJourneyActivity::Event || activity==XeenJourneyActivity::Reward) {
						if(pending) {
							switch(pending->response) {
							case XeenPresentationResponseRequirement::YesNo: input(yes?PlayerAction{YesAction{}}:PlayerAction{NoAction{}});break;
							case XeenPresentationResponseRequirement::CharacterSelection: input(SelectMemberAction{3});break;
							default: input(AcknowledgeAction{});break;
							}
						}else input(AcknowledgeAction{});
						continue;
					}
					if(flow->canSave())return;
					pulse();
				}
				throw std::runtime_error("M35 production CLI work bound");
			};
			const auto route=[&](std::string_view steps) {
				for(char key:steps) {
					check(flow->canSave(),"Production route input is not quiet");
					input(key=='U'?PlayerAction{NavigationAction::MoveForward}:
						key=='L'?PlayerAction{NavigationAction::TurnLeft}:PlayerAction{NavigationAction::TurnRight});
					settle();
				}
			};
			const auto cacheReload=[&] {
				const auto before=snapshot();
				world->discardMapCache();XeenInventoryTestAccess::discardRegionalCaches(*flow);
				flow->refresh(true);present();
				save_test::sameSnapshot(before,snapshot());
				std::cout<<"M35 CLI ORIGINAL CACHE RECONSTRUCTION PASS "<<stage<<'\n';
			};
			const auto checkpoint=[&](const char *name) {
				check(flow->canSave(),"Production CLI checkpoint is not quiet");
				input(SaveGameAction{});
				check(saves>0 && std::filesystem::exists(*target),"Production CLI F9 did not write save");
				const auto live=snapshot(),saved=XeenSaveFile::read(*target);
				save_test::sameSnapshot(saved,live);
				check(XeenSaveFormat::encode(saved)==XeenSaveFormat::encode(live),"Production CLI F9 exact bytes");
				const auto rng=*world->sessionState().journeyRandom();
				std::cout<<"M35 CLI OBS "<<name<<" camera="<<camera->x<<','<<camera->y<<','<<unsigned(camera->direction)
					<<" root="<<party->questItems.at(17)<<" q2="<<party->questFlags.isSet(2)
					<<" flag16="<<party->regionalRecovery->worldFlag16
					<<" hp11="<<party->roster.at(11).currentHp<<" poison18="<<unsigned(party->roster.at(18).conditions[3])
					<<" minute="<<party->encounterContext->minutes<<" rng="<<rng.count<<','<<rng.state
					<<" composed="<<compositions<<'\n';
				if(stage=="full")std::filesystem::copy_file(*target,target->string()+"."+name,
					std::filesystem::copy_options::overwrite_existing);
			};
			if(stage=="entry" || stage=="full") {
				checkpoint("entry");if(stage=="entry")return true;
			}
			if(resume)cacheReload();
			if(stage=="request" || stage=="full") {
				input(InteractionAction{});settle();
				check(party->questFlags.isSet(2) && !party->questItems.at(17),"Production Myra request");
				checkpoint("request");if(stage=="request")return true;
				cacheReload();
			}
			if(stage=="before-phirna" || stage=="full") {
				route("LUUURUULURUULUUUUUUURRUURUUUL");
				check(camera->x==8 && camera->y==2 && camera->direction==XeenDirection::North,
					"Production Phirna arrival");
				checkpoint("before-phirna");if(stage=="before-phirna")return true;
				cacheReload();
			}
			if(stage=="collected" || stage=="full") {
				check(camera->x==8 && camera->y==2 && camera->direction==XeenDirection::North,
					"Production pre-interaction Phirna checkpoint");
				input(InteractionAction{});settle(true);
				check(party->questItems.at(17)==1 && world->sessionState().disabledObjects().count({23,13})==1,
					"Production Phirna grant and Remove");
				checkpoint("collected");if(stage=="collected")return true;
				cacheReload();
			}
			if(stage=="return" || stage=="full") {
				route("UULUUURUUU");
				check(party->questItems.at(17)==1,"Production Root retained during return");
				checkpoint("return");if(stage=="return")return true;
			}
			if(stage=="exchange" || stage=="full") {
				route("RUULURUULUUUL");
				check(camera->x==9 && camera->y==11 && camera->direction==XeenDirection::West &&
					party->encounterContext->minutes==848 && world->sessionState().journeyRandom()->count==281,
					"Production Myra return trace");
				input(InteractionAction{});settle();
				check(!party->questItems.at(17) && !party->questFlags.isSet(2),"Production Myra exchange");
				unsigned potions=0;for(auto owner:kXeenCombatOwners)for(const auto &item:party->roster.at(owner).miscellaneous)
					potions+=item.material==10 && item.id==37 && item.state==1;
				check(potions==5,"Production Myra delivered five actual antidotes");
				checkpoint("exchange");if(stage=="exchange")return true;
				cacheReload();
			}
			if(stage=="well" || stage=="full") {
				route("LUUURUULU");
				check(camera->x==7 && camera->y==7 && camera->direction==XeenDirection::South &&
					party->encounterContext->minutes==910 && world->sessionState().journeyRandom()->count==315,
					"Production well arrival trace");
				const auto hp=party->roster.at(11).currentHp;
				input(InteractionAction{});input(SelectMemberAction{3});
				check(party->roster.at(11).currentHp==hp+25 && !party->regionalRecovery->worldFlag16,
					"Production well HP before flag");
				settle();
				check(party->regionalRecovery->worldFlag16 && party->roster.at(11).currentHp==27,
					"Production well independent flag acknowledgment");
				checkpoint("well");if(stage=="well")return true;
			}
			if(stage=="item" || stage=="full") {
				std::size_t slot=9;for(std::size_t i=0;i<9;++i){const auto &item=party->roster.at(0).miscellaneous[i];
					if(item.material==10 && item.id==37 && item.state==1){slot=i;break;}}
				check(slot<9 && party->roster.at(18).conditions[3]==1,"Production delivered antidote and Poison target");
				const auto minute=party->encounterContext->minutes;
				const auto draws=world->sessionState().journeyRandom()->count;
				input(InspectInventoryAction{});for(int i=0;i<3;++i)input(NavigationAction::TurnRight);
				input(SelectInventorySlotAction{slot});input(UseItemAction{});
				check(flow->inventorySelection().mode==XeenInventoryMode::UseConfirm,"Production antidote confirmation");
				input(AcknowledgeAction{});
				check(flow->inventorySelection().mode==XeenInventoryMode::UseTarget,"Production antidote target selector");
				input(SelectMemberAction{1});settle();
				check(!party->roster.at(18).conditions[3] && flow->encounter()->itemUseResult() &&
					flow->encounter()->result().movementOpportunities==1 &&
					party->encounterContext->minutes==minute && world->sessionState().journeyRandom()->count>=draws,
					"Production antidote and owed actor opportunity");
				checkpoint("item");if(stage=="item")return true;
			}
			if(stage=="continue" || stage=="full") {
				route("LU");for(unsigned i=0;i<6;++i){input(WaitAction{});settle();}
				check(party->encounterContext->minutes>=960,"Production post-cure condition crossing");
				checkpoint("continue");if(stage=="continue")return true;
			}
			input(WaitAction{});settle();checkpoint("post");
			std::cout<<"M35 CLI CONNECTED ORIGINAL SCENE PASS "<<stage<<'\n';
			return true;
		};
		return launch(services);
	} catch(const std::exception &error) {
		std::cerr<<"M35 CLI witness: "<<error.what()<<'\n';return 8;
	}
}
}

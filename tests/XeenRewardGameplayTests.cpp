#include "XeenRewardTestSupport.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>
using namespace reward_test;
namespace fs=std::filesystem;
const XeenCamera start{1,1,1,XeenDirection::North};
struct CaptureOutput {
	std::ostringstream text;std::streambuf *old=std::cout.rdbuf(text.rdbuf());
	~CaptureOutput(){std::cout.rdbuf(old);}
};
void application(const fs::path &path,bool fullPacks){
	fs::remove(path);Fixture f;if(fullPacks)full(f.initial,true);
	f.initial.party=XeenParty::fromRosterIds({1,0,1});f.initial.roster.at(1).name="Live recipient";
	f.initial.roster.at(29).miscellaneous[3]={255,254,253,252};
	f.scripts[1]={pause(),record(1,1,1,12,{0,0,20,7}),record(1,1,2,0x12)};
	auto services=f.services();CaptureOutput output;bool reentrant=false;unsigned inspections=0;
	services.show=[&](const auto&,const auto &handle,const auto &escape,const auto&,const auto &status){
		check(output.text.str().find("Setup Inventory:")!=std::string::npos&&output.text.str().find("[2->1 alias]")!=std::string::npos&&
			output.text.str().find("Owner 29  inactive")!=std::string::npos&&output.text.str().find("ID=254")!=std::string::npos,"setup owner/alias snapshot");
		const auto reads=f.eventReads;handle(InspectInventoryAction{});++inspections;check(reads==f.eventReads,"I dispatched event");
		handle(InteractionAction{});XeenRewardTestAccess::seed(*f.flow,10);
		const auto refused=[&]{const auto gen=f.flow->presentationGeneration();const auto phase=XeenRewardTestAccess::state(*f.flow).rewardPhase;
			const auto n=f.eventReads,c=f.compositions;const auto out=output.text.str();handle(InspectInventoryAction{});
			check(output.text.str()==out,"pending I produced snapshot");handle(SaveGameAction{});
			check(!fs::exists(path)&&f.eventReads==n&&f.compositions==c&&f.flow->presentationGeneration()==gen&&XeenRewardTestAccess::state(*f.flow).rewardPhase==phase,"refused F9 captured/advanced/I/O");};
		refused();
		f.flow->reportManual=[&](const auto &result){if(const auto *s=std::get_if<XeenEventExecutionSuspended>(&result)){
			if(s->request.kind==XeenPresentationKind::RewardReceipt){handle(SaveGameAction{});check(!fs::exists(path)&&status().find("idle gameplay boundary")!=std::string::npos,"reentrant save allowed");reentrant=true;}}};
		handle(AcknowledgeAction{});check(escape(),"reward Escape not intercepted");refused();
		if(fullPacks){while(XeenRewardTestAccess::state(*f.flow).rewardPhase==XeenRewardPhase::Warning)handle(CancelInteractionAction{});refused();}
		check(XeenRewardTestAccess::state(*f.flow).rewardReceipt.delivered==(fullPacks?0U:10U),"production receipt result");
		while(f.flow->blocksGameplay())handle(CancelInteractionAction{});
		check(!fs::exists(path),"deferred save occurred");handle(InspectInventoryAction{});++inspections;
		handle(SaveGameAction{});check(status().find("Saved")!=std::string::npos,"post-ACK F9 failed");
		auto saved=XeenSaveFile::read(path);check(saved.gameFlags[7]&&saved.characters[1].miscellaneous[0].id==(fullPacks?0:37)&&saved.characters[29].miscellaneous[3].id==254,"save lost durable inserted/inactive items");return true;};
	check(Application().playGameplay(services,start,path,false)==0&&reentrant&&inspections==2,"Application reward flow");
	Fixture next;next.automatic=true;auto resumed=next.services();
	resumed.show=[&](const auto&,const auto &handle,const auto&,const auto&,const auto &status){
		check(next.eventReads==0&&!next.flow->blocksGameplay()&&!next.flow->presentationGeneration(),"resume dispatched/restored transient state");
		const auto before=output.text.str().size();handle(InspectInventoryAction{});
		const auto text=output.text.str().substr(before);check(text.find("Live recipient")!=std::string::npos&&text.find("ID=254")!=std::string::npos&&
			(fullPacks||text.find("ID=37")!=std::string::npos),"I reconstructed initial records instead of live resume");
		check(status().find("Inventory:")!=std::string::npos&&next.eventReads==0,"resume inspection dispatch/status");return true;};
	check(Application().playGameplay(resumed,start,path,true)==0,"production reward resume");
}
void setupOrder(){
	Fixture f;f.automatic=true;f.scripts[1]={record(1,1,0,12,{0,0,21,99}),record(1,1,1,0x12)};
	auto services=f.services();CaptureOutput out;
	services.show=[&](const auto&,const auto &handle,const auto&,const auto&,const auto&){
		check(out.text.str().find("Root=0 Q2=0")!=std::string::npos&&f.eventReads==1,"setup snapshot after initial dispatch");
		const auto count=f.eventReads;handle(InspectInventoryAction{});
		check(out.text.str().find("Root=1 Q2=0")!=std::string::npos&&f.eventReads==count,"I not live/read-only");return true;};
	check(Application().playGameplay(services,start,{},false)==0,"setup ordering");
}
void cleanupSaving(const fs::path &path){
	for(bool automatic:{false,true})for(bool afterDelivery:{false,true}){
		fs::remove(path);Fixture f;f.automatic=automatic;f.scripts[1]={pause(),pause(1),record(1,1,2,0x12)};
		auto services=f.services();CaptureOutput output;
		services.show=[&](const auto&,const auto &handle,const auto&,const auto&,const auto &status){
			if(!automatic)handle(InteractionAction{});XeenRewardTestAccess::seed(*f.flow,3);
			const auto report=[&](const auto &r){
				if(const auto *s=std::get_if<XeenEventExecutionSuspended>(&r)){
					if(afterDelivery==(s->request.kind==XeenPresentationKind::RewardReceipt))throw std::runtime_error("injected reporting failure");
				}else if(automatic&&std::holds_alternative<XeenEventExecutionError>(r))throw std::runtime_error("fatal automatic reward error");
			};
			f.flow->reportManual=report;f.flow->reportAutomatic=report;
			bool threw=false;try{handle(AcknowledgeAction{});if(afterDelivery)handle(AcknowledgeAction{});}catch(const std::exception&){threw=true;}
			check(threw==automatic&&!f.flow->blocksGameplay()&&!f.flow->presentationGeneration(),"manual/fatal reward cleanup");
			handle(SaveGameAction{});
			if(automatic)check(!fs::exists(path)&&status().find("idle gameplay boundary")!=std::string::npos,"fatal automatic state saveable");
			else {const auto saved=XeenSaveFile::read(path);check(saved.characters[0].miscellaneous[0].id==(afterDelivery?37:0),"recoverable cleanup lost inserted items");}
			return !automatic;};
		check(Application().playGameplay(services,start,path,false)==(automatic?4:0),"cleanup Application outcome");
	}
}
void sdl(const fs::path &path,bool fullPacks){
	fs::remove(path);Fixture f;if(fullPacks)full(f.initial,true);f.scripts[1]={pause(),record(1,1,1,0x12)};auto services=f.services();CaptureOutput output;
	unsigned refused=0,saved=0;bool seeded=false,delivered=false,receiptSeen=false;
	services.show=[&](const auto &first,const auto &handle,const auto &escape,const auto &idle,const auto &status){
		std::atomic<bool> finished{false};
		std::thread sender([&]{while(!finished){if(SDL_WasInit(SDL_INIT_VIDEO)){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_SPACE;SDL_PushEvent(&e);return;}std::this_thread::sleep_for(std::chrono::milliseconds(10));}});
		const auto key=[](SDL_Keycode k){SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=k;check(SDL_PushEvent(&e)==1,"SDL push");};
		const bool ok=SdlWindow().showInteractive(first,"Reward F9 input",[&](const PlayerAction &action){
			auto frame=handle(action);
			if(std::holds_alternative<InteractionAction>(action)&&!seeded){XeenRewardTestAccess::seed(*f.flow,10);seeded=true;key(SDLK_F9);}
			else if(std::holds_alternative<SaveGameAction>(action)){
				if(f.flow->blocksGameplay()){++refused;check(!fs::exists(path),"SDL refused F9 wrote file");key(delivered?SDLK_ESCAPE:SDLK_RETURN);}
				else{++saved;SDL_Event q{};q.type=SDL_QUIT;SDL_PushEvent(&q);}
			}else if(std::holds_alternative<AcknowledgeAction>(action)){delivered=true;receiptSeen=!fullPacks;key(SDLK_F9);}
			else if(std::holds_alternative<CancelInteractionAction>(action)){
				if(f.flow->blocksGameplay()&&XeenRewardTestAccess::state(*f.flow).rewardPhase==XeenRewardPhase::Receipt&&!receiptSeen){receiptSeen=true;key(SDLK_F9);}
				else key(f.flow->blocksGameplay()?SDLK_ESCAPE:SDLK_F9);
			}
			return frame;
		},escape,idle,status);
		finished=true;sender.join();return ok;};
	check(Application().playGameplay(services,start,path,false)==0&&seeded&&refused==(fullPacks?3U:2U)&&saved==1,"actual SDL F9 reward flow");
	check(XeenSaveFile::read(path).characters[0].miscellaneous[0].id==(fullPacks?0:37),"SDL saved inventory");
}
int main(int argc,char**){try{const auto dir=fs::current_path()/"reward-gameplay-tests";fs::create_directories(dir);
	if(argc>1){sdl(dir/"sdl.mmsave",false);sdl(dir/"sdl-full.mmsave",true);}else{application(dir/"delivery.mmsave",false);application(dir/"loss.mmsave",true);setupOrder();cleanupSaving(dir/"cleanup.mmsave");}
	std::cout<<"Reward Application/save/resume/inspection checks passed\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

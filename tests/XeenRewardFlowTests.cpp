#include "XeenTreasureDeliveryTestSupport.h"
#include "XeenTestInstallation.h"
#include "XeenRewardTestSupport.h"
#include "XeenVisualRemoveTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include <iostream>
using namespace reward_test;
IndexedFrame base(){IndexedFrame b;b.width=320;b.height=200;b.pixels.assign(64000,90);for(int i=0;i<256;++i)for(int j=0;j<3;++j)b.palette[i*3+j]=i;return b;}
void portraitRequiresOwner(){
    mmodern::XeenProgramState programState;
 Execution e;auto state=e.seed({pause()},0);
 state.pendingPresentation->request.kind=XeenPresentationKind::PortraitEffect;
 state.pendingPresentation->request.portrait=XeenEventPortraitEffect{XeenEventPortraitStage::Sparkle,0,0};
 state.pendingPresentation->request.response=XeenPresentationResponseRequirement::Presented;
 XeenEventSystem events(e.scripts,e.texts);auto image=base();
 XeenEventFlow flow(programState,e.world,events,e.f.initial,e.camera,e.flags,e.f.font,[&](auto){return XeenEventFlow::Composition{image,false};});
 bool rejected=false;try{flow.acceptManual(XeenEventExecutionSuspended{state,state.pendingPresentation->request});}
 catch(const std::logic_error &error){rejected=std::string(error.what()).find("Encounter-owned presentation")!=std::string::npos;}
 check(rejected,"Injected legacy portrait request bypassed its Encounter ownership fence");
}
// Part 0, 0x2429e: delivery entry clears once, including warning/currency-only entry.
void treasureEntryClear() {
 for(unsigned mode=0;mode<3;++mode) {
  Execution e;if(mode==2)full(e.f.initial,true);
  auto state=e.seed({pause(),record(1,1,1,0x12)},mode?2:0);
  state.treasure.gold=1;state.treasure.ready=true;
  auto result=e.resumeOne(state);
  for(unsigned n=0;;++n) {
   check(n<20,"treasure clear test bounded");
   if(const auto *pending=std::get_if<XeenEventExecutionSuspended>(&result)) {
    check(pending->state.inputClearCount==1 && pending->state.inputClearSite==0x2429e,
     "treasure entry clear missing or repeated across item phases");
    if(!n)check(pending->state.reward.phase==(mode==0?XeenRewardPhase::Receipt:mode==1?XeenRewardPhase::Blank:XeenRewardPhase::Warning),
     "treasure clear test covers all entry phases");
    result=e.resumeOne(pending->state);
   } else {
    const auto *done=std::get_if<XeenEventExecutionCompleted>(&result);
    check(done && done->inputClearCount==1,"treasure finalization repeated its input clear");break;
   }
  }
 }
}
void flows(){
    mmodern::XeenProgramState programState;
	for(bool global:{false,true}){Execution e;if(global)full(e.f.initial,true);
		auto s=e.seed({pause(),record(1,1,1,12,{0,0,20,7}),record(1,1,2,0x12)},10);
		XeenEventSystem events(e.scripts,e.texts);auto b=base();
		XeenEventFlow flow(programState,e.world,events,e.f.initial,e.camera,e.flags,e.f.font,[&](std::uint64_t){return XeenEventFlow::Composition{b, false};});
  flow.dialogText=&dos_test::text();
		flow.acceptManual(XeenEventExecutionSuspended{s,s.pendingPresentation->request});
		const auto ordinary=*flow.presentationGeneration();flow.handle(AcknowledgeAction{});
		check(flow.blocksGameplay()&&!e.flags.isSet(7)&&flow.handlesEscape(),"reward save/escape/publication boundary");
		const auto gen=*flow.presentationGeneration();const auto frame=flow.frame().pixels;
		check(!flow.respond(ordinary,XeenPresentationResponse::Acknowledged)&&!flow.respond(gen,XeenPresentationResponse::Yes)&&
			!flow.respond(gen,SelectedCharacter{0})&&flow.frame().pixels==frame&&flow.presentationGeneration()==gen,"wrong/stale response consumed state");
		flow.initial();check(flow.presentationGeneration()==gen,"new initial dispatched while pending");
		if(!global)flow.handle(InteractionAction{}); // Space advances one receipt page only.
		for(auto a:std::vector<PlayerAction>{NavigationAction::TurnRight,SelectMemberAction{0},YesAction{},NoAction{},InspectInventoryAction{},EquipmentInventoryAction{}}){
			const auto page=flow.presenter().pageIndex();flow.handle(a);check(flow.presenter().pageIndex()==page,"unrelated input advanced receipt");}
		if(global){
			check(XeenRewardTestAccess::state(flow).pendingPresentation->request.text.find("Remaining items will be lost. Gold and gems are still credited.")!=std::string::npos,"Full-pack warning preserves currency");
			check(XeenRewardTestAccess::state(flow).reward.phase==XeenRewardPhase::Warning&&xeenPrepareRewardReceipt(XeenRewardTestAccess::state(flow).treasure).count==10,"warning inserted/lost early");
			while(XeenRewardTestAccess::state(flow).reward.phase==XeenRewardPhase::Warning)flow.handle(CancelInteractionAction{});
			check(flow.presenter().pageIndex()==0&&XeenRewardTestAccess::state(flow).reward.receipt.lost==10,"one input consumed warning and receipt");
		}else {
			check(XeenRewardTestAccess::state(flow).reward.receipt.delivered<=2,"Input transferred more than the next item");
			while(XeenRewardTestAccess::state(flow).reward.phase!=XeenRewardPhase::Receipt)flow.handle(CancelInteractionAction{});
			check(XeenRewardTestAccess::state(flow).reward.receipt.delivered==10,"receipt missing insertion");
		}
		const auto snapshot=xeenInventoryInspection(e.f.initial);const auto receiptGen=flow.presentationGeneration();
		flow.refresh(true);check(snapshot==xeenInventoryInspection(e.f.initial)&&receiptGen==flow.presentationGeneration(),"rebase replayed delivery");
		while(flow.blocksGameplay())flow.handle(CancelInteractionAction{});
		check(e.flags.isSet(7)&&flow.frame().pixels==b.pixels&&!flow.presentationGeneration(),"final receipt ACK/layer cleanup");
		check(!flow.respond(*receiptGen,XeenPresentationResponse::Acknowledged)&&snapshot==xeenInventoryInspection(e.f.initial),"duplicate ACK replay");
	}
}
void producedFlow(){
    mmodern::XeenProgramState programState;
	for(int outcome=0;outcome<4;++outcome) {
		Execution e;e.f.initial.questItems.increment(17);e.f.initial.questFlags.set(2);
		if(outcome==1)full(e.f.initial,true);
		e.f.scripts[1]={pause(),record(1,1,1,12,{21,99}),record(1,1,2,12,{104,2}),record(1,1,3,0x2c,{70,37,0,1}),
			pause(4),record(1,1,5,12,{0,0,20,7})};
		XeenEventSystem events(e.scripts,e.texts);auto b=base();
		XeenEventFlow flow(programState,e.world,events,e.f.initial,e.camera,e.flags,e.f.font,[&](std::uint64_t){return XeenEventFlow::Composition{b, false};});
  flow.dialogText=&dos_test::text();
		flow.handle(InteractionAction{});
		check(e.f.initial.questItems.at(17)==1 && e.f.initial.questFlags.isSet(2),"pre-ACK quest mutation");
		const auto first=*flow.presentationGeneration();flow.handle(AcknowledgeAction{});
		check(e.f.initial.questItems.at(17)==0 && !e.f.initial.questFlags.isSet(2) &&
			xeenPrepareRewardReceipt(XeenRewardTestAccess::state(flow).treasure).count==1,"actual Flow production");
		if(outcome==2){flow.abandonPresentation();check(!e.f.initial.roster.at(0).miscellaneous[0].id,"abandon delivered staging");}
		else {
			if(outcome==3)for(auto id:e.f.initial.party.activeRosterIds())e.f.initial.roster.at(id).conditions[static_cast<std::size_t>(XeenCondition::Dead)]=1;
			flow.handle(AcknowledgeAction{});
			check(flow.blocksGameplay() && !e.flags.isSet(7),"termination publication gap");
			unsigned acks=0;
			while(flow.blocksGameplay()){
				check(++acks<100,"real reward acknowledgment bound");
				const auto gen=*flow.presentationGeneration(),page=flow.presenter().pageIndex();
				const auto inventory=xeenInventoryInspection(e.f.initial);
				check(!flow.respond(first,XeenPresentationResponse::Acknowledged) && !flow.respond(gen,XeenPresentationResponse::Yes),"stale/wrong kind replay");
				flow.handle(SaveGameAction{});flow.handle(InspectInventoryAction{});flow.handle(EquipmentInventoryAction{});flow.handle(NavigationAction::TurnRight);
				events.discardScriptCache();events.discardTextCache();e.world.discardMapCache();
				flow.refresh(true);
				check(flow.presentationGeneration()==gen && flow.presenter().pageIndex()==page && xeenInventoryInspection(e.f.initial)==inventory,"blocked inputs/rebase replay");
				flow.handle(CancelInteractionAction{});
				if(!flow.blocksGameplay())check(!flow.respond(gen,XeenPresentationResponse::Acknowledged),"final generation replay");
			}
			check(e.flags.isSet(7) && (e.f.initial.roster.at(0).miscellaneous[0].id==37)==(outcome==0 || outcome==3),"actual final delivery includes dead fallback");
			const auto inventory=xeenInventoryInspection(e.f.initial);flow.handle(AcknowledgeAction{});
			check(xeenInventoryInspection(e.f.initial)==inventory,"extra input redelivered");
		}
		check(e.f.initial.questItems.at(17)==0 && !e.f.initial.questFlags.isSet(2),"loss/abandon refunded Root");
	}
}
void failures(){
    mmodern::XeenProgramState programState;
	// Composition/rebase before presentation; reporting before layout; layout
	// failure after transient layer push; and abandonment on both sides of insertion.
	for(int fault=0;fault<6;++fault)for(bool delivered:{false,true}){
		Execution e;auto s=e.seed({pause(),record(1,1,1,0x12)},3);
		if(delivered)s=pending(e.resume(s));
		if(fault==5&&!delivered){full(e.f.initial,true);s=pending(e.resume(s));}
		XeenEventSystem events(e.scripts,e.texts);auto b=base();bool failCompose=false;
		XeenEventFlow flow(programState,e.world,events,e.f.initial,e.camera,e.flags,e.f.font,[&](std::uint64_t){if(failCompose)throw std::runtime_error("compose fault");return XeenEventFlow::Composition{b, false};});
  flow.dialogText=&dos_test::text();
		std::optional<XeenEventExecutionError> error;unsigned reports=0;
		flow.reportManual=[&](const auto &r){if(const auto *v=std::get_if<XeenEventExecutionError>(&r))error=*v;else if(fault==1&&++reports==1)throw std::runtime_error("report fault");};
		if(fault==0){e.camera.direction=XeenDirection::East;failCompose=true;}
		if(fault==2){s.pendingPresentation->request.kind=XeenPresentationKind::NpcAcknowledgment;s.pendingPresentation->request.npc=XeenEventNpc{};}
		if(fault==5)flow.dialogText=nullptr; // Missing original-text provider is a presentation failure.
		flow.acceptManual(XeenEventExecutionSuspended{s,s.pendingPresentation->request});
		if(fault==3){failCompose=true;flow.refresh(true);}
		if(fault==4){flow.abandonPresentation();}
		check(!flow.blocksGameplay()&&!flow.presentationGeneration()&&!flow.presenter().blocksGameplay(),"failed pending state leaked");
		check(e.f.initial.roster.at(0).miscellaneous[0].id==(delivered?37:0),"failure undid/replayed insertion");
		if(fault!=4)check(error&&error->rewards.discarded==(delivered?0U:3U)&&error->rewards.delivered==(delivered?3U:0U),"failure outcome lost");
		failCompose=false;flow.refresh(true);
		check(flow.frame().pixels==b.pixels,"failed transient layer survived rebase");
	}
	Execution e;auto s=e.seed({pause(),record(1,1,1,0x12)});XeenEventSystem events(e.scripts,e.texts);auto b=base();
	XeenEventFlow flow(programState,e.world,events,e.f.initial,e.camera,e.flags,e.f.font,[&](std::uint64_t){return XeenEventFlow::Composition{b, false};});
  flow.dialogText=&dos_test::text();
	flow.acceptManual(XeenEventExecutionSuspended{s,s.pendingPresentation->request});const auto generation=flow.presentationGeneration();
	bool rejected=false;try{flow.acceptManual(XeenManualEventNoEvent{});}catch(const std::logic_error&){rejected=true;}
	check(rejected&&flow.presentationGeneration()==generation,"pending replacement accepted");flow.abandonPresentation();
	flow.handle(InteractionAction{});check(flow.presentationGeneration()!=generation,"explicit abandonment did not permit fresh dispatch");
}
void animatedRewardLayers(){
    mmodern::XeenProgramState programState;
 for(bool warning:{false,true}){
  Execution e;if(warning)full(e.f.initial,true);
  auto state=e.seed({pause(),record(1,1,1,0x12)},3);state=pending(e.resume(state));
  XeenEventSystem events(e.scripts,e.texts);auto b=base();std::uint64_t now=0,phase=0;
  XeenEventFlow flow(programState,e.world,events,e.f.initial,e.camera,e.flags,e.f.font,[&](std::uint64_t value){phase=value;auto current=b;current.pixels[100]=value;return XeenEventFlow::Composition{current,true};},{},[&]{return now;});
  flow.dialogText=&dos_test::text();
  flow.acceptManual(XeenEventExecutionSuspended{state,state.pendingPresentation->request});
  const auto gen=flow.presentationGeneration();const auto page=flow.presenter().pageIndex();const auto inventory=xeenInventoryInspection(e.f.initial);
  auto expectedBase=b;expectedBase.pixels[100]=1;auto oracle=flow.presenter();const auto expected=oracle.rebase(expectedBase);
  now=110;flow.updatePresentation();
  check(phase==1 && flow.frame().pixels==expected.pixels && flow.presentationGeneration()==gen && flow.presenter().pageIndex()==page &&
   xeenInventoryInspection(e.f.initial)==inventory,"animated warning/receipt rebase changed response or delivery");
  check(XeenRewardTestAccess::state(flow).reward.phase==(warning?XeenRewardPhase::Warning:XeenRewardPhase::Receipt),"animated reward phase changed");
  flow.abandonPresentation();check(flow.frame().pixels==expectedBase.pixels,"reward dismissal revealed stale base");
 }
}
void animationFailures(){
    mmodern::XeenProgramState programState;
 for(bool automatic:{false,true})for(bool delivered:{false,true}){
  Execution e;auto state=e.seed({pause(),record(1,1,1,0x12)},3);
  if(delivered)state=pending(e.resume(state));
  XeenEventSystem events(e.scripts,e.texts);auto b=base();std::uint64_t now=0;bool fail=false;
  std::vector<std::uint64_t> phases;unsigned reports=0;
  XeenEventFlow flow(programState,e.world,events,e.f.initial,e.camera,e.flags,e.f.font,[&](std::uint64_t phase){
   phases.push_back(phase);if(fail)throw std::runtime_error("idle animation fault");return XeenEventFlow::Composition{b,true};},{},[&]{return now;});
  flow.dialogText=&dos_test::text();
  auto suspension=XeenEventExecutionSuspended{state,state.pendingPresentation->request};
  if(automatic)flow.acceptAutomatic(suspension);else flow.acceptManual(suspension);
  const auto gen=flow.presentationGeneration();std::optional<XeenEventExecutionError> error;
  auto report=[&](const auto&r){if(const auto *v=std::get_if<XeenEventExecutionError>(&r)){++reports;error=*v;flow.abandonPresentation();if(automatic)throw std::runtime_error("automatic animation report");}};
  flow.reportManual=report;flow.reportAutomatic=report;
  fail=true;now=110;bool threw=false;try{flow.updatePresentation();}catch(const std::runtime_error&){threw=true;}
  check(threw==automatic && reports==1 && !flow.blocksGameplay() && !flow.presentationGeneration(),"idle failure routing/cleanup");
  check(error && error->rewards.discarded==(delivered?0U:3U) && error->rewards.delivered==(delivered?3U:0U),"idle failure reward accounting");
  check(e.f.initial.roster.at(0).miscellaneous[0].id==(delivered?37:0) && !flow.respond(*gen,XeenPresentationResponse::Acknowledged),"idle failure replay/rollback");
  fail=false;flow.refresh(true);check(phases.back()==1,"idle failure rolled logical step back");
 }
 // An initial input refresh that cleans pending execution cannot dispatch that input anew.
 Execution e;auto state=e.seed({pause(),record(1,1,1,0x12)},1);XeenEventSystem events(e.scripts,e.texts);auto b=base();bool fail=false;
 unsigned compositions=0,reports=0;
 XeenEventFlow flow(programState,e.world,events,e.f.initial,e.camera,e.flags,e.f.font,[&](std::uint64_t){++compositions;if(fail)throw std::runtime_error("rebase failure");return XeenEventFlow::Composition{b,true};});
  flow.dialogText=&dos_test::text();
 flow.acceptManual(XeenEventExecutionSuspended{state,state.pendingPresentation->request});
 flow.reportManual=[&](const auto&){++reports;};e.camera.x=2;fail=true;const auto before=compositions;
 flow.handle(InteractionAction{});check(reports==1 && compositions==before+1 && !flow.blocksGameplay(),"failure dispatched same input as new action");
}
void presentation(const XeenFontFormat &font,IndexedFrame b,const std::filesystem::path &output){
	Fixture f;f.initial.roster.at(0).name="Owner Alpha";f.initial.roster.at(1).name="Owner Beta";
	XeenTreasureStaging q;for(unsigned i=0;i<12;++i)q.insert(XeenRewardCategory::Miscellaneous,{255,static_cast<std::uint8_t>(200+i),255,255});
	const auto receipt=xeenTestDeliverTreasure(q,f.initial,{});
	for(auto kind:{XeenPresentationKind::RewardWarning,XeenPresentationKind::RewardReceipt})for(int key=0;key<3;++key){
		XeenEventPresenter p(font);XeenPresentationRequest label;label.kind=XeenPresentationKind::SceneLabelSign;label.text="Retained underlay";
		p.present(b,label);const auto underlay=p.frame();
		XeenPresentationRequest request;request.kind=kind;request.response=XeenPresentationResponseRequirement::Acknowledgment;
		request.treasure.emplace();
        for(std::size_t i=0;i<receipt.count;++i)if(receipt.entries[i].owner)
            request.treasure->items.push_back({f.initial.roster.at(*receipt.entries[i].owner).name,"item",false});
        p.originalAssets(&dos_test::text(),{});
        p.present(underlay,request);
        check(p.pageCount()==(kind==XeenPresentationKind::RewardReceipt?receipt.delivered+1:1),"item/final or modal-warning page count");
		const auto count=p.pageCount();
		for(std::size_t page=0;page<count;++page){
			check(p.pageIndex()==page,"wrong reward page");
			check(!p.handle(SelectMemberAction{0}).response&&!p.handle(NavigationAction::TurnLeft).response,"unrelated dismissal");
			const auto pixels=p.frame().pixels;p.rebase(b);check(p.frame().pixels==pixels,"rebase changed page/underlay");
			if(!output.empty()&&key==0)visual_remove_test::save(p.frame(),output/((kind==XeenPresentationKind::RewardReceipt?"receipt-":"warning-")+std::to_string(page)+".bmp"));
			auto update=p.handle(key==0?PlayerAction(InteractionAction{}):key==1?PlayerAction(AcknowledgeAction{}):PlayerAction(CancelInteractionAction{}));
			check(bool(update.response)==(page+1==count),"nonfinal ACK completed reward");
		}
		p.finishPresentation();check(p.frame().pixels==underlay.pixels&&p.rebase(b).pixels==underlay.pixels,"transient erased retained underlay");
		std::cout<<(kind==XeenPresentationKind::RewardReceipt?"Receipt":"Warning")<<" pages="<<count<<" key="<<key<<'\n';
	}
}
void timedItems() {
    mmodern::XeenProgramState programState;
 Execution e;e.f.initial.purse=XeenPurse{};
 auto state=e.seed({pause(),record(1,1,1,0x12)},2);state.treasure.gold+=19u;
 XeenEventSystem events(e.scripts,e.texts);auto b=base();std::uint64_t now=0;
 XeenEventFlow flow(programState,e.world,events,e.f.initial,e.camera,e.flags,e.f.font,
  [&](std::uint64_t){return XeenEventFlow::Composition{b,true};},{},[&]{return now;});
 flow.dialogText=&dos_test::text();flow.acceptManual(XeenEventExecutionSuspended{state,state.pendingPresentation->request});
 flow.handle(AcknowledgeAction{});check(XeenRewardTestAccess::state(flow).reward.phase==XeenRewardPhase::Blank,"First timed item missing");
 const auto blank=*flow.presentationGeneration();const auto inventory=xeenInventoryInspection(e.f.initial);
 flow.updatePresentation();
 check(flow.presentationGeneration()==blank && xeenInventoryInspection(e.f.initial)==inventory,"Unacquired blank advanced treasure");
 now=0;flow.framePresented(flow.frame().presentation());now=109;flow.updatePresentation();
 check(flow.presentationGeneration()==blank,"One-tick blank bypassed nested two-tick scene draw");
 now=110;flow.updatePresentation();
 check(XeenRewardTestAccess::state(flow).reward.phase==XeenRewardPhase::Found &&
  XeenRewardTestAccess::state(flow).reward.receipt.delivered==1,"Blank auto completion published another item");
 flow.framePresented(flow.frame().presentation());now=220;flow.updatePresentation();
 check(XeenRewardTestAccess::state(flow).reward.phase==XeenRewardPhase::Found,"Found hold omitted ten ticks");
 now=10000;flow.updatePresentation();
 check(XeenRewardTestAccess::state(flow).reward.phase==XeenRewardPhase::Blank &&
  XeenRewardTestAccess::state(flow).reward.receipt.delivered==2,"Late wake caught up multiple treasure frames");
 const auto second=*flow.presentationGeneration();flow.updatePresentation();
 check(flow.presentationGeneration()==second,"Late unacquired item caught up");
 flow.framePresented(flow.frame().presentation());now+=110;flow.updatePresentation();
 const auto found=*flow.presentationGeneration();
 check(XeenRewardTestAccess::state(flow).reward.phase==XeenRewardPhase::Found,"Second found line missing");
 flow.framePresented(flow.frame().presentation());now+=110;
 flow.handle(CancelInteractionAction{});
 check(XeenRewardTestAccess::state(flow).reward.phase==XeenRewardPhase::Receipt && !e.f.initial.purse->gold,
  "Skipping found hold manufactured final acknowledgment");
 check(!flow.respond(found,XeenPresentationResponse::Acknowledged) && !e.f.initial.purse->gold,
  "Stale found frame credited currency");
 flow.framePresented(flow.frame().presentation());now+=110;flow.handle(AcknowledgeAction{});
 check(!flow.blocksGameplay() && e.f.initial.purse->gold==19,"Final acknowledgment did not credit currency");
 auto abandoned=e.seed({pause(),record(1,1,1,0x12)},1);
 flow.acceptManual(XeenEventExecutionSuspended{abandoned,abandoned.pendingPresentation->request});
 flow.handle(AcknowledgeAction{});flow.framePresented(flow.frame().presentation());
 const auto prefix=xeenInventoryInspection(e.f.initial);
 flow.abandonPresentation();now+=10000;flow.updatePresentation();
 check(!flow.blocksGameplay() && xeenInventoryInspection(e.f.initial)==prefix && e.f.initial.purse->gold==19,
  "Abandoned timed hold resumed or replayed its committed prefix");
}
int main(int argc,char **argv){try{
	portraitRequiresOwner();treasureEntryClear();flows();producedFlow();failures();animatedRewardLayers();animationFailures();timedItems();Fixture f;presentation(f.font,base(),{});
	if(argc==3){const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"installation missing");XeenAssetSource assets(*installation,320,200);
		assets.loadPalette("mm4.pal");auto b=assets.snapshot();b.pixels.assign(64000,90);std::filesystem::create_directories(argv[2]);
		presentation(XeenFontFormat(assets.readArchiveResource("fnt")),b,argv[2]);}
	std::cout<<"Reward Flow/failure/presentation matrix passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

#include "XeenRewardTestSupport.h"
#include "XeenVisualRemoveTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/XeenInstallationDetector.h"
#include <iostream>
using namespace reward_test;
IndexedFrame base(){IndexedFrame b;b.width=320;b.height=200;b.pixels.assign(64000,90);for(int i=0;i<256;++i)for(int j=0;j<3;++j)b.palette[i*3+j]=i;return b;}
void flows(){
	for(bool global:{false,true}){Execution e;if(global)full(e.f.initial,true);
		auto s=e.seed({pause(),record(1,1,1,12,{0,0,20,7}),record(1,1,2,0x12)},10);
		XeenEventSystem events(e.scripts,e.texts);auto b=base();
		XeenEventFlow flow(e.world,events,e.f.initial,e.camera,e.flags,e.f.font,[&]{return b;});
		flow.acceptManual(XeenEventExecutionSuspended{s,s.pendingPresentation->request});
		const auto ordinary=*flow.presentationGeneration();flow.handle(AcknowledgeAction{});
		check(flow.blocksGameplay()&&!e.flags.isSet(7)&&flow.handlesEscape(),"reward save/escape/publication boundary");
		const auto gen=*flow.presentationGeneration();const auto frame=flow.frame().pixels;
		check(!flow.respond(ordinary,XeenPresentationResponse::Acknowledged)&&!flow.respond(gen,XeenPresentationResponse::Yes)&&
			!flow.respond(gen,SelectedCharacter{0})&&flow.frame().pixels==frame&&flow.presentationGeneration()==gen,"wrong/stale response consumed state");
		flow.initial();check(flow.presentationGeneration()==gen,"new initial dispatched while pending");
		if(!global)flow.handle(InteractionAction{}); // Space advances one receipt page only.
		for(auto a:std::vector<PlayerAction>{NavigationAction::TurnRight,SelectMemberAction{0},YesAction{},NoAction{},InspectInventoryAction{}}){
			const auto page=flow.presenter().pageIndex();flow.handle(a);check(flow.presenter().pageIndex()==page,"unrelated input advanced receipt");}
		if(global){
			check(XeenRewardTestAccess::state(flow).rewardPhase==XeenRewardPhase::Warning&&XeenRewardTestAccess::state(flow).pendingRewards.size()==10,"warning inserted/lost early");
			while(XeenRewardTestAccess::state(flow).rewardPhase==XeenRewardPhase::Warning)flow.handle(CancelInteractionAction{});
			check(flow.presenter().pageIndex()==0&&XeenRewardTestAccess::state(flow).rewardReceipt.lost==10,"one input consumed warning and receipt");
		}else check(XeenRewardTestAccess::state(flow).rewardReceipt.delivered==10,"receipt missing insertion");
		const auto snapshot=xeenInventoryInspection(e.f.initial);const auto receiptGen=flow.presentationGeneration();
		flow.refresh(true);check(snapshot==xeenInventoryInspection(e.f.initial)&&receiptGen==flow.presentationGeneration(),"rebase replayed delivery");
		while(flow.blocksGameplay())flow.handle(CancelInteractionAction{});
		check(e.flags.isSet(7)&&flow.frame().pixels==b.pixels&&!flow.presentationGeneration(),"final receipt ACK/layer cleanup");
		check(!flow.respond(*receiptGen,XeenPresentationResponse::Acknowledged)&&snapshot==xeenInventoryInspection(e.f.initial),"duplicate ACK replay");
	}
}
void producedFlow(){
	for(int outcome=0;outcome<4;++outcome) {
		Execution e;e.f.initial.questItems.increment(17);e.f.initial.questFlags.set(2);
		if(outcome==1)full(e.f.initial,true);
		e.f.scripts[1]={pause(),record(1,1,1,12,{21,99}),record(1,1,2,12,{104,2}),record(1,1,3,0x2c,{70,37,0,1}),
			pause(4),record(1,1,5,12,{0,0,20,7})};
		XeenEventSystem events(e.scripts,e.texts);auto b=base();
		XeenEventFlow flow(e.world,events,e.f.initial,e.camera,e.flags,e.f.font,[&]{return b;});
		flow.handle(InteractionAction{});
		check(e.f.initial.questItems.at(17)==1 && e.f.initial.questFlags.isSet(2),"pre-ACK quest mutation");
		const auto first=*flow.presentationGeneration();flow.handle(AcknowledgeAction{});
		check(e.f.initial.questItems.at(17)==0 && !e.f.initial.questFlags.isSet(2) &&
			XeenRewardTestAccess::state(flow).pendingRewards.size()==1,"actual Flow production");
		if(outcome==2){flow.abandonPresentation();check(!e.f.initial.roster.at(0).miscellaneous[0].id,"abandon delivered queue");}
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
				flow.handle(SaveGameAction{});flow.handle(InspectInventoryAction{});flow.handle(NavigationAction::TurnRight);
				events.discardScriptCache();events.discardTextCache();e.world.discardMapCache();
				flow.refresh(true);
				check(flow.presentationGeneration()==gen && flow.presenter().pageIndex()==page && xeenInventoryInspection(e.f.initial)==inventory,"blocked inputs/rebase replay");
				flow.handle(CancelInteractionAction{});
				if(!flow.blocksGameplay())check(!flow.respond(gen,XeenPresentationResponse::Acknowledged),"final generation replay");
			}
			check(e.flags.isSet(7) && (e.f.initial.roster.at(0).miscellaneous[0].id==37)==(outcome==0),"actual final delivery/loss");
			const auto inventory=xeenInventoryInspection(e.f.initial);flow.handle(AcknowledgeAction{});
			check(xeenInventoryInspection(e.f.initial)==inventory,"extra input redelivered");
		}
		check(e.f.initial.questItems.at(17)==0 && !e.f.initial.questFlags.isSet(2),"loss/abandon refunded Root");
	}
}
void failures(){
	// Composition/rebase before presentation; reporting before layout; layout
	// failure after transient layer push; and abandonment on both sides of insertion.
	for(int fault=0;fault<6;++fault)for(bool delivered:{false,true}){
		Execution e;auto s=e.seed({pause(),record(1,1,1,0x12)},3);
		if(delivered)s=pending(e.resume(s));
		if(fault==5&&!delivered){full(e.f.initial,true);s=pending(e.resume(s));}
		XeenEventSystem events(e.scripts,e.texts);auto b=base();bool failCompose=false;
		XeenEventFlow flow(e.world,events,e.f.initial,e.camera,e.flags,e.f.font,[&]{if(failCompose)throw std::runtime_error("compose fault");return b;});
		std::optional<XeenEventExecutionError> error;unsigned reports=0;
		flow.reportManual=[&](const auto &r){if(const auto *v=std::get_if<XeenEventExecutionError>(&r))error=*v;else if(fault==1&&++reports==1)throw std::runtime_error("report fault");};
		if(fault==0){e.camera.direction=XeenDirection::East;failCompose=true;}
		if(fault==2){s.pendingPresentation->request.kind=XeenPresentationKind::NpcAcknowledgment;s.pendingPresentation->request.npc=XeenEventNpc{};}
		if(fault==5){s.pendingPresentation->request.text += '\3';flow.reportText=[](const auto&){throw std::runtime_error("diagnostic reporting fault");};}
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
	XeenEventFlow flow(e.world,events,e.f.initial,e.camera,e.flags,e.f.font,[&]{return b;});
	flow.acceptManual(XeenEventExecutionSuspended{s,s.pendingPresentation->request});const auto generation=flow.presentationGeneration();
	bool rejected=false;try{flow.acceptManual(XeenManualEventNoEvent{});}catch(const std::logic_error&){rejected=true;}
	check(rejected&&flow.presentationGeneration()==generation,"pending replacement accepted");flow.abandonPresentation();
	flow.handle(InteractionAction{});check(flow.presentationGeneration()!=generation,"explicit abandonment did not permit fresh dispatch");
}
void presentation(const XeenFontFormat &font,IndexedFrame b,const std::filesystem::path &output){
	Fixture f;f.initial.roster.at(0).name="Owner Alpha";f.initial.roster.at(1).name="Owner Beta";
	XeenPendingRewards q;for(unsigned i=0;i<12;++i)q.enqueue({255,static_cast<std::uint8_t>(200+i),255,255});q.enqueue({});
	const auto receipt=xeenDeliverRewards(q,f.initial,{});
	for(auto kind:{XeenPresentationKind::RewardWarning,XeenPresentationKind::RewardReceipt})for(int key=0;key<3;++key){
		XeenEventPresenter p(font);XeenPresentationRequest label;label.kind=XeenPresentationKind::SceneLabelSign;label.text="Retained underlay";
		p.present(b,label);const auto underlay=p.frame();
		XeenPresentationRequest request;request.kind=kind;request.response=XeenPresentationResponseRequirement::Acknowledgment;
		request.text=kind==XeenPresentationKind::RewardReceipt?xeenRewardReceiptText(receipt,f.initial.roster):
			"Capacity warning\nAll category tails full. Acknowledge before delivery.\n";
		if(kind==XeenPresentationKind::RewardWarning)for(int i=0;i<8;++i)request.text +=
			"Active member "+std::to_string(i%6)+": weapons, armor, accessories and miscellaneous tails occupied.\n";
		p.present(underlay,request);check(p.pageCount()>1,"reward presentation not paginated");
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
int main(int argc,char **argv){try{
	flows();producedFlow();failures();Fixture f;presentation(f.font,base(),{});
	if(argc==3){const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"installation missing");XeenAssetSource assets(*installation,320,200);
		assets.loadPalette("mm4.pal");auto b=assets.snapshot();b.pixels.assign(64000,90);std::filesystem::create_directories(argv[2]);
		presentation(XeenFontFormat(assets.readArchiveResource("fnt")),b,argv[2]);}
	std::cout<<"Reward Flow/failure/presentation matrix passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

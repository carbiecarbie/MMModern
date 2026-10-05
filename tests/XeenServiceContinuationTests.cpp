#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenRestoreGuard.h"
#include <iostream>
using namespace training_test;
namespace {
void refusalUnchanged(Fixture &fixture,const std::function<void()> &operation,const std::string &message) {
	XeenRestoreGuard before(fixture.w,fixture.p,fixture.c,fixture.f);
	const auto generation=fixture.flow->presentationGeneration();
	const auto retained=XeenTrainingTestAccess::serviceContinuationRetained(*fixture.flow);
	const auto pending=XeenTrainingTestAccess::serviceState(*fixture.flow);
	const XeenEventContinuation continuation(pending);
	save_test::rejects(operation,message);
	before.check();
	check(generation==fixture.flow->presentationGeneration() && retained==XeenTrainingTestAccess::serviceContinuationRetained(*fixture.flow),
		"Rejected service consumer changed its generation or retained capability");
	XeenTrainingTestAccess::checkPendingService(*fixture.flow,continuation);
}
void service(Inputs &inputs,unsigned action,bool relocated) {
	auto source=inputs.service();
	source.camera=action==1?XeenCamera{28,8,4,XeenDirection::West}:
		action==4?XeenCamera{28,15,28,XeenDirection::North}:XeenCamera{28,10,11,XeenDirection::North};
	XeenEventFile city{28,"maze0028.evt",true,{}};
	if(relocated) {
		source.camera={28,20,20,XeenDirection::North};
		XeenWorld query(inputs.mapLoader());const auto cell=query.sampleCell(28,20,20);
		check(bool(cell),"Relocated service geometry is absent");
		bool found=false;
		for(unsigned facing=0;facing<4;++facing) {
			const auto wall=wallAt(*cell->cell,XeenDirection(facing));
			if(wall!=1 && wall!=6 && wall!=9 && wall!=13) {source.camera.direction=XeenDirection(facing);found=true;break;}
		}
		check(found,"Relocated fixture has no ordinary Event-facing wall");
		XeenEventRecord r;r.x=20;r.y=20;r.direction=4;r.line=0;r.opcode=0x11;r.parameters={std::uint8_t(action)};r.lengthField=6;
		city.records.push_back(r);
	}
	Fixture fixture(inputs,source,false,relocated?&city:nullptr);
	fixture.flow->drawSmithArt=[&inputs](auto &image){inputs.assets.drawSmith(image);};
	fixture.flow->drawTempleArt=[&inputs](auto &image){inputs.assets.drawTemple(image);};
	unsigned completed=0,beforeSettlement=0,afterSettlement=0;
	fixture.flow->reportManual=[&](const auto &result) {
		if(std::holds_alternative<XeenManualEventCompleted>(result))++completed;
	};
	fixture.act(InteractionAction{});fixture.prepare();
	check(fixture.w.sessionState().journeyActivity()==XeenJourneyActivity::Service && completed==0 &&
		XeenTrainingTestAccess::serviceContinuationRetained(*fixture.flow),"Service entry consumed its Event continuation");
	refusalUnchanged(fixture,[&]{XeenTrainingTestAccess::genericServiceResume(*fixture.flow);},"settlement owner");
	const auto before=[&] {
		++beforeSettlement;
		refusalUnchanged(fixture,[&]{XeenTrainingTestAccess::genericServiceResume(*fixture.flow);},"settlement owner");
		int otherOwner=0;
		refusalUnchanged(fixture,[&]{XeenTrainingTestAccess::wrongServiceOwner(*fixture.flow,&otherOwner);},"absent or stale");
	};
	const auto after=[&] {
		++afterSettlement;
		check(completed==1 && !XeenTrainingTestAccess::serviceContinuationRetained(*fixture.flow),"Owner did not resume its Event exactly once");
		refusalUnchanged(fixture,[&]{XeenTrainingTestAccess::repeatServiceResume(*fixture.flow);},"already consumed");
		refusalUnchanged(fixture,[&]{XeenTrainingTestAccess::repeatServiceSettlement(*fixture.flow,action==5);},"recursively");
	};
	fixture.flow->smithBoundary=[&](auto stage) {
		if(stage==XeenSmithBoundary::BeforeEventSettlement)before();
		if(stage==XeenSmithBoundary::AfterEventSettlement)after();
	};
	fixture.flow->trainingBoundary=[&](auto stage) {
		if(stage==XeenTrainingBoundary::BeforeEventSettlement)before();
		if(stage==XeenTrainingBoundary::AfterEventSettlement)after();
	};
	for(unsigned input=0;input<5 && !fixture.flow->canSave();++input)fixture.act(CancelInteractionAction{});
	check(fixture.flow->canSave() && completed==1 && beforeSettlement==1 && afterSettlement==1 &&
		fixture.p.encounterContext->day==9,"Service failed to leave, resumed twice, or charged departure twice");
	const auto finished=XeenSaveFormat::encode(fixture.snapshot());
	XeenRestoreGuard finalOwners(fixture.w,fixture.p,fixture.c,fixture.f);
	save_test::rejects([&]{XeenTrainingTestAccess::repeatServiceSettlement(*fixture.flow,action==5);},"absent or stale");
	finalOwners.check();
	check(completed==1 && finished==XeenSaveFormat::encode(fixture.snapshot()),"Second settlement changed the completed service state");
}
}
int main(int argc,char **argv) {
	try {
		check(argc==2,"usage: service-continuation <original-installation>");
		const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"Installation unavailable");
		Inputs inputs(*installation);
		for(unsigned action:{1u,4u,5u})for(bool relocated:{false,true})service(inputs,action,relocated);
		std::cout<<"Capability-dispatched service owners, generic refusals and single settlement resumes passed\n";
		return 0;
	}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}

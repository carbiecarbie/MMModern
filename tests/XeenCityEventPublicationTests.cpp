#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenEventTrigger.h"
#include <iostream>
namespace mmodern {
struct XeenCityEventTestAccess {
	static void composer(XeenEventFlow &flow,XeenEventFlow::TransitionCompose compose) {flow._transitionCompose=std::move(compose);}
};
}
using namespace training_test;
namespace {
XeenEventRecord record(unsigned line,unsigned opcode,std::vector<std::uint8_t> parameters={}) {
	XeenEventRecord r;r.x=20;r.y=20;r.direction=4;r.line=line;r.opcode=opcode;
	r.parameters=std::move(parameters);r.lengthField=5+r.parameters.size();r.fileOffset=line*10;return r;
}
XeenSaveSnapshot source(Inputs &inputs) {
	auto s=inputs.service();s.camera={28,20,20,XeenDirection::North};s.questFlags[3]=false;
	XeenWorld query(inputs.mapLoader());const auto cell=query.sampleCell(28,20,20);
	check(bool(cell),"City Event fixture geometry is absent");
	for(unsigned direction=0;direction<4;++direction) {
		const auto wall=wallAt(*cell->cell,XeenDirection(direction));
		if(wall!=1 && wall!=6 && wall!=9 && wall!=13) {s.camera.direction=XeenDirection(direction);return s;}
	}
	throw std::runtime_error("City Event fixture has no ordinary facing");
}
void select(Fixture &f) {
	const auto state=XeenTrainingTestAccess::serviceState(*f.flow);
	for(const auto &member:state.pendingPresentation->request.members)if(member.eligible) {f.act(SelectMemberAction{member.partyIndex});return;}
	throw std::runtime_error("City Event fixture has no eligible member");
}
void automatic(Inputs &inputs,const XeenSaveSnapshot &initial,bool unsupported) {
	auto start=initial;start.camera={28,20,19,XeenDirection::North};
	XeenEventFile city{28,"maze0028.evt",true,unsupported?
		std::vector<XeenEventRecord>{record(0,0x0c,{0,0,104,3}),record(1,0x10,{50,13,15,0}),record(2,0x14)}:
		std::vector<XeenEventRecord>{record(0,0x27,{0}),record(1,0x12)}};
	const auto geometry=[&inputs](auto id) {
		auto map=inputs.maps.loadGeometryMap(inputs.assets,id);
		if(!map.geometry.isOutdoors())for(auto &cell:map.geometry.cells) {
			cell.rawWord=0;cell.geometry=XeenIndoorWalls{};cell.surfaceIndex=1;
			cell.rawAttributes=(cell.rawAttributes&0xf0&~kXeenAutomaticEventFlag)|1;
		}
		if(id==XeenMapIdentity(111))map.geometry.cells[4*16+4].rawAttributes|=kXeenAutomaticEventFlag;
		return map;
	};
	Fixture f(inputs,start,false,&city,geometry);unsigned completed=0,refused=0;
	f.flow->reportManual=[&](const auto &result) {
		if(std::holds_alternative<XeenManualEventCompleted>(result))++completed;
		if(std::holds_alternative<XeenEventExecutionError>(result))++refused;
	};
	f.act(NavigationAction::MoveForward);
	for(unsigned pulse=0;pulse<1000 && !f.flow->canSave();++pulse) {
		f.now+=125;f.flow->beginCycle(++f.cycle);
		if(const auto frame=f.flow->updatePresentation())f.present(*frame);
	}
	check(f.flow->canSave() && f.c.x==20 && f.c.y==20 &&
		completed==(unsupported?0u:1u) && refused==(unsupported?1u:0u) && !f.p.questFlags.isSet(3) &&
		f.w.sessionState().regionalActors(28).size()==46 && !f.w.sessionState().disabledEventCount(),
		"Automatic city Event did not complete/refuse once and return input without partial effects");
}
}
int main(int argc,char **argv) {
	try {
		check(argc==2,"usage: city-event-publication <original-installation>");
		const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"Installation unavailable");
		Inputs inputs(*installation);const auto initial=source(inputs);
		XeenEventFile city{28,"maze0028.evt",true,{record(0,0x20,{0,0}),record(1,0x0c,{0,0,104,3}),
			record(2,0x10,{50,13,15,0}),record(3,0x18,{0,0}),record(4,0x12)}};
		Fixture f(inputs,initial,false,&city);
		const auto before=XeenSaveFormat::encode(f.snapshot());const auto random=f.w.sessionState().journeyRandom();
		f.act(InteractionAction{});
		check(f.flow->canCancelInteraction() && !f.p.questFlags.isSet(3) && f.w.sessionState().regionalActors(28).size()==46,
			"WhoWill published town effects before selection");
		f.act(CancelInteractionAction{});
		check(f.flow->canSave() && before==XeenSaveFormat::encode(f.snapshot()),"WhoWill cancellation changed durable state");
		f.act(InteractionAction{});select(f);
		check(f.flow->canSave() && f.p.questFlags.isSet(3) && f.w.sessionState().regionalActors(28).size()==51 &&
			f.w.sessionState().disabledEvents().count({28,0})==1 && f.w.sessionState().journeyRandom()==random,
			"Same-map guarded publication lost flags, Spawn, AfterEvent or RNG independence");
		for(unsigned slot=46;slot<50;++slot)check(f.w.sessionState().regionalActors(28)[slot].lifecycle==XeenActorLifecycle::Unresolved,
			"Same-map publication materialized a resize gap");
		const auto saved=f.snapshot();Fixture restored(inputs,saved,false,&city);
		check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(saved),
			"Same-map flags, Event overlay and scripted slots did not restore byte-identically");

		city.records={record(0,0x0c,{0,0,104,3}),record(1,0x10,{50,13,15,0}),record(2,0x14)};
		Fixture refused(inputs,initial,false,&city);const auto unchanged=XeenSaveFormat::encode(refused.snapshot());
		refused.act(InteractionAction{});
		check(refused.flow->canSave() && unchanged==XeenSaveFormat::encode(refused.snapshot()),
			"Unsupported dependent suffix retained a partial flag or Spawn effect");

		city.records={record(0,0x20,{0,0}),record(1,0x0c,{0,0,104,3}),record(2,0x10,{50,13,15,0}),record(3,0x18,{0,0}),record(4,0x12)};
		Fixture tampered(inputs,initial,false,&city);bool injected=false;
		XeenCityEventTestAccess::composer(*tampered.flow,[&](auto &world,const auto &party,const auto &,auto,auto) {
			if(world.sessionState().disabledEventCount()) {injected=true;++const_cast<XeenPartyState &>(party).roster.at(0).currentHp;}
			return XeenEventFlow::Composition{frame(),false};
		});
		tampered.act(InteractionAction{});
		save_test::rejects([&]{select(tampered);});
		check(injected && !tampered.flow->canSave() && !tampered.p.questFlags.isSet(3) &&
			tampered.w.sessionState().regionalActors(28).size()==46 && !tampered.w.sessionState().disabledEventCount(),
			"Candidate composer tampering published effects or reopened quiet authority");
		automatic(inputs,initial,false);automatic(inputs,initial,true);
		std::cout<<"Same-map Event publication, cancellation, refusal, round-trip and candidate tampering passed\n";
		return 0;
	}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}

#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenIndoorSceneTables.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenEventSystem.h"
#include "games/xeen/XeenEventContinuation.h"
#include <iostream>
#include <stdexcept>

using namespace mmodern;
namespace mmodern {
struct XeenTrainingTestAccess {
	static void seed(XeenWorld &world) {world._sessionState._journeyRandom=XeenJourneyRandomState{77,12};}
};
}
namespace {
void check(bool value,const char *message) {if (!value) throw std::runtime_error(message);}
template<class F> void rejects(F operation) {
	bool refused=false;try {operation();} catch(const std::invalid_argument &) {refused=true;}
	check(refused,"Unsupported capability must refuse before work");
}
XeenMap tile(XeenMapIdentity id) {
	XeenMap m;m.geometry.id=id.number;m.side=id.side;m.geometry.difficulties[0]=7;
	// A second indoor logical map with arbitrary resource IDs, deliberately
	// independent of Vertigo's tile IDs. N before E is observable diagonally.
	if(id.number==60)m.geometry.neighbors=std::array<std::uint16_t,4>{62,61,0,0};
	if(id.number==61)m.geometry.neighbors=std::array<std::uint16_t,4>{63,0,0,60};
	if(id.number==62)m.geometry.neighbors=std::array<std::uint16_t,4>{0,63,60,0};
	if(id.number==63)m.geometry.neighbors=std::array<std::uint16_t,4>{0,0,61,62};
	for(auto &cell:m.geometry.cells) {cell.geometry=XeenIndoorWalls{};cell.surfaceIndex=1;}
	return m;
}
void geometry() {
	XeenWorld world(tile);
	for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
		const auto sampled=world.sampleCell(60,x,y),scene=world.sceneCell(60,x,y);
		const unsigned expected=60+(x>=16?1:0)+(y>=16?2:0);
		check(sampled&&scene&&sampled->mapId==XeenMapIdentity(expected)&&sampled->x==x%16&&sampled->y==y%16&&
			sampled->cell==scene->cell,"Logical geometry must resolve original neighbor tiles");
		for(unsigned facing=0;facing<4;++facing) {
			XeenCamera c{60,x,y,XeenDirection(facing)};
			constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
			auto next=c;const int nx=x+dx[facing],ny=y+dy[facing];
			const bool inBounds=nx>=0&&nx<32&&ny>=0&&ny<32;
			check(XeenMovement().apply(world,next,NavigationAction::MoveForward)==
				(inBounds?XeenMovementResult::Moved:XeenMovementResult::BlockedByMapBoundary),
				"Indoor navigation must follow shared logical geometry at every cell/facing");
			check(next.mapId==c.mapId&&next.x==(inBounds?nx:x)&&next.y==(inBounds?ny:y),
				"Tile seams must retain logical map identity and blocked movement must be atomic");
			const auto gameplay=XeenIndoorScene().sampleWalls(world,c,false);
			const auto presentation=XeenIndoorScene().sampleWalls(world,c,true);
			for(unsigned q=0;q<gameplay.size();++q) {
				check(gameplay[q].sourceMapId==presentation[q].sourceMapId&&gameplay[q].wallValue==presentation[q].wallValue,
					"Every facing and ray query must share gameplay/scene sampling");
				const auto cell=world.sampleCell(60,gameplay[q].sourceX,gameplay[q].sourceY);
				check(bool(cell)==bool(gameplay[q].wallValue),"Ray query boundary differs from shared sampler");
				if(cell)check(*gameplay[q].wallValue==wallAt(*cell->cell,gameplay[q].sourceFace),"Ray wall differs from resolved tile");
			}
		}
	}
	check(!world.sampleCell(60,-1,0)&&!world.sampleCell(60,0,-1)&&!world.sampleCell(60,32,0),"Indoor geometry edge must remain bounded");
	const auto back=world.sampleCell(63,-1,-1);
	check(back&&back->mapId==60&&back->x==15&&back->y==15,"Negative diagonal must resolve Y before X");
	XeenActor a;a.id={60,0};a.original.resourceId=0;a.x=15;a.y=15;a.hp=5;
	a.lifecycle=XeenActorLifecycle::Present;a.statistics.emplace();a.statistics->raw[20]=5;
	const auto view=XeenIndoorScene().classifyActors(world,{60,15,15,XeenDirection::North},{a});
	check(view.engaged()&&view.activation[0],"Generic indoor actor classification must support a second logical map");
	check(xeenIndoorActorTerrain(world,a,16,15)==XeenMonsterTerrain::Allowed,"Indoor actor must cross a resource tile seam");
	a.statistics->raw[32]=1;
	check(xeenIndoorActorTerrain(world,a,16,15)==XeenMonsterTerrain::Unsupported,"Part A must refuse unsupported indoor ranged movement");
}
void capabilities() {
	XeenConsequenceCharacters chars;XeenConsequenceInputs inputs;
	for(unsigned i=0;i<6;++i) {chars[i].rosterId=i;chars[i].permanentLevel=3;chars[i].birthYear=592;
		chars[i].currentHp=50;chars[i].endurance={15,0};
		inputs[i].might=inputs[i].speed=inputs[i].accuracy={15,0};inputs[i].luck=XeenAttributeValue{15,0};
		inputs[i].poisonResistance=XeenAttributeValue{};}
	XeenMonsterRecord m;m.raw[24]=1;m.raw[25]=16;m.raw[26]=1;m.raw[28]=8;m.raw[29]=5;
	// Mechanics do not depend on name, experience, sprite or animation effects.
	for(unsigned loop:{0u,1u})for(unsigned effect=0;effect<=15;++effect) {
		m.raw[48]=loop;m.raw[49]=effect;m.validateAttackCapabilities();
		XeenCombatRandom rng(std::vector<XeenCombatRandom::Draw>{{0,5,0},{1,8,8},{1,40,40},{1,40,40}});
		XeenEnemyAttackCandidate attack(chars,inputs,m,610,63);
		for(unsigned n=0;n<20;++n) {XeenConsequenceDraw draw{rng,1,{}};if(attack.service(draw))break;}
		check(rng.position()==4&&attack.result.damage==8&&attack.characters[0].currentHp==42,
			"Generic poison profile must retain reference target/dice/save order");
	}
	for(unsigned field:{26u,28u,29u,30u,32u}) {
		auto bad=m;bad.raw[field]=field==29?2:field==30||field==32?1:0;
		rejects([&]{XeenEnemyAttackCandidate attack(chars,inputs,bad,610,63);});
		check(chars[0].currentHp==50,"Rejected attack must leave source party untouched");
	}
	// Existing physical abilities/ranged admission remain shared with outdoors.
	m.raw[29]=0;m.raw[31]=5;m.raw[32]=1;
	for(unsigned special:{0u,5u,7u,9u}) {m.raw[30]=special;m.validateAttackCapabilities();}
	m.raw[30]=1;rejects([&]{m.validateAttackCapabilities();});
}

void cityEvents() {
	XeenObjectFile mob{28,"maze0028.mob",true,{}};
	for(unsigned slot=0;slot<46;++slot)mob.entities.monsters.push_back({int(slot%15),int(slot/15+1),0,0,slot==1?2:slot==2?73:0});
	std::vector<XeenMonsterRecord> statistics(74);
	for(auto &m:statistics){m.raw[20]=10;m.raw[24]=1;m.raw[25]=16;m.raw[26]=1;m.raw[28]=2;m.raw[29]=5;}
	const auto geometry=[](auto id) {
		auto m=tile(id);
		if(id==XeenMapIdentity(28))m.geometry.neighbors=std::array<std::uint16_t,4>{110,109,0,0};
		if(id==XeenMapIdentity(109))m.geometry.neighbors=std::array<std::uint16_t,4>{111,0,0,28};
		if(id==XeenMapIdentity(110))m.geometry.neighbors=std::array<std::uint16_t,4>{0,111,28,0};
		if(id==XeenMapIdentity(111))m.geometry.neighbors=std::array<std::uint16_t,4>{0,0,109,110};
		return m;
	};
	XeenWorld source(geometry,[&](auto){return mob;});source.markEncounterSession(XeenEncounterEntry::Journey);
	XeenTrainingTestAccess::seed(source);
	auto world=source.transitionCandidate();world->stageVertigoActors(mob,statistics);
	XeenPartyState party;XeenGameFlags flags;
	XeenCamera camera{28,24,10,XeenDirection::North};
	const auto record=[&](unsigned line,unsigned opcode,std::vector<std::uint8_t> operands) {
		XeenEventRecord r;r.x=24;r.y=10;r.direction=4;r.line=line;r.opcode=opcode;r.parameters=std::move(operands);
		r.lengthField=5+r.parameters.size();return r;
	};
	XeenEventFile file{28,"maze0028.evt",true,{record(0,0x27,{3}),record(1,0x12,{})}};
	XeenEventSystem system([&](auto){return XeenEventScript(file);},[&](auto){
		XeenEventTextFile text{28,"aaze0028.txt",true,std::vector<std::string>(64)};text.strings[3]="Small seat sign";return text;
	});
	auto label=system.runManualEvent(*world,party,camera,flags);
	const auto *pending=std::get_if<XeenEventExecutionSuspended>(&label);
	check(pending && pending->request.kind==XeenPresentationKind::SceneLabelSignReduced &&
		pending->request.text=="Small seat sign" && pending->request.response==XeenPresentationResponseRequirement::Presented,
		"Off-route small sign must suspend for one scene draw with resource text");
	{
		auto state=pending->state;
		state.callStack.push_back({{28,75,76,4}});
		state.pendingTransferSource=state.pendingPresentation->request.source;
		state.pendingPresentation->conditional=XeenEventConditional{XeenEventComparison::Equal,44,0,7};
		state.pendingPresentation->request.members.push_back({0,0,"Witness",true});
		state.pendingPresentation->request.npc=XeenEventNpc{1,2,3,1,5};
		const XeenEventContinuation continuation(state);continuation.check(state);
		const std::vector<std::function<void(XeenEventExecutionState &)>> tamper{
			[](auto &s){++s.logicalAddress.x;},[](auto &s){++s.logicalAddress.y;},
			[](auto &s){++s.logicalAddress.line;},[](auto &s){s.logicalAddress.mapId=23;},
			[](auto &s){s.lookupDirection=XeenDirection::West;},[](auto &s){++s.workingCamera.x;},
			[](auto &s){s.workingGameFlags.set(12);},[](auto &s){++s.instructionCount;},
			[](auto &s){++s.callStack[0].returnAddress.line;},[](auto &s){s.callStack.clear();},
			[](auto &s){s.selectedObject=XeenObjectIdentity{28,1};},[](auto &s){s.activeCharacterIndex=3;},
			[](auto &s){s.preferredRewardRecipient=2;},[](auto &s){s.rewardReceipt.delivered=1;},
			[](auto &s){s.rewardReceipt.entries[0].item.id=1;},[](auto &s){s.pendingRewards.enqueue({1,1,0,0});},
			[](auto &s){s.missingInstructionPolicy=XeenEventMissingInstructionPolicy::ExplicitCall;},
			[](auto &s){++s.pendingTransferSource->fileOffset;},
			[](auto &s){auto f=s.currentScript->file();f.records[0].opcode=0x12;s.currentScript=XeenEventScript(f);},
			[](auto &s){auto f=s.currentScript->file();f.resourceName="replacement.evt";s.currentScript=XeenEventScript(f);},
			[](auto &s){s.currentScript.reset();},[](auto &s){s.pendingPresentation.reset();},
			[](auto &s){s.pendingPresentation->request.kind=XeenPresentationKind::TempleService;},
			[](auto &s){s.pendingPresentation->request.response=XeenPresentationResponseRequirement::YesNo;},
			[](auto &s){++s.pendingPresentation->request.source.fileOffset;},
			[](auto &s){s.pendingPresentation->request.text="Changed";},
			[](auto &s){s.pendingPresentation->request.members[0].eligible=false;},
			[](auto &s){s.pendingPresentation->request.npc->targetLine=6;},
			[](auto &s){s.pendingPresentation->conditional->targetLine=8;},
			[](auto &s){s.pendingPresentation->continuation=XeenEventPendingContinuation::Terminate;}
		};
		for(const auto &mutation:tamper) {
			auto changed=state;mutation(changed);bool rejected=false;
			try {continuation.check(changed);}catch(const std::logic_error &){rejected=true;}
			check(rejected,"Detached continuation tampering escaped its exact value preimage");
		}
	}
	check(std::holds_alternative<XeenManualEventCompleted>(system.resumeManualEvent(pending->state,
		XeenPresentationResponse::Presented,*world,party,camera,flags)),"Small sign must continue after its draw");

	// No prefix of an incomplete effect group may change detached values.
	file.records={record(0,0x0c,{0,0,20,12}),record(1,0x10,{50,7,6,0}),record(2,0x18,{0,0}),record(3,0x14,{})};
	XeenEventSystem unsupported([&](auto){return XeenEventScript(file);});
	const auto random=world->sessionState().journeyRandom();
	const auto refused=unsupported.runManualEvent(*world,party,camera,flags);
	check(std::holds_alternative<XeenEventExecutionError>(refused) && !flags.isSet(12) &&
		world->sessionState().regionalActors(28).size()==46 && !world->sessionState().disabledEventCount() &&
		world->sessionState().journeyRandom()==random,"Unsupported suffix must refuse before Spawn/flags/AfterEvent/RNG");

	file.records.pop_back();file.records.push_back(record(3,0x12,{}));
	XeenEventSystem supported([&](auto){return XeenEventScript(file);});
	check(std::holds_alternative<XeenManualEventCompleted>(supported.runManualEvent(*world,party,camera,flags)),
		"Generic flag/Spawn/AfterEvent group must execute in a detached city candidate");
	check(flags.isSet(12) && world->sessionState().disabledEvents().count({28,0})==1 &&
		world->sessionState().regionalActors(28).size()==51 && world->sessionState().journeyRandom()==random,
		"Spawn/AfterEvent must retain generic effect identities without gameplay RNG draws");
	for(unsigned slot=46;slot<50;++slot) {
		const auto &gap=world->sessionState().regionalActors(28)[slot];
		check(gap.lifecycle==XeenActorLifecycle::Unresolved && !gap.statistics && !gap.original.hasResource(),
			"Resize gap slots 46-49 must stay unresolved");
	}
	const auto &spawned=world->sessionState().regionalActors(28)[50];
	check(spawned.original.resourceId==0 && spawned.x==7 && spawned.y==6 && spawned.hp==10 && !spawned.activated,
		"Explicit new Spawn slot must use the reference default type and full HP");
	world->applySpawn(1,9,8,0);
	check(world->sessionState().regionalActors(28)[1].original.resourceId==2,
		"Spawn must reuse an original slot's sprite type");
	xeenValidateVertigoActors(*world,world->sessionState().regionalActors(28));
	const auto presentation=source.prepareSpawnPresentation(*world);
	check(presentation.animation({28,50}) && presentation.animation({28,50})->frame<8 &&
		presentation.animation({28,1}) && presentation.animation({28,1})->frame<8,
		"Prepared Spawn presentation must carry cosmetic frames for new and reused slots");
	check(source.sessionState().journeyRandom()==random,"Spawn preparation must leave live gameplay RNG untouched");

	file.records={record(0,0x11,{5})};
	check(xeenRegionalService(file,camera)==5 && xeenRegionalInteraction(file,camera)==XeenRegionalInteraction::Training,
		"Terminal service identity must come from its opcode at an arbitrary resource cell");
	file.records[0].parameters={3};
	check(!xeenRegionalService(file,camera),"Unsupported service action must not acquire service authority");
	file.records[0].parameters={5,0};
	check(!xeenRegionalService(file,camera),"Malformed service operands must not acquire service authority");
}
}
int main() {
	try {geometry();capabilities();cityEvents();std::cout<<"Whole-map geometry, generic attacks and detached city Event capabilities passed\n";return 0;}
	catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}

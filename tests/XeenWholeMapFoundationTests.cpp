#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenIndoorSceneTables.h"
#include "games/xeen/XeenCombatRules.h"
#include <iostream>
#include <stdexcept>

using namespace mmodern;
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
}
int main() {
	try {geometry();capabilities();std::cout<<"Whole-map geometry and generic attack foundations passed\n";return 0;}
	catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}

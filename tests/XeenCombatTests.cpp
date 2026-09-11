#include "XeenCombatTestSupport.h"
#include <iostream>
using namespace combat_test;
void mixed() {
	CombatFixture f{XeenCombatRandom(mixedTape())};const auto before=f.p.roster.characters();f.enter();
	const auto old=f.combat->approachState();check(!XeenActorApproach::authoritative(f.w,f.p,f.camera,old),"old approach remains stale");
	check(f.p.encounterContext->minutes==490&&f.p.encounterContext->ctr24==1,"entry time");
	check(f.action(Command::Attack).damage==7,"literal first attack7");f.action(Command::Block);
	check(f.action(Command::Attack).damage==4,"literal ranger attack4");check(f.action(Command::Attack).damage==0,"literal miss");
	f.action(Command::Block);f.action(Command::Block);check(f.combat->pending()==Work::Enemy,"enemy cannot disappear");
	const auto enemy=f.combat->ticket();check(f.combat->command(enemy,Command::Attack).status==Status::Refused,"automatic phase refuses player");
	check(f.service().injuryCount==0,"blocked cleric miss");check(f.combat->service(enemy).status==Status::Stale,"enemy replay");
	f.service();check(f.p.encounterContext->minutes==491,"round minute");
	const auto lethal=f.action(Command::Attack);check(lethal.damage==13&&lethal.xpCount==6,"lethal13 and six receipts");
	check(f.combat->phase()==Phase::VictoryAwaitingEnd,"lethal is not Victory");
	for(auto id:kXeenCombatOwners)check(f.p.roster.combatInputs(id)->experience==82,"XP divide before multiply");
	const auto &a=f.w.sessionState().actors()[5];check(a.hp==0&&a.x==-128&&a.y==-128&&a.lifecycle==XeenActorLifecycle::Defeated,"identity-preserving removal");
	check(f.service().status==Status::Victory&&f.p.encounterContext->minutes==492,"separate victory end minute");
	f.combat->fail(f.combat->ticket());check(f.combat->phase()==Phase::Victory,"terminal observation failure preserves Victory");
	f.w.discardMapCache();f.w.map(20);f.w.objectFile(20);check(a.hp==0&&f.p.roster.combatInputs(0)->experience==82,"cache does not heal or reward");
	for(unsigned i=0;i<30;++i)remove_test::checkSameCharacter(before[i],f.p.roster.at(i));
}
void defeat() {
	CombatFixture f{XeenCombatRandom(defeatTape())};f.enter();
	const int targets[]{1,0,18,14,11,6},finalHp[]{-17,0,-8,-12,-14,-19};
	for(unsigned round=0;round<6;++round){f.blockRound();const auto r=f.service();
		check(r.injuryCount==(round==1?1u:2u),"critical applications");
		check(f.p.roster.at(targets[round]).currentHp==finalHp[round],"critical HP trace");
		if(round==0)check(r.injuries[0].afterHp==-5&&r.injuries[0].conditions[12]==1&&r.injuries[1].conditions[12]==1&&r.injuries[1].conditions[13]==1,"simultaneous injury conditions");
		if(round==5)check(r.injuries[0].afterAc==1&&r.injuries[1].beforeAc==1,"critical uses broken armor AC");
		if(round<5){check(f.combat->pending()==Work::Round,"pending round");f.service();}
	}
	check(f.combat->phase()==Phase::Defeat&&f.p.encounterContext->minutes==495,"real all-Block defeat without final minute");
	check(f.w.sessionState().actors()[5].hp==20,"defeat preserves enemy HP");for(auto id:kXeenCombatOwners)check(f.p.roster.combatInputs(id)->experience==0,"no defeat XP");
}
void seeds() {
	CombatFixture f;f.enter();f.blockRound();f.service();f.service();unsigned actions=0;
	while(f.combat->phase()!=Phase::Victory&&actions++<100){if(f.combat->phase()==Phase::PlayerReady)f.action(Command::Attack);else f.service();}
	check(f.combat->phase()==Phase::Victory&&f.p.encounterContext->minutes==493,"seed1 victory493");
	check(f.p.roster.at(1).currentHp==-4&&f.p.roster.at(1).conditions[12]==1,"seed1 unconscious XP recipient");
	CombatFixture loss{XeenCombatRandom(19)};
	for(unsigned i=0;i<6;++i)for(unsigned slot=0;slot<9;++slot)if(loss.p.roster.at(kXeenCombatOwners[i]).armor[slot].id)
		check(loss.combat->equipment(loss.combat->ticket(),i,XeenInventoryCategory::Armor,slot,XeenEquipmentOperation::Remove).status==XeenEquipmentStatus::Success,"real armor removal");
	loss.enter();unsigned blocks=0,enemies=0;
	while(loss.combat->phase()!=Phase::Defeat&&blocks<100){if(loss.combat->phase()==Phase::PlayerReady){loss.action(Command::Block);++blocks;}else{if(loss.combat->pending()==Work::Enemy)++enemies;loss.service();}}
	check(blocks==35&&enemies==11&&loss.p.encounterContext->minutes==500,"seed19 defeat500 with singleton draws");
	const int hp[]{-7,-1,-1,-5,-3,-7};for(unsigned i=0;i<6;++i)check(loss.p.roster.at(kXeenCombatOwners[i]).currentHp==hp[i],"seed19 HP");
}
void injuryAndBare() {
	std::vector<Draw> tape{{1,20,20},{1,6,6},{1,6,6},{1,4,4},{1,6,6},{1,6,6},
		{1,3,1},{1,3,1},{1,20,1},{0,5,1},{1,20,18},{1,4,2},{1,6,2},{1,6,2},
		{0,5,1},{1,20,20},{1,6,6},{1,6,6},{1,4,4},{1,6,5},{1,6,5}};
	CombatFixture f{XeenCombatRandom(tape)};f.enter();f.blockRound();f.service();f.service();
	f.action(Command::Block);f.action(Command::Attack);while(f.combat->phase()==Phase::PlayerReady)f.action(Command::Block);
	f.service();check(f.p.roster.at(18).currentHp==12,"unblocked Tyro HP12");f.service();f.blockRound();auto r=f.service();
	check(r.injuries[0].afterHp==0&&r.injuries[1].afterHp==-10&&r.injuries[1].conditions[12]==1&&r.injuries[1].conditions[13]==0,"unconscious at -10 with armor break");
	check(r.armorCount==4,"all four equipped Tyro armor slots break");
	for(unsigned i=0;i<r.armorCount;++i)check(r.armor[i].after.state==(r.armor[i].before.state|0x80)&&
		r.armor[i].after.id==r.armor[i].before.id&&r.armor[i].after.material==r.armor[i].before.material&&r.armor[i].after.frame==r.armor[i].before.frame,"raw armor bytes preserved");
	CombatFixture bare{XeenCombatRandom(std::vector<Draw>{{1,20,10}})};
	bare.combat->equipment(bare.combat->ticket(),4,XeenInventoryCategory::Weapons,0,XeenEquipmentOperation::Remove);bare.enter();
	for(unsigned i=0;i<4;++i)bare.action(Command::Block);check(bare.action(Command::Attack).damage==0&&bare.combat->random().position()==1,"bare cleric legitimate zero after resistance");
	CombatFixture bow{XeenCombatRandom(std::vector<Draw>{{1,20,10}})};
	bow.combat->equipment(bow.combat->ticket(),2,XeenInventoryCategory::Weapons,0,XeenEquipmentOperation::Remove);bow.enter();
	bow.action(Command::Block);bow.action(Command::Block);check(bow.action(Command::Attack).damage==1&&bow.combat->random().position()==1,"bow-only uses bare melee with no weapon dice");
}
void fixedObservations() {
	using Operation=XeenCombatOperation;using Outcome=XeenCombatAttackOutcome;
	// Each returned value outlives every coordinator, owner and RNG used to create it.
	const auto bare=[](unsigned roll) {
		CombatFixture f{XeenCombatRandom(std::vector<Draw>{{1,20,roll}})};
		f.combat->equipment(f.combat->ticket(),4,XeenInventoryCategory::Weapons,0,XeenEquipmentOperation::Remove);f.enter();
		for(unsigned i=0;i<4;++i)f.action(Command::Block);
		const XeenCombatResult result=f.action(Command::Attack);return result;
	};
	const XeenCombatResult miss=bare(1),zero=bare(10);
	for(const auto &r:{miss,zero})check(r.operation==Operation::PlayerAttack&&r.actingOwner==1&&r.participant==4&&
		r.targetMonster==XeenMonsterIdentity{20,5}&&!r.actingMonster&&!r.targetOwner&&r.damage==0&&
		r.status==Status::Advanced&&r.phase==Phase::PlayerReady&&r.revision==r.oldRevision+1&&r.generation>0,
		"copied Rebecca attack identity and publication facts");
	check(miss.attackOutcome==Outcome::Miss&&zero.attackOutcome==Outcome::HitZeroDamage,"bare miss differs from successful zero-damage hit");
	const XeenCombatResult positive=[] {
		CombatFixture f{XeenCombatRandom(mixedTape())};f.enter();return f.action(Command::Attack);
	}();
	check(positive.operation==Operation::PlayerAttack&&positive.actingOwner==0&&positive.participant==0&&
		positive.targetMonster==XeenMonsterIdentity{20,5}&&positive.attackOutcome==Outcome::HitPositiveDamage&&positive.damage==7,
		"copied positive player hit");
	const XeenCombatResult block=[] {CombatFixture f;f.enter();return f.action(Command::Block);}();
	check(block.operation==Operation::Block&&block.actingOwner==0&&block.participant==0&&
		block.attackOutcome==Outcome::NotApplicable&&!block.targetOwner&&!block.targetMonster&&!block.critical,
		"Block is not a zero-damage attack");
	const XeenCombatResult ordinary=[] {
		CombatFixture f{XeenCombatRandom(std::vector<Draw>{{1,20,19},{1,4,4},{1,6,2},{1,6,3}})};
		f.enter();f.blockRound();return f.service();
	}();
	check(ordinary.operation==Operation::EnemyAttack&&ordinary.participant==6&&!ordinary.actingOwner&&
		ordinary.actingMonster==XeenMonsterIdentity{20,5}&&ordinary.targetOwner==1&&!ordinary.targetMonster&&
		ordinary.attackOutcome==Outcome::HitPositiveDamage&&!ordinary.critical&&ordinary.damage==5&&ordinary.injuryCount==1&&
		ordinary.injuries[0].owner==1&&ordinary.injuries[0].beforeHp==7&&ordinary.injuries[0].afterHp==2,
		"copied ordinary enemy hit and selected target");
	for(unsigned selection:{0u,1u}) {
		const auto observations=[&] {
			std::vector<Draw> tape{{1,20,20},{1,6,6},{1,6,6},{1,4,4},{1,6,6},{1,6,6},{0,5,selection},{1,20,1}};
			CombatFixture f{XeenCombatRandom(tape)};f.enter();f.blockRound();const auto critical=f.service();
			f.service();f.blockRound();const auto miss=f.service();return std::array<XeenCombatResult,2>{critical,miss};
		}();
		const auto critical=observations[0],enemyMiss=observations[1];
		check(critical.operation==Operation::EnemyAttack&&critical.actingMonster==XeenMonsterIdentity{20,5}&&critical.targetOwner==1&&
			critical.critical&&critical.attackOutcome==Outcome::HitPositiveDamage&&critical.damage==24&&critical.injuryCount==2&&
			critical.injuries[0].owner==1&&critical.injuries[1].owner==1&&critical.injuries[0].beforeHp==7&&
			critical.injuries[0].afterHp==-5&&critical.injuries[1].beforeHp==-5&&critical.injuries[1].afterHp==-17,
			"copied critical preserves same target and both ordered applications");
		check(enemyMiss.operation==Operation::EnemyAttack&&enemyMiss.actingMonster==XeenMonsterIdentity{20,5}&&
			enemyMiss.targetOwner==(selection==0?0:18)&&enemyMiss.participant==6&&!enemyMiss.actingOwner&&
			enemyMiss.attackOutcome==Outcome::Miss&&!enemyMiss.critical&&enemyMiss.damage==0&&enemyMiss.injuryCount==0,
			"copied enemy natural-one miss retains selected owner0 or owner18");
	}
}
int main(){try{mixed();defeat();seeds();injuryAndBare();fixedObservations();std::cout<<"Combat literal mixed victory, all-Block defeat, seeds, injury and fixed observations passed\n";return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

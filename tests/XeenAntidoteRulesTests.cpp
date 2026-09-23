#include "games/xeen/XeenAntidoteUse.h"
#include <iostream>
#include <stdexcept>

using namespace mmodern;
namespace {
void check(bool value,const char *message) { if(!value) throw std::runtime_error(message); }
}
int main() {
	try {
		const XeenItem original{10,37,1,255};
		check(XeenAntidoteUse::eligible(original),"Opaque frame must be admitted");
		for(auto bad:{XeenItem{10,37,0,0},XeenItem{10,37,0x41,0},XeenItem{10,37,0x81,0},
			XeenItem{11,37,1,0},XeenItem{10,38,1,0}})
			check(!XeenAntidoteUse::eligible(bad),"Ineligible antidote bytes were admitted");
		const auto debited=XeenAntidoteUse::debit(original);
		check(debited.material==10 && debited.id==37 && debited.state==0 && debited.frame==255,"Debit must change only charge bits");
		for(const auto charges:{2,63}) {
			const XeenItem charged{10,37,static_cast<std::uint8_t>(charges),173};
			const auto next=XeenAntidoteUse::debit(charged);
			check(next.material==charged.material && next.id==charged.id && next.frame==charged.frame &&
				next.state==charges-1,"Multiple charges must debit exactly once");
		}
		bool invalidDebit=false;
		try {XeenAntidoteUse::debit({10,37,0,0});} catch(const std::invalid_argument &) {invalidDebit=true;}
		check(invalidDebit,"Zero-charge antidote cannot be debited");
		XeenCharacter living;living.conditions[3]=2;living.conditions[12]=1;living.currentHp=10;
		const auto cured=XeenAntidoteUse::effect(0,18,&living,1,true);
		check(cured.source==0 && cured.target==18 && cured.poisonBefore==2 && cured.poisonAfter==0 &&
			living.conditions[3]==0 && living.conditions[12]==0 && living.currentHp==10,"Living target condition effect");
		XeenCharacter dead;dead.currentHp=0;dead.conditions[3]=1;dead.conditions[13]=1;dead.conditions[12]=1;
		XeenAntidoteUse::effect(0,11,&dead,1,true);
		check(dead.conditions[3]==0 && dead.conditions[13]==1 && dead.conditions[12]==1 && dead.currentHp==0,"Dead target is not resurrected");
		XeenCharacter cancelled;cancelled.conditions[3]=1;
		const auto noTarget=XeenAntidoteUse::effect(0,{},nullptr,1,true);
		check(!noTarget.target && cancelled.conditions[3]==1,"Cancellation has no target effect");
		for(const auto blocked:{13,14,15}) {
			XeenCharacter impaired;impaired.currentHp=7;impaired.conditions[3]=1;
			impaired.conditions[12]=1;impaired.conditions[blocked]=1;
			XeenAntidoteUse::effect(0,0,&impaired,2,false);
			check(impaired.conditions[3]==0 && impaired.conditions[12]==1 && impaired.conditions[blocked]==1 &&
				impaired.currentHp==7,"Dead, stoned and eradicated conditions cannot clear unconsciousness");
		}
		XeenCharacter healthy;healthy.currentHp=9;healthy.currentSp=4;healthy.conditions[2]=3;
		const auto healthyResult=XeenAntidoteUse::effect(0,0,&healthy,63,false);
		check(healthyResult.poisonBefore==0 && healthyResult.poisonAfter==0 && healthyResult.spentCharge==63 &&
			!healthyResult.exhausted && healthy.currentHp==9 && healthy.currentSp==4 && healthy.conditions[2]==3,
			"Healthy same-owner target spends charge without changing other state");
		XeenCharacter zeroHp;zeroHp.currentHp=0;zeroHp.conditions[3]=2;zeroHp.conditions[12]=1;
		XeenAntidoteUse::effect(0,18,&zeroHp,1,true);
		check(zeroHp.currentHp==0 && zeroHp.conditions[3]==0 && zeroHp.conditions[12]==1,
			"Zero-HP target clears Poison without numerical or unconsciousness change");
		XeenItemCategory items{};items[0]={2,9,3,4};items[1]=debited;items[2]={3,8,7,6};items[4]={4,7,5,4};
		XeenAntidoteUse::settle(items,1,true);
		check(xeenSameItem(items[0],{2,9,3,4}) && xeenSameItem(items[1],{3,8,7,6}) &&
			xeenSameItem(items[2],{4,7,5,4}) && !items[3].id && !items[8].frame,"Exhaustion compacts only occupied misc bytes");
		XeenItemCategory remaining{};remaining[0]={0,0,17,8};remaining[1]={10,37,2,255};
		XeenAntidoteUse::settle(remaining,1,false);
		check(remaining[0].state==17 && remaining[0].frame==8 && remaining[1].state==2 && remaining[1].frame==255,
			"Remaining charges preserve holes and metadata");
		std::cout<<"Antidote byte, condition and compaction rules passed\n";
		return 0;
	} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}

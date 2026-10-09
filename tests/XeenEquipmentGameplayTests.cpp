#include "XeenSaveGameplayTestSupport.h"
#include "XeenRegionalSaveGameplayTestSupport.h"
#include "XeenEquipmentTestSupport.h"
#include "XeenChildProcessTestSupport.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include <fstream>
#include <iostream>
#include <limits>

using namespace gameplay_test;
using equipment_test::sameItem;
using Category = XeenInventoryCategory;
using Operation = XeenEquipmentOperation;
using Status = XeenEquipmentStatus;
namespace mmodern {
struct XeenInventoryTestAccess {
	static void bumpEpoch(XeenEventFlow &flow) { ++flow._inventoryEpoch; }
	static void driftSource(XeenEventFlow &flow) { flow._inventory.source=1; }
	static void driftCategory(XeenEventFlow &flow) { flow._inventory.category=XeenInventoryCategory::Armor; }
	static void driftSlot(XeenEventFlow &flow) { flow._inventory.slot=1; }
	static void exhaust(XeenEventFlow &flow) { flow._inventoryEpoch=std::numeric_limits<std::uint64_t>::max()-2; }
	static const char *feedback(const XeenEventFlow &flow) { return flow._inventoryFeedback; }
	static bool equipmentArmed(const XeenEventFlow &flow) { return flow._equipmentSelection.has_value(); }
};
}
namespace {
const XeenCamera start{1,1,1,XeenDirection::North};
struct Quiet { std::ostringstream out; std::streambuf *old=std::cout.rdbuf(out.rdbuf()); ~Quiet(){std::cout.rdbuf(old);} };

void character(XeenCharacter &c, std::uint8_t owner) {
	c.rosterId=owner; c.name="Equipment owner"; c.characterClass=XeenCharacterClass::Paladin;
	c.birthYear=592; c.permanentLevel=1; c.intellect.permanent=11;
	c.personality.permanent=13; c.endurance.permanent=19; c.hasSpells=true;
	c.currentHp=32767; c.currentSp=-32768;
}
void observerLifetimeAndRecovery() {
	for(bool throws:{false,true}) {
		Fixture f;character(f.initial.roster.at(0),0);character(f.initial.roster.at(1),1);
		f.initial.roster.at(0).armor[0]={105,10,0,0};auto services=f.services();XeenPartyState *party=nullptr;
		services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){party=&const_cast<XeenPartyState &>(p);};
		services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
			handle(InspectInventoryAction{});handle(DialogKeyAction{'a'});handle(SelectInventorySlotAction{0});
			XeenEquipmentResult copy;bool observed=false;
			f.flow->reportEquipment=[&](const auto &r){observed=true;copy=r;f.flow->invalidateInventory();
				f.flow->abandonPresentation();handle(EquipmentInventoryAction{});handle(TransferInventoryAction{});handle(InspectInventoryAction{});
				check(f.flow->inventoryOpen(),"reentrant abandonment closed inventory");if(throws)throw std::runtime_error("equipment observer failure");};
			handle(EquipmentInventoryAction{});
			check(observed&&copy.status==Status::Success&&copy.afterItem->frame==9&&party->roster.at(0).armor[0].frame==9,
				"observer invalidated borrowed result/publication");
			check(f.flow->inventoryOpen()!=throws,"observer recovery state");
			if(throws){handle(InspectInventoryAction{});check(f.flow->inventoryOpen()&&party->roster.at(0).armor[0].frame==9,"reopen lost published frame");}
			return true;
		};Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"equipment observer recovery");
	}
}

void certificateInvalidationMatrix() {
	using Mutation=std::function<void(XeenEventFlow &,XeenPartyState &)>;
	const std::vector<std::pair<const char *,Mutation>> cases{
		{"epoch",[](auto &flow,auto &){XeenInventoryTestAccess::bumpEpoch(flow);}},
		{"membership",[](auto &,auto &party){party.party=XeenParty::fromRosterIds({0,1,1});}},
		{"source",[](auto &flow,auto &){XeenInventoryTestAccess::driftSource(flow);}},
		{"category",[](auto &flow,auto &){XeenInventoryTestAccess::driftCategory(flow);}},
		{"slot",[](auto &flow,auto &){XeenInventoryTestAccess::driftSlot(flow);}},
		{"material",[](auto &,auto &party){party.roster.at(0).weapons[0].material=77;}},
		{"id",[](auto &,auto &party){party.roster.at(0).weapons[0].id=13;}},
		{"state",[](auto &,auto &party){party.roster.at(0).weapons[0].state=8;}},
		{"frame",[](auto &,auto &party){party.roster.at(0).weapons[0].frame=1;}},
		{"rosterId",[](auto &,auto &party){party.roster.at(0).rosterId=7;}}
	};
	for(const auto &[name,mutate]:cases) {
		Fixture f; character(f.initial.roster.at(0),0); character(f.initial.roster.at(1),1);
		f.initial.party=XeenParty::fromRosterIds({0,1,0});
		for(auto owner:{0u,1u}) { f.initial.roster.at(owner).weapons[0]={69,12,0,0};f.initial.roster.at(owner).weapons[1]={69,12,0,0};f.initial.roster.at(owner).armor[0]={69,10,0,0}; }
		auto services=f.services();XeenPartyState *party=nullptr;unsigned reports=0;
		services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){party=&const_cast<XeenPartyState &>(p);};
		services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
			handle(InspectInventoryAction{});handle(SelectInventorySlotAction{0});mutate(*f.flow,*party);handle(EquipmentInventoryAction{});
			check(reports==0,"stale certificate invoked equipment helper");handle(EquipmentInventoryAction{});
			check(reports==0,"consumed stale certificate replayed");return true;
		};
		f.flow=nullptr;services.configureFlow=[&](auto &flow,const auto &){f.flow=&flow;flow.dialogText=&dos_test::text();flow.reportEquipment=[&](const auto &){++reports;};};
		Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,name);
	}
	// Reconstruct, close/reopen, invalid F-key and exhaustion all disarm without
	// authorizing a retained viewing highlight.
	for(unsigned mode=0;mode<4;++mode) {
		Fixture f;character(f.initial.roster.at(0),0);character(f.initial.roster.at(1),1);f.initial.roster.at(0).weapons[0]={69,12,0,0};
		auto services=f.services();unsigned reports=0;
		services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
			handle(InspectInventoryAction{});handle(SelectInventorySlotAction{0});
			if(mode==0)f.flow->refresh(true);
			else if(mode==1){handle(CancelInteractionAction{});handle(InspectInventoryAction{});}
			else if(mode==2)handle(DialogKeyAction{'a'});
			else XeenInventoryTestAccess::exhaust(*f.flow);
			handle(EquipmentInventoryAction{});check(reports==0,"invalidation path replayed equipment");return true;
		};
		services.configureFlow=[&](auto &flow,const auto &){f.flow=&flow;flow.dialogText=&dos_test::text();flow.reportEquipment=[&](const auto &){++reports;};};
		Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"certificate invalidation path");
	}
	// An actual ordinary-animation idle rebase preserves the certificate.
	{
		Fixture f;character(f.initial.roster.at(0),0);character(f.initial.roster.at(1),1);f.initial.roster.at(0).weapons[0]={69,12,0,0};f.ordinary=true;
		auto services=f.services();std::uint64_t now=99;services.clock=[&]{return now;};unsigned reports=0;
		services.show=[&](const auto &,const auto &handle,const auto &,const auto &idle,const auto &){
			handle(InspectInventoryAction{});handle(SelectInventorySlotAction{0});const auto phase=f.phases.back();now=200;idle();
			check(f.phases.back()==phase,"items dialog must pause ordinary composition");handle(EquipmentInventoryAction{});
			check(reports==1&&f.flow->equipmentResult()->status==Status::Success,"ordinary rebase invalidated certificate");return true;
		};
		services.configureFlow=[&](auto &flow,const auto &){f.flow=&flow;flow.dialogText=&dos_test::text();flow.reportEquipment=[&](const auto &){++reports;};};
		Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"timed equipment rebase");
	}
}

void publicationFailureAndHud() {
	// A layout failure after the helper publishes closes the disposable panel,
	// keeps the one frame mutation, and permits a clean reopen.
	{
		Fixture f;character(f.initial.roster.at(0),0);character(f.initial.roster.at(1),1);f.initial.roster.at(0).armor[0]={105,10,0,0};
		auto services=f.services();XeenPartyState *party=nullptr;unsigned reports=0;
		services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){party=&const_cast<XeenPartyState &>(p);};
		services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
			handle(InspectInventoryAction{});handle(DialogKeyAction{'a'});handle(SelectInventorySlotAction{0});
			// Neutral synthetic templates may use either font size. Force a
			// layout failure in both, independently of any DOS template wording.
			auto bytes=fontBytes();for(unsigned c=0;c<128;++c)bytes[0x1000+c]=bytes[0x1080+c]=255;f.font=XeenFontFormat(bytes);
			handle(EquipmentInventoryAction{});check(reports==1&&party->roster.at(0).armor[0].frame==9&&!f.flow->inventoryOpen(),"post-publication layout recovery");
			f.font=XeenFontFormat(fontBytes());handle(InspectInventoryAction{});check(f.flow->inventoryOpen(),"layout recovery could not reopen");return true;
		};
		services.configureFlow=[&](auto &flow,const auto &){f.flow=&flow;flow.dialogText=&dos_test::text();flow.reportEquipment=[&](const auto &){++reports;};};
		Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"equipment layout recovery");
	}
	// The first clean-scene composition may fail after publication; one bounded
	// recovery composition succeeds without replaying the helper.
	{
		Fixture f;character(f.initial.roster.at(0),0);character(f.initial.roster.at(1),1);f.initial.roster.at(0).armor[0]={105,10,0,0};
		auto services=f.services();auto compose=services.compose;unsigned failures=0,reports=0;XeenPartyState *party=nullptr;
		services.compose=[&](auto &w,const auto &p,const auto &c,std::uint64_t phase){if(f.failCompose){f.failCompose=false;++failures;throw std::runtime_error("equipment scene failure");}return compose(w,p,c,phase);};
		services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){party=&const_cast<XeenPartyState &>(p);};
		services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){handle(InspectInventoryAction{});handle(DialogKeyAction{'a'});handle(SelectInventorySlotAction{0});f.failCompose=true;
			handle(EquipmentInventoryAction{});check(failures==1&&reports==1&&party->roster.at(0).armor[0].frame==9&&!f.flow->inventoryOpen(),"bounded scene recovery");
			handle(InspectInventoryAction{});check(f.flow->inventoryOpen(),"scene recovery could not reopen");return true;};
		services.configureFlow=[&](auto &flow,const auto &){f.flow=&flow;flow.dialogText=&dos_test::text();flow.reportEquipment=[&](const auto &){++reports;};};
		Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"equipment scene recovery");
	}
	// A failed clean-base recovery is terminal to the Application handler and a
	// later F9 cannot write, while the already published frame remains observed.
	{
		Fixture f;character(f.initial.roster.at(0),0);character(f.initial.roster.at(1),1);f.initial.roster.at(0).armor[0]={105,10,0,0};
		auto services=f.services();auto compose=services.compose;unsigned remaining=0,reports=0;XeenItem published{};
		services.compose=[&](auto &w,const auto &p,const auto &c,std::uint64_t phase){if(remaining){--remaining;throw std::runtime_error("persistent equipment scene failure");}return compose(w,p,c,phase);};
		const auto target=std::filesystem::current_path()/("equipment-fatal-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64())+".mmsave");
		services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){handle(InspectInventoryAction{});handle(DialogKeyAction{'a'});handle(SelectInventorySlotAction{0});remaining=2;
			try{handle(EquipmentInventoryAction{});check(false,"fatal recovery did not throw");}catch(const std::runtime_error &){}
			check(reports==1&&published.frame==9&&!handle(SaveGameAction{})&&!std::filesystem::exists(target),"fatal equipment flow remained saveable/replayed");return true;};
		services.configureFlow=[&](auto &flow,const auto &){f.flow=&flow;flow.dialogText=&dos_test::text();flow.reportEquipment=[&](const auto &r){++reports;published=*r.afterItem;};};
		Quiet quiet;check(Application().playGameplay(services,start,target,false)==0,"fatal equipment recovery harness");
	}
	// Recomposition exposes the changed maximum to the HUD while preserving a
	// current value above the lowered maximum after contextual removal.
	{
		Fixture f;character(f.initial.roster.at(0),0);character(f.initial.roster.at(1),1);auto &c=f.initial.roster.at(0);c.currentHp=16;c.armor[0]={105,10,0,0};
		auto services=f.services();auto compose=services.compose;std::vector<int> maxima;const XeenPartyState *party=nullptr;
		const XeenCharacterRulesContext rules{610};
		services.compose=[&](auto &w,const auto &p,const auto &camera,std::uint64_t phase){auto result=compose(w,p,camera,phase);maxima.push_back(XeenCharacterRules::maxHp(p.roster.at(0),rules));
			for(const auto &hp:CloudsUiComposer::buildHpPlacements(p,rules))result.frame.pixels[hp.y*320+hp.x]=static_cast<std::uint8_t>(hp.frame);return result;};
		services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){party=&p;};
		services.show=[&](const auto &first,const auto &handle,const auto &,const auto &,const auto &){check(first.pixels[182*320+13]==3&&maxima.back()==12,"initial equipment HUD maximum");
			handle(InspectInventoryAction{});handle(DialogKeyAction{'a'});handle(SelectInventorySlotAction{0});handle(EquipmentInventoryAction{});
			check(maxima.back()==16&&f.flow->frame().pixels[182*320+13]==0,"equip HUD maximum not recomposed");
			handle(SelectInventorySlotAction{0});handle(DialogKeyAction{'r'});
			check(maxima.back()==12&&f.flow->frame().pixels[182*320+13]==3&&party->roster.at(0).currentHp==16,"remove normalized current HP or stale HUD");return true;};
		Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"equipment HUD recomposition");
	}
}

void representativeRulesAndFeedback() {
	struct Case { std::uint8_t material,state; const char *feedback; };
	for(const auto c:{Case{77,0,"Equipped; PER 13 -> 15"},Case{105,0,"Equipped; HPmax 12 -> 16"},
		Case{110,0,"Equipped; SPmax 2 -> 6"},Case{105,128,"Equipped"}}) {
		Fixture f;character(f.initial.roster.at(0),0);character(f.initial.roster.at(1),1);
		f.initial.roster.at(0).armor[0]={c.material,10,c.state,0};auto services=f.services();XeenPartyState *party=nullptr;
		services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){party=&const_cast<XeenPartyState &>(p);};
		services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
			handle(InspectInventoryAction{});handle(DialogKeyAction{'a'});handle(SelectInventorySlotAction{0});handle(EquipmentInventoryAction{});
			check(f.flow->equipmentResult()&&f.flow->equipmentResult()->status==Status::Success,"modeled integration success");
			return true;
		};Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"modeled feedback gameplay");
	}
	{
		Fixture f;character(f.initial.roster.at(0),0);character(f.initial.roster.at(1),1);
		auto &owner=f.initial.roster.at(0);owner.weapons[0]={0,18,0,0};owner.armor[0]={0,8,0,0};
		auto services=f.services();XeenPartyState *party=nullptr;
		services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){party=&const_cast<XeenPartyState &>(p);};
		services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
			handle(InspectInventoryAction{});handle(SelectInventorySlotAction{0});handle(EquipmentInventoryAction{});
			check(party->roster.at(0).weapons[0].frame==13,"two-handed target frame");
			handle(DialogKeyAction{'a'});handle(SelectInventorySlotAction{0});handle(EquipmentInventoryAction{});
			check(f.flow->equipmentResult()->status==Status::Conflict&&f.flow->equipmentResult()->conflict->category==Category::Weapons&&
				f.flow->equipmentResult()->conflict->physicalSlot==0,"shield/two-handed conflict feedback facts");return true;
		};Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"two-handed integration");
	}
	{
		Fixture f;character(f.initial.roster.at(0),0);character(f.initial.roster.at(1),1);
		auto &owner=f.initial.roster.at(0);owner.accessories[0]={0,2,64,0};owner.accessories[2]={255,0,192,7};
		owner.accessories[3]={1,255,0,7};owner.accessories[4]={0,5,0,0};auto services=f.services();XeenPartyState *party=nullptr;
		services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){party=&const_cast<XeenPartyState &>(p);};
		services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
			handle(InspectInventoryAction{});handle(DialogKeyAction{'c'});
			handle(SelectInventorySlotAction{0});handle(EquipmentInventoryAction{});check(party->roster.at(0).accessories[0].frame==12,"cursed equip refused");
			handle(SelectInventorySlotAction{0});handle(DialogKeyAction{'r'});check(f.flow->equipmentResult()->status==Status::Cursed&&
			party->roster.at(0).accessories[0].frame==12,"cursed remove published");handle(AcknowledgeAction{});
			handle(SelectInventorySlotAction{4});handle(EquipmentInventoryAction{});check(f.flow->equipmentResult()->status==Status::MedalLimit&&
			f.flow->equipmentResult()->matchingFrameCount==2,"ID-zero/opaque medal capacity");return true;
		};Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"curse/medal integration");
	}
	{
		Fixture f;character(f.initial.roster.at(0),0);character(f.initial.roster.at(1),1);
		f.initial.roster.at(0).intellect.permanent=std::numeric_limits<int>::max()-1;
		f.initial.roster.at(0).armor[0]={69,10,0,0};auto services=f.services();
		services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
			handle(InspectInventoryAction{});handle(DialogKeyAction{'a'});handle(SelectInventorySlotAction{0});handle(EquipmentInventoryAction{});
			check(f.flow->equipmentResult()&&f.flow->equipmentResult()->status==Status::UnsafeRules&&f.initial.roster.at(0).armor[0].frame==0,
				"unsafe proposed rules published");return true;
		};Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"unsafe integration");
	}
}

int persistenceChild(const std::filesystem::path &path, bool resume) {
 regional_save_test::Fixture f;
 f.saved.characters[0].weapons={};f.saved.characters[0].weapons[0]={0,18,0,0};
 f.saved.characters[0].weapons[8]={255,0,193,7};
 f.saved.characters[0].miscellaneous[3]={255,0,222,99};
 f.saved.characters[29].weapons[2]={254,255,223,11};
 f.saved.characters[29].accessories[7]={253,254,221,12};
 auto expected=f.saved;expected.characters[0].weapons[0].frame=13;
 if(!resume)XeenSaveFile::write(path,f.saved);
 auto services=f.services();
 services.show=[&](const auto &,const auto &raw,const auto &,const auto &,const auto &){
  f.present(raw);const auto handle=[&](const PlayerAction &a){f.send(raw,a);};
  check(!f.flow->inventoryOpen()&&!f.flow->equipmentResult(),"restart persisted transient equipment UI");handle(InspectInventoryAction{});
  handle(SelectMemberAction{0});handle(SelectInventorySlotAction{0});
  if(!resume){handle(EquipmentInventoryAction{});check(f.flow->equipmentResult()&&f.flow->equipmentResult()->status==Status::Success&&f.party->roster.at(0).weapons[0].frame==13,"producer did not equip two-handed weapon");}
  else {check(f.flow->inventorySelection().sourceOwner==0&&f.flow->inventorySelection().record.frame==13,"consumer did not inspect restored equipment");f.flow->refresh(true);f.present(raw);check(f.party->roster.at(0).weapons[0].frame==13,"refresh normalized restored frame");}
  handle(CancelInteractionAction{});sameSnapshot(expected,f.capture());if(!resume)handle(SaveGameAction{});sameSnapshot(expected,XeenSaveFile::read(path));return true;
 };
 Quiet quiet;const int result=Application().playGameplay(services,{},path,true);if(result)std::cerr<<quiet.out.str();return result;
}

void persistenceRestart(const std::filesystem::path &exe) {
	const auto dir=std::filesystem::current_path()/("equipment-restart-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
	std::filesystem::create_directory(dir); const auto path=dir/"equipment.mmsave";
	const auto producer=child_test::launch(exe,{L"equipment-producer",path.wstring()},dir/"producer.log");
	check(producer.exit==0,"equipment producer failed");
	std::ifstream input(path,std::ios::binary); const std::string bytes{std::istreambuf_iterator<char>(input),{}}; input.close();
	const auto consumer=child_test::launch(exe,{L"equipment-consumer",path.wstring()},dir/"consumer.log");
	check(consumer.exit==0&&producer.pid!=consumer.pid,"fresh equipment consumer failed");
	std::ifstream after(path,std::ios::binary);
	check(bytes==std::string(std::istreambuf_iterator<char>(after),{}),"equipment consumer rewrote producer disk");
}
}
int main(int argc,char **argv){try{
	if(argc==3)return persistenceChild(std::filesystem::absolute(argv[2]),std::string(argv[1])=="equipment-consumer");
	observerLifetimeAndRecovery();certificateInvalidationMatrix();publicationFailureAndHud();
	representativeRulesAndFeedback();
	persistenceRestart(std::filesystem::absolute(argv[0]));
	std::cout<<"Equipment Flow certificate, contextual action, feedback, recovery and bounds passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

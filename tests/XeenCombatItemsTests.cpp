#include "XeenRegionalGameplayTestSupport.h"
#include "XeenTrainingTestSupport.h"
#include "XeenTestInstallation.h"
#include "games/xeen/XeenItemServiceRules.h"
#include <iostream>
using namespace regional_gameplay_test;
namespace {
using Category=XeenInventoryCategory;
void inventory(XeenSaveSnapshot &s) {
    for(unsigned i=0;i<6;++i) {
        auto &c=s.characters[s.activeRosterIds[i]];
        c.conditions.fill(0);c.currentHp=200;c.characterClass=XeenCharacterClass::Knight;
        c.hasSpells=false;c.currentSp=3;c.weapons={};c.armor={};c.accessories={};c.miscellaneous={};
        c.weapons[0]={0,1,0,0};c.weapons[1]={0,2,0,0};c.weapons[2]={0,3,0x40,0};c.weapons[3]={0,34,0,0};
        c.armor[0]={0,1,0,0};c.armor[1]={0,2,0,0};c.accessories[0]={0,1,0,0};
        c.miscellaneous[0]={1,3,2,0};c.miscellaneous[1]={1,9,2,0};c.miscellaneous[2]={1,55,2,0};
        c.miscellaneous[3]={1,0,2,0};c.miscellaneous[4]={1,3,0,0};
        c.miscellaneous[5]={1,3,0x82,0};c.miscellaneous[6]={1,3,0x42,0};
        c.miscellaneous[7]={1,16,2,0};c.miscellaneous[8]={1,4,1,0};
        s.journey->supplements[c.rosterId].inputs.speed={int(100-i*5),0};
    }
    auto &full=s.characters[s.activeRosterIds[5]];
    for(auto &item:full.weapons)item={0,1,0,0};
    auto &wizard=s.characters[s.activeRosterIds[2]];wizard.characterClass=XeenCharacterClass::Sorcerer;
    wizard.accessories[0]={0,1,0,8};wizard.accessories[1]={0,1,0,8};wizard.accessories[2]={0,1,0,0};
    s.journey->treasure->gems=123;
}
void admission() {
    XeenItem item{1,3,2,0};XeenCharacter c;
    check(xeenItemUseAdmission(Category::Miscellaneous,item,false,true)==XeenItemUseAdmission::NoWorld,"Map zero Use admitted");
    for(unsigned condition:{8u,11u,12u,13u,14u,15u}) {
        c.conditions.fill(0);c.conditions[condition]=1;
        check(xeenItemUseAdmission(Category::Miscellaneous,item,true,c.canAct())==XeenItemUseAdmission::CannotAct,"Disabled Use admitted");
    }
    for(unsigned condition:{9u,10u}) {c.conditions.fill(0);c.conditions[condition]=1;check(c.canAct(),"Depressed/Confused Use denied");}
    for(auto invalid:{XeenItem{1,0,2,0},XeenItem{1,3,0,0},XeenItem{1,3,0x82,0},XeenItem{1,3,0x42,0}})
        check(xeenItemUseAdmission(Category::Miscellaneous,invalid,true,true)==XeenItemUseAdmission::NoAbility,"Bad item Use admitted");
    XeenItemCategory items{};items[0]={0,34,0,0};
    check(!xeenDiscardItems(items,0,Category::Weapons),"Slayer Discard admitted");
    items[0]={1,34,1,0};check(bool(xeenDiscardItems(items,0,Category::Miscellaneous)),"Slayer refusal leaked to Misc");
}
void synthetic(bool ran=false,bool healing=false,bool cancel=false,unsigned power=0) {
    Harness h;auto services=h.services();auto saved=regional_combat_test::combatSnapshot(XeenCombatRandom(1));inventory(saved);
    if(ran)saved.journey->supplements[saved.activeRosterIds[0]].inputs.speed={120,0};
    if(power)saved.characters[saved.activeRosterIds[0]].miscellaneous[0]={1,std::uint8_t(power),2,0};
    if(healing)saved.characters[saved.activeRosterIds[1]].currentHp=1;
    const auto configure=services.configureFlow;
    services.configureFlow=[&](auto &flow,const auto &camera){configure(flow,camera);flow.drawDialogSprite=[](auto &,const char *,unsigned,int,int){};};
    const auto statistics=services.resources.loadMonsterStatistics;
    services.resources.loadMonsterStatistics=[=]{auto values=statistics();for(auto &m:values)m.raw[23]=1;return values;};
    const auto map=services.maps;
    services.maps=[=](auto id){auto value=map(id);if(ran)value.geometry.difficulties[7]=127;return value;};
    services.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &) {
        const auto tick=[&]{h.tick(handler,idle);if(handler.completeInputHandoff)handler.completeInputHandoff(h.flow->frame().presentation());};
        const auto send=[&](PlayerAction a){h.press(handler,a);if(handler.completeInputHandoff)handler.completeInputHandoff(h.flow->frame().presentation());};
        const auto key=[&](unsigned k){send(DialogKeyAction{k});};
        const auto ready=[&]{for(unsigned n=0;n<1000;++n){if(!h.retired() && h.phase()==Phase::PlayerReady && !h.world->scenePresentation().feedbackActive())return;tick();}throw std::runtime_error("Combat Items never ready");};
        send(WaitAction{});ready();
        if(ran) {send(RunAction{});ready();check(h.fight().participants()==0x3e,"Run fixture failed");}
        const auto actor=h.fight().participant();const auto minute=h.party->encounterContext->minutes;const auto rng=h.fight().random();
        const auto effects=h.party->encounterContext->effects;
        const auto free=[&]{check(h.fight().participant()==actor && h.phase()==Phase::PlayerReady && h.party->encounterContext->minutes==minute && h.fight().random()==rng,"Free Items action spent turn/time/RNG");};
        if(!healing && !cancel && !power) {
            send(SelectMemberAction{0});key('i');
            const auto owner=saved.activeRosterIds[ran?1:0];
            check(h.flow->inventorySelection().sourceOwner==owner,"Combat sheet selected escaped member");
            key('1');key('e');free();check(h.party->roster.at(owner).weapons[0].frame==1,"Combat Equip failed");
            key('2');key('e');free();send(AcknowledgeAction{}); // Slot conflict.
            key('1');key('r');free();check(h.party->roster.at(owner).weapons[0].frame==0,"Combat Remove failed");
            key('a');key('1');key('e');free();check(h.party->roster.at(owner).armor[0].frame==3,"Combat armor Equip failed");
            key('1');key('r');key('c');key('1');key('e');free();check(h.party->roster.at(owner).accessories[0].frame!=0,"Combat accessory Equip failed");
            key('w');key('3');key('r');free();send(AcknowledgeAction{});
            key('d');free();send(AcknowledgeAction{});
            key(InputKey::F1+1);free();send(AcknowledgeAction{}); // Cursed transfer.
            key('4');key('d');free();send(AcknowledgeAction{});
            check(h.party->roster.at(owner).weapons[3].id==34,"Slayer discarded");
            key('2');key(InputKey::F1+(ran?4:5));free();send(AcknowledgeAction{}); // Full pack.
            key('2');key(InputKey::F1+1);free();
            check(h.party->roster.at(saved.activeRosterIds[ran?2:1]).weapons[4].id==2,"Combat transfer used wrong party slot");
            key('1');key('d');key('n');free();check(h.party->roster.at(owner).weapons[0].id==1,"Discard No changed item");
            key('1');key('d');key('y');free();check(h.party->roster.at(owner).weapons[0].id==3,"Combat Discard did not compact");
            key('m');key('1');key('u');free();send(AcknowledgeAction{}); // Sheet Use refusal.
            key(InputKey::Escape);key(InputKey::Escape);free();
            send(SelectMemberAction{ran?1u:2u});key('i');key('1');key('e');free();
            check(h.party->roster.at(saved.activeRosterIds[2]).weapons[0].frame==0,"Combat proficiency refusal failed");
            send(AcknowledgeAction{});key('c');key('3');key('e');free();
            check(h.party->roster.at(saved.activeRosterIds[2]).accessories[2].frame==0,"Combat ring limit refusal failed");
            send(AcknowledgeAction{});key(InputKey::Escape);key(InputKey::Escape);
        }
        send(UseItemAction{});check(h.flow->inventorySelection().category==Category::Miscellaneous,"Combat Use did not open Misc");
        key(InputKey::Escape);free();send(UseItemAction{});key('e');free();
        const auto user=h.flow->inventorySelection().sourceOwner;
        key(InputKey::F1+1);check(h.flow->inventorySelection().sourceOwner==user,"Use dialog accepted F2");free();
        key('w');key('1');key(InputKey::F1+1);free();check(h.party->roster.at(*user).weapons[0].id!=0,"Use dialog transferred selected item");
        key('m');
        if(!healing && !cancel && !power)for(unsigned slot:{4u,5u,6u,7u}) {
            key('0'+slot);key('u');free();send(AcknowledgeAction{});key('0'+slot);
        }
        const unsigned row=cancel?9:healing?2:1;
        key('0'+row);key('u');
        check(!h.flow->inventoryOpen() && h.fight().cast() && h.fight().cast()->committed,"Combat Use was not admitted");
        check(h.party->roster.at(*user).miscellaneous[row-1].state==(cancel?0:1),"Combat charge payment/removal");
        if(healing || cancel)send(cancel?PlayerAction{CancelInteractionAction{}}:PlayerAction{SelectMemberAction{1}});
        for(unsigned n=0;n<1000 && !h.retired() && h.fight().cast();++n) {
            if(h.fight().cast()->phase==XeenCombatCastPhase::Result && h.fight().cast()->result.failed)send(AcknowledgeAction{});
            else tick();
        }
        check(h.retired() || !h.fight().cast(),"Combat item effect failed to settle");
        check(h.party->roster.at(*user).currentSp==(cancel?4:3) && h.party->monsterTreasure->gems==123,"Item charged SP/gems or lost cancellation refund");
        if(!h.retired())check(h.fight().pending()==Work::Movement,"Item Use skipped its normal monster response");
        if(power==16 || power==21)check(h.party->encounterContext->effects==effects,"Combat-prohibited item applied its effect");
        for(unsigned n=0;n<1000 && !h.retired() && h.phase()!=Phase::PlayerReady;++n)tick();
        check(h.retired() || (h.fight().participant()!=actor && h.party->encounterContext->minutes==minute),"Item Use did not end only its actor's turn");
        if(healing)check(h.party->roster.at(saved.activeRosterIds[1]).currentHp>1,"Combat healing item had no effect");
        return true;
    };
    auto directory=child_test::freshDirectory(std::filesystem::current_path()/"m77-items");h.runDirectories.push_back(directory);
    auto path=directory/"fixture.mmsave";XeenSaveFile::write(path,saved);
    check(Application(services.templePrices).playGameplay(services,{},path,true)==0,"Combat Items gameplay failed");
}
void original(const std::filesystem::path &game) {
    const auto installation=xeenTestInstallationDetector().detect(game);check(bool(installation),"Original installation missing");
    training_test::Inputs in(*installation);
    for(unsigned choice=0;choice<=5;++choice) {
        const bool portal=choice!=0;
        auto saved=in.service();saved.camera={23,9,11,XeenDirection::West};inventory(saved);
        if(choice>1) {
            const auto id=XeenMapIdentity(27+choice);
            const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,id),in.statistics);
            auto &table=saved.map(id).monsters;table.emplace(XeenSaveMonsterTable{std::uint16_t(actors.size()),{}});
            for(const auto &a:actors) {
                const bool defeated=a.lifecycle==XeenActorLifecycle::Present;
                table->slots.push_back({defeated?-128:int(a.x),defeated?-128:int(a.y),defeated?XeenActorLifecycle::Defeated:XeenActorLifecycle(a.lifecycle),a.status,defeated,-1});
            }
        }
        for(auto &a:saved.map(23).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
        auto &monster=saved.map(23).monsters->slots[5];monster.x=8;monster.y=11;monster.lifecycle=XeenActorLifecycle::Present;monster.accounted=false;
        training_test::Fixture f(in,saved);
        f.flow->mapName=[&](auto id){return in.assets.mapName(id);};
        const auto tick=[&]{f.now+=125;f.flow->beginCycle(++f.cycle);if(auto frame=f.flow->updatePresentation())f.present(*frame);};
        const auto key=[&](unsigned k){f.act(DialogKeyAction{k});};
        f.act(WaitAction{});
        for(unsigned n=0;n<1500 && (!f.flow->encounter()->combat() || f.flow->encounter()->combat()->phase()!=Phase::PlayerReady);++n)tick();
        check(f.flow->encounter()->combat() && f.flow->encounter()->combat()->phase()==Phase::PlayerReady,"Original mainland combat did not start");
        const auto actor=f.flow->encounter()->combat()->participant();const auto owner=saved.activeRosterIds[actor];
        const auto minute=f.p.encounterContext->minutes;const auto sp=f.p.roster.at(owner).currentSp;const auto gems=f.p.monsterTreasure->gems;
        bool slayerRefusal=false;f.flow->reportText=[&](const auto &text){if(text==in.assets.uiText().scalar("CANNOT_DISCARD_XEEN_SLAYER"))slayerRefusal=true;};
        f.act(SelectMemberAction{0});key('i');key('4');key('d');
        check(slayerRefusal,"Combat Slayer Discard used wrong text");
        f.act(AcknowledgeAction{});key('1');key('e');
        check(f.p.roster.at(saved.activeRosterIds[0]).weapons[0].frame==1,"Original combat equip failed");
        key(InputKey::Escape);key(InputKey::Escape);
        const auto hpBefore=f.w.sessionState().regionalActors(23)[5].hp;
        f.act(UseItemAction{});key(portal?'3':'1');key('u');
        if(portal) {
            f.act(AcknowledgeAction{});key('0'+choice);key(InputKey::Enter);
            check(f.c.mapId==XeenMapIdentity(27+choice) && !f.flow->inventoryOpen(),"Town Portal did not load the selected town before finishing Use");
        } else {
            for(unsigned n=0;n<1500 && f.flow->encounter()->combat() && f.flow->encounter()->combat()->cast();++n)tick();
            check(f.w.sessionState().regionalActors(23)[5].hp<hpBefore,"Original combat damage item did no damage");
        }
        for(unsigned n=0;n<2000 && (f.flow->encounter()->combat() || !f.flow->canSave());++n) {
            if(!portal && f.flow->encounter()->combat() && !f.flow->encounter()->combat()->cast() &&
                f.flow->encounter()->combat()->phase()==Phase::PlayerReady)f.act(AttackAction{});
            if(f.flow->inputContext(f.flow->frame().presentation()).dialog && f.flow->inputContext(f.flow->frame().presentation()).dialog->anyKey)f.act(AcknowledgeAction{});
            tick();
        }
        if(!f.flow->canSave() || f.flow->encounter()->combat())throw std::runtime_error("Original item combat did not finish: portal="+std::to_string(portal)+" map="+std::to_string(f.c.mapId.number)+" activity="+std::to_string(unsigned(f.w.sessionState().journeyActivity()))+" inventory="+std::to_string(f.flow->inventoryOpen())+" combat="+(f.flow->encounter()->combat()?std::to_string(unsigned(f.flow->encounter()->combat()->phase())):"retired"));
        check(f.p.roster.at(owner).miscellaneous[portal?2:0].state==1,"Original item spent wrong charge count");
        check(f.p.roster.at(owner).currentSp==sp && f.p.monsterTreasure->gems==gems,"Original combat item charged SP/gems");
        if(portal) {
            const auto &g=f.w.map(27+choice).geometry;
            check(f.c.mapId==XeenMapIdentity(27+choice) && f.c.x==g.runX && f.c.y==g.runY && f.p.encounterContext->minutes==minute+1,"Combat Portal run location or ordinary End minute differs");
        }
        const auto bytes=XeenSaveFormat::encode(f.snapshot());training_test::Fixture restored(in,XeenSaveFormat::decode(bytes));
        check(bytes==XeenSaveFormat::encode(restored.snapshot()),"Combat item save/load replayed time/effects/RNG");
    }
}
}
int main(int argc,char **argv) {try {
    std::ostringstream output;auto *before=std::cout.rdbuf(output.rdbuf());
    try {
        if(argc==2)original(std::filesystem::u8path(argv[1]));
        else {admission();synthetic();synthetic(true);synthetic(false,true);synthetic(false,false,true);for(unsigned power:{16u,21u,33u,64u,67u})synthetic(false,false,false,power);}
    }catch(...){std::cout.rdbuf(before);throw;}
    std::cout.rdbuf(before);
    std::cout<<"Combat Items actions, refusals, turn/charge cost and travel passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

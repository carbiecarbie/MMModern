#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "app/XeenAudio.h"
#include "games/xeen/XeenFmEffects.h"
#include "games/xeen/XeenEventTrigger.h"
#include <fstream>
#include <iostream>
#include <set>
using namespace training_test;
namespace {
void inventory(const std::filesystem::path &rules) {
    std::ifstream in(rules);const std::string text((std::istreambuf_iterator<char>(in)),{});
    // Each reconciled direct DOS site remains independently accounted for.
    const std::pair<const char *,const char *> sites[]{
        {"0x0b2f1","33"},
        {"0x0b308","34"},
        {"0x0c903","10"},
        {"0x0cf48","21 from 0xcf46; 46 from 0xcf6a via 0xcf6c"},
        {"0x0d442","7 + toggled stepping bit (7 or 8)"},
        {"0x0d5c1","21"},
        {"0x1b1a1","28"},
        {"0x1c4b5","45"},
        {"0x1d918","49"},
        {"0x1d922","21"},
        {"0x1db60","46"},
        {"0x1e8d2","21"},
        {"0x1f073","20 on enabling Efx; off branch 0 is gated out"},
        {"0x1f249","51"},
        {"0x1f881","38"},
        {"0x21107","20"},
        {"0x236c6","10"},
        {"0x236d8","21"},
        {"0x23eb8","Physical 29; Magical 27; Fire 22; Electrical 23; Cold 24; Poison 26; Energy 25; fallback 38"},
        {"0x240fe","51"},
        {"0x2428e","54"},
        {"0x2456b","20"},
        {"0x26ff3","Event record parameter byte +6"},
        {"0x270a8","51"},
        {"0x270ea","45 for selected object sprite 47; 44 for sprite 48"},
        {"0x28f1c","21"},
        {"0x2c105","60 + byte at DAT 0x4d660[attack weapon ID]"},
        {"0x2c17c","6"},
        {"0x2c23c","4"},
        {"0x2d379","Physical 29; Magical 27; Fire 22; Electrical 23; Cold 24; Poison 26; Energy 25"},
        {"0x2d5f0","37"},
        {"0x2d62e","37"},
        {"0x2d65e","38"},
        {"0x2d6af","26 Poison/Disease; 28 Insane/Confuse; 36 Sleep/Weaken; 37 CurseItem/DrainSP/Curse/Paralyze/Unconscious/BreakWeapon/Aging"},
        {"0x2dc0b","6"},
        {"0x2e1fe","51"},
        {"0x2e7e6","monster-record byte +0x32"},
        {"0x32d5b","31"},
        {"0x34d71","21"},
        {"0x36a07","attack type 0..6 maps to 49,18,13,14,15,17,16"},
        {"0x2f1d3","21"},
        {"0x2f25c","21"},
        {"0x2f3ac","39"},
        {"0x2f3ee","30"},
        {"0x2f44a","30"},
        {"0x2f476","18"},
        {"0x2f496","30"},
        {"0x2f53a","30"},
        {"0x2f575","14"},
        {"0x2f5b7","16"},
        {"0x2f5da","18"},
        {"0x2f5fd","18"},
        {"0x2f620","17"},
        {"0x2f68c","20"},
        {"0x2f6fb","20"},
        {"0x2f72e","18"},
        {"0x2f825","51"},
        {"0x2f895","20"},
        {"0x2f8b7","30"},
        {"0x2f8d8","20"},
        {"0x2f8eb","20"},
        {"0x2fa69","20"},
        {"0x2faf6","30"},
        {"0x2fb65","30"},
        {"0x2fbf3","30"},
        {"0x2fc3e","14"},
        {"0x2fc4f","30"},
        {"0x2fc70","20"},
        {"0x2fc8b","15"},
        {"0x2fee7","51"},
        {"0x2ff65","20"},
        {"0x2ffc5","20"},
        {"0x2fff0","30"},
        {"0x30072","13"},
        {"0x301ee","20"},
        {"0x30250","17"},
        {"0x30292","15"},
        {"0x302b2","30"},
        {"0x30316","51"},
        {"0x30339","13"},
        {"0x30557","51"},
        {"0x3057c","18"},
        {"0x3059c","30"},
        {"0x3060c","30"},
        {"0x30633","13"},
        {"0x3064d","20"},
        {"0x3076e","51"},
        {"0x30836","30"},
        {"0x308ee","15"},
        {"0x3095d","30"},
        {"0x30a01","51"},
        {"0x30a43","18"},
        {"0x30a6f","18"},
        {"0x30a93","16"},
        {"0x30aa7","30"},
        {"0x30af4","18"},
        {"0x30b6c","13"},
        {"0x30baa","stormFx[rand(2..5)-2] = {13,14,15,17}"},
        {"0x30bdf","18"},
        {"0x30c38","30"},
        {"0x30cb4","14"},
        {"0x30cd7","13"},
        {"0x30cfa","16"},
        {"0x30d1d","18"},
        {"0x30d40","13"},
        {"0x30de2","20"},
        {"0x30e07","18"},
        {"0x30e1d","20"},
        {"0x30e53","20"},
        {"0x30e82","20"},
        {"0x30e9d","18"},
        {"0x30edb","16"},
        {"0x30efe","18"},
        {"0x30f21","22"},
        {"0x30f44","17"},
        {"0x30fd7","49"},
        {"0x31009","16"},
        {"0x31bfd","20"},
        {"0x31c44","21"},
        {"0x31d3d","19"},
        {"0x31dc3","21"},
        {"0x31e1e","21"},
        {"0x31e4c","20"},
        {"0x31ea9","20"},
        {"0x31f07","20"},
        {"0x31f61","20"},
    };
    check(std::size(sites)==126,"C1/C2 inventory size changed");
    const auto count=[&](const char *needle){unsigned n=0;for(std::size_t p=0;(p=text.find(needle,p))!=text.npos;p+=std::char_traits<char>::length(needle))++n;return n;};
    check(count(" | C1 | High:")==40 && count(" | C2 | High:")==86 && count(" | Not reachable yet:")==7,
        "full 133-site reconciliation changed");
    std::set<std::string> unique;
    for(const auto &[site,expression]:sites) {
        check(unique.insert(site).second,"duplicate FM site");
        const auto start=text.find(std::string("| `")+site+"` | ");
        check(start!=text.npos,"reconciled site missing from rules");
        const auto row=text.substr(start,text.find('\n',start)-start);
        check(row.find(std::string("| ")+expression+" |")!=row.npos && (row.find(" | C1 | High:")!=row.npos || row.find(" | C2 | High:")!=row.npos),
            "site id/expression, classification or confidence changed");
    }
    constexpr unsigned damage[]{29,27,22,23,24,26,25,38};
    for(unsigned type=0;type<std::size(damage);++type)check(xeenDamageFmEffect(type)==damage[type],"damage FM selector");
    constexpr unsigned ranged[]{49,18,13,14,15,17,16};
    for(unsigned type=0;type<std::size(ranged);++type)check(xeenRangedFmEffect(type)==ranged[type],"ranged FM selector");
    for(const auto &[special,fx]:std::vector<std::pair<unsigned,unsigned>>{{5,26},{7,26},{8,28},{16,28},{9,36},{18,36},
        {10,37},{12,37},{13,37},{14,37},{15,37},{17,37},{19,37},{20,37},{21,38},{22,37}})
        check(xeenSecondaryFmEffect(special)==fx,"secondary FM selector");
    check(!xeenSecondaryFmEffect(0) && !xeenSecondaryFmEffect(1),"non-audible secondary attack");
    check(!xeenMissileReachesFmWall(4,true,0) && xeenMissileReachesFmWall(1,true,4) &&
        xeenMissileReachesFmWall(2,false,1) && !xeenMissileReachesFmWall(3,false,1),
        "wall check precedes spent-missile exit only at the next wall");
}
void tick(Fixture &f) {
    f.now+=125;f.flow->beginCycle(++f.cycle);
    if(auto frame=f.flow->updatePresentation())f.present(*frame);
}
void quiet(Fixture &f) {
    for(unsigned i=0;i<1500 && !f.flow->canSave();++i)tick(f);
    check(f.flow->canSave(),"FM witness did not settle");
}
void attach(Fixture &f,const GameInstallation &installation,std::vector<unsigned> &effects) {
    f.programState.audio=XeenAudio::installed(installation,f.in.assets,0,false);
    f.programState.audio->effectSink([&effects](unsigned id){effects.push_back(id);});
}
bool heard(const std::vector<unsigned> &effects,unsigned id) {return std::find(effects.begin(),effects.end(),id)!=effects.end();}
std::vector<std::uint8_t> wall(Inputs &in,const GameInstallation &installation,bool enabled) {
    auto start=in.service();start.camera={28,8,4,XeenDirection::West};start.soundOptions.efx=enabled;
    Fixture f(in,start);std::vector<unsigned> fx;attach(f,installation,fx);
    const auto before=XeenSaveFormat::encode(f.snapshot());
    f.act(NavigationAction::MoveForward);quiet(f);
    check(f.c.x==8 && f.c.y==4,"original wall bump moved the party");
    check(enabled?fx==std::vector<unsigned>{46}:fx.empty(),"wall bump must request 46 exactly once through the Efx gate");
    check(XeenSaveFormat::encode(f.snapshot())==before,"wall bump changed time, RNG or saves");
    f.act(BashAction{});quiet(f);
    check(enabled?heard(fx,31):fx.empty(),"indoor Bash effect31 absent or Efx gate failed");
    const auto count=fx.size();
    f.act(NavigationAction::TurnRight);quiet(f);f.act(NavigationAction::TurnLeft);quiet(f);
    check(enabled?(fx.size()==count+2 && fx[count]==8 && fx[count+1]==7):fx.empty(),"step suffix must toggle 8/7, including while Effects is off");
    check(f.programState.steppingFx==0,"step process bit must toggle even when muted");
    auto after=f.snapshot();after.soundOptions.efx=true;return XeenSaveFormat::encode(after);
}
std::vector<std::uint8_t> service(Inputs &in,const GameInstallation &installation,bool enabled) {
    auto start=in.service();start.soundOptions.efx=enabled;start.journey->purse->gold=0;
    Fixture f(in,start);std::vector<unsigned> fx;attach(f,installation,fx);
    f.enter();f.act(SelectMemberAction{1});f.act(DialogKeyAction{'t'});
    check(enabled?fx==std::vector<unsigned>{21}:fx.empty(),"original Training insufficient-gold effect must be 21");
    f.act(AcknowledgeAction{});f.act(DialogKeyAction{InputKey::Escape});quiet(f);
    auto after=f.snapshot();after.soundOptions.efx=true;return XeenSaveFormat::encode(after);
}
std::vector<std::uint8_t> combat(Inputs &in,const GameInstallation &installation,bool enabled) {
    // Original mainland monster, map and party. Only placement/liveness isolates one encounter.
    auto start=in.service();start.camera={23,9,11,XeenDirection::West};start.soundOptions.efx=enabled;
    for(auto &a:start.map(23).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
    auto &monster=start.map(23).monsters->slots[5];monster.x=8;monster.y=11;monster.lifecycle=XeenActorLifecycle::Present;monster.accounted=false;
    Fixture f(in,start);std::vector<unsigned> fx;attach(f,installation,fx);
    f.act(WaitAction{});unsigned attacks=0;
    for(unsigned i=0;i<2500;++i) {
        if(f.flow->encounter()->combat() && f.flow->encounter()->combat()->phase()==XeenCombatPhase::PlayerReady){f.act(AttackAction{});++attacks;}
        if(f.flow->inputContext(f.flow->frame().presentation()).dialog && f.flow->inputContext(f.flow->frame().presentation()).dialog->anyKey)f.act(AcknowledgeAction{});
        tick(f);
        if(attacks && !f.flow->encounter()->combat() && f.flow->canSave())break;
    }
    check(attacks && !f.flow->encounter()->combat() && f.flow->canSave(),"original combat witness did not finish");
    if(enabled) {
        check(std::any_of(fx.begin(),fx.end(),[](unsigned id){return id>=60;}),"positive combat impact missing");
        check(heard(fx,4),"original monster-death cue missing");
    }else check(fx.empty(),"combat requested an effect with Efx off");
    auto after=f.snapshot();after.soundOptions.efx=true;return XeenSaveFormat::encode(after);
}
std::vector<std::uint8_t> event(Inputs &in,const GameInstallation &installation,bool enabled) {
    auto start=in.service();start.soundOptions.efx=enabled;
    // Relocate one original PlayFX instruction to isolate its operand/continuation.
    const auto original=std::find_if(in.city.records.begin(),in.city.records.end(),[](const auto &r){return r.opcode==0x06;});
    check(original!=in.city.records.end() && original->parameters==std::vector<std::uint8_t>{19},"original Vertigo PlayFX calibration changed");
    auto cue=*original;cue.x=10;cue.y=11;cue.direction=4;cue.line=0;
    auto exit=cue;exit.line=1;exit.opcode=0x12;exit.parameters.clear();exit.lengthField=5;exit.fileOffset=cue.fileOffset+cue.lengthField+1;
    XeenEventFile script{28,"maze0028.evt",true,{cue,exit}};
    Fixture f(in,start,false,&script);std::vector<unsigned> fx;attach(f,installation,fx);
    const auto before=XeenSaveFormat::encode(f.snapshot());
    f.act(InteractionAction{});
    check(f.flow->canSave(),"PlayFX added a presentation or wait");
    check(enabled?fx==std::vector<unsigned>{19}:fx.empty(),"PlayFX operand or Efx gate differs");
    auto after=f.snapshot();after.soundOptions.efx=true;return XeenSaveFormat::encode(after);
}
std::vector<std::uint8_t> wrongFacing(Inputs &in,const GameInstallation &installation,bool enabled) {
    auto start=in.service();start.camera={28,14,10,XeenDirection::West};start.soundOptions.efx=enabled;
    XeenEventRecord cue;cue.x=14;cue.y=10;cue.direction=unsigned(XeenDirection::East);cue.opcode=0x12;cue.lengthField=5;
    XeenEventFile script{28,"maze0028.evt",true,{cue}};
    Fixture f(in,start,false,&script);std::vector<unsigned> fx;attach(f,installation,fx);
    check(bool(f.w.selectObject(f.c)) && !(f.w.sampleCell(28,14,10)->cell->rawAttributes&kXeenAutomaticEventFlag),"original mirror object refusal fixture changed");
    unsigned noEvents=0,completed=0;
    f.flow->reportManual=[&](const auto &r){noEvents+=std::holds_alternative<XeenManualEventNoEvent>(r);completed+=std::holds_alternative<XeenManualEventCompleted>(r);};
    f.act(InteractionAction{});
    check(enabled?fx==std::vector<unsigned>{21}:fx.empty(),"wrong-facing object refusal must request 21 only once");
    check(f.flow->canSave() && noEvents==1 && !completed,"wrong-facing refusal executed an Event");
    auto after=f.snapshot();after.soundOptions.efx=true;return XeenSaveFormat::encode(after);
}
std::vector<std::uint8_t> passableBash(Inputs &in,const GameInstallation &installation,bool enabled) {
    auto start=in.service();start.camera={28,20,20,XeenDirection::North};start.soundOptions.efx=enabled;
    for(auto &a:start.map(28).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
    XeenWorld query(in.mapLoader());bool found=false;
    constexpr int dx[]{0,1,0,-1},dy[]{1,0,-1,0};
    for(int y=1;y<31 && !found;++y)for(int x=1;x<31 && !found;++x)for(unsigned d=0;d<4;++d) {
        const auto cell=query.sampleCell(28,x,y),next=query.sampleCell(28,x+dx[d],y+dy[d]);
        if(cell && next && cell->cell->surfaceIndex==1 && next->cell->surfaceIndex==1 &&
            wallAt(*cell->cell,XeenDirection(d))<query.map(28).geometry.difficulties[0] && !(next->cell->rawAttributes&kXeenAutomaticEventFlag)) {
            start.camera={28,x,y,XeenDirection(d)};found=true;break;
        }
    }
    check(found,"original passable Bash fixture changed");
    Fixture f(in,start);std::vector<unsigned> fx;attach(f,installation,fx);
    f.act(BashAction{});quiet(f);
    check(f.c.x!=start.camera.x || f.c.y!=start.camera.y,"passable Bash did not move");
    check(enabled?fx==std::vector<unsigned>{8}:fx.empty(),"passable Bash must take the step FX path, without FX31");
    check(f.programState.steppingFx==1,"passable Bash did not toggle the process step bit while muted");
    auto after=f.snapshot();after.soundOptions.efx=true;return XeenSaveFormat::encode(after);
}
std::vector<std::uint8_t> projectileWall(Inputs &in,const GameInstallation &installation,bool wall,bool spell,bool enabled) {
    auto start=in.service();start.soundOptions.efx=enabled;start.camera={28,8,4,XeenDirection::West};
    for(auto &a:start.map(28).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
    XeenWorld query(in.mapLoader());
    if(!wall) {
        bool found=false;
        for(int y=1;y<31 && !found;++y)for(int x=1;x<31 && !found;++x)for(unsigned d=0;d<4;++d) {
            const auto cell=query.sampleCell(28,x,y);XeenCamera at{28,x,y,XeenDirection(d)};
            if(cell && cell->cell->surfaceIndex==1 && !(cell->cell->rawAttributes&kXeenAutomaticEventFlag) && xeenPlayerRayRows(query,at)==4) {
                start.camera=at;found=true;break;
            }
        }
        check(found,"original clear projectile ray changed");
    }
    const auto rows=xeenPlayerRayRows(query,start.camera);
    check((xeenPlayerRayWall(query,start.camera,rows)<4)==wall,"original wall ray calibration changed");
    auto &caster=start.characters[start.activeRosterIds[4]];caster.characterClass=XeenCharacterClass::Sorcerer;
    for(auto *items:{&caster.weapons,&caster.armor,&caster.accessories})for(auto &item:*items)item.frame=0;
    caster.hasSpells=true;caster.currentSp=100;caster.learnedSpells.emplace();
    caster.currentSpell=*XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Wizardry,unsigned(XeenLearnedSpell::MagicArrow));
    caster.learnedSpells->at(caster.currentSpell)=1;
    Fixture f(in,start);std::vector<unsigned> fx;attach(f,installation,fx);
    if(spell){f.act(CastSpellAction{});f.act(SelectMemberAction{4});f.act(DialogKeyAction{'c'});}
    else f.act(ShootAction{});
    quiet(f);
    const std::vector<unsigned> expected=spell?(wall?std::vector<unsigned>{46}:std::vector<unsigned>{}):
        (wall?std::vector<unsigned>{49,46}:std::vector<unsigned>{49});
    check(enabled?fx==expected:fx.empty(),"shared projectile wall cue or clear-ray silence differs");
    auto after=f.snapshot();after.soundOptions.efx=true;return XeenSaveFormat::encode(after);
}
// Synthetic roll tapes isolate the distinct DOS monster, melee and missile miss paths.
void missEffects() {
    XeenConsequenceCharacters party;XeenConsequenceInputs inputs;
    for(unsigned n=0;n<6;++n) {
        auto &c=party[n];c.rosterId=n;c.birthYear=592;c.permanentLevel=3;c.currentHp=100;
        c.characterClass=XeenCharacterClass::Cleric;c.intellect=c.personality=c.endurance={15,0};
        auto &i=inputs[n];i.might=i.speed=i.accuracy={15,0};i.luck=XeenAttributeValue{15,0};i.resistances=XeenCombatResistances{};
    }
    inputs[0].temporaryAc=100;
    XeenMonsterRecord monster;monster.raw[22]=255;monster.raw[24]=1;monster.raw[25]=3;
    monster.raw[26]=1;monster.raw[28]=10;monster.raw[31]=5;
    using Draw=XeenCombatRandom::Draw;
    std::vector<unsigned> effects;
    const auto finish=[&](auto &candidate,const std::vector<Draw> &tape,const char *name) {
        try {
        effects.clear();XeenCombatRandom rng(tape);bool done=false;
        for(unsigned n=0;n<50 && !done;++n) {
            XeenConsequenceDraw draw{rng,1,{},[&](unsigned id){effects.push_back(id);}};
            done=candidate.service(draw);
        }
        check(done && rng.position()==tape.size(),"miss effect changed the draw sequence");
        XeenConsequenceDraw repeat{rng,1,{},[&](unsigned id){effects.push_back(id);}};
        const auto count=effects.size();check(candidate.service(repeat) && effects.size()==count,"miss cue repeated on completed candidate");
        } catch(const std::exception &e){throw std::runtime_error(std::string(name)+": "+e.what());}
    };
    XeenEnemyAttackCandidate naturalMiss(party,inputs,monster,610,1);
    finish(naturalMiss,{{1,20,1}},"natural");check(effects==std::vector<unsigned>{6},"monster natural miss cue");
    XeenEnemyAttackCandidate armorMiss(party,inputs,monster,610,1);
    finish(armorMiss,{{1,20,2},{1,5,1}},"armor");check(effects==std::vector<unsigned>{6},"monster AC refusal cue");
    XeenEnemyAttackCandidate criticalMiss(party,inputs,monster,610,1);
    finish(criticalMiss,{{1,20,20},{1,10,1},{1,5,1}},"critical");
    check(effects==std::vector<unsigned>({29,6}) && criticalMiss.result.injuryCount==1,"critical first hit then second miss cue order");
    XeenPhysicalPlayerCandidate melee(party[0],inputs[0],monster,1,610,false,XeenDifficulty::Adventurer);
    finish(melee,{{1,20,1}},"melee");check(effects==std::vector<unsigned>{6},"zero melee total must enter attack2");
    party[0].weapons[0].id=30;party[0].weapons[0].frame=4;
    const std::vector<Draw> missileTape{{1,2,1},{1,2,1},{1,2,1},{1,20,1}};
    XeenPhysicalPlayerCandidate missile(party[0],inputs[0],monster,1,610,true,XeenDifficulty::Adventurer);
    finish(missile,missileTape,"missile");check(effects.empty(),"missed missile must bypass attack2 cue");
    XeenSpellDamageCandidate poison(party[0],inputs[0],monster,1,610,XeenDifficulty::Adventurer,XeenActorStatus::Physical,true);
    finish(poison,missileTape,"poison");check(effects.empty(),"missed Poison Volley must bypass attack2 cue");
    for(unsigned special=0;special<=22;++special) {
        auto c=party[0];std::vector<unsigned> cues;int observedHp=-1;bool conditioned=false,broken=false;
        xeenApplySpecialAttack(c,special,[&](unsigned id){
            cues.push_back(id);observedHp=c.currentHp;
            if(special==19 || special==21 || special==22)conditioned=c.conditions[special==19?15:special==21?13:14]==1;
            broken=(c.weapons[0].state&0x80) && !c.weapons[0].frame;
        });
        const auto selected=xeenSecondaryFmEffect(special);
        check(cues==(selected?std::vector<unsigned>{*selected}:std::vector<unsigned>{}),"secondary helper cue lost or repeated");
        if(special==19 || special==21 || special==22) {
            check(observedHp==100 && conditioned && c.currentHp==0,"secondary cue must precede HP zero after condition mutation");
            if(special==19)check(broken,"Eradicate cue must follow inventory break");
        }
    }
}
void commonDamage(Inputs &in,const GameInstallation &installation) {
    std::vector<unsigned> fx;auto audio=XeenAudio::installed(installation,in.assets,0,false);
    audio->effectSink([&](unsigned id){fx.push_back(id);});
    const auto sink=[&](unsigned id){audio->effect(id,0);};
    // Magic-resistance refusal never enters attack2; zero-damage attack2 does.
    XeenCombatRandom resistance(std::vector<XeenCombatRandom::Draw>{{1,103,1}});
    XeenConsequenceDraw draw{resistance,64,{},sink};
    XeenSpellDamageCandidate refused(3,0,100,1,XeenLearnedSpell::MagicArrow);
    check(refused.service(draw) && fx.empty(),"magic resistance emitted attack2 cue");
    XeenSpellDamageCandidate boss(3,0,0,89,XeenLearnedSpell::MagicArrow);
    check(boss.service(draw) && fx==std::vector<unsigned>{6},"boss zero-damage path missing cue");
    check(boss.service(draw) && fx.size()==1,"completed damage candidate repeated effect");
    std::ifstream file(installation.uiModule->physicalPath,std::ios::binary);
    const std::vector<std::uint8_t> module((std::istreambuf_iterator<char>(file)),{});
    for(unsigned weapon=0;weapon<35;++weapon){fx.clear();audio->weaponEffect(weapon,0);check(fx==std::vector<unsigned>{60u+module.at(0x4d660+weapon)},"original weapon FM expression");}
    XeenSoundOptions off;off.efx=false;audio->observeOptions(off);fx.clear();
    for(unsigned id=0;id<128;++id)audio->effect(id,0);
    for(unsigned weapon=0;weapon<35;++weapon)audio->weaponEffect(weapon,0);
    check(fx.empty(),"audio owner did not gate every effect request");
    audio->observeOptions(XeenSoundOptions{});audio->effectSink([](unsigned){throw std::runtime_error("recording output failure");});audio->effect(20,0);
}
void spellSelectors() {
    using S=XeenLearnedSpell;
    const std::pair<S,unsigned> launches[]{
        {S::FlyingFist,18},{S::Sparks,14},{S::EnergyBlast,16},{S::Sleep,18},{S::Pain,18},{S::ToxicCloud,17},
        {S::TurnUndead,18},{S::LightningBolt,14},{S::Fireball,13},{S::AcidSpray,17},{S::ColdRay,15},
        {S::DeadlySwarm,13},{S::FingerOfDeath,18},{S::FieryFlail,13},{S::FantasticFreeze,15},{S::DancingSword,18},
        {S::PrismaticLight,18},{S::MoonRay,16},{S::MassDistortion,18},{S::HolyWord,18},{S::MegaVolts,14},
        {S::Inferno,13},{S::SunRay,16},{S::Implosion,18},{S::StarBurst,13},{S::BeastMaster,18},
        {S::DragonSleep,18},{S::GolemStopper,16},{S::Hypnotize,18},{S::Incinerate,22},{S::InsectSpray,17},
        {S::PoisonVolley,49},{S::Shrapmetal,16}};
    for(const auto &[spell,fx]:launches)check(xeenSpellLaunchFmEffect(spell,0)==fx,"spell launch selector differs");
    check(!xeenSpellLaunchFmEffect(S::Frostbite,4) && xeenSpellLaunchFmEffect(S::Frostbite,4,33)==15 &&
        xeenSpellLaunchFmEffect(S::Incinerate,2,64)==13 && !xeenSpellLaunchFmEffect(S::MagicArrow,0),"learned/item variants differ");
    constexpr unsigned storm[]{13,14,15,17};
    for(unsigned n=0;n<4;++n)check(xeenSpellLaunchFmEffect(S::ElementalStorm,n+2)==storm[n],"storm type must reuse its draw");
    const std::pair<S,unsigned> results[]{
        {S::Light,39},{S::Awaken,30},{S::FirstAid,30},{S::Revitalize,30},{S::CureWounds,30},{S::SuppressPoison,20},
        {S::SuppressDisease,20},{S::Bless,30},{S::Levitate,20},{S::WizardEye,20},{S::HolyBonus,30},{S::NaturesCure,30},
        {S::Heroism,30},{S::WalkOnWater,20},{S::PowerShield,20},{S::CurePoison,30},{S::CureDisease,30},
        {S::CureParalysis,30},{S::CreateFood,20},{S::StoneToFlesh,30},{S::RaiseDead,30},{S::Resurrection,30},
        {S::DivineIntervention,20},{S::Clairvoyance,20},{S::DayOfProtection,20},{S::DayOfSorcery,20}};
    for(const auto &[spell,fx]:results) {
        check(xeenSpellResultFmEffect(spell,false)==fx,"spell result selector differs");
        check(!xeenSpellResultFmEffect(spell,true) && !xeenSpellResultFmEffect(spell,false,true),"failed/cancelled member emitted result cue");
    }
    check(xeenSpellResultFmEffect(S::ProtectionFromElements,false,true)==20,"element Escape must reach cue20");
    for(auto spell:{S::Jump,S::TimeDistortion,S::Teleport,S::Etherealize,S::TownPortal})
        check(xeenTravelFmEffect(spell,false,true)==51 && !xeenTravelFmEffect(spell,true,true) &&
            !xeenTravelFmEffect(spell,false,false),"travel refusal/arrival cue differs");
    check(xeenTravelFmEffect(S::LloydsBeacon,false,false,1)==20 && xeenTravelFmEffect(S::LloydsBeacon,false,true,2)==51 &&
        !xeenTravelFmEffect(S::LloydsBeacon,false,false,0) && xeenTravelFmEffect(S::SuperShelter,false,false)==30,"Beacon/Shelter selector differs");
    XeenCombatRandom random(std::vector<XeenCombatRandom::Draw>{{2,6,4}});
    std::vector<unsigned> fx;XeenConsequenceDraw draw{random,64,{},[&](unsigned id){check(random.position()==1,"Energy cue preceded preparation draw");fx.push_back(id);}};
    XeenSpellDamagePreparation energy(S::EnergyBlast,7);energy.requestFmEffect(draw);
    check(fx.empty() && energy.service(draw),"Energy cue must await scalar");
    energy.requestFmEffect(draw);energy.requestFmEffect(draw);
    check(fx==std::vector<unsigned>{16} && random.position()==1,"launch cue repeated or drew RNG");
    for(bool valid:{false,true}) {
        XeenCombatRandom rng(valid?std::vector<XeenCombatRandom::Draw>{{1,6,4}}:std::vector<XeenCombatRandom::Draw>{});fx.clear();
        XeenConsequenceDraw request{rng,64,{},[&](unsigned id){check(rng.position()==unsigned(valid),"Recharge cue/draw order differs");fx.push_back(id);}};
        XeenItemSpellPreparation recharge(S::RechargeItem,7,XeenInventoryCategory::Miscellaneous,{1,std::uint8_t(valid?1:53),2,0});
        check(recharge.service(request) && recharge.service(request) && fx==std::vector<unsigned>{valid?20u:21u},"Recharge success/refusal cue differs");
    }
}
std::vector<std::uint8_t> spellWitness(Inputs &in,const GameInstallation &installation,unsigned mode,bool enabled) {
    constexpr unsigned spells[]{20,10,21,10,10,1,55,48,20,20,44,71};const auto spell=spells[mode];
    const bool combat=mode==0 || mode>=8;
    auto s=in.service();s.camera={23,9,11,combat?XeenDirection::West:XeenDirection::East};s.soundOptions.efx=enabled;
    for(auto map:{23u,28u})for(auto &a:s.map(map).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
    // Original party/resources with disclosed spell books, SP and an injured target.
    for(auto owner:s.activeRosterIds) {
        auto &c=s.characters[owner];c.characterClass=XeenLearnedSpellRules::slotForSpell(XeenSpellCategory::Wizardry,spell)?XeenCharacterClass::Sorcerer:XeenCharacterClass::Cleric;
        c.hasSpells=true;c.currentSp=100;c.learnedSpells.emplace();c.learnedSpells->fill(0);
        c.currentSpell=*XeenLearnedSpellRules::slotForSpell(*XeenLearnedSpellRules::categoryForClass(c.characterClass),spell);
        c.learnedSpells->at(c.currentSpell)=1;
        c.quickOption=1;
        if(mode==8)c.currentSp=0;
        if(mode==11){c.miscellaneous={};c.miscellaneous[0]={1,55,2,0};}
        if(mode==7)c.currentHp=1;
        for(auto *items:{&c.weapons,&c.armor,&c.accessories})for(auto &item:*items)item.frame=0;
    }
    s.journey->purse->gems=100;s.characters[s.activeRosterIds[1]].currentHp=1;
    if(mode==4)s.characters[s.activeRosterIds[4]].currentSp=0;
    if(mode==9)s.journey->purse->gems=0;
    if(combat) {auto &a=s.map(23).monsters->slots[5];a.x=8;a.y=11;a.lifecycle=XeenActorLifecycle::Present;a.accounted=false;}
    if(mode==3){auto &items=s.characters[s.activeRosterIds[0]].miscellaneous;items={};items[0]={1,9,2,0};}
    Fixture f(in,s);std::vector<unsigned> fx;attach(f,installation,fx);
    if(combat) {
        f.act(WaitAction{});bool cast=false;
        for(unsigned n=0;n<2500;++n) {
            if(f.flow->encounter()->combat() && f.flow->encounter()->combat()->phase()==XeenCombatPhase::PlayerReady) {
                if(!cast){
                    if(mode>=8 && mode<=10)f.act(QuickFightAction{});
                    else if(mode==11) {
                        f.flow->mapName=[&](auto id){return in.assets.mapName(id);};
                        f.act(UseItemAction{});for(auto key:{'1','u'})f.act(DialogKeyAction{unsigned(key)});
                        check(enabled?std::count(fx.begin(),fx.end(),21)==1:fx.empty(),"combat Portal item engaged notice cue");
                        f.act(AcknowledgeAction{});f.act(DialogKeyAction{InputKey::Escape});
                    } else {f.act(CastSpellAction{});f.act(DialogKeyAction{'c'});}
                    cast=true;
                }else f.act(AttackAction{});
            }
            const auto ui=f.flow->inputContext(f.flow->frame().presentation());if(ui.dialog && ui.dialog->anyKey)f.act(AcknowledgeAction{});
            tick(f);if(cast && !f.flow->encounter()->combat() && f.flow->canSave())break;
        }
        check(f.flow->canSave(),"damage spell combat witness did not settle");
    } else if(mode==3) {
        f.act(SelectMemberAction{0});for(auto key:{'i','m','1','u'})f.act(DialogKeyAction{unsigned(key)});
        f.act(SelectMemberAction{1});quiet(f);
    } else {
        f.act(CastSpellAction{});f.act(SelectMemberAction{4});f.act(DialogKeyAction{'c'});
        if(mode==1)f.act(SelectMemberAction{1});
        if(mode==4){f.act(AcknowledgeAction{});f.act(DialogKeyAction{InputKey::Escape});}
        if(mode==6)f.act(DialogKeyAction{InputKey::Escape});
        quiet(f);
    }
    constexpr unsigned expected[]{16,30,51,30,21,30,20,16,21,21,21,21};
    check(enabled?heard(fx,expected[mode]):fx.empty(),("spell/item/travel/refusal cue missing or Efx gate failed (mode "+std::to_string(mode)+")").c_str());
    if(mode!=0 && enabled)check(std::count(fx.begin(),fx.end(),expected[mode])==1,("spell cue repeated (mode "+std::to_string(mode)+")").c_str());
    if(mode==7 && enabled) {
        std::vector<unsigned> healing;for(auto cue:fx)if(cue==30 || cue==20)healing.push_back(cue);
        check(healing==std::vector<unsigned>{30,20,30,20,30,20,30,20,30,20,30,20},"Moon Ray member cues must interleave with sparkle animations");
    }
    auto after=f.snapshot();after.soundOptions.efx=true;return XeenSaveFormat::encode(after);
}
}
int main(int argc,char **argv) {
    try {
        check(argc==3,"usage: fm-effects <installation> <audio-rules>");inventory(argv[2]);
        const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"original installation unavailable");
        Inputs in(*installation);missEffects();commonDamage(in,*installation);spellSelectors();
        for(unsigned mode=0;mode<12;++mode)check(spellWitness(in,*installation,mode,true)==spellWitness(in,*installation,mode,false),"spell Efx setting altered gameplay snapshot");
        for(bool wall:{false,true})for(bool spell:{false,true})
            check(projectileWall(in,*installation,wall,spell,true)==projectileWall(in,*installation,wall,spell,false),"projectile wall Efx setting altered gameplay snapshot");
        check(wrongFacing(in,*installation,true)==wrongFacing(in,*installation,false),"wrong-facing Efx setting altered gameplay snapshot");
        check(event(in,*installation,true)==event(in,*installation,false),"PlayFX Efx setting altered gameplay snapshot");
        check(wall(in,*installation,true)==wall(in,*installation,false),"wall/Bash Efx setting altered gameplay snapshot");
        check(passableBash(in,*installation,true)==passableBash(in,*installation,false),"passable Bash Efx setting altered gameplay snapshot");
        check(service(in,*installation,true)==service(in,*installation,false),"service Efx setting altered gameplay snapshot");
        check(combat(in,*installation,true)==combat(in,*installation,false),"combat Efx setting altered gameplay snapshot");
        std::cout<<"126 reconciled C1/C2 sites; original spell/item/travel/refusal and C1 witnesses; Efx-off snapshots identical\n";
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

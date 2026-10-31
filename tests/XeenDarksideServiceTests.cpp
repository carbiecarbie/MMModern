#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "app/XeenGameplayServices.h"
#include "app/XeenAudio.h"
#include "games/xeen/XeenEquipmentPurchase.h"
#include "games/xeen/XeenFoodBank.h"
#include "games/xeen/XeenInnDoor.h"
#include "games/xeen/XeenCharacterCreation.h"
#include "games/xeen/CloudsUiComposer.h"
#include "formats/xeen/XeenVoc.h"
#include <iostream>
#include <fstream>
#include <set>

using namespace training_test;
namespace mmodern {
struct XeenInnTestAccess {
    static unsigned trainingIndex(const XeenEventFlow &flow) {return flow._trainingUi->animation.index;}
};
}
namespace {
constexpr auto dark=XeenSide::Darkside;
constexpr auto archive=XeenSceneArchive::DarksideOnly;
XeenSaveSnapshot town(Inputs &in,unsigned map,unsigned action) {
    auto s=in.base();const XeenMapIdentity id{dark,std::uint16_t(map)};
    const auto events=in.events.load(id);bool found=false;
    for(const auto &e:events.records)if(e.opcode==17 && e.line==0 && e.parameters==std::vector<std::uint8_t>{std::uint8_t(action)}) {
        s.camera={id,e.x,e.y,e.direction<4?XeenDirection(e.direction):XeenDirection::North};found=true;break;
    }
    check(found,"Missing original Darkside service Event");
    s.darksideSong=7*xeenMapMusicRow(in.maps.loadGeometryMap(in.assets,id).geometry);
    const auto stats=XeenMonsterFormat::parse(*in.assets.readMonsterStatistics(dark));
    const auto actors=XeenActorApproach::actorsFromResources(in.maps.loadObjects(in.assets,id),stats);
    auto &table=s.map(id).monsters;table=XeenSaveMonsterTable{std::uint16_t(actors.size()),{}};
    for(const auto &a:actors) {
        if(a.lifecycle==XeenActorLifecycle::Present)table->slots.push_back({-128,-128,XeenActorLifecycle::Defeated,XeenActorStatus::Physical,true,-1});
        else table->slots.push_back({a.x,a.y,a.lifecycle,a.status,false,-1});
    }
    s.journey->context->day=8;s.journey->context->minutes=600;s.journey->purse->gold=1000000;s.food=1;
    s.soundOptions={true,false,true,false};return s;
}
struct Visit:Fixture {
    std::vector<XeenSaveSnapshot> saves;
    std::vector<std::string> samples;
    Visit(Inputs &in,const XeenSaveSnapshot &s,const XeenItemCatalog &catalog):Fixture(in,s,false,nullptr,{}, {},{},&catalog) {
        flow->drawDialogSprite=[&](auto &image,const char *name,unsigned index,int x,int y){in.assets.drawDialogSprite(image,name,index,x,y,xeenPictureArchive(c.mapId));};
        flow->drawSmithArt=[&](auto &image){in.assets.drawSmith(image,xeenPictureArchive(c.mapId));};
        flow->drawTrainingArt=[&](auto &image){in.assets.drawTraining(image,xeenPictureArchive(c.mapId));};
        flow->drawTempleArt=[&](auto &image){in.assets.drawTemple(image,xeenPictureArchive(c.mapId));};
        flow->drawBankArt=[&](auto &image){in.assets.drawBank(image,xeenPictureArchive(c.mapId));};
        flow->drawGuildArt=[&](auto &image){in.assets.drawGuild(image,xeenPictureArchive(c.mapId));};
        flow->drawTavernArt=[&](auto &image){in.assets.drawTavern(image,xeenPictureArchive(c.mapId));};
        flow->tavernFoodOffer=[&](unsigned n){return in.assets.tavernFoodOffer(n,xeenPictureArchive(c.mapId));};
        flow->tavernString=[&](std::size_t n){return in.assets.tavernString(n,xeenPictureArchive(c.mapId));};
        flow->loadSpellDescriptions=[&]{return in.assets.readArchiveResource("spldesc.bin",xeenPictureArchive(c.mapId));};
        flow->loadSampleDuration=[&](const char *name,XeenSceneArchive side){
            check(side==XeenSceneArchive::Darkside || side==archive,"Darkside voice requested from Clouds");
            samples.emplace_back(name);return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,side));
        };
        flow->loadInnScreen=[&](const auto &party){
            in.assets.loadPalette("mm4.pal",archive);in.assets.loadRawFramebuffer("back.raw",archive);
            CloudsUiComposer::drawBorder(party,[&](const char *name,unsigned index,int x,int y){in.assets.drawSprite(name,index,x,y);});
            return in.assets.snapshot();
        };
        flow->saveInn=[&]{
            XeenGameplayServices services{programState,in.resources(),{},in.mapLoader(),in.objectLoader(),{},in.font,{},{},{}};
            xeenSaveGameplay(services,w,p,c,f,*flow,{},[](auto &,const auto &,const auto &,const auto &){},nullptr,
                [&](const auto &snapshot,const auto &check){check();saves.push_back(snapshot);});
        };
    }
    void key(unsigned n){act(DialogKeyAction{n});}
    void tick(){now+=200;flow->beginCycle(++cycle);if(auto image=flow->updatePresentation())present(*image);}
    auto dialog(){return flow->inputContext(flow->frame().presentation()).dialog;}
    void until(const std::function<bool()> &done) {for(unsigned n=0;n<1000 && !done();++n)tick();check(done(),"Darkside service did not settle");}
    void leave(){
        const auto before=*w.sessionState().journeyRandom();XeenCombatRandom expected(before);expected.range(0,6);
        key(InputKey::Escape);until([&]{return flow->canSave();});
        check(w.sessionState().journeyRandom()==expected.continuation(),"Darkside departure makes one map-song draw");
    }
    void roundtrip() {
        const auto bytes=XeenSaveFormat::encode(snapshot());const auto catalog=loadXeenItemCatalog(in.assets);
        Visit restored(in,XeenSaveFormat::decode(bytes),catalog.catalog);
        check(XeenSaveFormat::encode(restored.snapshot())==bytes,"Darkside service restore changed saved state");
    }
};
void rules(Inputs &in,const GameInstallation &installation) {
    auto stream=installation.uiModule->open();std::vector<std::uint8_t> module(stream->size());
    check(stream->read(module.data(),module.size())==module.size(),"Read installed DOS catalogue tables");
    std::vector<std::pair<std::string,XeenSceneArchive>> songs;
    const auto loader=[&](const std::string &name,XeenSceneArchive side) {songs.emplace_back(name,side);return in.assets.readArchiveResource(name,side);};
    XeenAudio audio(3,std::make_shared<const std::vector<std::uint8_t>>(in.assets.readArchiveResource("promus",XeenSceneArchive::IntroOnly)),loader,module,0,false);
    constexpr const char *cloudSongs[]{"bank.m","smith.m","guild.m","tavern.m","temple.m","grounds.m"};
    constexpr const char *darkSongs[]{"bank.m","sf09.m","guild.m","tavern.m","temple.m","smith.m"};
    unsigned tick=0;
    for(auto side:{XeenSide::Clouds,dark}) {
        for(unsigned action=0;action<6;++action) {
            const auto name=(side==dark?darkSongs:cloudSongs)[action];
            check(audio.locationSong(action,side)==name,"Location song by side");
            audio.requestLocation(action,PromusDriver::clockBoundary(tick),side);tick+=400;audio.advance(PromusDriver::clockBoundary(tick));
            check(audio.loadedName()==name && songs.back()==std::pair<std::string,XeenSceneArchive>{name,side==dark?archive:XeenSceneArchive::Clouds},"Location song archive");
        }
        audio.requestInn(PromusDriver::clockBoundary(tick),side);tick+=400;audio.advance(PromusDriver::clockBoundary(tick));
        check(audio.loadedName()==(side==dark?"newbrigh.m":"inn.m"),"World CD Inn song by side");
    }
    constexpr unsigned clouds[]{28,30,73,49},towns[]{29,31,37,43},caps[]{30,50,200,100};
    for(unsigned i=0;i<4;++i) {
        check(xeenIronworksShop(clouds[i])==i && xeenIronworksShop({dark,std::uint16_t(towns[i])})==i,"Side-aware Smith shop");
        check(xeenTrainingCap({dark,std::uint16_t(towns[i])})==caps[i],"Darkside Training cap");
    }
    check(xeenIronworksShop({dark,100})==0 && xeenIronworksShop(29)==0,"Smith default");
    check(xeenTrainingCap(28)==10 && xeenTrainingCap(30)==15 && xeenTrainingCap(37)==20 && xeenTrainingCap({dark,100})==100,"Training side/default");
    constexpr unsigned food[]{15,60,120},gold[]{10,100,250},maps[]{29,31,37};
    constexpr int x[]{17,14,9},y[]{24,13,4};constexpr XeenDirection facing[]{XeenDirection::West,XeenDirection::East,XeenDirection::South};
    for(unsigned i=0;i<3;++i) {
        const XeenMapIdentity id{dark,std::uint16_t(maps[i])};const auto offer=*xeenTavernFoodOffer(id);
        check(offer.perMember==food[i] && offer.gold==gold[i] && offer.text==60+i,"Darkside Food rule");
        check(!in.assets.tavernFoodOffer(offer.text,archive).empty(),"Darkside Food resource");
        for(unsigned n:{1u,3u,6u}) {
            const auto bought=xeenBuyFood(n,0,1000,true,id);
            check(bought.foodAfter==food[i]*n && bought.goldAfter==1000-gold[i],"Food refill amount/payment");
            check(xeenBuyFood(n,food[i]*n,0,true,id).outcome==XeenFoodOutcome::Full,"Full precedes payment");
        }
        const auto door=*xeenInnDoor(id);
        check(door.mapId==id && door.x==x[i] && door.y==y[i] && door.direction==facing[i] && xeenAtInnDoor(door),"Darkside Inn door");
    }
    check(!xeenInnDoor({dark,43}) && xeenInnDoor(49)->x==11,"Inn side/default and Shangri-La");
    check(xeenTavernFoodOffer({dark,43})->gold==250 && !xeenTavernFoodOffer(29),"Food side/default");
    auto p=XeenPartyLoader().loadInitialParty(in.assets);
    for(unsigned i=0;i<5;++i) {
        const XeenMapIdentity id{dark,std::uint16_t(29+2*i)};auto &c=p.roster.at(p.party.activeRosterIds()[0]);c.awards.fill(0);
        check(!xeenGuildMember(p,0,id),"Guild without membership");c.awards[83+i]=1;
        check(xeenGuildMember(p,0,id) && !xeenGuildMember(p,0,28),"Guild award/side");
    }
    for(unsigned cls=0;cls<10;++cls) {
        auto c=p.party.member(p.roster,0);c.characterClass=XeenCharacterClass(cls);c.learnedSpells.emplace();c.learnedSpells->fill(0);
        const auto category=XeenLearnedSpellRules::categoryForClass(c.characterClass);
        const auto all=XeenLearnedSpellRules::rows(c,true,true,{dark,37});
        std::vector<std::size_t> expected;
        for(unsigned id=0;category && id<76;++id)if(const auto slot=XeenLearnedSpellRules::slotForSpell(*category,id))expected.push_back(*slot);
        check(all==expected,"Olympus all class-allowed spells in global order");
        for(unsigned map:{29u,31u,33u,35u}) {
            auto rows=XeenLearnedSpellRules::rows(c,true,true,{dark,std::uint16_t(map)});
            if(!category)check(rows.empty(),"Noncaster Guild catalogue");
            else {
                const auto cat=unsigned(*category),group=(map-29)/2;
                std::vector<std::size_t> original;
                const auto begin=module.at(0x546c2+(cat*4+group)*2),end=module.at(0x546c3+(cat*4+group)*2);
                for(unsigned i=begin;i<end;++i)if(const auto slot=XeenLearnedSpellRules::slotForSpell(*category,module.at(0x4f839+cat*39+i)))original.push_back(*slot);
                check(rows==original,"Guild catalogue order/range differs from installed DOS tables");
                for(auto slot:rows)check(XeenLearnedSpellRules::spellForSlot(*category,slot).has_value(),"Guild class filter");
                c.learnedSpells->at(rows.front())=1;
                const auto buy=XeenLearnedSpellRules::rows(c,true,false,{dark,std::uint16_t(map)});
                check(std::find(buy.begin(),buy.end(),rows.front())==buy.end() && XeenLearnedSpellRules::rows(c,true,true,{dark,std::uint16_t(map)})==rows,"Guild Buy/Info filtering");
                c.learnedSpells->fill(0);
            }
        }
    }
    for(auto side:{XeenSide::Clouds,dark}) {
        XeenGameplayContext context;context.year=610;
        const auto created=xeenCreatedCharacter(0,"New",0,{{16,16,16,16,16,16,16}},{side,29},context);
        check(created.first.xeenSide==unsigned(side) && created.first.savedMaze==29,"Created Inn character lodging side");
    }
}
void depletion(Inputs &in,const XeenItemCatalog &catalog) {
    auto s=town(in,29,1);
    for(unsigned shop=0;shop<4;++shop)for(unsigned category=0;category<4;++category) {
        auto &stock=s.journey->serviceEconomy->wares[1][shop][category];
        unsigned count=0;for(const auto &item:stock)count+=item.id!=0;
        if(count) {stock[count/2]={};xeenCompactItems(stock);}
        xeenValidateCurrentServiceEconomy(*s.journey->serviceEconomy);
        Visit live(in,s,catalog);live.roundtrip();
    }
    for(auto &shop:s.journey->serviceEconomy->wares[1])for(auto &category:shop)category={};
    Visit empty(in,s,catalog);empty.roundtrip();
    auto invalid=*s.journey->serviceEconomy;invalid.wares[1][0][0][0]={0,1,0,1};
    bool refused=false;try{xeenValidateCurrentServiceEconomy(invalid);}catch(const std::invalid_argument &){refused=true;}
    check(refused,"Darkside depletion retains occupied-prefix validation");
}
void animation(Inputs &in) {
    constexpr unsigned moduli[]{26,19,48,27,26,37};
    constexpr const char *greetings[]{"bank1.voc","see2.voc","parrot1.voc","hello1.voc","help2.voc","youtrn1.voc"};
    for(unsigned i=0;i<6;++i) {
        const auto kind=XeenServiceArt(i);
        check(XeenServiceAnimation::modulus(kind,dark)==moduli[i] && XeenServiceAnimation::next(kind,moduli[i]-1,dark)==0,"Darkside animation cycle");
        check(std::string(XeenServiceAnimation::greeting(kind,dark))==greetings[i],"Darkside greeting");
        check(xeenVocDurationNanoseconds(in.assets.readArchiveResource(greetings[i],archive))>0,"Original greeting resource");
        for(bool busy:{false,true})for(unsigned index=0;index<moduli[i];++index) {
            const auto frame=XeenServiceAnimation::darkside(kind,index,busy,0,[](auto lo,auto){return lo;});
            for(const auto &p:frame.sprites)check(p.sprite.frame<in.assets.spriteFrameCount(p.sprite.resource,archive),"Darkside animation frame exceeds original resource");
        }
    }
    unsigned draws=0;const auto draw=[&](unsigned lo,unsigned){++draws;return lo;};unsigned selector=2;
    for(int expected=11;expected>=0;--expected) {
        const auto f=XeenServiceAnimation::darkside(XeenServiceArt::Bank,0,false,selector,draw);selector=f.selector;
        check(f.sprites.size()==2 && f.sprites.back().sprite.frame==unsigned(expected),"Bank DOS countdown");
    }
    check(!selector && !draws && XeenServiceAnimation::darkside(XeenServiceArt::Bank,0,false,selector,draw).sprites.size()==1,"Bank countdown finishes once");
    selector=5;for(unsigned expected:{4u,5u,4u}) {
        const auto f=XeenServiceAnimation::darkside(XeenServiceArt::Guild,0,true,selector,draw);selector=f.selector;
        check(f.sprites.back().sprite.frame==expected && f.sprites.back().x==8 && f.sprites.back().y==106,"Guild skull frames");
    }
    check(XeenServiceAnimation::darkside(XeenServiceArt::Guild,0,false,5,draw).sprites.size()==1,"Guild idle has no overlay");
    const auto trainer=XeenServiceAnimation::darkside(XeenServiceArt::Training,32,true,7,draw);
    check(trainer.sprites[0].sprite.resource=="trng1.twn" && trainer.sprites[0].sprite.frame==0 && trainer.sprites[1].x==61 && trainer.sprites[1].y==74,"Trainer speaking composition");
    check(XeenServiceAnimation::firstRepeat(XeenServiceArt::Guild)==17 && XeenServiceAnimation::firstRepeat(XeenServiceArt::Temple)==20,"Clouds loopbacks unchanged");
    for(const auto pair:{std::pair{XeenServiceArt::Smith,"come1.voc"},std::pair{XeenServiceArt::Tavern,"gdluck1.voc"}})
        check(std::string(XeenServiceAnimation::voice(pair.first,XeenServiceVoice::Farewell,dark))==pair.second,"Darkside farewell");
    for(auto kind:{XeenServiceArt::Bank,XeenServiceArt::Guild,XeenServiceArt::Temple,XeenServiceArt::Training})check(!XeenServiceAnimation::voice(kind,XeenServiceVoice::Farewell,dark),"Unvoiced departure");
    for(const auto &voice:std::vector<std::tuple<XeenServiceArt,XeenServiceVoice,const char *>>{
            {XeenServiceArt::Smith,XeenServiceVoice::Purchase,"choice2.voc"},
            {XeenServiceArt::Guild,XeenServiceVoice::Purchase,"parrot2.voc"},
            {XeenServiceArt::Guild,XeenServiceVoice::Refusal,"skull1.voc"},
            {XeenServiceArt::Tavern,XeenServiceVoice::Thanks,"thanks2.voc"},
            {XeenServiceArt::Training,XeenServiceVoice::Purchase,"prtygd.voc"},
            {XeenServiceArt::Training,XeenServiceVoice::TrainingCap,"gtlost.voc"},
            {XeenServiceArt::Training,XeenServiceVoice::TrainingExperience,"gtlost.voc"},
            {XeenServiceArt::Training,XeenServiceVoice::TrainingGold,"gtlost.voc"}}) {
        const auto name=XeenServiceAnimation::voice(std::get<0>(voice),std::get<1>(voice),dark);
        check(std::string(name)==std::get<2>(voice) && xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive))>0,"Darkside action/refusal voice");
    }
}
void visits(Inputs &in,const XeenItemCatalog &catalog) {
    // Every resource-backed DoTownEvent 0-5 site enters the existing live owner.
    for(unsigned map:{29u,31u,33u,35u,37u,43u}) {
        std::set<unsigned> actions;
        for(const auto &e:in.events.load({dark,std::uint16_t(map)}).records)
            if(e.opcode==17 && e.parameters.size()==1 && e.parameters[0]<=5)actions.insert(e.parameters[0]);
        for(auto action:actions) {
            std::cout<<"Town "<<map<<" service "<<action<<std::endl;
            auto s=town(in,map,action);Visit live(in,s,catalog);live.enter();
            check(!live.samples.empty() && live.samples.front()==XeenServiceAnimation::greeting(XeenServiceArt(action),dark),"Live Darkside greeting");
            if(action==0) {
                XeenCombatRandom expected(*s.journey->random);expected.range(13,18);expected.range(13,18);
                check(live.w.sessionState().journeyRandom()==expected.continuation(),"Bank has exactly two pre-greeting overlay draws");
            }
            if(const auto *path=std::getenv("MMODERN_M102_FRAMES")) {
                const std::filesystem::path out=path;std::filesystem::create_directories(out);
                std::ofstream file(out/(std::to_string(map)+"-"+std::to_string(action)+".ppm"),std::ios::binary);file<<"P6\n320 200\n255\n";
                const auto palette=in.assets.readArchiveResource("mm4.pal",archive);
                for(auto pixel:live.flow->frame().pixels)for(unsigned channel=0;channel<3;++channel) {const unsigned char color=palette[pixel*3+channel]*4;file.write(reinterpret_cast<const char *>(&color),1);}
            }
            live.leave();live.roundtrip();
        }
    }
    {
        auto s=town(in,31,1);Visit live(in,s,catalog);live.enter();live.key('b');
        const auto owner=live.p.party.activeRosterIds()[0];const auto before=live.p.purse->gold;
        const auto offer=live.p.serviceEconomy->wares[1][1][0][0];const auto price=*xeenEquipmentPurchasePrice(XeenInventoryCategory::Weapons,offer);
        live.key('1');live.key('y');
        check(live.p.purse->gold==before-price && !xeenSameItem(live.p.serviceEconomy->wares[1][1][0][0],offer),"Darkside Smith payment/depletion");
        check(std::any_of(live.p.roster.at(owner).weapons.begin(),live.p.roster.at(owner).weapons.end(),[&](const auto &item){return xeenSameItem(item,offer);}),"Darkside Smith delivery");
        live.key(InputKey::Escape);live.leave();live.roundtrip();
    }
    {
        auto s=town(in,29,5);const auto owner=s.activeRosterIds[0];s.journey->supplements[owner].inputs.experience=20000000;
        Visit live(in,s,catalog);live.enter();const auto before=live.p.roster.at(owner).permanentLevel;live.key('t');
        live.until([&]{return live.p.roster.at(owner).permanentLevel==before+1;});live.leave();live.roundtrip();
    }
    {
        auto s=town(in,29,5);s.journey->supplements[s.activeRosterIds[0]].inputs.experience=20000000;s.journey->purse->gold=0;
        Visit live(in,s,catalog);live.enter();for(unsigned i=0;i<5;++i)live.tick();
        const auto index=XeenInnTestAccess::trainingIndex(*live.flow);live.key('t');
        check(live.samples.back()!="gtlost.voc","Training gold voice waits for the payment refusal dialog");
        live.act(AcknowledgeAction{});
        check(live.samples.back()=="gtlost.voc" && XeenInnTestAccess::trainingIndex(*live.flow)==index,"Training gold refusal preserves the logical animation index");
        live.leave();
    }
    {
        auto s=town(in,37,2);const auto owner=s.activeRosterIds[0];auto &c=s.characters[owner];
        c.weapons={};c.armor={};c.accessories={};c.characterClass=XeenCharacterClass::Cleric;c.learnedSpells.emplace();c.learnedSpells->fill(0);c.awards[87]=1;
        const auto rows=XeenLearnedSpellRules::rows(c,true,false,{dark,37});const auto slot=rows.front();
        const auto id=*XeenLearnedSpellRules::spellForSlot(XeenSpellCategory::Clerical,slot);
        Visit live(in,s,catalog);live.enter();const auto gold=live.p.purse->gold;live.key('b');live.key('1');live.key('y');
        check(live.p.roster.at(owner).learnedSpells->at(slot)==1 && live.p.purse->gold==gold-XeenLearnedSpellRules::purchasePrice(id,c.characterClass),"Olympus Guild purchase");
        live.key(InputKey::Escape);live.leave();live.roundtrip();
    }
    for(unsigned map:{29u,31u,37u}) {
        auto s=town(in,map,3);Visit live(in,s,catalog);live.enter();const auto day=live.p.encounterContext->day;
        live.key('s');live.until([&]{return live.p.insideInn && live.dialog() && live.dialog()->key('r');});
        check(live.p.encounterContext->day==day+1 && live.saves.size()==1 && xeenAtInnDoor(live.c),"Darkside Sign In day/save/door");
        for(auto owner:live.p.party.activeRosterIds())check(live.p.roster.at(owner).xeenSide==1 && live.p.roster.at(owner).savedMaze==map,"Darkside Sign In lodging side");
        live.key('r');live.key(InputKey::F1+5);check(live.p.party.size()==5,"Darkside Inn removes member");
        live.key(InputKey::Escape);live.until([&]{return live.flow->canSave();});
        check(!live.p.insideInn && live.saves.size()==2 && xeenAtInnDoor(live.c),"Darkside Inn return");live.roundtrip();
    }
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2,"usage: darkside-service-tests <installation>");const auto install=xeenTestInstallationDetector().detect(argv[1]);check(bool(install),"Missing original installation");
        Inputs in(*install);const auto catalog=loadXeenItemCatalog(in.assets);rules(in,*install);animation(in);depletion(in,catalog.catalog);visits(in,catalog.catalog);
        std::cout<<"Darkside service rules, original town visits/purchases, Inn lodging and exact restoration passed\n";return 0;
    } catch(const std::exception &e){std::cerr<<"Darkside services: "<<e.what()<<'\n';return 1;}
}

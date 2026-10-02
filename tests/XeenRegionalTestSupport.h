#ifndef MMODERN_TESTS_REGIONAL_SUPPORT_H
#define MMODERN_TESTS_REGIONAL_SUPPORT_H
#include "app/XeenEncounterFlow.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenSaveState.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
namespace regional_test {
using namespace mmodern;
using Bytes=std::vector<std::uint8_t>;
inline void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
inline Bytes partyBytes(){
    Bytes b(812);b[0]=b[1]=6;
    std::copy(kXeenCombatOwners.begin(),kXeenCombatOwners.end(),b.begin()+2);
    b[8]=b[9]=255;b[612]=1;b[614]=0x62;b[615]=2;b[616]=0xe0;b[617]=1;
    return b;
}
inline Bytes characterBytes(){
    Bytes b(30*354);
    for(unsigned owner=0;owner<30;++owner){
        const auto n=owner*354;b[n]='T';b[n+19]=1;
        for(unsigned a:{20u,22u,24u,26u,28u,30u,32u})b[n+a]=15;
        b[n+35]=1;b[n+342]=10;b[n+346]=80;b[n+347]=2;
        b[n+167]=6;b[n+169]=1;
    }
    return b;
}
inline void fingerprint(XeenMonsterRecord &r,std::uint32_t expected){
    const auto base=r.fingerprint();
    std::array<std::uint32_t,32> basis{},masks{};
    for(unsigned i=0;i<32;++i){
        r.raw[4+i/8]^=1u<<(i%8);
        auto delta=r.fingerprint()^base;auto mask=std::uint32_t(1)<<i;
        r.raw[4+i/8]^=1u<<(i%8);
        for(int bit=31;bit>=0;--bit)if(delta&(std::uint32_t(1)<<bit)){
            if(basis[bit]){delta^=basis[bit];mask^=masks[bit];}
            else {basis[bit]=delta;masks[bit]=mask;break;}
        }
    }
    auto delta=expected^base;std::uint32_t mask=0;
    for(int bit=31;bit>=0;--bit)if(delta&(std::uint32_t(1)<<bit)){
        check(basis[bit]!=0,"Synthetic CRC basis");delta^=basis[bit];mask^=masks[bit];
    }
    for(unsigned i=0;i<32;++i)if(mask&(std::uint32_t(1)<<i))r.raw[4+i/8]^=1u<<(i%8);
    check(r.fingerprint()==expected,"Synthetic MON fingerprint");
}
inline std::vector<XeenMonsterRecord> statistics(){
    std::vector<XeenMonsterRecord> out(14);
    auto &r=out[8];r.raw[0]='T';r.raw[16]=250;r.raw[20]=20;r.raw[22]=5;r.raw[23]=10;
    r.raw[24]=1;r.raw[25]=3;r.raw[26]=2;r.raw[28]=6;r.raw[31]=4;r.raw[33]=4;
    for(unsigned i=34;i<38;++i)r.raw[i]=50;r.raw[40]=50;r.raw[47]=8;
    fingerprint(r,0xe36833c6);return out;
}
inline XeenMap map(XeenMapIdentity id={23}){
    XeenMap m;m.geometry.id=id.number;m.side=id.side;m.geometry.flags2=0x8000;
    for(unsigned i=0;i<16;++i)m.geometry.surfaceTypes[i]=i;
    for(auto &c:m.geometry.cells){c.rawWord=1;c.surfaceIndex=1;c.geometry=XeenOutdoorLayers{1,0,0,0};}
    return m;
}
inline XeenObjectFile objects(XeenMapIdentity id={23}){
    XeenObjectFile m{id,"synthetic.mob",true,{}};
    if(id==XeenMapIdentity(23))for(unsigned i=0;i<19;++i)
        m.entities.monsters.push_back({int(i%7),int(i/7),0,0,8});
    m.entities.objects.push_back({9,11,0,0,7});
    return m;
}
struct Site{unsigned index,offset,x,y,direction,line,opcode;std::vector<std::uint8_t> bytes;};
inline void exact(XeenEventFile &f,const Site &s){
    auto &r=f.records.at(s.index);r.fileOffset=s.offset;r.lengthField=5+s.bytes.size();
    r.x=s.x;r.y=s.y;r.direction=s.direction;r.line=s.line;r.opcode=s.opcode;r.parameters=s.bytes;
}
inline std::pair<XeenEventFile,XeenEventFile> eventFiles(){
    XeenEventFile mainland{23,"maze0023.evt",true,{}},city{28,"maze0028.evt",true,{}};
    mainland.records.resize(170);city.records.resize(847);
    for(auto *f:{&mainland,&city})for(unsigned i=0;i<f->records.size();++i){
        auto &r=f->records[i];r.fileOffset=i*6;r.lengthField=5;r.x=200;r.y=200;r.direction=4;r.line=i%256;r.opcode=0x12;
    }
    constexpr unsigned content=14;
	if (xeenJourneyContent(content).armorRepair()) exact(city,{0,0,8,4,4,0,0x11,{1}});
	if (xeenJourneyContent(content).training()) {
		exact(city,{3,23,10,11,4,0,0x11,{5}});
		exact(city,{538,4464,10,8,0,0,0x02,{32}});
	}
	if (xeenJourneyContent(content).templeRecovery()) {
		exact(city,{543,4499,15,21,0,0,0x02,{37}});
		exact(city,{6,44,15,28,4,0,0x11,{4}});
	}
	for(const Site &s:{
		Site{136,1141,10,13,4,0,0x01,{33}},Site{137,1148,10,13,4,1,0x09,{44,0,3}},
		Site{138,1157,10,13,4,2,0x12,{}},Site{139,1163,10,13,4,3,0x07,{28,15,0}}
	})exact(mainland,s);
	for(const Site &s:{
		Site{539,4471,13,4,3,0,0x02,{33}},
		Site{760,6468,15,0,2,0,0x19,{75,76,0}},Site{761,6477,15,0,2,1,0x01,{57}},
		Site{762,6484,15,0,2,2,0x09,{44,0,4}},Site{763,6493,15,0,2,3,0x12,{}},
		Site{764,6499,15,0,2,4,0x2f,{}},Site{765,6505,15,0,2,5,0x18,{4,0}},
		Site{766,6513,15,0,2,6,0x09,{20,9,8}},Site{767,6522,15,0,2,7,0x19,{100,100,0}},
		Site{768,6531,15,0,4,8,0x1b,{84,2}},Site{769,6539,15,0,4,9,0x07,{23,10,12}},
		Site{816,6998,75,76,4,0,0x09,{20,231,2}},Site{817,7007,75,76,4,1,0x08,{9,0,3}},
		Site{818,7016,75,76,4,2,0x1a,{}},Site{819,7022,75,76,4,3,0x0c,{0,0,20,231}},
		Site{846,7292,75,76,4,30,0x1a,{}},Site{813,6978,100,100,4,43,0x1a,{}}
	})exact(city,s);
	for(unsigned line=4;line<=29;++line) {
		const unsigned flag=line<=27 ? 232+line-4 : line==28 ? 57 : 58;
		exact(city,{816+line,7032+10*(line-4),75,76,4,line,0x0c,{20,static_cast<std::uint8_t>(flag),0,0}});
	}
	struct Spawn {unsigned slot,x,y,unused;};
	static constexpr std::array<Spawn,43> reset{{
		{0,1,11,0},{1,1,11,0},{2,2,9,0},{3,3,10,0},{4,3,11,0},
		{5,3,11,0},{6,3,13,0},{7,3,13,0},{8,3,27,0},{9,4,27,0},
		{10,4,26,0},{11,4,25,0},{12,4,12,0},{13,4,7,0},{14,4,7,0},
		{15,4,3,0},{16,4,3,0},{17,4,3,0},{18,5,12,0},{19,9,18,0},
		{20,25,14,0},{21,28,9,0},{22,30,9,0},{23,30,6,0},{24,29,15,0},
		{25,8,24,0},{26,8,24,0},{27,7,23,0},{28,7,23,0},{29,8,27,0},
		{30,8,27,0},{31,9,18,0},{32,6,2,1},{33,7,1,1},{34,6,6,1},
		{35,7,7,1},{36,15,4,0},{37,22,9,0},{38,21,1,0},{39,22,1,0},
		{40,30,1,0},{50,7,24,0},{51,6,27,0}
	}};
	for(unsigned line=0;line<reset.size();++line) {
		const auto s=reset[line];
		exact(city,{770+line,6548+10*line,100,100,4,line,0x10,
			{static_cast<std::uint8_t>(s.slot),static_cast<std::uint8_t>(s.x),
			static_cast<std::uint8_t>(s.y),static_cast<std::uint8_t>(s.unused)}});
	}
    return {mainland,city};
}
inline XeenEventFile events(XeenMapIdentity id){auto files=eventFiles();return id==XeenMapIdentity(28)?files.second:files.first;}
inline XeenEventTextFile texts(XeenMapIdentity id){
    return {id,id==XeenMapIdentity(28)?"aaze0028.txt":"aaze0023.txt",true,std::vector<std::string>(64,"Synthetic text")};
}
inline XeenSaveResourceSignature signature(){return {{1,2},XeenArchiveFingerprint{3,4}};}
inline XeenSaveState::Resources resources(){
    XeenSaveState::Resources r;r.signature=signature();r.loadInitialCharacters=characterBytes;
    r.loadEvents=events;r.loadMonsterStatistics=statistics;
    r.regionalManifest=[](const auto &,const auto &,const auto &,const auto &){};
    r.vertigoManifest=[](auto &,const auto &,const auto &){};
    r.loadRegionalText=texts;r.loadLearnedSpellNames=[]{return XeenLearnedSpellNames{};};
    r.loadInitialParty=[]{return XeenPartyLoader().loadFromResources(characterBytes(),partyBytes());};
    r.loadInitialContext=[]{return XeenGameplayContextFormat::parse(partyBytes());};
    r.loadInitialPurse=[]{return XeenMonsterTreasure{};};
    r.loadInitialRegionalRecovery=[]{return XeenRegionalRecoveryState{};};
    r.loadInitialBankBalances=[]{return XeenBankBalances{};};
    return r;
}
struct Fixture{
    Bytes bytes=characterBytes();
    std::vector<XeenMonsterRecord> monsters=statistics();
    XeenEventFile event=events(23);
    XeenPartyState p;
    XeenCamera camera=xeenJourneyContent(14).entry;
    XeenGameFlags flags;
    std::function<void()> onMap,onObjects;
    std::function<void(XeenMap &)> transformMap;
    XeenWorld w{[this](auto id){if(onMap)onMap();auto value=map(id);if(transformMap)transformMap(value);return value;},[this](auto id){if(onObjects)onObjects();return objects(id);}};
    XeenEventPresenter::Clock clock=[]{return 0;};
    std::unique_ptr<XeenEncounterFlow> flow;
    explicit Fixture(const std::optional<XeenSaveSnapshot> &saved={}){
        if(saved){
            XeenSaveState::restoreBeforeGameplay(*saved,resources(),p,camera,flags,w,[](auto &,const auto &,const auto &,const auto &){});
            flow=std::make_unique<XeenEncounterFlow>(w,p,camera,flags,clock,XeenJourneyRestoreTag{});
        }else{
            p=XeenPartyLoader().loadFromResources(bytes,partyBytes());
            const auto r=resources();
            XeenJourneySetup setup{bytes,XeenGameplayContextFormat::parse(partyBytes()),monsters,event,1,14,r.regionalManifest};
            setup.purse=XeenMonsterTreasure{};setup.regionalRecovery=XeenRegionalRecoveryState{};
            setup.regionalText=texts(23);setup.learnedNames=XeenLearnedSpellNames{};
            setup.learnedNamesProvider=r.loadLearnedSpellNames;setup.vertigoManifest=r.vertigoManifest;
            setup.bank=XeenBankBalances{};setup.cityEventsProvider=[]{return events(28);};
            flow=std::make_unique<XeenEncounterFlow>(w,p,camera,flags,clock,setup);
        }
        present();
    }
    void present(){
        if(w.sessionState().journeyActivity()==XeenJourneyActivity::Presentation){
            check(flow->prepareJourneyFrame(flow->ticket(),[]{}),"Synthetic regional frame preparation");
            check(flow->presentJourney(flow->ticket()),"Synthetic regional frame presentation");
        }
    }
    XeenSaveSnapshot snapshot(){return XeenSaveState::capture(signature(),p,camera,flags,w);}
    auto action(XeenEncounterAction a){auto r=flow->journeyAction(flow->ticket(),a);present();return r;}
    auto pulse(){auto r=flow->journeyPulse(flow->ticket());present();return r;}
};
inline XeenSaveSnapshot snapshot(){Fixture f;return f.snapshot();}
}
#endif

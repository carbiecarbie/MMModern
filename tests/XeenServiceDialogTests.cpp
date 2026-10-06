#include "XeenPurchaseTestSupport.h"
#include <fstream>
#include <iostream>
using namespace purchase_test;
namespace {
void click(training_test::Fixture &f,int x,int y) {
    const auto context=f.flow->inputContext(f.flow->frame().presentation());
    check(!context.acceptsQueuedInput && context.dialog && context.readyForAction,"service is not a strict actionable dialog");
    const auto action=context.dialog->click(x,y);check(bool(action),"original service hit absent");f.act(*action);
}
void keyboard(training_test::Fixture &f,unsigned key){f.act(DialogKeyAction{key});}
void layout(Inputs &in) {
    Fixture f(in,service(in));auto base=training_test::frame();base.pixels.assign(64000,77);
    const auto draw=f.flow->drawDialogSprite;
    for(const auto location:{XeenLocationDialog::Smith,XeenLocationDialog::Training,XeenLocationDialog::Temple}) {
        auto art=base;
        if(location==XeenLocationDialog::Smith)in.assets.drawSmith(art);
        else if(location==XeenLocationDialog::Training)in.assets.drawTraining(art);
        else in.assets.drawTemple(art);
        for(unsigned member=0;member<6;++member) {
            auto rendered=drawXeenLocation(base,art,in.font,location,f.p,member,draw);
            for(int y=8;y<140;++y)for(int x=8;x<224;++x)
                check(rendered.pixels[y*320+x]==art.pixels[y*320+x],"location panel covers town art");
            check(rendered.pixels[155*320+230]==77,"location overwrites party strip");
            if(const auto *path=std::getenv("MMODERN_SERVICE_PREVIEW")) {
                std::ofstream out(std::string(path)+"/location-"+std::to_string(unsigned(location))+"-"+std::to_string(member)+".ppm",std::ios::binary);
                out<<"P6\n320 200\n255\n";
                auto palette=in.assets.readArchiveResource("mm4.pal");
                for(const auto pixel:rendered.pixels) for(unsigned channel=0;channel<3;++channel) {
                    const unsigned char color=palette[pixel*3+channel]*4;out.write(reinterpret_cast<const char *>(&color),1);
                }
            }
            XeenInventorySelection selection;selection.source=member;
            for(unsigned category=0;category<4;++category)for(bool repair:{false,true}) {
                selection.category=static_cast<XeenInventoryCategory>(category);
                check(drawXeenBuy(rendered,in.font,loadXeenItemCatalog(in.assets).catalog,f.p,selection,repair,draw).isValid(),"original Buy/Fix template overflow");
            }
        }
    }
    check(drawXeenErrorScroll(base,in.font,xeenNotEnoughGold()).isValid(),"gold refusal layout");
    check(drawXeenConfirm(base,in.font,xeenServiceConfirm(true,"Plate Armor",200),false,draw).isValid(),"Fix Confirm layout");
}
void refusals(Inputs &in) {
    for(bool temple:{false,true}) {
        auto source=in.service();source.journey->treasure->gold=0;
        if(temple) {source.camera={28,15,28,XeenDirection::North};source.characters[6].currentHp=-15;
            source.characters[6].conditions[13]=1;source.characters[6].armor[0].state|=64;}
        Fixture f(in,source);f.flow->drawTempleArt=[&](auto &image){in.assets.drawTemple(image);};
        f.act(InteractionAction{});f.prepare();keyboard(f,InputKey::F1+(temple?5:1));
        const Owners before(f);std::string notice;f.flow->reportText=[&](const auto &text){notice=text;};
        keyboard(f,temple?'h':'t');check(notice==xeenNotEnoughGold(),"one-step service lacks original gold refusal");before.unchanged(f);
        keyboard(f,InputKey::Enter);
        if(temple) {keyboard(f,'u');check(notice.find("not supported yet")!=std::string::npos,"Uncurse lacks refusal");before.unchanged(f);keyboard(f,InputKey::Enter);}
        keyboard(f,InputKey::Escape);check(f.flow->canSave() && f.p.encounterContext->day==9,"refused service lost unpaid departure");
    }
    Fixture f(in,service(in));f.enter();keyboard(f,'b');keyboard(f,'c');const Owners before(f);
    std::string notice;f.flow->reportText=[&](const auto &text){notice=text;};keyboard(f,'1');
    check(notice.find("not supported yet")!=std::string::npos,"Accessories Buy lacks refusal");before.unchanged(f);
}
void dialogs(Inputs &in,bool mouse) {
    auto source=service(in);source.characters[0].armor[0].state=128;
    Fixture smith(in,source);smith.enter();
    const auto before=Owners(smith);
    keyboard(smith,'r');keyboard(smith,InputKey::Enter);before.unchanged(smith);
    check(XeenPurchaseTestAccess::repairArmor(*smith.flow),"Lobby R did not retain Armor Repair");
    keyboard(smith,InputKey::Escape);before.unchanged(smith);
    if(mouse)click(smith,235,65);else keyboard(smith,'b');
    for(const auto key:{'s','i'}) {
        std::string notice;smith.flow->reportText=[&](const auto &text){notice=text;};
        keyboard(smith,key);check(notice.find("not supported yet")!=std::string::npos,"Smith unsupported action lacks notice");before.unchanged(smith);keyboard(smith,InputKey::Enter);
    }
    smith.flow->reportText={};
    if(mouse)click(smith,47,110);else keyboard(smith,'a');
    if(mouse)click(smith,20,48);else keyboard(smith,'4');
    check(XeenPurchaseTestAccess::quote(*smith.flow),"row does not open original Confirm");
    keyboard(smith,InputKey::Enter);before.unchanged(smith);
    if(mouse)click(smith,186,113);else keyboard(smith,'n');before.unchanged(smith);
    if(mouse)click(smith,20,48);else keyboard(smith,'4');
    if(mouse)click(smith,130,113);else keyboard(smith,'y');armorPaid(smith);
    check(XeenPurchaseTestAccess::browse(*smith.flow),"Buy retained result screen");
    if(mouse)click(smith,251,110);else keyboard(smith,'f');
    if(mouse)click(smith,20,21);else keyboard(smith,'1');
    if(mouse)click(smith,130,113);else keyboard(smith,'y');
    check(smith.p.monsterTreasure->gold==650 && !smith.p.roster.at(0).armor[0].state,"Fix did not apply inherited twenty-gold repair");
    std::string notice;smith.flow->reportText=[&](const auto &text){notice=text;};
    keyboard(smith,'1');check(notice==xeenDialogText(XeenDialogText::ItemNotBroken),"intact Fix lacks original refusal");keyboard(smith,InputKey::Enter);
    keyboard(smith,'b');keyboard(smith,'m');const auto paid=Owners(smith);
    keyboard(smith,'1');check(notice.find("not supported yet")!=std::string::npos,"Misc Buy lacks refusal");paid.unchanged(smith);keyboard(smith,InputKey::Enter);
    smith.leave();check(smith.p.encounterContext->day==9,"Buy/Fix visit changed departure charge");
    training_test::Fixture train(in,in.service());train.enter();
    if(mouse)click(train,46,151);else keyboard(train,InputKey::F1+1);
    if(mouse)click(train,243,109);else keyboard(train,'t');train.prepare();
    check(train.p.roster.at(18).permanentLevel==4 && train.p.monsterTreasure->gold==710 && train.p.encounterContext->day==9 && XeenTrainingTestAccess::menu(*train.flow),"one-step Training differs");
    if(mouse)click(train,282,109);else keyboard(train,InputKey::Escape);check(train.flow->canSave() && train.p.encounterContext->day==10,"Training departure differs");
    auto templeSource=in.service();templeSource.camera={28,15,28,XeenDirection::North};templeSource.journey->treasure->gold=810;
    templeSource.characters[6].currentHp=-15;templeSource.characters[6].conditions[12]=templeSource.characters[6].conditions[13]=1;
    templeSource.characters[1].currentHp=0;templeSource.characters[1].conditions[12]=1;
    Fixture temple(in,templeSource);temple.flow->drawTempleArt=[&](auto &image){in.assets.drawTemple(image);};temple.act(InteractionAction{});temple.prepare();
    std::string message;temple.flow->reportText=[&](const auto &text){message=text;};
    const auto unhealed=Owners(temple);keyboard(temple,'d');check(message.find("not supported yet")!=std::string::npos,"Donation missing refusal");unhealed.unchanged(temple);keyboard(temple,InputKey::Enter);
    for(unsigned member:{5u,4u}) {
        if(mouse)click(temple,member==5?190:154,151);else keyboard(temple,InputKey::F1+member);
        if(mouse)click(temple,235,55);else keyboard(temple,'h');temple.prepare();
        check(XeenTrainingTestAccess::templeLobby(*temple.flow),"Heal retained result screen");
    }
    check(temple.p.monsterTreasure->gold==340 && temple.p.roster.at(6).currentHp==15 && !temple.p.roster.at(6).conditions[13] && temple.p.roster.at(1).currentHp==21,"one-step Heal/resurrection differs");
    if(mouse)click(temple,262,109);else keyboard(temple,InputKey::Escape);
    check(temple.flow->canSave() && temple.p.encounterContext->day==10,"paid Temple departure differs");
}
}
int main(int argc,char **argv){try{
    check(argc==2,"usage: service-dialogs <installation>");const auto installation=XeenInstallationDetector().detect(argv[1]);check(bool(installation),"installation unavailable");Inputs in(*installation);
    layout(in);refusals(in);dialogs(in,false);dialogs(in,true);std::cout<<"Original service layouts, hit actions, Buy/Fix, one-step Training and Heal passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

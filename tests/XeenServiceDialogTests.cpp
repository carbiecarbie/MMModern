#include "XeenTestInstallation.h"
#include "XeenPurchaseTestSupport.h"
#include "formats/xeen/XeenVocDuration.h"
#include <fstream>
#include <iostream>
using namespace purchase_test;
namespace {
void preview(Inputs &in,const IndexedFrame &image,const std::string &name) {
    if(const auto *path=std::getenv("MMODERN_SERVICE_PREVIEW")) {
        std::ofstream out(std::string(path)+"/"+name+".ppm",std::ios::binary);out<<"P6\n320 200\n255\n";
        const auto palette=in.assets.readArchiveResource("mm4.pal");
        for(auto pixel:image.pixels)for(unsigned channel=0;channel<3;++channel){const unsigned char c=palette[pixel*3+channel]*4;out.write(reinterpret_cast<const char *>(&c),1);}
    }
}
void click(training_test::Fixture &f,int x,int y) {
    const auto context=f.flow->inputContext(f.flow->frame().presentation());
    check(!context.acceptsQueuedInput && context.dialog && context.readyForAction,"service is not a strict actionable dialog");
    const auto action=context.dialog->click(x,y);check(bool(action),"original service hit absent");f.act(*action);
}
void keyboard(training_test::Fixture &f,unsigned key){f.act(DialogKeyAction{key});}
void layout(Inputs &in) {
    Fixture f(in,service(in));auto base=training_test::frame();base.pixels.assign(64000,77);
    XeenTextRenderOptions border;border.originalControls=border.drawWindow=true;
    border.windowBounds={0,0,176,136};border.bounds={8,8,168,128};border.x=8;border.y=8;
    const auto referenceCorner=XeenTextRenderer(in.font).render(base,"",border).pages.front();
    for(const auto bounds:{XeenTextRect{65,23,250,163},XeenTextRect{27,6,207,142}}) {
        border.windowBounds=bounds;border.bounds={bounds.left+8,bounds.top+8,bounds.right-8,bounds.bottom-8};border.x=border.bounds.left;border.y=border.bounds.top;
        const auto window=XeenTextRenderer(in.font).render(base,"",border).pages.front();
        for(bool bottom:{false,true})for(int y=0;y<8;++y)for(int x=0;x<8;++x) {
            const auto color=referenceCorner.pixels[(bottom?128+y:y)*320+168+x];
            if(color!=77)check(window.pixels[(bottom?bounds.bottom-8+y:bounds.top+y)*320+bounds.right-8+x]==color,"window edge overwrote original right corner");
        }
    }
    const auto draw=f.flow->drawDialogSprite;
    for(const auto location:{XeenLocationDialog::Smith,XeenLocationDialog::Training,XeenLocationDialog::Temple,XeenLocationDialog::Tavern,XeenLocationDialog::Bank,XeenLocationDialog::Guild}) {
        auto art=base;
        if(location==XeenLocationDialog::Smith)in.assets.drawSmith(art);
        else if(location==XeenLocationDialog::Training)in.assets.drawTraining(art);
        else if(location==XeenLocationDialog::Temple)in.assets.drawTemple(art);
        else if(location==XeenLocationDialog::Tavern)in.assets.drawTavern(art);
        else if(location==XeenLocationDialog::Guild)in.assets.drawGuild(art);
        else in.assets.drawBank(art);
        for(unsigned member=0;member<6;++member) {
            auto rendered=drawXeenLocation(in.assets.uiText(),base,art,in.font,location,f.p,member,XeenMapIdentity(28),draw);
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
                check(drawXeenBuy(in.assets.uiText(),rendered,in.font,loadXeenItemCatalog(in.assets).catalog,f.p,selection,repair,draw).isValid(),"original Buy/Fix template overflow");
            }
        }
    }
    const auto &text=in.assets.uiText();
    const auto offer=in.assets.tavernFoodOffer();check(!offer.empty(),"missing original Tavern offer");
    check(drawXeenFoodOffer(base,in.font,offer,draw).isValid(),"original Food offer layout");
    check(drawXeenErrorScroll(base,in.font,std::string(text.scalar("FOOD_PACKS_FULL"))).isValid(),"full packs layout");
    for(bool withdrawal:{false,true}) {
        f.p.serviceEconomy->bank.gold=1234567;f.p.serviceEconomy->bank.gems=4294967295u;
        auto bank=drawXeenBankCurrency(text,base,in.font,f.p,withdrawal,draw);
        check(bank.isValid(),"original Bank currency layout");
        check(drawXeenBankCurrency(text,base,in.font,f.p,withdrawal,draw,true).isValid(),"original Bank refreshed currency layout");
        check(!xeenBankText(text,f.p,withdrawal,true).empty(),"Bank refresh template");
        for(const auto digits:{"","0","4294967295","4294967296","9999999999"})
            for(char cursor:{' ','|','~',char(127)})
                check(drawXeenBankAmount(text,bank,in.font,digits,draw,cursor).isValid(),"original ten-digit Bank amount layout");
    }
    const auto parity=[](const DialogInput &input,int x,int y,unsigned key) {
        const auto mouse=input.click(x,y),keyboard=input.key(key);
        check(mouse && keyboard && std::get<DialogKeyAction>(*mouse).key==std::get<DialogKeyAction>(*keyboard).key,"original service mouse/key mismatch");
    };
    const auto guild=xeenLocationInput(XeenLocationDialog::Guild);
    parity(guild,235,65,'b');parity(guild,235,75,'s');parity(guild,262,109,InputKey::Escape);parity(guild,46,151,InputKey::F1+1);
    const auto tavern=xeenLocationInput(XeenLocationDialog::Tavern),bank=xeenLocationInput(XeenLocationDialog::Bank);
    for(unsigned i=0;i<4;++i)parity(tavern,235,55+i*10,std::string("dftr")[i]);
    parity(tavern,243,109,'s');parity(tavern,282,109,InputKey::Escape);
    check(!bank.key(InputKey::F1+1) && !bank.click(46,151),"Bank unexpectedly selects party members");
    parity(tavern,46,151,InputKey::F1+1);
    parity(bank,235,109,'d');parity(bank,262,109,'w');parity(bank,289,109,InputKey::Escape);
    parity(xeenBankInput(),235,109,'o');parity(xeenBankInput(),262,109,'e');parity(xeenBankInput(),289,109,InputKey::Escape);
    parity(xeenFoodConfirmInput(),236,76,'y');parity(xeenFoodConfirmInput(),261,76,'n');
    for(unsigned i=0;i<10;++i)parity(xeenSpellListInput(true),40,28+9*i,i==9?'0':'1'+i);
    parity(xeenSpellListInput(true),187,26,InputKey::Up);parity(xeenSpellListInput(true),187,111,InputKey::Down);
    parity(xeenSpellListInput(true),187,50,InputKey::PageUp);parity(xeenSpellListInput(true),187,90,InputKey::PageDown);
    parity(xeenSpellListInput(true),132,123,'s');parity(xeenSpellListInput(true),174,123,InputKey::Escape);
    parity(xeenCastInput(),234,108,'c');parity(xeenCastInput(),261,108,'n');parity(xeenCastInput(),288,108,InputKey::Escape);
    parity(xeenSpellConfirmInput(false),120,133,'y');parity(xeenSpellConfirmInput(false),176,133,'n');
    for(unsigned i=0;i<6;++i)parity(xeenSpellTargetInput(),10+int(i?35*i+(i>1?i-1:0):0),150,InputKey::F1+i);
    check(!xeenCastInput(false).key(InputKey::F1) && !xeenSpellListInput(true,false).click(10,150),"combat caster switching accepted");
    check(drawXeenErrorScroll(base,in.font,xeenNotEnoughGold(in.assets.uiText())).isValid(),"gold refusal layout");
    check(drawXeenConfirm(base,in.font,xeenServiceConfirm(in.assets.uiText(),true,"Plate Armor",200),false,draw).isValid(),"Fix Confirm layout");
}
void originalServiceSequences(Inputs &in) {
    for(unsigned action=0;action<7;++action) {
        const unsigned service=action==6?1:action;
        auto source=in.service(action==6?10:8);source.camera={28,20,20,XeenDirection::North};
        if(action==6)source.journey->random->count=UINT64_MAX-XeenServiceDayCandidate::kDeferredRandomReserve;
        if(action==3)source.food=0;
        if(action==4)source.characters[source.activeRosterIds[0]].currentHp=1;
        XeenWorld query(in.mapLoader());const auto cell=query.sampleCell(28,20,20);check(bool(cell),"Service fixture geometry absent");
        for(unsigned facing=0;facing<4;++facing) {
            const auto wall=wallAt(*cell->cell,XeenDirection(facing));
            if(wall!=1 && wall!=6 && wall!=9 && wall!=13){source.camera.direction=XeenDirection(facing);break;}
        }
        XeenEventFile city{28,"maze0028.evt",true,{}};
        XeenEventRecord record;record.x=20;record.y=20;record.direction=4;record.line=0;
        record.opcode=0x11;record.parameters={std::uint8_t(service)};record.lengthField=6;city.records.push_back(record);
        training_test::Fixture f(in,source,false,&city);
        f.flow->drawSmithArt=[&](auto &image){in.assets.drawSmith(image);};
        f.flow->drawBankArt=[&](auto &image){in.assets.drawBank(image);};
        f.flow->drawGuildArt=[&](auto &image){in.assets.drawGuild(image);};
        f.flow->drawTavernArt=[&](auto &image){in.assets.drawTavern(image);};
        f.flow->tavernFoodOffer=[&]{return in.assets.tavernFoodOffer();};
        f.flow->drawTempleArt=[&](auto &image){in.assets.drawTemple(image);};
        std::vector<std::string> samples;
        f.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){
            check(archive==XeenSceneArchive::Clouds,"Shop sample left Clouds archive");samples.emplace_back(name);
            return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive));
        };
        f.act(InteractionAction{});f.prepare();
        const auto kind=static_cast<XeenServiceArt>(service);
        check(samples==std::vector<std::string>{XeenServiceAnimation::greeting(kind)},"Original service greeting sample differs");
        const auto before=*f.w.sessionState().journeyRandom();
        auto advance=[&] {f.flow->beginCycle(++f.cycle);return f.flow->updatePresentation();};
        f.now=150;check(!advance(),"Shop drew before the three-tick wait");
        std::vector<XeenServiceSprite> sprites;const auto draw=f.flow->drawDialogSprite;bool failOnce=action==1;
        f.flow->drawDialogSprite=[&](auto &image,const char *resource,unsigned frame,int x,int y){
            if(std::string(resource).find(".twn")!=std::string::npos) {
                sprites.push_back({resource,frame});
                if(failOnce){failOnce=false;throw std::runtime_error("Synthetic service sprite failure");}
            }
            draw(image,resource,frame,x,y);
        };
        f.now=165;auto next=advance();
        if(action==6) {
            check(!next && *f.w.sessionState().journeyRandom()==before,"Optional speaking spent the mandatory departure RNG suffix");
            f.act(CancelInteractionAction{});f.finishDeparture();
            check(f.flow->canSave() && f.p.encounterContext->day==11,"Speaking capacity fence blocked the reserved departure");
            continue;
        }
        if(action==1) {
            check(!next && *f.w.sessionState().journeyRandom()==before,"Failed service composition published RNG");
            // Expiration between attempts must retain the already chosen speaking result.
            f.now=1500;sprites.clear();next=advance();
        }
        check(bool(next),"Original service pulse did not publish");
        XeenCombatRandom expected(before);std::optional<unsigned> speaking;
        if(const auto range=XeenServiceAnimation::speaking(kind))while(!speaking)speaking=expected.draw(range->first,range->last);
        check(sprites==XeenServiceAnimation::compose(kind,1,speaking),"Service base/replacement/speaking range or retry changed");
        check(*f.w.sessionState().journeyRandom()==expected.continuation(),"Speaking frame did not consume the shared RNG exactly once");
        const auto retained=*f.w.sessionState().journeyRandom();f.now+=1000;
        check(!advance() && *f.w.sessionState().journeyRandom()==retained,"Unacquired service frame advanced RNG/animation");
        f.present(*next);sprites.clear();f.now+=1000;next=advance();check(bool(next),"Silent service cycle did not resume");
        check(sprites==XeenServiceAnimation::compose(kind,2) && *f.w.sessionState().journeyRandom()==retained,
            "Completed sample kept a speaking overlay or lost the Smith logical index");f.present(*next);
        if(action==0) {
            f.act(DialogKeyAction{'d'});f.act(DialogKeyAction{'o'});f.act(TextInputAction{"5"});f.act(DialogKeyAction{InputKey::Enter});
            check(samples.back()=="coina.voc" && f.p.serviceEconomy->bank.gold==5,"Bank action did not replace the active sample");
            f.act(CancelInteractionAction{}); // Currency -> lobby.
        } else if(action==2) {
            check(!xeenVertigoGuildMember(f.p,1),"Guild refusal fixture unexpectedly has membership");
            f.act(SelectMemberAction{1});check(samples.back()=="guild11.voc","Guild refusal did not replace the greeting");
        } else if(action==3) {
            f.act(DialogKeyAction{'f'});f.act(YesAction{});
            check(samples.back()=="thankyou.voc" && f.p.food>0,"Tavern food purchase did not replace the greeting");
        } else if(action==4) {
            f.act(DialogKeyAction{'h'});f.prepare();
            check(samples.back()=="ahh.voc" && f.p.roster.at(source.activeRosterIds[0]).currentHp>1,"Temple action did not replace the sample after healing");
        } else if(action==5) {
            check(xeenQuoteTraining(f.p.roster.at(source.activeRosterIds[0]),*f.p.roster.combatInputs(source.activeRosterIds[0]),
                f.p.monsterTreasure->gold,*f.p.encounterContext).outcome==XeenTrainingOutcome::MissingExperience,"Training refusal fixture XP differs");
            f.act(DialogKeyAction{'t'});check(samples.back()=="trainin0.voc","Training experience refusal did not replace greeting");
            f.act(SelectMemberAction{1});f.act(DialogKeyAction{'t'});f.prepare();
            check(samples.back()=="trainin2.voc" && f.p.roster.at(18).permanentLevel==4,"Successful Training sample/recovery differs");
        }
        if(action==3) {
            const auto last=f.flow->frame().pixels;const auto start=f.now;f.act(CancelInteractionAction{});
            check(samples.back()=="goodbye.voc" && !f.flow->canSave() && f.flow->frame().pixels==last,"Tavern did not hold its last screen");
            f.now=start+1009;check(!advance(),"Tavern goodbye ended before sample completion");
            f.now=start+1010;next=advance();check(next && f.flow->frame().pixels!=last,"Tavern goodbye did not return after completion");f.present(*next);
            check(f.flow->canSave(),"Tavern goodbye charged/settled incorrectly");
        } else {
            f.act(CancelInteractionAction{});f.finishDeparture();
            check(f.flow->canSave(),"Service departure did not settle");
        }
    }
    auto source=in.service();source.characters[source.activeRosterIds[0]].permanentLevel=10;
    training_test::Fixture capped(in,source);std::string sample;
    capped.flow->loadSampleDuration=[&](const char *name,XeenSceneArchive archive){sample=name;return xeenVocDurationNanoseconds(in.assets.readArchiveResource(name,archive));};
    capped.enter();capped.act(DialogKeyAction{'t'});check(sample=="trainin1.voc","Training level-cap refusal sample differs");
    capped.act(CancelInteractionAction{});check(capped.now==0 && capped.flow->canSave(),"Active service sample made Escape uninterruptible");
}

void refusals(Inputs &in) {
    for(bool temple:{false,true}) {
        auto source=in.service();source.journey->treasure->gold=0;
        if(temple) {source.camera={28,15,28,XeenDirection::North};source.characters[6].currentHp=-15;
            source.characters[6].conditions[13]=1;source.characters[6].armor[0].state|=64;}
        Fixture f(in,source);f.flow->drawTempleArt=[&](auto &image){in.assets.drawTemple(image);};
        f.act(InteractionAction{});f.prepare();keyboard(f,InputKey::F1+(temple?5:1));
        const Owners before(f);std::string notice;f.flow->reportText=[&](const auto &text){notice=text;};
        keyboard(f,temple?'h':'t');check(notice==xeenNotEnoughGold(in.assets.uiText()),"one-step service lacks original gold refusal");before.unchanged(f);
        keyboard(f,InputKey::Enter);
        if(temple) {keyboard(f,'u');check(notice==xeenNotEnoughGold(in.assets.uiText()),"Uncurse lacks original gold refusal");before.unchanged(f);keyboard(f,InputKey::Enter);}
        keyboard(f,InputKey::Escape);check(f.flow->canSave() && f.p.encounterContext->day==9,"refused service lost unpaid departure");
    }
    auto source=service(in);source.journey->treasure->gold=0;
    Fixture f(in,source);f.enter();keyboard(f,'b');keyboard(f,'c');const Owners before(f);
    std::string notice;f.flow->reportText=[&](const auto &text){notice=text;};keyboard(f,'1');
    check(XeenPurchaseTestAccess::quote(*f.flow),"Accessories Buy lacks quote");keyboard(f,'y');
    check(notice==xeenNotEnoughGold(in.assets.uiText()),"Accessories Buy lacks gold refusal");before.unchanged(f);
}
void dialogs(Inputs &in,bool mouse) {
    auto source=service(in);source.characters[0].armor[0].state=128;
    Fixture smith(in,source);smith.enter();
    const auto before=Owners(smith);
    keyboard(smith,'r');keyboard(smith,InputKey::Enter);before.unchanged(smith);
    check(XeenPurchaseTestAccess::repairArmor(*smith.flow),"Lobby R did not retain Armor Repair");
    keyboard(smith,InputKey::Escape);before.unchanged(smith);
    if(mouse)click(smith,235,65);else keyboard(smith,'b');
    smith.flow->reportText={};
    if(mouse)click(smith,47,110);else keyboard(smith,'a');
    if(mouse)click(smith,20,21+9*smith.armorOffer());else keyboard(smith,'1'+smith.armorOffer());
    check(XeenPurchaseTestAccess::quote(*smith.flow),"row does not open original Confirm");
    keyboard(smith,InputKey::Enter);before.unchanged(smith);
    if(mouse)click(smith,186,113);else keyboard(smith,'n');before.unchanged(smith);
    if(mouse)click(smith,20,21+9*smith.armorOffer());else keyboard(smith,'1'+smith.armorOffer());
    if(mouse)click(smith,130,113);else keyboard(smith,'y');armorPaid(smith);
    check(XeenPurchaseTestAccess::browse(*smith.flow),"Buy retained result screen");
    if(mouse)click(smith,251,110);else keyboard(smith,'f');
    if(mouse)click(smith,20,21);else keyboard(smith,'1');
    if(mouse)click(smith,130,113);else keyboard(smith,'y');
    check(smith.p.monsterTreasure->gold==650 && !smith.p.roster.at(0).armor[0].state,"Fix did not apply inherited twenty-gold repair");
    std::string notice;smith.flow->reportText=[&](const auto &text){notice=text;};
    keyboard(smith,'1');check(notice==xeenDialogText(in.assets.uiText(),XeenDialogText::ItemNotBroken),"intact Fix lacks original refusal");keyboard(smith,InputKey::Enter);
    keyboard(smith,'b');keyboard(smith,'m');const auto paid=Owners(smith);
    keyboard(smith,'1');check(XeenPurchaseTestAccess::quote(*smith.flow),"Misc Buy lacks quote");paid.unchanged(smith);keyboard(smith,'n');
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
    auto donated=Owners(temple);donated.treasure.gold-=10;keyboard(temple,'d');
    donated.unchanged(temple); // First donation on day8 pays10 without blessing.
    for(unsigned member:{5u,4u}) {
        if(mouse)click(temple,member==5?190:154,151);else keyboard(temple,InputKey::F1+member);
        if(mouse)click(temple,235,55);else keyboard(temple,'h');temple.prepare();
        check(XeenTrainingTestAccess::templeLobby(*temple.flow),"Heal retained result screen");
    }
    // DOS unsigned HP comparison: Seymour at HP -15 pays 380, Rebecca 60.
    check(temple.p.monsterTreasure->gold==360 && temple.p.roster.at(6).currentHp==15 && !temple.p.roster.at(6).conditions[13] && temple.p.roster.at(1).currentHp==21,"one-step Donation/Heal/resurrection differs");
    if(mouse)click(temple,262,109);else keyboard(temple,InputKey::Escape);
    temple.finishDeparture();
    check(temple.flow->canSave() && temple.p.encounterContext->day==11,"two paid Temple actions require three farewell days");
}
void spellDialogs(Inputs &in,bool mouse) {
    const auto &text=in.assets.uiText();
    const auto descriptions=xeenSpellDescriptions(in.assets.readArchiveResource("spldesc.bin"));
    check(descriptions.size()>=76,"description catalogue absent");
    for(unsigned id=0;id<76;++id) {
        const auto info=drawXeenSpellConfirm(training_test::frame(),in.font,xeenDialogFormat(text.scalar("SPELL_INFO"),{in.names.names[id],descriptions[id]}),true,{});
        check(info.isValid(),"Info layout");
        if(id==1 || id==20 || id==75)preview(in,info,"spell-info-"+std::to_string(id));
        check(drawXeenSpellConfirm(training_test::frame(),in.font,xeenDialogFormat(text.scalar("SPELL_PURCHASE"),{descriptions[id],in.names.names[id],"500"}),false,{}).isValid(),"Purchase layout");
    }
    const auto choose=[&](training_test::Fixture &f,unsigned key) {
        if(!mouse){keyboard(f,key);return;}
        const auto ctx=f.flow->inputContext(f.flow->frame().presentation());
        check(ctx.dialog && !ctx.acceptsQueuedInput,"spell dialog must accept fresh input only");
        for(const auto &hit:ctx.dialog->hits)if(hit.key==key){click(f,hit.left,hit.top);return;}
        keyboard(f,key); // acknowledgement-only popups have no button regions.
    };
    for(unsigned book=0;book<3;++book) {
        auto source=in.service();source.camera={28,20,13,XeenDirection::North};
        source.journey->treasure->gold=20000;
        const auto owner=source.activeRosterIds[0];auto &c=source.characters[owner];
        c.characterClass=book==0?XeenCharacterClass::Cleric:book==1?XeenCharacterClass::Sorcerer:XeenCharacterClass::Druid;
        c.hasSpells=true;c.learnedSpells.emplace();c.awards[0]=1;
        c.weapons={};c.armor={};c.accessories={};c.miscellaneous={};
        Fixture f(in,source);f.flow->drawGuildArt=[&in](auto &image){in.assets.drawGuild(image);};
        check(drawXeenSpellList(text,training_test::frame(),in.font,f.p.roster.at(owner),in.names,{},20000,true,true,f.flow->drawDialogSprite,f.p.encounterContext->year).isValid(),"Guild list layout");
        f.act(InteractionAction{});f.prepare();
        const auto before=Owners(f);choose(f,'s');preview(in,f.flow->frame(),"guild-info-list-"+std::to_string(book));
        const auto offers=XeenLearnedSpellRules::rows(f.p.roster.at(owner),true,true);
        for(std::size_t row=0;row<std::min<std::size_t>(10,offers.size());++row) {
            const auto id=*XeenLearnedSpellRules::spellForSlot(static_cast<XeenSpellCategory>(book),offers[row]);
            const auto &name=in.names.names[id];const int x=35+(row?10:11),y=29+9*row;
            const int right=x+XeenTextRenderer(in.font).textWidth(name,XeenFontSize::Reduced)+5;
            auto clean=f.flow->frame();for(int yy=y;yy<y+9;++yy)for(int xx=x;xx<right;++xx)clean.pixels[yy*320+xx]=0x99;
            XeenTextRenderOptions rowText;rowText.originalControls=true;rowText.size=XeenFontSize::Reduced;rowText.colorIndex=9;
            rowText.bounds={35,14,187,134};rowText.x=x;rowText.y=y;
            const auto expected=XeenTextRenderer(in.font).render(clean,name,rowText).pages.front();
            for(int yy=y;yy<y+9;++yy)for(int xx=x;xx<right;++xx)
                check(expected.pixels[yy*320+xx]==f.flow->frame().pixels[yy*320+xx],"Guild list lost original reduced font/row spacing");
        }
        choose(f,'1');
        preview(in,f.flow->frame(),"guild-info-dialog-"+std::to_string(book));
        check(XeenPurchaseTestAccess::quote(*f.flow),"Guild Info did not immediately open description");
        check(XeenPurchaseTestAccess::text(*f.flow).find(in.names.names[1])!=std::string::npos,"Info missing resource name");
        choose(f,'y');before.unchanged(f);choose(f,InputKey::Escape);
        choose(f,'b');choose(f,'1');
        const auto old=f.flow->frame().presentation();const auto epoch=*f.flow->displayedInput();
        choose(f,'n');before.unchanged(f);choose(f,'1');
        f.flow->beginCycle(++f.cycle);f.present(f.flow->handle(DialogKeyAction{'y'},epoch,old));before.unchanged(f);
        bool fault=false;f.flow->smithBoundary=[&](auto boundary){if(boundary==XeenSmithBoundary::BeforePurchase && !fault){fault=true;throw std::bad_alloc();}};
        choose(f,'y');check(fault,"Guild pre-publication fault absent");before.unchanged(f);f.flow->smithBoundary={};
        choose(f,InputKey::Enter); // dismiss retry notice
        choose(f,'1');choose(f,'y');
        const auto slot=*XeenLearnedSpellRules::slotForSpell(static_cast<XeenSpellCategory>(book),1);
        check(f.p.roster.at(owner).learnedSpells->at(slot)==1 && f.p.monsterTreasure->gold==19900,"Guild buy did not debit and learn once");
        check(*f.p.encounterContext==before.context && *f.w.sessionState().journeyRandom()==before.random,"Guild purchase changed clock/RNG");
        const auto paid=Owners(f);f.flow->beginCycle(++f.cycle);f.present(f.flow->handle(DialogKeyAction{'y'},epoch,old));paid.unchanged(f);
        const auto remaining=XeenLearnedSpellRules::rows(f.p.roster.at(owner),true);
        const auto unsupported=std::find_if(remaining.begin(),remaining.end(),[&](auto slot){return !XeenLearnedSpellRules::supported(*XeenLearnedSpellRules::spellForSlot(static_cast<XeenSpellCategory>(book),slot));});
        check(unsupported!=remaining.end() && unsupported-remaining.begin()<10,"unsupported purchase fixture missing from first page");
        const auto nextSlot=*unsupported;
        const auto nextId=*XeenLearnedSpellRules::spellForSlot(static_cast<XeenSpellCategory>(book),nextSlot);
        check(!XeenLearnedSpellRules::supported(nextId),"unsupported purchase fixture");
        choose(f,'1'+unsigned(unsupported-remaining.begin()));choose(f,'y');
        check(f.p.roster.at(owner).learnedSpells->at(nextSlot)==1 && f.p.monsterTreasure->gold==19900-XeenLearnedSpellRules::purchasePrice(nextId,c.characterClass),"unsupported spell was not purchasable");
        choose(f,InputKey::F1+1);check(XeenPurchaseTestAccess::lobby(*f.flow),"Guild did not refresh selected character membership");
        f.leave();check(f.p.encounterContext->day==9,"Guild departed with wrong charge");
        Fixture restored(in,f.snapshot());check(XeenSaveFormat::encode(restored.snapshot())==XeenSaveFormat::encode(f.snapshot()),"Purchased book restore is not exact");
    }
    for(unsigned kind=0;kind<7;++kind) {
        auto source=in.service();source.camera={28,20,13,XeenDirection::North};source.journey->treasure->gold=1000;
        const auto owner=source.activeRosterIds[0];auto &c=source.characters[owner];
        c.characterClass=XeenCharacterClass::Cleric;c.hasSpells=true;c.learnedSpells.emplace();c.awards[0]=1;
        c.weapons={};c.armor={};c.accessories={};c.miscellaneous={};
        if(kind==0)source.journey->treasure->gold=99;
        if(kind==1)c.learnedSpells->fill(1);
        if(kind==2){c.characterClass=XeenCharacterClass::Knight;c.hasSpells=false;}
        if(kind==3)c.conditions[8]=1;
        if(kind==4){c.characterClass=XeenCharacterClass::Paladin;c.permanentLevel=1;c.temporaryLevel=0;c.personality={1,0};c.skills[12]=0;}
        Fixture f(in,source);f.flow->drawGuildArt=[&in](auto &image){in.assets.drawGuild(image);};f.act(InteractionAction{});f.prepare();
        const auto before=Owners(f);
        if(kind==5){f.flow->loadSpellDescriptions=[]{throw std::bad_alloc();return std::vector<std::uint8_t>{};};}
        choose(f,'b');
        if(kind==3 || kind==5) {
            check(XeenPurchaseTestAccess::lobby(*f.flow),"disabled/preparation failure entered Buy");before.unchanged(f);choose(f,InputKey::Enter);
            if(kind==3){choose(f,'s');check(XeenPurchaseTestAccess::browse(*f.flow),"disabled member cannot use Info");choose(f,InputKey::Escape);}
        } else if(kind==1 || kind==2) {
            choose(f,'1');check(XeenPurchaseTestAccess::browse(*f.flow),"empty Guild list selected an offer");before.unchanged(f);choose(f,InputKey::Escape);
        } else {
            if(kind==4)check(XeenCharacterRules::maxSp(f.p.roster.at(owner),{f.p.encounterContext->year})==0,"hybrid zero maximum SP fixture");
            choose(f,'1');check(XeenPurchaseTestAccess::quote(*f.flow),"Guild offer missing");
            bool published=false;
            if(kind==6)f.flow->smithBoundary=[&](auto boundary){if(boundary==XeenSmithBoundary::AfterPurchase){published=true;throw std::bad_alloc();}};
            choose(f,'y');
            if(kind==0){before.unchanged(f);check(XeenPurchaseTestAccess::text(*f.flow)==xeenNotEnoughGold(text),"Guild lacks gold refusal");choose(f,InputKey::Enter);}
            else {
                check(f.p.roster.at(owner).learnedSpells->at(1)==1 && f.p.monsterTreasure->gold==(kind==4?800:900),"zero-SP/published Guild purchase differs");
                if(kind==6){check(published,"post-publication fault absent");const auto paid=Owners(f);f.flow->smithBoundary={};choose(f,InputKey::Enter);choose(f,'y');paid.unchanged(f);}
            }
            choose(f,InputKey::Escape);
        }
        f.leave();check(f.flow->canSave(),"Guild boundary case retained Save exclusion");
        if(kind==4) {
            const auto saved=Owners(f);f.act(CastSpellAction{});choose(f,InputKey::F1);choose(f,'n');choose(f,'1');choose(f,'s');
            saved.unchanged(f);choose(f,InputKey::Escape);choose(f,InputKey::Escape);check(f.flow->canSave(),"zero-SP New manufactured a selection");
        }
    }
    auto source=in.service();source.camera={28,10,10,XeenDirection::North};
    for(auto &a:source.map(28).monsters->slots){a.x=a.y=-128;a.lifecycle=XeenActorLifecycle::Defeated;a.accounted=true;}
    const auto owner=source.activeRosterIds[4];auto &caster=source.characters[owner];
    caster.characterClass=XeenCharacterClass::Cleric;caster.hasSpells=true;caster.currentSp=30;caster.learnedSpells.emplace();
    for(auto &flag:*caster.learnedSpells)flag=1;caster.currentSpell=255;
    Fixture cast(in,source);const auto initial=Owners(cast);
    cast.act(CastSpellAction{});check(!cast.flow->canSave(),"Cast permits Save");
    choose(cast,InputKey::F1+4);choose(cast,'n');choose(cast,InputKey::PageDown);choose(cast,InputKey::Up);
    choose(cast,InputKey::F1+5);choose(cast,InputKey::F1+4);initial.unchanged(cast);
    choose(cast,InputKey::Escape);initial.unchanged(cast);
    choose(cast,'n');choose(cast,'2');choose(cast,'s');
    check(cast.p.roster.at(owner).currentSpell==1,"New did not store book slot");
    const auto oldFrame=cast.flow->frame().presentation();const auto oldInput=*cast.flow->displayedInput();
    const auto selected=Owners(cast);choose(cast,InputKey::Escape);selected.unchanged(cast);
    cast.act(CastSpellAction{});choose(cast,'n');choose(cast,InputKey::PageDown);choose(cast,'5');choose(cast,'s');
    check(cast.p.roster.at(owner).currentSpell==14,"New scroll/row/select differs");
    cast.flow->beginCycle(++cast.cycle);cast.present(cast.flow->handle(DialogKeyAction{'c'},oldInput,oldFrame));
    check(cast.p.roster.at(owner).currentSp==30 && !cast.flow->encounter()->castingCommitted(),"stale Cast frame committed cost");
    choose(cast,'c');check(cast.p.roster.at(owner).currentSp==29,"First Aid debit differs");choose(cast,InputKey::Escape);
    for(unsigned i=0;i<1000 && !cast.flow->canSave();++i){cast.now+=100;cast.flow->beginCycle(++cast.cycle);if(auto frame=cast.flow->updatePresentation())cast.present(*frame);}
    check(cast.flow->canSave() && cast.p.roster.at(owner).currentSp==30 && cast.p.encounterContext->minutes==source.journey->context->minutes+1,"Indoor target cancellation refund/time differs");
    cast.act(CastSpellAction{});choose(cast,'n');choose(cast,'3');choose(cast,'s'); // unsupported clerical Bless
    const auto unsupported=Owners(cast);choose(cast,'c');unsupported.unchanged(cast);
    choose(cast,InputKey::Enter);choose(cast,InputKey::Escape);unsupported.unchanged(cast);
    auto terminal=source;terminal.characters[terminal.activeRosterIds[0]].currentHp=0;terminal.characters[terminal.activeRosterIds[0]].conditions[13]=1;
    Fixture failure(in,terminal);failure.act(CastSpellAction{});choose(failure,InputKey::F1+4);choose(failure,'n');choose(failure,InputKey::PageDown);choose(failure,'5');choose(failure,'s');choose(failure,'c');choose(failure,InputKey::F1);
    check(failure.flow->inputContext(failure.flow->frame().presentation()).dialog->anyKey,"failed First Aid lacks original modal");
    for(unsigned i=0;i<3;++i){failure.now+=100;failure.flow->beginCycle(++failure.cycle);check(!failure.flow->updatePresentation(),"failed cast settled before acknowledgement");}
    check(failure.p.roster.at(owner).currentSp==29 && failure.p.encounterContext->minutes==terminal.journey->context->minutes && !failure.flow->canSave(),"failed First Aid cost/modal boundary differs");
    choose(failure,'n'); // any key acknowledges, including another Cast command.
    for(unsigned i=0;i<1000 && !failure.flow->canSave();++i){failure.now+=100;failure.flow->beginCycle(++failure.cycle);if(auto frame=failure.flow->updatePresentation())failure.present(*frame);}
    check(failure.flow->canSave() && failure.p.encounterContext->minutes==terminal.journey->context->minutes+1 && failure.p.roster.at(owner).currentSp==29,"failed First Aid lost its paid time/action");
    check(failure.flow->encounter()->notice().find("SP spent")==std::string::npos,"technical casting result leaked into player notice");
    check(text.scalar("SPELLS_DIALOG_SPELLS").find("%lu")!=std::string_view::npos,"DOS list lost unsigned long balance");
}

}
int main(int argc,char **argv){try{
    check(argc==2,"usage: service-dialogs <installation>");const auto installation=xeenTestInstallationDetector().detect(argv[1]);check(bool(installation),"installation unavailable");Inputs in(*installation);
    layout(in);originalServiceSequences(in);refusals(in);dialogs(in,false);dialogs(in,true);spellDialogs(in,false);spellDialogs(in,true);std::cout<<"Original service layouts, hit actions, Buy/Fix, one-step Training and Heal passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

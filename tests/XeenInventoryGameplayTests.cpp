#include "XeenSaveGameplayTestSupport.h"
#include "XeenChildProcessTestSupport.h"
#include "XeenCheckpointTestSupport.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include <iostream>
using namespace gameplay_test;
namespace fs=std::filesystem;
using Mode=XeenInventoryMode;
using Status=XeenTransferStatus;
namespace mmodern {
struct XeenInventoryTestAccess {
	static void exhaust(XeenEventFlow &flow) { flow._inventoryEpoch=std::numeric_limits<std::uint64_t>::max()-2; }
};
}
namespace {
const XeenCamera start{1,1,1,XeenDirection::North};
struct Quiet { std::ostringstream out;std::streambuf *old=std::cout.rdbuf(out.rdbuf());~Quiet(){std::cout.rdbuf(old);} };
void seed(Fixture &f) {
	f.initial.roster.at(0).name="gyp%\x01\xff";
	f.initial.roster.at(1).name="Receiver";
	for(auto *items:{&f.initial.roster.at(0).weapons,&f.initial.roster.at(0).armor,&f.initial.roster.at(0).accessories,&f.initial.roster.at(0).miscellaneous})
		*items={{{10,37,1,0},{10,37,1,0},{},{},{},{},{},{},{}}};
}
void arm(const SdlWindow::FrameUpdateHandler &handle,unsigned destination=1) {
	handle(SelectInventorySlotAction{0});handle(TransferInventoryAction{});handle(SelectMemberAction{destination});
}
void gameplay() {
	Fixture f;seed(f);f.ordinary=true;auto services=f.services();std::uint64_t now=99;services.clock=[&]{return now;};
	const XeenPartyState *live=nullptr;services.observeGameplay=[&](auto &w,auto &,const auto &p,auto &,const auto &){f.world=&w;live=&p;};
	services.show=[&](const auto &,const auto &handle,const auto &escape,const auto &idle,const auto &) {
		const auto before=remove_test::partySnapshot(*live);const auto phase=f.phases.back();
		handle(TransferInventoryAction{});handle(SelectInventorySlotAction{0});check(!f.flow->blocksGameplay(),"closed inventory-only dispatch");
		handle(InspectInventoryAction{});check(escape()&&f.flow->inventoryOpen(),"I did not open");
		handle(NavigationAction::MoveForward);check(f.flow->inventorySelection().slot==8,"up from none");
		handle(NavigationAction::MoveBackward);check(f.flow->inventorySelection().slot==0,"slot wrapping");
		handle(SelectMemberAction{5});check(f.flow->inventorySelection().source==0,"invalid source indexed");
		for(auto a:std::vector<PlayerAction>{InteractionAction{},YesAction{},NoAction{},AcknowledgeAction{}})handle(a);
		check(remove_test::partySnapshot(*live)==before,"browse mutated");
		arm(handle);const auto token=f.flow->inventoryConfirmation();check(token.has_value(),"confirmation unarmed");
		const auto count=f.compositions;handle(SaveGameAction{});
		check(f.compositions==count&&f.flow->inventoryConfirmation()==token&&f.phases.back()==phase,"F9 changed underlay/token/time");
		now=200;idle();check(f.flow->inventoryConfirmation()==token&&f.phases.back()==phase+1,"timed rebase invalidated token");
		f.flow->initial();check(!f.flow->respond(1,XeenPresentationResponse::Acknowledged),"inventory accepted event response");
		save_test::rejects([&]{f.flow->acceptManual(XeenManualEventNoEvent{});});
		for(auto a:std::vector<PlayerAction>{InspectInventoryAction{},TransferInventoryAction{},SelectInventorySlotAction{1},NavigationAction::TurnRight})handle(a);
		check(f.flow->inventorySelection().slot==0&&f.flow->inventoryConfirmation()==token,"frozen source changed");
		handle(SelectMemberAction{5});check(!f.flow->inventoryConfirmation()&&f.flow->inventorySelection().mode==Mode::ChooseDestination,"invalid F-key kept destination");
		handle(AcknowledgeAction{});check(remove_test::partySnapshot(*live)==before,"Enter used obsolete destination");
		handle(SelectMemberAction{1});check(f.flow->inventoryConfirmation()!=token,"generation reused");
		handle(NoAction{});check(f.flow->inventorySelection().mode==Mode::Browse&&remove_test::partySnapshot(*live)==before,"N cancellation");
		arm(handle,0);handle(AcknowledgeAction{});check(f.flow->transferResult().status==Status::SameOwner&&remove_test::partySnapshot(*live)==before,"self refusal");
		arm(handle);f.flow->refresh(true);check(!f.flow->inventoryConfirmation()&&f.flow->inventorySelection().slot==0,"reconstruction invalidation");
		handle(AcknowledgeAction{});check(remove_test::partySnapshot(*live)==before,"reconstruction moved item");
		arm(handle);f.flow->invalidateInventory();handle(AcknowledgeAction{});check(remove_test::partySnapshot(*live)==before,"explicit ABA notification ignored");
		arm(handle);bool called=false;
		f.flow->reportInventory=[&](const auto &r){called=true;check(r.status==Status::Success&&!f.flow->inventoryConfirmation(),"report preceded fixed result");handle(AcknowledgeAction{});handle(SaveGameAction{});idle();f.flow->handle(InspectInventoryAction{});};
		handle(AcknowledgeAction{});check(called&&f.flow->inventorySelection().mode==Mode::Browse&&!f.flow->inventorySelection().slot,"success state");
		check(live->roster.at(0).weapons[0].id==37&&live->roster.at(0).weapons[1].id==0&&live->roster.at(1).weapons[0].id==37,"exact one quantity");
		const auto after=remove_test::partySnapshot(*live);handle(AcknowledgeAction{});handle(AcknowledgeAction{});check(remove_test::partySnapshot(*live)==after,"repeated Enter replay");
		handle(CancelInteractionAction{});check(!escape()&&!f.flow->inventoryOpen(),"browse Escape");
		check(f.flow->frame().pixels==f.frames.back().pixels,"close leaked panel");
		return true;
	};
	Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"inventory gameplay failed");
}
void failure() {
	for(bool persistent:{false,true}) {
		Fixture f;seed(f);auto services=f.services();const XeenPartyState *live=nullptr;
		unsigned recoveryAttempts=0;
		auto compose=services.compose;
		services.compose=[&](auto &w,const auto &p,const auto &c,std::uint64_t phase){
			if(f.failCompose) {
				++recoveryAttempts;f.failCompose=false;
				throw std::runtime_error("clean base recovery failed once; must be fatal without retry");
			}
			return compose(w,p,c,phase);
		};
		services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){live=&p;};
		services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &status){
			handle(InspectInventoryAction{});arm(handle);
			f.flow->reportInventory=[&](const auto &){f.failCompose=persistent;throw std::runtime_error("post-publication reporting failure");};
			bool threw=false;try{handle(AcknowledgeAction{});}catch(...){threw=true;}
			check(threw==persistent&&!f.flow->inventoryOpen()&&recoveryAttempts==(persistent?1U:0U),"inventory recovery boundary");
			check(live->roster.at(0).weapons[1].id==0&&live->roster.at(1).weapons[0].id==37,"publication undone/replayed");
			if(persistent){handle(SaveGameAction{});check(status().find("idle gameplay boundary")!=std::string::npos,"fatal save allowed");}
			else{handle(InspectInventoryAction{});check(f.flow->inventoryOpen(),"recoverable reopen");handle(AcknowledgeAction{});check(live->roster.at(1).weapons[1].id==0,"reopen replay");}
			return !persistent;
		};Quiet quiet;check(Application().playGameplay(services,start,{},false)==(persistent?4:0),"failure exit");
	}
}
void invalidation() {
	Fixture f;seed(f);auto services=f.services();XeenPartyState *live=nullptr;
	services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){live=&const_cast<XeenPartyState &>(p);};
	services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
		handle(InspectInventoryAction{});arm(handle);const auto token=f.flow->inventoryConfirmation();
		const auto replacement=live->roster.at(0);live->roster.at(0)=replacement;
		f.flow->invalidateInventory();handle(AcknowledgeAction{});
		check(!f.flow->inventoryConfirmation()&&live->roster.at(1).weapons[0].id==0,"byte-identical replacement replay");
		arm(handle);check(f.flow->inventoryConfirmation()!=token,"replacement reused generation");
		live->party=XeenParty::fromRosterIds({1,0});f.flow->invalidateInventory();
		check(!f.flow->inventorySelection().slot&&f.flow->inventorySelection().sourceOwner==1,"membership reinterpreted selected source");
		handle(AcknowledgeAction{});check(live->roster.at(1).weapons[0].id==0,"membership stale Enter moved");
		handle(SelectMemberAction{1});arm(handle,0);
		live->roster.at(0).weapons[0].id=99;f.flow->invalidateInventory();
		handle(AcknowledgeAction{});check(!f.flow->inventorySelection().slot&&live->roster.at(1).weapons[0].id==0,"source change stale Enter");
		live->party=XeenParty::fromRosterIds({});f.flow->invalidateInventory();f.flow->refresh(true);
		handle(TransferInventoryAction{});check(f.flow->inventoryOpen(),"empty panel closed");
		XeenInventoryTestAccess::exhaust(*f.flow);handle(SelectMemberAction{0});handle(InspectInventoryAction{});
		check(!f.flow->inventoryOpen()&&!f.flow->inventoryConfirmation(),"exhausted generation rearmed");return true;
	};Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"invalidation boundary");
}
void automaticGuard() {
	Fixture f;seed(f);f.automatic=true;f.scripts[1]={record(1,1,0,9,{44,1,1}),record(1,1,1,0x12)};
	auto services=f.services();services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
		const auto gen=f.flow->presentationGeneration();const auto pixels=f.flow->frame().pixels;const auto count=f.compositions;
		check(gen.has_value(),"automatic fixture did not suspend");
		for(auto a:std::vector<PlayerAction>{InspectInventoryAction{},TransferInventoryAction{},SelectInventorySlotAction{0}})handle(a);
		check(!f.flow->inventoryOpen()&&f.flow->presentationGeneration()==gen&&f.flow->frame().pixels==pixels&&f.compositions==count,"automatic inventory requests disturbed pending event");
		handle(AcknowledgeAction{});check(!f.flow->inventoryOpen()&&!f.flow->blocksGameplay(),"final event ACK fell through");return true;
	};Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"automatic modal guard");
}
void sdl() {
	Fixture f;seed(f);auto services=f.services();unsigned index=0;bool queued=false,quit=false;
	const std::vector<checkpoint_test::Input> inputs={
		{SDLK_i,InspectInventoryAction{}},{SDLK_1,SelectInventorySlotAction{0}},
		{SDLK_t,TransferInventoryAction{}},{SDLK_ESCAPE,CancelInteractionAction{}},
		{SDLK_t,TransferInventoryAction{}},{SDLK_F2,SelectMemberAction{1}},
		{SDLK_ESCAPE,CancelInteractionAction{}},{SDLK_t,TransferInventoryAction{}},
		{SDLK_F2,SelectMemberAction{1}},{SDLK_F9,SaveGameAction{}},
		{SDLK_RETURN,AcknowledgeAction{}},{SDLK_RETURN,AcknowledgeAction{}},
		{SDLK_SPACE,InteractionAction{}},{SDLK_ESCAPE,CancelInteractionAction{}},
		{SDLK_i,InspectInventoryAction{}}};
	services.show=[&](const auto &first,const auto &handle,const auto &escape,const auto &idle,const auto &status){
		return SdlWindow().showInteractive(first,"Inventory SDL",[&](const PlayerAction &a){
			check(index<inputs.size()&&a.index()==inputs[index].action.index(),"SDL inventory order/repeat/quit");
			auto frame=handle(a);++index;queued=false;return frame;
		},escape,[&]()->std::optional<IndexedFrame>{
			if(!queued&&index<inputs.size()) {
				SDL_Event e{};e.type=SDL_KEYDOWN;e.key.keysym.sym=inputs[index].key;check(SDL_PushEvent(&e)==1,"inventory SDL key");
				e.key.repeat=1;SDL_PushEvent(&e);e.type=SDL_KEYUP;e.key.repeat=0;SDL_PushEvent(&e);queued=true;
			}else if(index==inputs.size()&&!quit){quit=true;SDL_Event e{};e.type=SDL_QUIT;SDL_PushEvent(&e);e.type=SDL_KEYDOWN;e.key.keysym.sym=SDLK_RETURN;SDL_PushEvent(&e);}
			return idle();
		},status);
	};Quiet quiet;check(Application().playGameplay(services,start,{},false)==0&&index==inputs.size(),"SDL modal lifecycle");
}
void selectedNameWrapping() {
	Fixture f;seed(f);auto bytes=fontBytes();
	for(unsigned c=0;c<128;++c){bytes[0x1080+c]=5;for(unsigned y=0;y<8;++y)bytes[0x800+c*16+y*2]=0x55;}
	XeenFontFormat font(bytes);XeenTextRenderer renderer(font);
	XeenMaterialNameParseResult materials;materials.availability=XeenMaterialAvailability::Ready;
	materials.names[58]="obsidian";
	const auto catalog=loadXeenItemCatalogWithMaterials(materials);
	f.initial.roster.at(0).weapons[0]={58,1,6,1};
	const std::string expected="Obsidian long sword Beast Bopper";
	check(catalog.describe(XeenInventoryCategory::Weapons,{58,1,6,1}).displayName==expected,"synthetic catalog literal differs");
	XeenInventorySelection s;s.mode=Mode::Confirm;s.sourceOwner=0;s.slot=0;s.destination=1;s.destinationOwner=1;
	const auto lines=xeenInventoryLayout(font,catalog,f.initial,s,"gyp feedback");
	std::string selected;unsigned nameLines=0,rows=0;bool rowElided=false;
	for(const auto &line:lines){
		if(line.bounds.left==10&&line.bounds.top>=44&&line.bounds.top<=116){++rows;if(line.bounds.top==44)rowElided=line.text.find("...")!=std::string::npos;}
		if(line.bounds.left==154&&line.bounds.top>=44&&line.bounds.top<62&&line.text.find("Slot ")!=0){
			if(!selected.empty())selected+=' ';selected+=line.text;++nameLines;
		}
	}
	check(selected==expected&&nameLines==2,"selected detail must expose complete literal Obsidian long sword Beast Bopper over two lines");
	check(rows==9&&rowElided,"nine visible slots and elidable row name");
	const auto has=[&](const std::string &text){return std::any_of(lines.begin(),lines.end(),[&](const auto &line){return line.text==text;});};
	check(has("Weapons - Slot 1")&&has("Equipped")&&has("Unbroken / Uncursed")&&has("Counter 6")&&has("M=58 ID=1")&&has("S=6 F=1")&&has("To F2 [owner 1]")&&has("Receiver")&&has("gyp feedback")&&has("Enter confirms; F1-F6 changes recipient; Esc/N cancels"),"mandatory detail/destination/help fields missing");
	check(std::any_of(lines.begin(),lines.end(),[](const auto &l){return l.text.find("Condition: ")==0;})&&
		std::any_of(lines.begin(),lines.end(),[](const auto &l){return l.text.find("HP ")==0&&l.text.find(" / ")!=std::string::npos;})&&
		std::any_of(lines.begin(),lines.end(),[](const auto &l){return l.text.find("SP ")==0&&l.text.find(" / ")!=std::string::npos;}),"character condition/current/max fields missing");
	IndexedFrame base;base.width=320;base.height=200;base.pixels.assign(64000,77);
	for(std::size_t i=0;i<lines.size();++i){
		const auto a=lines[i].bounds;
		check(a.left>=10&&a.right<=310&&a.top>=8&&a.bottom<=146&&a.bottom-a.top==9,"required rectangle outside panel/line budget");
		for(std::size_t j=i+1;j<lines.size();++j){const auto b=lines[j].bounds;check(a.right<=b.left||b.right<=a.left||a.bottom<=b.top||b.bottom<=a.top,"wrapped detail rectangle overlap");}
		XeenTextRenderOptions options;options.bounds=a;options.x=a.left;options.y=a.top;options.size=XeenFontSize::Reduced;
		const auto isolated=renderer.render(base,lines[i].text,options).pages.front();
		for(int y=0;y<200;++y)for(int x=0;x<320;++x)if(x<a.left||x>=a.right||y<a.top||y>=a.bottom)
			check(isolated.pixels[y*320+x]==77,"glyph escaped its intended rectangle");
	}
	const auto frame=drawXeenInventory(base,font,catalog,f.initial,s,"gyp feedback");
	// Independent literal lines, including Bopper's ninth-pixel descenders.
	for(const auto &entry:std::vector<std::pair<int,std::string>>{{44,"Obsidian long sword Beast"},{53,"Bopper"}}){
		XeenTextRenderOptions o;o.bounds={154,entry.first,310,entry.first+9};o.x=154;o.y=entry.first;o.size=XeenFontSize::Reduced;
		auto background=base;background.pixels.assign(64000,0x99);
		const auto expectedFrame=renderer.render(background,entry.second,o).pages.front();
		for(int y=o.bounds.top;y<o.bounds.bottom;++y)for(int x=154;x<310;++x)
			check(frame.pixels[y*320+x]==expectedFrame.pixels[y*320+x],"selected literal glyphs missing/clipped or overlapping");
	}
	check(frame.pixels[61*320+164]==0x19&&frame.pixels[62*320+164]==0x99,"selected-name descender crosses next field");
	// A catalog-bounded description exceeding both lines uses the first line
	// fully before deterministic final-line elision, without moving any fields.
	materials.names[58]="gyp gyp gyp gyp gyp gyp gyp gyp gyp gyp gyp gyp gyp gyp gyp gyp";
	const auto longCatalog=loadXeenItemCatalogWithMaterials(materials);
	const auto longer=xeenInventoryLayout(font,longCatalog,f.initial,s,"gyp feedback");
	check(longer.size()==lines.size(),"long name displaced required fields");
	for(std::size_t i=0;i<lines.size();++i){const auto a=lines[i].bounds,b=longer[i].bounds;
		check(a.left==b.left&&a.right==b.right&&a.top==b.top&&a.bottom==b.bottom,"long name moved rectangle");
		if(b.left==154&&b.top==44)check(longer[i].text=="Gyp gyp gyp gyp gyp gyp gyp gyp","long name did not expose first wrapped line");
		else if(b.left==154&&b.top==53)check(longer[i].text.size()>=3&&longer[i].text.substr(longer[i].text.size()-3)=="..."&&renderer.textWidth(longer[i].text,XeenFontSize::Reduced)<=149,"overflow final line not bounded/elided");
		else if(!(b.left==10&&b.top==44))check(longer[i].text==lines[i].text,"long name changed mandatory field");
	}
}
void layout() {
	Fixture f;seed(f);auto bytes=fontBytes();
	for(unsigned c=0;c<128;++c){bytes[0x1080+c]=5;for(unsigned y=0;y<8;++y)bytes[0x800+c*16+y*2]=0x55;}
	XeenFontFormat font(bytes);auto &c=f.initial.roster.at(0);c.name=std::string(100,'g')+"%\x01";c.currentHp=-32768;c.currentSp=32767;
	c.birthYear=592;c.permanentLevel=100000000;c.endurance.permanent=11;
	XeenInventorySelection s;s.mode=Mode::Browse;s.sourceOwner=0;s.slot=0;
	const auto lines=xeenInventoryLayout(font,f.catalog,f.initial,s,"feedback");
	unsigned rows=0;bool hp=false,sp=false;
	for(const auto &line:lines){if(line.bounds.left==10&&line.bounds.top>=44&&line.bounds.top<=116)++rows;
		hp|=line.text=="HP -32768 / 1000000000";sp|=line.text=="SP 32767 / 0";
		for(unsigned char b:line.text)check(b>=32&&b<=126,"unsafe view text");
	}
	check(rows==9&&hp&&sp,"required nine rows/full numbers");
	for(std::size_t i=0;i<lines.size();++i)for(std::size_t j=i+1;j<lines.size();++j){const auto a=lines[i].bounds,b=lines[j].bounds;check(a.right<=b.left||b.right<=a.left||a.bottom<=b.top||b.bottom<=a.top,"layout overlap");}
	IndexedFrame base;base.width=320;base.height=200;base.pixels.assign(64000,77);
	const auto frame=drawXeenInventory(base,font,f.catalog,f.initial,s,"gyp");
	check(frame.pixels[134*320+10]!=frame.pixels[135*320+10],"reduced descender ninth pixel clipped");
	for(int y=0;y<200;++y)for(int x=0;x<320;++x)if(x<4||x>=316||y<4||y>=149)check(frame.pixels[y*320+x]==77,"inventory escaped panel");
	check(c.name==std::string(100,'g')+"%\x01","view mutated name");
}
void hud() {
	Fixture f;auto &source=f.initial.roster.at(0);auto &destination=f.initial.roster.at(1);
	for(auto *c:{&source,&destination}){c->birthYear=592;c->permanentLevel=1;c->characterClass=XeenCharacterClass::Paladin;c->endurance.permanent=19;c->currentHp=16;}
	source.weapons[0]={105,12,0,1};destination.weapons[8]={105,0,0,1};
	auto services=f.services();auto compose=services.compose;
	services.compose=[&](auto &w,const auto &p,const auto &c,std::uint64_t phase){
		auto result=compose(w,p,c,phase);
		for(const auto &hp:CloudsUiComposer::buildHpPlacements(p,{610}))result.frame.pixels[hp.y*320+hp.x]=static_cast<std::uint8_t>(hp.frame);
		return result;
	};
	services.show=[&](const auto &first,const auto &handle,const auto &,const auto &,const auto &){
		check(first.pixels[182*320+13]==0&&first.pixels[182*320+50]==0,"initial independent HUD maxima");
		handle(InspectInventoryAction{});arm(handle);handle(AcknowledgeAction{});
		check(f.flow->frame().pixels[182*320+13]==3&&f.flow->frame().pixels[182*320+50]==3,"both changed maxima missing from live HUD");
		handle(CancelInteractionAction{});check(f.flow->frame().pixels[182*320+13]==3,"close restored obsolete HUD");return true;
	};Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"HUD transfer recomposition");
}
void beforePublicationFailure() {
	Fixture f;seed(f);auto bytes=fontBytes();for(unsigned c=0;c<128;++c)bytes[0x1080+c]=255;
	f.font=XeenFontFormat(bytes);auto services=f.services();const XeenPartyState *live=nullptr;
	services.observeGameplay=[&](auto &,auto &,const auto &p,auto &,const auto &){live=&p;};
	services.show=[&](const auto &,const auto &handle,const auto &,const auto &,const auto &){
		const auto before=remove_test::partySnapshot(*live);handle(InspectInventoryAction{});
		check(!f.flow->inventoryOpen()&&remove_test::partySnapshot(*live)==before,"failed opening published/mutated");
		f.font=XeenFontFormat(fontBytes());handle(InspectInventoryAction{});check(f.flow->inventoryOpen(),"new open after draw failure");return true;
	};Quiet quiet;check(Application().playGameplay(services,start,{},false)==0,"pre-publication failure recovery");
}
int child(const fs::path &path,bool resume) {
	Fixture f;seed(f);auto services=f.services();auto expected=f.saved();
	SdlWindow::FrameUpdateHandler nested;
	SdlWindow::IdleFrameHandler nestedIdle;
	bool preflightReentered=false;
	auto compose=services.compose;
	services.compose=[&](auto &w,const auto &p,const auto &c,std::uint64_t phase){
		if(nested && &w!=f.world) {
			const auto count=f.compositions;
			nested(InspectInventoryAction{});nested(SaveGameAction{});nestedIdle();
			check(!f.flow->inventoryOpen()&&f.compositions==count,"save preflight allowed reentrancy");preflightReentered=true;
		}
		return compose(w,p,c,phase);
	};
	for(auto *items:{&expected.characters[0].weapons,&expected.characters[0].armor,&expected.characters[0].accessories,&expected.characters[0].miscellaneous})
		*items={{{10,37,1,0},{},{},{},{},{},{},{},{}}};
	for(auto *items:{&expected.characters[1].weapons,&expected.characters[1].armor,&expected.characters[1].accessories,&expected.characters[1].miscellaneous})
		*items={{{10,37,1,0},{},{},{},{},{},{},{},{}}};
	const XeenPartyState *live=nullptr;XeenCamera *camera=nullptr;const XeenGameFlags *flags=nullptr;
	services.observeGameplay=[&](auto &w,auto &,const auto &p,auto &c,const auto &g){f.world=&w;live=&p;camera=&c;flags=&g;};
	services.show=[&](const auto &,const auto &handle,const auto &,const auto &idle,const auto &status){
		nested=handle;nestedIdle=idle;
		check(!f.flow->inventoryOpen(),"startup inventory persisted");handle(InspectInventoryAction{});
		for(unsigned category=0;category<4;++category){
			if(!resume){arm(handle);handle(SaveGameAction{});check(!fs::exists(path),"open F9 wrote");handle(AcknowledgeAction{});f.flow->refresh(true);}
			else for(unsigned owner=0;owner<2;++owner){handle(SelectMemberAction{owner});handle(SelectInventorySlotAction{0});check(f.flow->inventorySelection().record.id==37,"restored UI did not inspect owner");}
			if(category!=3)handle(NavigationAction::TurnRight);
		}
		sameSnapshot(expected,XeenSaveState::capture(f.signature,*live,*camera,*flags,*f.world));
		handle(CancelInteractionAction{});
		if(!resume){check(!fs::exists(path),"deferred save");handle(SaveGameAction{});check(status().find("Saved")!=std::string::npos&&preflightReentered,"production F9/preflight guard failed");}
		sameSnapshot(expected,XeenSaveFile::read(path));return true;
	};
	Quiet quiet;const int result=Application().playGameplay(services,start,path,resume);
	if(result)std::cerr<<quiet.out.str();return result;
}
void restart(const fs::path &exe) {
	const auto dir=fs::current_path()/("inventory-restart-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));fs::create_directory(dir);
	const auto path=dir/"transfer.mmsave";
	const auto p=child_test::launch(exe,{L"producer",path.wstring()},dir/"producer.log");check(p.exit==0,"producer failed");
	std::ifstream input(path,std::ios::binary);const std::string bytes{std::istreambuf_iterator<char>(input),{}};input.close();
	const auto c=child_test::launch(exe,{L"consumer",path.wstring()},dir/"consumer.log");check(c.exit==0&&p.pid!=c.pid,"fresh consumer failed");
	std::ifstream after(path,std::ios::binary);check(bytes==std::string(std::istreambuf_iterator<char>(after),{}),"consumer rewrote producer disk");
}
}
int main(int argc,char **argv){try{
	if(argc==3)return child(fs::absolute(argv[2]),std::string(argv[1])=="consumer");
	if(argc==2&&std::string(argv[1])=="sdl"){sdl();return 0;}
	gameplay();failure();beforePublicationFailure();invalidation();automaticGuard();selectedNameWrapping();layout();hud();restart(fs::absolute(argv[0]));
	std::cout<<"Inventory modal, rendering, recovery and production disk/fresh-process lifecycle passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

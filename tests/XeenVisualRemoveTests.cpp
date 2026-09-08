#include "XeenVisualRemoveTestSupport.h"
#include "SyntheticXeenArchive.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include <algorithm>
#include <chrono>
#include <iostream>

using namespace mmodern;
using namespace remove_test;
using namespace sprite_test;
namespace {
Bytes fontBytes() {
	Bytes b(XeenFontFormat::kMinimumSize);
	for(int c=0;c<128;++c){b[0x1000+c]=6;b[0x1080+c]=3;
		for(int y=0;y<8;++y){b[c*16+y*2]=0x55;b[0x800+c*16+y*2]=0x55;}}
	return b;
}
GameInstallation installation(const std::filesystem::path &dir) {
	std::filesystem::create_directories(dir);
	Bytes empty;word(empty,41);for(int i=0;i<41;++i){word(empty,166);word(empty,0);}
	const auto c=cell(0,0,0,0,{});empty.insert(empty.end(),c.begin(),c.end());
	std::map<std::string,Bytes> files;
	for(const char *n:{"sky.sky","water.out","water.srf","space.srf","global.icn","border.icn","fecp.brd","bless.icn","restorex.icn","main.icn"})files[n]=empty;
	files["back.raw"]=Bytes(64000,99);files["mm4.pal"]=Bytes(768);
	for(int i=0;i<768;++i)files["mm4.pal"][i]=(i/3)%64;
	Bytes rows;for(int y=0;y<30;++y){rows.push_back(43);rows.push_back(0);rows.push_back(31);rows.insert(rows.end(),32,7);rows.push_back(7);rows.insert(rows.end(),8,7);}
	files["111.0bj"]=sprite(cell(60,40,70,30,rows));
	archive(dir/"xeen.cc",files);archive(dir/"dark.cc",{{"clouds.dat",Bytes(1452)}});
	GameInstallation i;i.xeenArchive=dir/"xeen.cc";i.darkArchive=dir/"dark.cc";return i;
}
struct Fixture {
	int maps=0,objects=0,scripts=0,texts=0,compositions=0,completed=0;
	bool automatic=false;
	bool transfer=false;
	std::vector<XeenEventRecord> records{record(8,2,0,0x0e)};
	std::string text="AAAA";
	XeenAssetSource &assets;
	XeenWorld world{[this](XeenMapIdentity id){++maps;auto m=map(id);if(automatic)m.geometry.cells[2*16+8].rawAttributes=0x10;return m;},
		[this](XeenMapIdentity id){++objects;XeenObjectFile f;f.mapId=id;f.resourcePresent=true;
			f.entities.objects={{8,2,0,0,111},{9,3,0,0,111},{1,1,0,0,111}};return f;}};
	XeenEventSystem events{[this](XeenMapIdentity id){++scripts;
		if(transfer && id.number==23)return script(id,{record(8,2,0,0x1f,{24,8,2})});
		return script(id,records);},
		[this](XeenMapIdentity id){++texts;return XeenEventTextFile{id,"text",true,{text,"AAAA"}};}};
	XeenCamera camera{23,8,2,XeenDirection::North};
	XeenGameFlags flags;
	XeenPartyState party;
	XeenFontFormat font{fontBytes()};
	CloudsMapComposer composer;
	XeenEventFlow flow{world,events,party,camera,flags,font,[this]{++compositions;return composer.compose(assets,world,party,camera,{});}};
	explicit Fixture(XeenAssetSource &a):assets(a){flow.reportManual=[this](const auto &r){if(std::holds_alternative<XeenManualEventCompleted>(r))++completed;};}
	void absent(const IndexedFrame &f) {
		const auto reference=composer.compose(assets,world,party,camera,{});
		check(f.pixels==reference.pixels,"runtime frame differs from effective composition");
		check(world.isObjectDisabled({23,0}) && !world.isObjectDisabled({23,1}),"identity isolation");
		check(std::count(f.pixels.begin(),f.pixels.end(),7)>0,"shared-sprite neighbor disappeared");
	}
};
void basic(XeenAssetSource &assets,const std::filesystem::path &out) {
	for(bool automatic:{false,true}){
		Fixture f(assets);f.automatic=automatic;f.world.discardMapCache();const auto before=f.flow.frame();
		const auto resolver=XeenObjectVisualResolver::load(assets);
		const auto commands=XeenOutdoorScene().build(f.world,f.camera,&resolver);
		check(std::count_if(commands.begin(),commands.end(),[](const auto &c){return c.object()!=nullptr;})==2,"two simultaneously visible commands");
		if(!automatic){check(f.flow.initial().pixels==before.pixels && f.world.sessionState().disabledObjectCount()==0,"automatic trigger gate ignored");}
		const auto after=automatic?f.flow.initial():f.flow.handle(InteractionAction{});
		std::cout << "automatic=" << automatic << " disabled=" << f.world.sessionState().disabledObjectCount()
			<< " pixels before=" << std::count(before.pixels.begin(),before.pixels.end(),7)
			<< " after=" << std::count(after.pixels.begin(),after.pixels.end(),7) << '\n';
		check(before.pixels!=after.pixels,"no-motion Remove retained old pixels");f.absent(after);
		check(f.camera.mapId==23 && f.camera.x==8 && f.camera.y==2 && f.camera.direction==XeenDirection::North,"Remove moved camera");
		check(!f.world.isObjectDisabled({24,0}) && !f.world.isObjectDisabled({{XeenSide::Darkside,23},0}),"cross-map/side leak");
		const int n=f.compositions;for(int i=0;i<20;++i)f.flow.refresh();check(n==f.compositions,"continuous recomposition");
		if(!automatic){visual_remove_test::save(before,out/"synthetic-before.bmp");visual_remove_test::save(after,out/"synthetic-after.bmp");}
		const auto loads=assets.spriteLoadCount();assets.discardSpriteCache();check(assets.cachedSpriteCount()==0,"sprite cache not empty");
		f.world.discardMapCache();f.events.discardScriptCache();f.events.discardTextCache();f.absent(f.flow.refresh(true));
		check(assets.spriteLoadCount()>loads && f.maps>=2 && f.objects>=2,"reconstruction did not load");
		std::size_t instructions=0;f.flow.reportManual=[&](const auto &r){if(const auto *done=std::get_if<XeenManualEventCompleted>(&r))instructions=done->instructionCount;};
		f.flow.handle(InteractionAction{});check(f.scripts>=2 && instructions==1,"script cache not reloaded or Remove replayed");
		f.camera.mapId=24;f.flow.refresh();f.camera.mapId=23;f.absent(f.flow.refresh());
		Fixture fresh(assets);check(fresh.flow.frame().pixels==before.pixels && !fresh.flow.blocksGameplay(),"new session reused visual state");
	}
}
void presentation(XeenAssetSource &assets,const std::filesystem::path &out) {
	for(bool automatic:{false,true})for(bool yes:{false,true}){
		Fixture f(assets);f.automatic=automatic;f.world.discardMapCache();
		f.text.clear();for(int i=0;i<180;++i)f.text+=(i<90?"A ":"AAAA ");
		f.records={record(8,2,0,0x19,{7,8,0}),record(8,2,1,0x12),
			record(7,8,0,0x09,{20,5,4}),record(7,8,1,0x01,{0}),
			record(7,8,2,0x0c,{0,0,20,5}),record(7,8,3,0x0e),
			record(7,8,4,0x01,{1}),record(7,8,5,0x09,{44,0,7}),
			record(7,8,6,0x1a),record(7,8,7,0x1a)};
		automatic?f.flow.initial():f.flow.handle(InteractionAction{});
		check(f.flow.blocksGameplay() && !f.world.isObjectDisabled({23,0}),"presentation before mutation");
		f.flow.handle(AcknowledgeAction{});const auto page=f.flow.frame();
		const int n=f.compositions;f.flow.handle(NavigationAction::TurnRight);
		check(f.camera.direction==XeenDirection::North && f.flow.frame().pixels==page.pixels && n==f.compositions,"pending navigation not blocked");
		f.world.discardMapCache();f.events.discardScriptCache();f.events.discardTextCache();assets.discardSpriteCache();
		check(f.flow.refresh(true).pixels==page.pixels && f.flow.blocksGameplay(),"rebase reset current page");
		int steps=0;while(!f.world.isObjectDisabled({23,0}) && ++steps<100)f.flow.handle(AcknowledgeAction{});
		check(steps<100 && f.flow.blocksGameplay(),"continuation never reached Remove and choice");
		const auto effective=f.composer.compose(assets,f.world,f.party,f.camera,{});
		for(int y=8;y<140;++y)for(int x=8;x<223;++x)
			check(f.flow.frame().pixels[y*320+x]==effective.pixels[y*320+x],"post-mutation underlay stale");
		visual_remove_test::save(f.flow.frame(),out/"synthetic-presentation.bmp");
		yes?f.flow.handle(YesAction{}):f.flow.handle(NoAction{});
		check(!f.flow.blocksGameplay() && f.flags.isSet(5),"choice did not complete transaction");
		check(f.flow.frame().pixels!=effective.pixels,"completion erased valid text");
		f.flow.handle(NavigationAction::TurnRight);check(f.camera.direction==XeenDirection::East,"navigation did not resume");
	}
}
void errorsAndSuspendedMutation(XeenAssetSource &assets,const std::filesystem::path &out) {
	for(bool automatic:{false,true}) for(bool transfer:{false,true}) {
		Fixture f(assets);f.automatic=automatic;f.world.discardMapCache();
		f.transfer=transfer;const auto original=f.flow.frame();
		f.records={record(8,2,0,0x19,{7,8,0}),record(8,2,1,0x12),
			record(7,8,0,0x09,{20,5,3}),record(7,8,1,0x0c,{0,0,20,5}),
			record(7,8,2,0x0e),record(7,8,3,0x1a,{1})};
		bool error=false;auto verify=[&](const auto &r){if(const auto *e=std::get_if<XeenEventExecutionError>(&r)){error=e->kind==XeenEventExecutionErrorKind::MalformedInstruction;check(f.flow.frame().pixels==f.composer.compose(assets,f.world,f.party,f.camera,{}).pixels,"error reported before refresh");}};
		f.flow.reportManual=verify;f.flow.reportAutomatic=verify;
		automatic?f.flow.initial():f.flow.handle(InteractionAction{});
		check(error && !f.flags.isSet(5) && !f.flow.blocksGameplay() && f.camera.mapId==23 && f.camera.x==8 && f.camera.y==2,"error converted to success or camera/flags committed");
		if(transfer){
			check(f.flow.frame().pixels==original.pixels && f.world.isObjectDisabled({24,0}) && !f.world.isObjectDisabled({23,0}),"rollback lost destination mutation or previewed teleport");
			visual_remove_test::save(f.flow.frame(),out/"synthetic-error-rollback.bmp");
			f.camera.mapId=24;check(f.flow.refresh().pixels!=original.pixels,"destination mutation lost after rollback");
		}else{f.absent(f.flow.frame());visual_remove_test::save(f.flow.frame(),out/"synthetic-error.bmp");}
	}
	Fixture f(assets);f.records={record(8,2,0,0x09,{44,0,1}),record(8,2,1,0x12)};
	f.flow.handle(InteractionAction{});check(f.flow.blocksGameplay(),"choice missing");
	f.world.disableObject({23,0});f.flow.handle(NavigationAction::TurnLeft);
	check(f.flow.blocksGameplay() && f.camera.direction==XeenDirection::North,"refresh consumed response");
	f.flow.handle(NoAction{});f.absent(f.flow.frame());
	// External mutation on a later page refreshes every retained page, preserving
	// its index, then confirmation restores the newly composed underlay.
	Fixture paged(assets);paged.text=std::string(800,'A');
	paged.records={record(8,2,0,0x01,{0}),record(8,2,1,0x09,{44,1,2}),record(8,2,2,0x12)};
	paged.flow.handle(InteractionAction{});paged.flow.handle(AcknowledgeAction{});
	paged.world.disableObject({23,0});paged.flow.handle(NavigationAction::TurnLeft);
	int pages=0;while(paged.flow.blocksGameplay() && ++pages<100)paged.flow.handle(AcknowledgeAction{});
	check(pages<100,"pagination/acknowledgment stalled");
	const auto base=paged.composer.compose(assets,paged.world,paged.party,paged.camera,{});
	for(int y=8;y<140;++y)for(int x=8;x<223;++x)check(paged.flow.frame().pixels[y*320+x]==base.pixels[y*320+x],"later page resurrected sprite");
	Fixture fatal(assets);fatal.automatic=true;fatal.world.discardMapCache();fatal.records={
		record(8,2,0,0x19,{7,8,0}),record(7,8,0,0x09,{20,5,3}),
		record(7,8,1,0x0c,{0,0,20,5}),record(7,8,2,0x0e),record(7,8,3,0x1a,{1})};
	fatal.flow.reportAutomatic=[](const auto &r){if(std::holds_alternative<XeenEventExecutionError>(r))throw std::runtime_error("automatic diagnostic");};
	bool propagated=false;try{fatal.flow.initial();}catch(const std::runtime_error &e){propagated=std::string(e.what())=="automatic diagnostic";}
	check(propagated,"automatic error policy silently became success");fatal.absent(fatal.flow.frame());
	Fixture empty(assets);empty.records.clear();const auto untouched=empty.flow.frame();
	check(empty.flow.handle(InteractionAction{}).pixels==untouched.pixels && !empty.flow.blocksGameplay() && !empty.flags.isSet(5),"no-event interaction altered frame/state");
	Fixture label(assets);label.records={record(8,2,0,0x04,{0}),record(8,2,1,0x12)};
	const int labelCompositions=label.compositions;const auto labelBase=label.flow.frame();
	label.flow.handle(InteractionAction{});
	check(label.compositions==labelCompositions+1 && label.completed==1 &&
		label.flow.frame().pixels!=labelBase.pixels && !label.flow.blocksGameplay(),"no-mutation completion erased label or recomposed unnecessarily");
	// A completed label expires on the next gameplay action, even when the
	// fixture's surface blocks movement and no camera/world change requests redraw.
	const auto beforeBlockedCamera=label.camera;
	const auto beforeBlockedObjects=label.world.sessionState().disabledObjectCount();
	const int beforeBlockedCompositions=label.compositions;
	bool blocked=false;
	label.flow.reportMovement=[&](XeenMovementResult result){blocked=result==XeenMovementResult::BlockedBySurface;};
	const auto afterBlocked=label.flow.handle(NavigationAction::MoveForward);
	check(blocked && label.camera.mapId==beforeBlockedCamera.mapId &&
		label.camera.x==beforeBlockedCamera.x && label.camera.y==beforeBlockedCamera.y &&
		label.camera.direction==beforeBlockedCamera.direction &&
		label.world.sessionState().disabledObjectCount()==beforeBlockedObjects,"blocked label fixture changed camera/world");
	check(afterBlocked.pixels==labelBase.pixels && label.flow.frame().pixels==labelBase.pixels,
		"blocked movement left retained label visible");
	check(label.compositions==beforeBlockedCompositions+1,"blocked movement did not recompose clean base");
	Fixture suspended(assets);suspended.transfer=true;suspended.text=std::string(800,'A');
	suspended.records={record(8,2,0,0x19,{7,8,0}),record(8,2,1,0x12),
		record(7,8,0,0x09,{20,5,4}),record(7,8,1,0x01,{0}),
		record(7,8,2,0x0c,{0,0,20,5}),record(7,8,3,0x0e),record(7,8,4,0x1a,{1})};
	bool physicalLogical=false,failed=false;
	suspended.flow.reportManual=[&](const auto &r){
		if(const auto *s=std::get_if<XeenEventExecutionSuspended>(&r))physicalLogical=s->state.workingCamera.mapId==24 && s->state.logicalAddress.x==7 && s->state.workingCamera.x==8;
		if(const auto *e=std::get_if<XeenEventExecutionError>(&r))failed=e->kind==XeenEventExecutionErrorKind::MalformedInstruction;
	};
	const auto source=suspended.flow.frame();suspended.flow.handle(InteractionAction{});
	check(physicalLogical && suspended.flow.blocksGameplay() && suspended.camera.mapId==23 && !suspended.flags.isSet(5),"suspension committed working camera/flags");
	pages=0;while(suspended.flow.blocksGameplay() && ++pages<100)suspended.flow.handle(AcknowledgeAction{});
	check(failed && suspended.camera.mapId==23 && !suspended.flags.isSet(5) && suspended.world.isObjectDisabled({24,0}),"resumed failure rollback/world contract");
	for(int y=8;y<140;++y)for(int x=8;x<223;++x)check(suspended.flow.frame().pixels[y*320+x]==source.pixels[y*320+x],"suspended/resumed result previewed uncommitted teleport");
}
}
int main(int argc,char **argv){try{
	const auto dir=std::filesystem::temp_directory_path()/("mmodern-16c-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{dir};
	const std::filesystem::path output=argc>1?argv[1]:dir;std::filesystem::create_directories(output);
	XeenAssetSource assets(installation(dir),320,200);
	basic(assets,output);presentation(assets,output);errorsAndSuspendedMutation(assets,output);
	std::cout<<"Visual Remove runtime, pages, errors, isolation and lifecycle passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

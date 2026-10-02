#include "XeenEncounterTestSupport.h"
#include "XeenSaveGameplayTestSupport.h"
#include "SyntheticXeenArchive.h"
#include "XeenChildProcessTestSupport.h"
#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsMapComposer.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenEventLoader.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenGameFlagsLoader.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <fstream>
#include <iostream>

using namespace encounter_test;
namespace fs=std::filesystem;
namespace {
void sprites(const fs::path &out) {
	GameInstallation i;i.root=out;i.xeenArchive=out/"xeen.cc";i.edition=GameEdition::CloudsOfXeen;
	std::vector<std::pair<Bytes,Bytes>> frames;
	for(unsigned n=0;n<8;++n)frames.push_back({sprite_test::cell(20,1,20,1,{3,0,0,static_cast<std::uint8_t>(n+1)}),{}});
	const auto valid=sprite_test::multiFrameSprite(frames);
	const auto occluder=sprite_test::sprite(sprite_test::cell(20,1,20,1,{3,0,0,99}));
	sprite_test::archive(i.xeenArchive,{{"042.mon",valid},{"terrain",occluder},{"object",occluder},{"blank",Bytes(64000)}});
	{XeenAssetSource a(i,320,200);a.validateNormalMonster(42);check(a.spriteLoadCount()==1,"normal preflight cache");
		XeenSpriteDrawOptions options;options.sceneClipped=true;
		for(unsigned frame=0;frame<8;++frame){a.drawNormalMonster(42,frame,0,0,options);check(a.snapshot().pixels[20*320+20]==frame+1,"normal frame pixels");}
		a.discardSpriteCache();a.validateNormalMonster(42);check(a.spriteLoadCount()==2,"normal cache reconstruction");}
	{XeenAssetSource a(i,320,200);
		XeenOutdoorDrawCommand actor;actor.originalOrder=94;actor.content=XeenOutdoorActorDraw{{20,5},42,0,XeenMonsterSpriteKind::Normal,3,0,false};
		XeenOutdoorDrawCommand terrain;terrain.originalOrder=105;terrain.content=XeenOutdoorTerrainDraw{"terrain",0,{}};
		XeenObjectVisual visual;visual.identity={20,0};visual.spriteName="object";visual.status=XeenObjectVisualStatus::SupportedStatic;
		XeenOutdoorDrawCommand object;object.originalOrder=110;object.content=XeenOutdoorObjectDraw{visual,0,false};
		CloudsMapComposer composer;
		for(const auto &occluderCommand:{terrain,object}) {
			a.loadRawFramebuffer("blank");composer.drawOutdoorCommands(a,{actor,occluderCommand});
			check(a.snapshot().pixels[20*320+20]==99,"later original command did not obscure actor");
			a.loadRawFramebuffer("blank");composer.drawOutdoorCommands(a,{occluderCommand,actor});
			check(a.snapshot().pixels[20*320+20]==1,"ordered raster control did not reveal actor");
		}
		// One-pixel cell offsets travel through the real decoder and scene/bottom clips.
		for(const auto &[x,y,visible]:std::vector<std::tuple<int,int,bool>>{{7,20,false},{8,20,true},{222,20,true},
			{223,20,false},{20,7,false},{20,8,true},{20,140,true},{20,141,false}}) {
			a.loadRawFramebuffer("blank");actor.x=x-20;actor.y=y-20;composer.drawOutdoorCommands(a,{actor});
			check((a.snapshot().pixels[y*320+x]==1)==visible,"normal scene clip edge");
		}
		actor.x=0;actor.y=120;std::get<XeenOutdoorActorDraw>(actor.content).bottomClipped=true;
		a.loadRawFramebuffer("blank");composer.drawOutdoorCommands(a,{actor});
		check(a.snapshot().pixels[140*320+20]==0,"near bottom clip edge");
	}
	frames[7].first.pop_back();sprite_test::archive(i.xeenArchive,{{"042.mon",sprite_test::multiFrameSprite(frames)}});
	{XeenAssetSource a(i,320,200);rejects([&]{a.validateNormalMonster(42);});}
	{XeenAssetSource a(i,320,200);a.drawSprite("042.mon",0,0,0);rejects([&]{a.validateNormalMonster(42);});}
	frames.resize(4);
	const auto attack=sprite_test::multiFrameSprite(frames);
	sprite_test::archive(i.xeenArchive,{{"042.att",attack},{"blank",Bytes(64000)},{"terrain",occluder}});
	{XeenAssetSource a(i,320,200);a.validateAttackMonster(42);
		check(a.spriteLoadCount()==1,"ATT admission cache");
		rejects([&]{a.validateNormalMonster(42);});
		XeenSpriteDrawOptions options;options.sceneClipped=true;
		for(std::uint8_t f=0;f<4;++f){a.drawMonster(42,{XeenMonsterSpriteKind::Attack,f},0,0,options);
			check(a.snapshot().pixels[20*320+20]==f+1,"four ATT frame pixels");}
		rejects([&]{a.drawMonster(42,{XeenMonsterSpriteKind::Attack,4},0,0,options);});
		options.horizontalFlip=true;rejects([&]{a.drawMonster(42,{XeenMonsterSpriteKind::Attack,0},0,0,options);});
		options.horizontalFlip=false;
		a.discardSpriteCache();a.validateAttackMonster(42);check(a.spriteLoadCount()==2,"ATT cache reconstruction");
		CloudsMapComposer composer;
		XeenOutdoorDrawCommand actor;actor.originalOrder=121;
		actor.content=XeenOutdoorActorDraw{{20,5},42,0,XeenMonsterSpriteKind::Attack,0,0,true};
		XeenOutdoorDrawCommand terrain;terrain.originalOrder=120;terrain.content=XeenOutdoorTerrainDraw{"terrain",0,{}};
		a.loadRawFramebuffer("blank");composer.drawOutdoorCommands(a,{terrain,actor});
		check(a.snapshot().pixels[20*320+20]==1,"ATT order121 after120");
		a.loadRawFramebuffer("blank");composer.drawOutdoorCommands(a,{actor,terrain});
		check(a.snapshot().pixels[20*320+20]==99,"later terrain still occludes ATT");
		for(const auto &[x,y,visible]:std::vector<std::tuple<int,int,bool>>{{7,20,false},{8,20,true},{222,20,true},
			{223,20,false},{20,7,false},{20,8,true},{20,139,true},{20,140,false}}){
			a.loadRawFramebuffer("blank");actor.x=x-20;actor.y=y-20;composer.drawOutdoorCommands(a,{actor});
			check((a.snapshot().pixels[y*320+x]==1)==visible,"ATT scene/bottom clip edge");
		}
	}
	for(unsigned bad=0;bad<3;++bad){
		auto broken=frames;
		if(bad==0)broken.resize(3);
		if(bad==1)broken.push_back(frames[0]);
		if(bad==2)broken[3].first.pop_back();
		sprite_test::archive(i.xeenArchive,{{"042.att",sprite_test::multiFrameSprite(broken)}});
		{XeenAssetSource a(i,320,200);rejects([&]{a.validateAttackMonster(42);});check(a.cachedSpriteCount()==0,"bad ATT cached");}
		{XeenAssetSource a(i,320,200);a.drawSprite("042.att",0,0,0);rejects([&]{a.validateAttackMonster(42);});}
	}
	fs::remove(i.xeenArchive);
}
}
int main(){try{
 const auto out=child_test::freshDirectory(fs::current_path()/"encounter-sprites");sprites(out);
 std::cout<<"Shared monster sprite validation, ordering and clipping passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

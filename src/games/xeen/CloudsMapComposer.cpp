#include "games/xeen/CloudsMapComposer.h"

#include "formats/xeen/XeenAssetSource.h"
#include "games/xeen/CloudsUiComposer.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenWorld.h"
#include "games/xeen/XeenMapView.h"
#include "games/xeen/XeenSkills.h"
#include "games/xeen/XeenTextRenderer.h"
#include <algorithm>

namespace mmodern {

void CloudsMapComposer::drawOutdoorCommands(XeenAssetSource &assets,
		const std::vector<XeenOutdoorDrawCommand> &commands) const {
	for (const auto &command : commands) {
		if (const auto *object = command.object())
			assets.drawObjectVisual(object->visual, command.x, command.y, command.drawOptions());
		else if (const auto *actor = command.actor())
			assets.drawMonster(actor->image, {actor->kind, actor->frame}, command.x, command.y, command.drawOptions());
		else if(const auto *p=command.projectile())
			assets.drawProjectile(p->pow,p->enemy,p->row,command.x,command.y,command.drawOptions());
		else if(const auto *s=command.splat())assets.drawSceneSprite("charpow.icn",s->frame,command.x,command.y,command.drawOptions());
		else
			assets.drawSceneSprite(command.terrain().resourceName, command.terrain().frame,
				command.x, command.y, command.drawOptions());
	}
}

void CloudsMapComposer::drawIndoorCommands(XeenAssetSource &assets,
		const std::vector<XeenIndoorDrawCommand> &commands) const {
	for (const auto &command : commands) {
		if (const auto *object = command.object())
			assets.drawObjectVisual(object->visual, command.x, command.y,
				command.drawOptions());
		else if (const auto *actor=command.actor())
			assets.drawMonster(actor->image,{actor->kind,actor->frame},command.x,command.y,command.drawOptions());
		else if(const auto *p=command.projectile())assets.drawProjectile(p->pow,p->enemy,p->row,command.x,command.y,command.drawOptions());
		else if (const auto *wall=command.wallItem())
			assets.drawSceneSprite(wall->resourceName,wall->frame,command.x,command.y,command.drawOptions());
		else if(const auto *s=command.splat())assets.drawSceneSprite("charpow.icn",s->frame,command.x,command.y,command.drawOptions());
		else
			assets.drawSceneSprite(command.geometry().resourceName, command.geometry().frame,
				command.x, command.y, command.drawOptions());
	}
}

void CloudsMapComposer::drawInterfaceLayers(XeenAssetSource &assets,
		const XeenPartyState &partyState,
		const XeenCharacterRulesContext &context,XeenScenePresentation *presentation) const {
	// Interface::assembleBorder() redraws this frame after drawScene(). Parts of
	// the corner gems intentionally overlap the generic scene clipping rectangle.
	assets.drawSprite("global.icn", 0, 8, 8);
	// Resting-state border overlays from Interface::assembleBorder(). These fill
	// the intentionally transparent openings left by global.icn around the scene.
    const auto *buffs=partyState.encounterContext?&*partyState.encounterContext:nullptr;
    unsigned phase=0;
    const auto levitate=XeenScenePresentation::levitateBorderFrame(buffs && buffs->effects[XeenPartyEffect::Levitate],presentation?presentation->levitateFrame:phase);
    assets.drawSprite("border.icn",levitate,0,82);
	// Spot Secret Doors (skill 16) and Danger Sense (skill 17), each from its
	// own counter (0x1ba1e..0x1baab); not from one shared counter as in the pin.
	unsigned spotPhase=0,dangerPhase=0;
	const auto spot=XeenScenePresentation::animatedBorderFrame(presentation && presentation->thinWall &&
		xeenPartyHasSkill(partyState,16),28,presentation?presentation->spotDoorsFrame:spotPhase);
	assets.drawSprite("border.icn",spot,194,91);
	const auto danger=XeenScenePresentation::animatedBorderFrame(presentation && presentation->dangerSense &&
		xeenPartyHasSkill(partyState,17),40,presentation?presentation->dangerSenseFrame:dangerPhase);
	assets.drawSprite("border.icn",danger,107,9);
	assets.drawSprite("border.icn",  0,   0, 32); // Left clairvoyance statue, inactive.
	assets.drawSprite("border.icn",  8, 215, 32); // Right clairvoyance statue, inactive.
	// Default (no active resistance) corner indicators from assembleBorder().
    assets.drawSprite("fecp.brd", buffs && buffs->lightAndResistances[2]?1:0,   2,   2); // Fire.
	assets.drawSprite("fecp.brd", buffs && buffs->lightAndResistances[3]?3:2, 219,   2); // Electricity.
	assets.drawSprite("fecp.brd", buffs && buffs->lightAndResistances[4]?5:4,   2, 134); // Cold.
	assets.drawSprite("fecp.brd", buffs && buffs->lightAndResistances[5]?7:6, 219, 134); // Poison.
	assets.drawSprite("bless.icn", 16, 33, 137); // Blessed indicator, inactive.

	constexpr XeenPartyEffect indices[]{XeenPartyEffect::Bless,XeenPartyEffect::PowerShield,XeenPartyEffect::HolyBonus,XeenPartyEffect::Heroism};constexpr int x[]{33,55,160,182};
    for(unsigned i=0;i<4;++i)if(buffs && buffs->effects[indices[i]]) {
        const auto phase=presentation?(presentation->buffFrames[i]=(presentation->buffFrames[i]+1)%4):1;
        assets.drawSprite("bless.icn",4*i+phase,x[i],137);
    }
	// The compass/main button at y=137 overlaps the scene border in the original UI.
	CloudsUiComposer().drawInterface(assets, partyState, context);
}

IndexedFrame CloudsMapComposer::drawDirectionSense(XeenAssetSource &assets,IndexedFrame frame,
		const XeenPartyState &partyState,const XeenCamera &camera,const XeenScenePresentation &presentation) const {
	// The border's last write (0x1bdb8..0x1bdfd, format 0x52a8a): skill 6 and
	// no open text or number prompt; the first letter of the facing's name.
	if(presentation.textPrompt || !xeenPartyHasSkill(partyState,6))return frame;
	const auto direction=static_cast<unsigned>(camera.direction);
	if(direction>3)throw std::invalid_argument("Invalid Direction Sense facing");
	if(!_font)_font=std::make_shared<const XeenFontFormat>(assets.readArchiveResource("fnt"));
	XeenTextRenderOptions options;options.originalControls=true;options.bounds={0,0,320,200};
	const std::string text=std::string("\x02\f08\x03" "c\v139\t116")+"NESW"[direction]+"\fd\x01";
	auto rendered=XeenTextRenderer(*_font).render(frame,text,options);
	if(rendered.pages.size()!=1 || !rendered.diagnostics.empty())throw std::runtime_error("Direction Sense letter overflow");
	return std::move(rendered.pages.front());
}

IndexedFrame CloudsMapComposer::compose(XeenAssetSource &assets,
		XeenWorld &world, const XeenPartyState &partyState,
		const XeenCamera &camera,
		const XeenCharacterRulesContext &context,
		std::vector<XeenObjectVisual> *objectDiagnostics,
		std::optional<std::uint64_t> ordinaryPhase, bool *containsOrdinaryAnimation,
		std::optional<XeenMonsterAppearance> actorFrame) const {
	if (containsOrdinaryAnimation) *containsOrdinaryAnimation = false;
	bool emittedAnimation = false;
	if (objectDiagnostics) objectDiagnostics->clear();
	const auto actors=world.sceneActors(camera.mapId,[&] {
		const auto bytes=assets.readCloudsMonsterStatisticsFromDarkArchive();
		if (!bytes) throw std::runtime_error("Missing DARK.CC/xeen.mon for scene actors");
		return XeenMonsterFormat::parse(*bytes);
	});
	for (const auto &actor:actors) if (actor.statistics) {
		actor.statistics->validatePresentation();
		assets.validateNormalMonster(actor.statistics->image());
		assets.validateAttackMonster(actor.statistics->image());
	}
	CloudsUiComposer().loadBackground(assets);

	const XeenMap &map = world.map(camera.mapId);
	const bool night = partyState.encounterContext &&
		(partyState.encounterContext->minutes < 5 * 60 ||
		 partyState.encounterContext->minutes >= 21 * 60);
	if (map.geometry.isOutdoors()) {
		const auto resolver = XeenObjectVisualResolver::load(assets);
		const auto commands = XeenOutdoorScene().build(world, camera, &resolver, objectDiagnostics, ordinaryPhase, actorFrame,night);
		emittedAnimation = true; // The default outdoor water layer always animates.
		drawOutdoorCommands(assets, commands);
	} else {
		const auto resolver = XeenObjectVisualResolver::load(assets);
		const auto commands = XeenIndoorScene().build(
			world, camera, &resolver, objectDiagnostics,ordinaryPhase,actorFrame,night,
			[&](const auto &name) {return assets.spriteFrameCount(name,xeenPictureArchive(camera.mapId));});
		emittedAnimation=std::any_of(commands.begin(),commands.end(),[](const auto &command) {
			if(command.actor()) return true;
			if(const auto *wall=command.wallItem()) return wall->animated;
			if(const auto *object=command.object()) return object->visual.status==XeenObjectVisualStatus::SupportedAnimated;
			return !command.projectile() && !command.splat() && command.geometry().animated;
		});
		drawIndoorCommands(assets, commands);
	}

 // Obscure after scene/monsters, before overlapping border indicators.
 if(world.sceneIsBlack(partyState,camera))assets.blackenScene();
	drawInterfaceLayers(assets, partyState, context,&world.scenePresentation());
    if(partyState.encounterContext)for(auto i:{XeenPartyEffect::Levitate,XeenPartyEffect::Bless,XeenPartyEffect::PowerShield,XeenPartyEffect::HolyBonus,XeenPartyEffect::Heroism})
        emittedAnimation=emittedAnimation || partyState.encounterContext->effects[i]!=0;
	const auto &indicators=world.scenePresentation();
	emittedAnimation=emittedAnimation || (indicators.thinWall && xeenPartyHasSkill(partyState,16)) ||
		(indicators.dangerSense && xeenPartyHasSkill(partyState,17));
	auto result = drawXeenMinimap(world,camera,drawDirectionSense(assets,assets.snapshot(),partyState,camera,indicators),
        partyState.encounterContext && partyState.encounterContext->effects[XeenPartyEffect::AutomapOn]!=0,
        [&](IndexedFrame &frame,const char *name,unsigned index,int x,int y) {
            assets.drawDialogSprite(frame,name,index,x,y,xeenPictureArchive(camera.mapId));
        },ordinaryPhase.value_or(0),
        partyState.encounterContext && partyState.encounterContext->effects[XeenPartyEffect::WizardEye]!=0);
	if (containsOrdinaryAnimation) *containsOrdinaryAnimation = emittedAnimation;
	return result;
}

} // namespace mmodern

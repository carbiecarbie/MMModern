#include "games/xeen/XeenWorld.h"
namespace mmodern {
std::optional<XeenCellSample> XeenWorld::sceneCell(XeenMapIdentity mapId,int x,int y) {
	return sampleCell(mapId,x,y);
}
std::vector<XeenActor> XeenWorld::sceneActors(XeenMapIdentity mapId,
		const std::function<std::vector<XeenMonsterRecord>()> &loadStatistics) {
	_scenePresentation.enterMap(mapId);
	if (_sessionState.hasRegionalActors(mapId)) {
		const std::vector<XeenActor> live = _sessionState.regionalActors(mapId);
		_scenePresentation.include(live);
		return live;
	}
	const auto found=_sceneActors.find(mapId);
	if (found!=_sceneActors.end()) {_scenePresentation.include(found->second);return found->second;}
	if (!loadStatistics) {static const std::vector<XeenActor> empty;return empty;}
	const auto file=objectFile(mapId);
	std::vector<XeenActor> actors;
	if (!file.entities.monsters.empty()) {
		if (!_sceneStatistics) _sceneStatistics=loadStatistics();
		for (std::size_t i=0;i<file.entities.monsters.size();++i) {
			const auto &record=file.entities.monsters[i];
			XeenActor actor;actor.id={mapId,i};actor.original=record;actor.x=record.x;actor.y=record.y;
			if (record.hasResource()) {
				if (std::size_t(record.resourceId)>=_sceneStatistics->size())
					throw std::invalid_argument("Missing monster statistics in "+std::string(file.resourceName)+" record "+std::to_string(i));
				actor.statistics=_sceneStatistics->at(record.resourceId);
				actor.statistics->validatePresentation();
				actor.hp=actor.statistics->baseHp();actor.lifecycle=XeenActorLifecycle::Present;
			}
			if (record.isDisabled()) actor.lifecycle=XeenActorLifecycle::Disabled;
			actors.push_back(actor);
		}
	}
	_scenePresentation.include(actors);
	return _sceneActors.emplace(mapId,std::move(actors)).first->second;
}
}

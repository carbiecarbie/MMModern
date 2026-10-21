#include "app/XeenEncounterFlow.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenJumpRules.h"
#include "games/xeen/XeenIndoorScene.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenJourneyCapture.h"
#include "games/xeen/XeenJourneyProgression.h"
#include "games/xeen/XeenItemServiceRules.h"

#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace mmodern {
void XeenEncounterFlow::itemTravelRead(const std::function<void()> &read) {
    if(_combat)_combat->guardCallback(_combat->ticket(),read);
    else {XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);read();_journeyPreimage->check();}
}
bool XeenEncounterFlow::prepareCombatItem(const ItemUseSelection &selection,unsigned choice) {
    if(!_combat || !current(ticket()) || selection.sourceIndex!=std::size_t(_combat->participant()) ||
        selection.category!=XeenInventoryCategory::Miscellaneous || selection.slot>=9)return false;
    const auto &ids=_party.party.activeRosterIds();
    if(selection.membershipSize!=ids.size() || !std::equal(ids.begin(),ids.end(),selection.membership.begin()) ||
        ids.at(selection.sourceIndex)!=selection.sourceOwner)return false;
    const auto &source=_party.roster.at(selection.sourceOwner);
    if(!xeenSameItem(source.miscellaneous[selection.slot],selection.record) ||
        xeenItemUseAdmission(selection.category,selection.record,_camera.mapId.number!=0,source.canAct())!=XeenItemUseAdmission::Admitted ||
        !xeenItemUseSpell(selection.record.id))return false;
    bool supported=true;
    itemTravelRead([&] {
        if(selection.record.id==55)supported=!XeenLearnedSpellRules::travel(_world,_camera,XeenLearnedSpell::TownPortal,choice,&source).unsupported;
    });
    return supported;
}
bool XeenEncounterFlow::useCombatItem(const Ticket &entry,const ItemUseSelection &selection,unsigned choice) {
    if(!current(entry) || !_boundary.quiet() || !prepareCombatItem(selection,choice))return false;
    XeenSpellTravel travel;SpellMapWork work;
    if(selection.record.id==55)itemTravelRead([&] {
        travel=XeenLearnedSpellRules::travel(_world,_camera,XeenLearnedSpell::TownPortal,choice,&_party.roster.at(selection.sourceOwner));
        work=prepareSpellMap(travel);
    });
    const auto events=work.events;
    XeenCombat::CastResponse response(_combat->ticket());
    if(!acceptCombatResult(_combat->beginItemCast(response,selection.slot,selection.record,travel,std::move(work.world),std::move(work.events))))return false;
    if(selection.record.id==55 && _combat->cast() && _combat->cast()->committed) {
        if(!acceptCombatResult(_combat->serviceCast(_combat->ticket(),_lastTime)))return false;
        if(travel.reload && travel.destination)_journeyEvents=events;
    }
    observeCombat();scheduleCombat(_lastTime);return true;
}
namespace {
struct BusyCast {
	bool &flag;
	explicit BusyCast(bool &value) : flag(value) { flag=true; }
	~BusyCast() { flag=false; }
};

XeenConsequenceCharacters activeCharacters(const XeenPartyState &party,const XeenPartyOrder &order) {
	XeenConsequenceCharacters characters;
	for (unsigned i=0;i<6;++i) characters[i]=party.roster.at(order[i]);
	return characters;
}
XeenConsequenceInputs activeInputs(const XeenPartyState &party,const XeenPartyOrder &order) {
	XeenConsequenceInputs inputs;
	for (unsigned i=0;i<6;++i) inputs[i]=*party.roster.combatInputs(order[i]);
	return inputs;
}
}

std::unique_ptr<XeenShootCandidate> XeenEncounterFlow::prepareSpellProjectile(
        XeenLearnedSpell spell,std::uint8_t owner,unsigned charge) {
    auto projectile=std::make_unique<XeenShootCandidate>();auto &work=*projectile;
    work.spell=spell;work.spellCaster=owner;work.owners=_party.party.activeOrder();
    work.volley.eligible[0]=true;
    if(spell==XeenLearnedSpell::PoisonVolley)work.volley.eligible.fill(true);
    work.charge=charge;
    work.random=XeenCombatRandom(*_world.sessionState().journeyRandom());
    work.rows=xeenPlayerRayRows(_world,_camera);work.projectileEnd.fill(work.rows-1);
    work.spellPreparation.emplace(spell,_party.roster.at(owner).currentLevel());
    return projectile;
}

bool XeenEncounterFlow::initializeBeacon(std::uint8_t owner) {
    _journeyPreimage->check();
    const auto &caster=_party.roster.at(owner);
    const auto travel=XeenLearnedSpellRules::travel(_world,_camera,XeenLearnedSpell::LloydsBeacon,0,&caster);
    _journeyPreimage->check();
    if(travel.failed)return false;
    if(!caster.lloydMap) {
        auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
        prepared->retainResources(*_journeyPreimage);
        travel.applyBeacon(prepared->characters[owner]);
        _journeyPreimage->check();
        travel.applyBeacon(_party.roster.at(owner));
        _journeyPreimage.swap(prepared);_journeyPreimage->adoptMutationBoundary();
    }
    return true;
}
XeenEncounterFlow::SpellMapWork XeenEncounterFlow::prepareSpellMap(const XeenSpellTravel &travel) {
    SpellMapWork result;
    if(!travel.reload || !travel.destination)return result;
    result.world=_world.transitionCandidate();
    XeenPartyState party;result.world->copyEventParty(_party,party);
    auto camera=_camera;auto flags=_flags;
    XeenRestoreGuard guard(*result.world,party,camera,flags);
    const auto entry=ticket();
    const auto check=[&] {if(_combat) {if(!current(entry))throw std::logic_error("Stale combat map preparation");}else _journeyPreimage->check();};
    XeenRestoreGuard::EventProviders providers(*result.world,party,camera,flags,guard,check,[&]{_journeyPreimage->failed=true;});
    result.world->loadEventDestination(_camera.mapId,travel.destination->mapId);
    result.events=_travelEventsProvider(travel.destination->mapId);
    check();
    if(result.events.mapId!=travel.destination->mapId)throw std::logic_error("Travel Events identity changed");
    return result;
}

bool XeenEncounterFlow::beginCasting(const Ticket &entry) {
	if (!current(entry) || !journeyMutable() || _casting || !_boundary.quiet() || _castingGeneration==std::numeric_limits<std::uint64_t>::max() ||
			!journeyCapacity()) return false;
	BusyCast busy(_busy);
	try {
		_journeyPreimage->check();
		xeenValidateJourneyParty(_party);
		auto next=std::make_unique<CastingContinuation>();next->owners=_party.party.activeOrder();
		next->generation=++_castingGeneration;
		next->lease=_boundary.hold(XeenCombatBoundary::Work::Casting);
		_casting=std::move(next);
		_castingResult.clear();
		_world._sessionState._journeyActivity=XeenJourneyActivity::Casting;
		++_world._sessionState._journeyGeneration;++_generation;
		_journeyPreimage->adoptJourneyCoordination();
		return true;
	} catch (...) { closeJourney();throw; }
}

void XeenEncounterFlow::authorizeCastingFrame(const Ticket &entry, std::uint64_t input,
		const IndexedFrame::Presentation &frame) {
	if (!_casting || !frame || !input || !current(entry) ||
		_world.sessionState().journeyActivity()!=XeenJourneyActivity::Casting ||
		!_boundary.holds(XeenCombatBoundary::Work::Casting,_casting->lease))
		throw std::logic_error("Casting frame authority unavailable");
	_casting->displayedInput=input;
	_casting->displayedFrame=frame;
}

bool XeenEncounterFlow::castingFrameCurrent(std::uint64_t input,
		const IndexedFrame::Presentation &frame) const noexcept {
	return _casting && !_failure && !_busy && frame && _casting->displayedFrame==frame &&
		_casting->displayedInput==input && current(ticket()) &&
		_world.sessionState().journeyActivity()==XeenJourneyActivity::Casting &&
		_boundary.holds(XeenCombatBoundary::Work::Casting,_casting->lease);
}

bool XeenEncounterFlow::cancelCasting(const Ticket &entry, std::uint64_t input,
		const IndexedFrame::Presentation &frame) {
	if (!current(entry) || !castingFrameCurrent(input,frame) || _casting->committed || !journeyCapacity()) return false;
	BusyCast busy(_busy);
	const auto lease=_casting->lease;
	_casting.reset();
	_world._sessionState._journeyActivity=XeenJourneyActivity::Presentation;
	++_world._sessionState._journeyGeneration;++_generation;
	_journeyPreimage->adoptJourneyCoordination();
	_boundary.release(XeenCombatBoundary::Work::Casting,lease);
	_journeyCapture->generation=_boundary.generation();
	return true;
}

bool XeenEncounterFlow::confirmCasting(const Ticket &entry, std::size_t casterIndex,
		std::size_t slot, std::uint64_t input, const IndexedFrame::Presentation &frame,unsigned distance) {
	if (!current(entry) || !castingFrameCurrent(input,frame) || _casting->committed ||
		casterIndex>=_party.party.size() || slot>=39 || !journeyCapacity()) return false;
	BusyCast busy(_busy);
	try {
		_journeyPreimage->check();
		xeenValidateJourneyParty(_party);
		if (!XeenLearnedSpellRules::eligible(_party,casterIndex,slot) ||
			!xeenRegionalContext(xeenPrepareTime(*_party.encounterContext,(_world.map(_camera.mapId).geometry.isOutdoors()?10:1)).context)) return false;
		const auto owner=_party.party.activeRosterIds()[casterIndex];
		const auto &caster=_party.roster.at(owner);
		const auto category=XeenLearnedSpellRules::categoryForClass(caster.characterClass);
		const auto id=category ? XeenLearnedSpellRules::spellForSlot(*category,slot) : std::nullopt;
		const auto spell=id ? XeenLearnedSpellRules::supportedIn(*id,false) : std::nullopt;
		if (!spell) return false;
		const auto cost=XeenLearnedSpellRules::cost(*id,caster.currentLevel());
        if(XeenLearnedSpellRules::check(caster,slot,_party.purse->gems,false)!=XeenQuickSpellCheck::Supported)return false;
        std::optional<XeenCamera> jump;
        XeenSpellTravel travel;
        std::unique_ptr<XeenShootCandidate> projectile;
        { XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
          if(*spell==XeenLearnedSpell::Jump) {
            const auto preflight=xeenJumpPreflight(_world,_camera);_journeyPreimage->check();
            if(preflight.admission==XeenJumpAdmission::Unsupported)return false;
            if(preflight.admission==XeenJumpAdmission::Ready) {
                jump=preflight.destination;travel.destination=jump;travel.reload=jump->mapId!=_camera.mapId;
            }
          }
          if(*id==44 || *id==71 || *id==21 || *id==66 || *id==69 || *id==70) {
            travel=XeenLearnedSpellRules::travel(_world,_camera,*spell,distance,&caster);
            _journeyPreimage->check();
            if(travel.unsupported) {_journeyRefusal="Spell not supported yet";return false;}
            jump=travel.destination;
          }
          auto ending=xeenPrepareTime(*_party.encounterContext,_world.map(_camera.mapId).geometry.isOutdoors()?10:1);
          XeenLearnedSpellRules::applyPartyEffect(*spell,ending.context,caster.currentLevel());
          if(ending.dailyProcessing)xeenResetPartyTemps(ending.context);
          if(travel.shelter)xeenResetPartyTemps(ending.context);
          const auto arrival=_world.prepareTerrainArrival(jump?*jump:_camera,activeCharacters(_party,_casting->owners),ending.context);
          _journeyPreimage->check();
          if(!arrival) {_journeyRefusal=arrival.transition.reason;return false;}
          if(XeenLearnedSpellRules::target(*id)==XeenSpellTarget::Contact) {
            projectile=prepareSpellProjectile(*spell,owner,_world.map(_camera.mapId).geometry.isOutdoors()?10:1);
          }
        }
        std::vector<XeenProjectileAppearance> projectiles;
        if(projectile && projectile->spellPreparation->ready())
            for(unsigned lane=0;lane<(*spell==XeenLearnedSpell::PoisonVolley?6u:1u);++lane)
                projectiles.push_back({false,0,lane,3,{},xeenSpellProjectile(*spell,projectile->spellPreparation->type)});
        auto mapWork=prepareSpellMap(travel);
        const auto after=static_cast<std::int16_t>(caster.currentSp-cost.sp);
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);
		prepared->characters[owner].currentSp=after;
        prepared->purse->gems-=cost.gems;
        travel.applyBeacon(prepared->characters[owner]);
        if(mapWork.world) {prepared->prepareSpellMapPublication(*mapWork.world);prepared->cameraValue=*jump;}
		_casting->casterOwner=owner;_casting->slot=static_cast<std::uint8_t>(slot);
		_casting->spell=*spell;_casting->originalSp=caster.currentSp;
        _casting->jumpDestination=jump;
        _casting->failed=travel.failed;_casting->shelterPending=travel.shelter;
		_casting->random=XeenCombatRandom(*_world.sessionState().journeyRandom());
		_journeyPreimage->check();
		if(mapWork.world) {_world.publishTransition(*mapWork.world);_camera=*jump;_journeyEvents=std::move(mapWork.events);}
        travel.applyBeacon(_party.roster.at(owner));
        _party.roster.at(owner).currentSp=after;
        _party.purse->gems-=cost.gems;
        if(projectile) {
          _projectiles.swap(projectiles);_projectileDeadline=0;
          _shoot=std::move(projectile);
        }
		_casting->committed=true;
		_casting->displayedFrame.reset();_casting->displayedInput=0;
		++_world._sessionState._journeyGeneration;++_generation;
		prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
		return true;
	} catch (...) { closeJourney();throw; }
}

bool XeenEncounterFlow::respondCastingTarget(const Ticket &entry,
		std::optional<std::size_t> targetIndex, std::uint64_t input,
		const IndexedFrame::Presentation &frame) {
	if (!current(entry) || !castingFrameCurrent(input,frame) || !_casting->committed ||
		(_casting->effectDone || _casting->targetPending) || !journeyCapacity() ||
		(XeenLearnedSpellRules::target(static_cast<std::uint8_t>(_casting->spell))!=XeenSpellTarget::Member &&
         XeenLearnedSpellRules::target(static_cast<std::uint8_t>(_casting->spell))!=XeenSpellTarget::Element &&
         XeenLearnedSpellRules::target(static_cast<std::uint8_t>(_casting->spell))!=XeenSpellTarget::Item) ||
		(targetIndex && *targetIndex>=(_casting->spell==XeenLearnedSpell::ProtectionFromElements?4:_party.party.size()))) return false;
    BusyCast busy(_busy);
    _casting->targetIndex=targetIndex;_casting->targetPending=true;
    _casting->displayedFrame.reset();_casting->displayedInput=0;
    return publishCastingTarget();
}

bool XeenEncounterFlow::publishCastingTarget() {
    const auto targetIndex=_casting->targetIndex;
    const bool element=_casting->spell==XeenLearnedSpell::ProtectionFromElements;
    try {
		_journeyPreimage->check();
        if(targetIndex && XeenLearnedSpellRules::target(static_cast<std::uint8_t>(_casting->spell))==XeenSpellTarget::Item) {
            _casting->itemSpell.emplace();_casting->itemSpell->member=*targetIndex;
            _casting->targetPending=false;
            ++_world._sessionState._journeyGeneration;++_generation;
            _journeyPreimage->adoptJourneyCoordination();return true;
        }
		XeenSpellPreparation preparedEffect;
        if(_casting->spell==XeenLearnedSpell::PowerCure && targetIndex) {
            XeenConsequenceDraw draw{_casting->random,64,[&]{_journeyPreimage->check();}};
            const auto effect=XeenLearnedSpellRules::preparePowerCure(_party,*targetIndex,_party.encounterContext->year,
                _party.roster.at(_casting->casterOwner).currentLevel(),draw);
            if(!effect)return true;
            preparedEffect=*effect;
        } else preparedEffect=XeenLearnedSpellRules::prepareEffect(_casting->spell,_party,element?std::nullopt:targetIndex,_party.encounterContext->year);
        const auto cost=XeenLearnedSpellRules::cost(static_cast<std::uint8_t>(_casting->spell),
            _party.roster.at(_casting->casterOwner).currentLevel());
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);
		std::string result;
		if (!targetIndex) {
			if (_party.roster.at(_casting->casterOwner).currentSp!=_casting->originalSp-int(cost.sp))
				throw std::logic_error("Casting refund preimage changed");
			prepared->characters[_casting->casterOwner].currentSp=_casting->originalSp;
            prepared->purse->gems+=cost.gems;
			result="Spell target cancelled; SP and gems refunded";
		} else if(element) {
            XeenLearnedSpellRules::applyPartyEffect(_casting->spell,*prepared->context,
                _party.roster.at(_casting->casterOwner).currentLevel(),unsigned(*targetIndex));
            result="Party elemental protection; costs spent";
        } else if (preparedEffect.failed) result="Spell failed; costs spent";
		else {
			const auto &effect=preparedEffect.effects.front();
			effect.apply(prepared->characters[effect.owner]);
			result="Healing: "+_party.roster.at(_casting->casterOwner).name+" -> "+
				_party.roster.at(effect.owner).name+" HP "+
				std::to_string(_party.roster.at(effect.owner).currentHp)+"->"+std::to_string(effect.hp)+
				"; SP spent";
		}
        prepared->s._journeyRandom=_casting->random.continuation();
        _journeyPreimage->check();
        _world._sessionState._journeyRandom=_casting->random.continuation();
        if (!targetIndex) {
            _party.roster.at(_casting->casterOwner).currentSp=_casting->originalSp;
            _party.purse->gems+=cost.gems;
        }
		else if(element) *_party.encounterContext=*prepared->context;
        else for (const auto &effect:preparedEffect.effects) {
			auto &live=_party.roster.at(effect.owner);
            if(effect.healing)_world.scenePresentation().spellEffect(effect.owner,_lastTime,true);
			effect.apply(live);
		}
		_castingResult.swap(result);
		_casting->failed=preparedEffect.failed;
		_casting->effectDone=true;
		_casting->displayedFrame.reset();_casting->displayedInput=0;
		++_world._sessionState._journeyGeneration;++_generation;
		prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
		return true;
	} catch (...) { closeJourney();throw; }
}

bool XeenEncounterFlow::publishAwaken(const Ticket &entry) {
	if (!current(entry) || !_casting || !_casting->committed || _casting->effectDone ||
		(XeenLearnedSpellRules::target(static_cast<std::uint8_t>(_casting->spell))!=XeenSpellTarget::Party &&
         XeenLearnedSpellRules::target(static_cast<std::uint8_t>(_casting->spell))!=XeenSpellTarget::Relocation) || !journeyCapacity()) return false;
	BusyCast busy(_busy);
	try {
		_journeyPreimage->check();
		const auto effect=XeenLearnedSpellRules::prepareEffect(_casting->spell,_party,{},_party.encounterContext->year,_casting->casterOwner,
            _casting->spell==XeenLearnedSpell::Light && _world.sceneIsBlack(_party,_camera));
        std::optional<XeenSpellInformation> information;
        if(_casting->spell==XeenLearnedSpell::DetectMonster || _casting->spell==XeenLearnedSpell::IdentifyMonster) {
            const auto &actors=_world.sessionState().regionalActors(_camera.mapId);
            const auto view=XeenActorApproach::classify(actors,_camera);
            std::array<std::optional<XeenMonsterIdentity>,3> slots;
            for(unsigned n=0;n<3;++n)slots[n]=view.slots[n];
            information=XeenLearnedSpellRules::information(_casting->spell,_camera,actors,slots);
        }
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);
		std::ostringstream message;
		message<<"Party spell: "<<_party.roster.at(_casting->casterOwner).name<<"; SP spent; ";
		unsigned cleared=0;
		for (const auto &value:effect.effects) {
			const auto &before=_party.roster.at(value.owner);
			cleared+=(before.conditions[8]!=0 && value.conditions[8]==0);
			value.apply(prepared->characters[value.owner]);
		}
		message<<cleared<<" Sleep cleared";
		auto result=message.str();
        if(effect.food)prepared->food=*effect.food;
        XeenLearnedSpellRules::applyPartyEffect(_casting->spell,*prepared->context,_party.roster.at(_casting->casterOwner).currentLevel());
        if(_casting->jumpDestination)prepared->cameraValue=*_casting->jumpDestination;
        _journeyPreimage->check();
        if(effect.food)_party.food=*effect.food;
        XeenLearnedSpellRules::applyPartyEffect(_casting->spell,*_party.encounterContext,_party.roster.at(_casting->casterOwner).currentLevel());
        if(effect.stepped || (_casting->spell==XeenLearnedSpell::Jump && _casting->jumpDestination))_world.scenePresentation().markLightingStep();
        if(_casting->spell==XeenLearnedSpell::Jump) {
          _casting->failed=!_casting->jumpDestination;
        }
        if(_casting->jumpDestination)_camera=*_casting->jumpDestination;
		for (const auto &value:effect.effects) {
            if(value.healing)_world.scenePresentation().spellEffect(value.owner,_lastTime,true);
            value.apply(_party.roster.at(value.owner));
        }
		_castingResult.swap(result);
        _casting->information=std::move(information);
        if(_casting->information && _casting->information->failed) {_casting->failed=true;_casting->information.reset();}
        _casting->effectDone=true;
        _casting->displayedFrame.reset();_casting->displayedInput=0;
        ++_world._sessionState._journeyGeneration;++_generation;
        prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
        return true;
    } catch (...) { closeJourney();throw; }
}

bool XeenEncounterFlow::respondSpellItem(const Ticket &entry,std::optional<XeenSpellItemSelection> selection,
        std::uint64_t input,const IndexedFrame::Presentation &frame) {
    if(!current(entry) || _busy)return false;
    auto *work=_casting && _casting->itemSpell?&*_casting->itemSpell:
        _itemUse && _itemUse->itemSpell?&*_itemUse->itemSpell:nullptr;
    if(!work || work->selected)return false;
    if(_casting) {if(!castingFrameCurrent(input,frame))return false;}
    else if(!_itemUse->selectorTicket || !_itemUse->selectorFrame || _itemUse->selectorFrame!=frame ||
        _itemUse->selectorInput!=input || !current(*_itemUse->selectorTicket))return false;
    _journeyPreimage->check();
    const auto &member=_party.party.member(_party.roster,work->member);
    if(selection) {
        const auto *items=xeenInventoryItems(member,selection->category);
        if(!items || selection->slot>=items->size() || !xeenSameItem(items->at(selection->slot),selection->record))return false;
        const auto spell=_casting?_casting->spell:*_itemUse->spell;
        const auto caster=_casting?_casting->casterOwner:_itemUse->sourceOwner;
        work->preparation.emplace(spell,_party.roster.at(caster).currentLevel(),selection->category,selection->record);
    }
    work->selection=selection;work->selected=true;
    if(_casting) {_casting->displayedFrame.reset();_casting->displayedInput=0;}
    else {_itemUse->selectorFrame.reset();_itemUse->selectorTicket.reset();}
    return serviceSpellItem();
}
bool XeenEncounterFlow::serviceSpellItem() {
    if(!journeyCapacity())return false;
    BusyCast busy(_busy);
    try {
    auto &work=_casting?*_casting->itemSpell:*_itemUse->itemSpell;
    if(!work.selected)return false;
    _journeyPreimage->check();
    auto &random=_casting?_casting->random:_itemUse->opportunity->random;
    XeenConsequenceDraw draw{random,64,[&]{_journeyPreimage->check();}};
    if(work.preparation && !work.preparation->service(draw))return true;
    auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
    prepared->retainResources(*_journeyPreimage);
    const auto owner=_party.party.activeRosterIds().at(work.member);
    if(work.preparation && !work.preparation->failed) {
        auto &items=*xeenInventoryItems(prepared->characters[owner],work.selection->category);
        items.at(work.selection->slot)=work.preparation->item;
        if(work.preparation->gold)xeenCompactItems(items);
        prepared->purse->gold+=work.preparation->gold;
    }
    prepared->s._journeyRandom=random.continuation();
    _journeyPreimage->check();
    if(work.preparation && !work.preparation->failed) {
        *xeenInventoryItems(_party.roster.at(owner),work.selection->category)=
            *xeenInventoryItems(prepared->characters[owner],work.selection->category);
        _party.purse->gold=prepared->purse->gold;
    }
    _world._sessionState._journeyRandom=random.continuation();
    const bool failed=work.preparation && work.preparation->failed;
    if(_casting) {_casting->failed=failed;_casting->effectDone=true;}
    else {_itemUse->failed=failed;_itemUse->effectDone=true;_itemUse->ready=true;}
    ++_world._sessionState._journeyGeneration;++_generation;
    prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);return true;
    } catch(...) {closeJourney();throw;}
}

bool XeenEncounterFlow::serviceCasting() {
    if(_rest || (_casting && _casting->shelterPending))return false;
    if(_casting && _casting->itemSpell && _casting->itemSpell->selected && !_casting->effectDone)return serviceSpellItem();
	if (!_casting || _casting->information || !_casting->committed || (!_casting->effectDone && !_casting->targetPending) || _busy || _failure ||
		!current(ticket()) || !journeyCapacity()) return false;
	BusyCast busy(_busy);
	try {
		_journeyPreimage->check();
		auto &work=*_casting;
        if(!work.effectDone)return publishCastingTarget();
		if (!work.time) work.time.emplace(*_party.encounterContext,(_world.map(_camera.mapId).geometry.isOutdoors()?10:1),activeCharacters(_party,work.owners),activeInputs(_party,work.owners),&*_party.serviceEconomy);
		XeenConsequenceDraw draw{work.random,64,[&] { _journeyPreimage->check(); }};
		if (!work.time->service(draw)) return true;
        if(!work.terrain) {
            auto rule=_world.prepareTerrainArrival(_camera,work.time->characters,work.time->context);
            if(!rule)throw std::logic_error("Casting arrival admission changed");
            if(rule.falling) {_terrainFall=rule;rule.damage=0;}
            work.terrain.emplace(rule,work.time->characters,work.time->inputs,work.time->context,
                work.time->economy?&*work.time->economy:nullptr);
        }
        if(!work.terrain->service(draw))return true;
        work.time->characters=work.terrain->characters;work.time->inputs=work.terrain->inputs;
        work.time->context=work.terrain->context;work.time->economy=work.terrain->economy;
        work.time->needsRest=work.time->needsRest || work.terrain->needsRest;
        _terrainDead=work.terrain->rule.dead;
		if (_world._sessionState._encounterRevision==std::numeric_limits<std::uint64_t>::max())
			throw std::overflow_error("Casting charge generation exhausted");
		std::vector<XeenExploredTile> exploration;
        bool automaticArrival=false;
        {XeenRestoreGuard::Providers providers(*_journeyPreimage,_world);
            exploration=_world.explorationAfterStep(_party,_camera);
            if(work.jumpDestination) {
              const auto sample=_world.sampleCell(_camera.mapId,_camera.x,_camera.y);
              automaticArrival=sample && (sample->cell->rawAttributes & kXeenAutomaticEventFlag) && bool(xeenRegionalEvent(_journeyEvents,_camera));
            }
            _journeyPreimage->check();}
		auto prepared=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
		prepared->retainResources(*_journeyPreimage);
		prepared->s.assignExploration(exploration);
		bool living=false;
		for (const auto &value:work.time->characters) {
			auto &owner=prepared->characters[value.rosterId];
			owner=value;
			living=living || xeenCombatTargetable(owner);
		}
        living=living && !_terrainDead;
		prepared->context=work.time->context;prepared->economy=work.time->economy;
		for(unsigned n=0;n<6;++n)prepared->inputs[work.owners[n]]=work.time->inputs[n];
		const auto continuation=work.random.continuation();
		prepared->s._journeyRandom=continuation;
		_journeyPreimage->check();
		for (const auto &value:work.time->characters) {
			auto &owner=_party.roster.at(value.rosterId);
			owner=value;
		}
		if(work.time->needsRest)_needsRestNotice=true;
		_party.encounterContext=work.time->context;_party.serviceEconomy=work.time->economy;
		for(unsigned n=0;n<6;++n)_party.roster._combatInputs[work.owners[n]]=work.time->inputs[n];
		auto &session=_world._sessionState;
		session.publishExploration(exploration);
        _world.scenePresentation().markLightingStep();
		session._journeyRandom=continuation;
        if(work.terrain->rule.damage)for(auto owner:work.owners)
            _world.scenePresentation().portraitDamage(owner,work.terrain->rule.type==XeenDamageType::Fire?1u:0u,_lastTime);
		_state._pending=living && !_terrainFall?3:0;
		if (!living) { _state._phase=XeenEncounterPhase::SupportStopped;
			_state._reason=XeenEncounterStop::Defeat;session._encounterTerminal=true; }
		++session._encounterRevision;_state._revision=session._encounterRevision;
		session._journeyActivity=living?XeenJourneyActivity::Approach:XeenJourneyActivity::SupportStopped;
		++session._journeyGeneration;++_generation;
		prepared->adoptJourneyCoordination();_journeyPreimage.swap(prepared);
		if(automaticArrival && living) {
          _regionalAutomatic=true;_regionalAutomaticAddress=XeenCombatLocation{_camera.mapId,_camera.x,_camera.y,_camera.direction};
        }
        _castingSettlement=true;
        const auto lease=work.lease;_casting.reset();
		_boundary.release(XeenCombatBoundary::Work::Casting,lease);
		_journeyCapture->generation=_boundary.generation();
		return true;
	} catch (...) { closeJourney();throw; }
}

} // namespace mmodern

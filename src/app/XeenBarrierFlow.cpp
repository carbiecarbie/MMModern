#include "app/XeenEventFlow.h"
#include "games/xeen/XeenRestoreGuard.h"
#include "games/xeen/XeenJourneyRules.h"
#include "games/xeen/XeenEventTrigger.h"
#include "games/xeen/XeenIndoorScene.h"

namespace mmodern {
bool XeenEventFlow::beginBarrier(bool bash) {
	if(_barrier || !_encounter->journeyQuiet() || (_camera.mapId!=XeenMapIdentity(28) && (!bash || _camera.mapId!=XeenMapIdentity(23))))return false;
	auto work=std::make_unique<BarrierWork>();work->bash=bash;
	work->world=_world.transitionCandidate();work->world->copyEventParty(_party,work->party);
	work->camera=_camera;work->flags=_flags;work->context=*_party.encounterContext;
	work->random=XeenCombatRandom(*_world.sessionState().journeyRandom());
	for(unsigned n=0;n<6;++n) {const auto owner=_party.party.activeRosterIds()[n];work->characters[n]=_party.roster.at(owner);work->inputs[n]=*_party.roster.combatInputs(owner);}
	work->guard=std::make_unique<XeenRestoreGuard>(*work->world,work->party,work->camera,work->flags);
	{
		XeenRestoreGuard::Providers providers(*work->guard,*work->world,[&]{_encounter->journeySavePreimage().check();});
		work->rule.emplace(*work->world,work->camera,work->characters,work->inputs,work->random.continuation(),work->context,bash);
	}
	if(!work->rule->handled)return false;
	if(bash) {
		const unsigned charge=work->world->map(_camera.mapId).geometry.isOutdoors()?10:1;
		static_cast<void>(xeenPrepareTime(work->context,charge));
		// chargeStep precedes HP costs and the Might roll. Recreate the rule only
		// after that bounded time continuation, before any gameplay publication.
		work->rule.reset();work->time.emplace(work->context,charge,work->characters,work->inputs,&*work->party.serviceEconomy);
	}
	_encounter->beginJourneyEvent(true);
	_presenter.clear();_journeyEventLayers=false;
	_barrier=std::move(work);
	if(_barrier->rule)_barrier->rule->deferInjury=true;
	if(_barrier->rule && _barrier->rule->selection) {
		XeenPresentationRequest request;request.kind=XeenPresentationKind::CharacterSelection;
		request.response=XeenPresentationResponseRequirement::CharacterSelection;
		request.mapId=_camera.mapId;request.verbIndex=13;
		request.text=(_barrier->rule->targetWall==1 || _barrier->rule->targetWall==13)?"Open Door":"Open Grate";
		for(unsigned n=0;n<6;++n)request.members.push_back({n,_barrier->characters[n].rosterId,std::string(_barrier->characters[n].name),_barrier->characters[n].canAct()});
		_presenter.present(_frame,request);_journeyEventLayers=true;
	} else serviceBarrier();
	return true;
}

bool XeenEventFlow::serviceBarrier() {
	if(!_barrier || _barrier->published || (_barrier->rule && _barrier->rule->selection))return false;
	auto &work=*_barrier;
	const auto entry=_encounter->ticket();
	const auto check=[&] {
		if(!_encounter->current(entry) || !_encounter->journeyEvent())throw std::logic_error("Stale barrier continuation");
		_encounter->journeySavePreimage().check();work.guard->check();
	};
	check();
	XeenConsequenceDraw draw{work.random,64,check};
	if(work.time) {
		if(!work.time->service(draw))return true;
		work.characters=work.time->characters;work.inputs=work.time->inputs;work.context=work.time->context;
		work.economy=work.time->economy;work.dailyReset=work.dailyReset || work.time->resetTemps;work.needsRest=work.needsRest || work.time->needsRest;work.time.reset();
		if(!work.rule) {
			XeenRestoreGuard::Providers providers(*work.guard,*work.world,check);
			work.rule.emplace(*work.world,work.camera,work.characters,work.inputs,work.random.continuation(),work.context,true);
		}
	}
	auto &rule=*work.rule;
	if(work.trapWaiting) {
		std::uint64_t now=0;_encounter->guardCallback(entry,[&]{now=_clock();});check();
		if(now<work.deadline)return false;
		rule.damagePauseAcknowledged=true;work.trapWaiting=false;
	}
	if(work.portraitWaiting) {
		if(!work.portraitPresented)return false;
		rule.injury->injuryAcknowledged=true;work.portraitWaiting=false;
	}
	// One cursor: conversions rejected by the gameplay RNG consume this call's
	// bounded budget and resume at the retained rule stage on the next pulse.
	if(!rule.service(draw)) {
		if(rule.injury && rule.injury->injuryReady)work.portraitWaiting=true;
		if(rule.injury && (rule.injury->injuryApplied || rule.damagePauseReady)) {
			std::uint64_t now=0;_encounter->guardCallback(entry,[&]{now=_clock();});check();
			if(now>std::numeric_limits<std::uint64_t>::max()-250)throw std::overflow_error("Trap animation clock exhausted");
			work.trapWaiting=true;work.deadline=now+250;
		}
		return true;
	}
	rule.random=work.random;
	if(rule.moved && !work.secondCharge) {
		work.secondCharge=true;
		static_cast<void>(xeenPrepareTime(work.context,1));
		work.time.emplace(work.context,1,rule.characters,rule.inputs,work.economy?&*work.economy:&*work.party.serviceEconomy);
		if(!work.time->service(draw))return true;
		work.characters=work.time->characters;work.inputs=work.time->inputs;work.context=work.time->context;
		work.economy=work.time->economy;work.dailyReset=work.dailyReset || work.time->resetTemps;work.needsRest=work.needsRest || work.time->needsRest;work.time.reset();rule.random=work.random;
	}
	if(work.secondCharge){rule.characters=work.characters;rule.inputs=work.inputs;}
	{
		XeenRestoreGuard::Providers providers(*work.guard,*work.world,check);
		if(rule.opened)work.world->setBarrier(work.camera,rule.targetWall,!work.bash);
	}
	// Candidate mutation is intentional and finished. Retain all immutable
	// resources across the exact new candidate preimage before composition.
	auto next=std::make_unique<XeenRestoreGuard>(*work.world,work.party,work.camera,work.flags);
	next->retainResources(*work.guard);work.guard.swap(next);
	for(const auto &c:rule.characters)work.party.roster.at(c.rosterId)=c;
	work.party.encounterContext=work.context;
	if(work.economy)work.party.serviceEconomy=work.economy;
	for(unsigned n=0;n<6;++n)work.party.roster._combatInputs[rule.characters[n].rosterId]=rule.inputs[n];
	work.camera=rule.camera;
	next=std::make_unique<XeenRestoreGuard>(*work.world,work.party,work.camera,work.flags);
	next->retainResources(*work.guard);work.guard.swap(next);
	{
		XeenRestoreGuard::Providers providers(*work.guard,*work.world,check);
		work.world->prepareTransitionArrival(work.camera);
	}
	next=std::make_unique<XeenRestoreGuard>(*work.world,work.party,work.camera,work.flags);
	next->retainResources(*work.guard);work.guard.swap(next);
	{
		XeenRestoreGuard::Providers providers(*work.guard,*work.world,check);
		const auto composed=[&] {
			try {auto result=_transitionCompose(*work.world,work.party,work.camera,0,XeenMonsterAppearance{0});check();return result;}
			catch(...) {check();throw;}
		}();
		if(!composed.frame.isValid())throw std::runtime_error("Invalid barrier candidate frame");
	}
	check();
	std::uint64_t now=0;
	_encounter->guardCallback(entry,[&]{now=_clock();});check();
	if(now>std::numeric_limits<std::uint64_t>::max()-100)throw std::overflow_error("Barrier animation clock exhausted");
	_encounter->publishBarrier(entry,*work.world,*work.guard,rule,work.context,now,work.dailyReset);
	const bool pause=work.bash && !rule.moved && rule.portraitMask;
	if(work.needsRest)_encounter->_needsRestNotice=true;
	work.published=true;work.deadline=now+(pause?100:0);
	_presenter.clear();_journeyEventLayers=false;
	if(rule.moved)_world.scenePresentation().navigation(NavigationAction::MoveForward,true);
	if(!pause) {
		const bool bash=work.bash;_barrier.reset();_encounter->endJourneyEvent();
		if(bash) {_encounter->journeyPulse(_encounter->ticket());prepareJourneyTransition();}
	}
	return true;
}

IndexedFrame XeenEventFlow::handleBarrier(const PlayerAction &action) {
	if(!_barrier || !_barrier->rule || !_barrier->rule->selection)return frameCopy();
	PlayerAction mapped=action;
	if(const auto *key=std::get_if<DialogKeyAction>(&action)) {
		if(key->key>=InputKey::F1 && key->key<InputKey::F1+6)mapped=SelectMemberAction{key->key-InputKey::F1};
		else if(key->key==InputKey::Escape)mapped=CancelInteractionAction{};
	}
	const auto update=_presenter.handle(mapped,false);
	if(!update.response)return frameCopy();
	_encounter->journeySavePreimage().check();_barrier->guard->check();
	if(std::holds_alternative<CharacterSelectionCancelled>(update.response->value)) {
		_barrier.reset();_presenter.clear();_journeyEventLayers=false;_encounter->endJourneyEvent();return renderEncounter();
	}
	if(const auto *selected=std::get_if<SelectedCharacter>(&update.response->value)) {
		if(selected->partyIndex>=6)return frameCopy();
		if(!_barrier->rule->characters[selected->partyIndex].canAct()) {
			XeenPresentationRequest request;request.kind=XeenPresentationKind::CharacterSelection;
			request.response=XeenPresentationResponseRequirement::CharacterSelection;request.mapId=_camera.mapId;
			request.verbIndex=13;request.text=(_barrier->rule->targetWall==1 || _barrier->rule->targetWall==13)?"Open Door":"Open Grate";
			for(unsigned n=0;n<6;++n)request.members.push_back({n,_barrier->characters[n].rosterId,std::string(_barrier->characters[n].name),_barrier->characters[n].canAct()});
			request.refusal=std::string(_barrier->characters[selected->partyIndex].name)+" is not in any condition to perform actions!";
			const auto base=_presenter.dismissSelection();_presenter.present(base,request);return renderEncounter();
		}
		_barrier->rule->choose(selected->partyIndex);_presenter.clear();_journeyEventLayers=false;
		serviceBarrier();return renderEncounter();
	}
	return frameCopy();
}

void XeenEncounterFlow::publishBarrier(const Ticket &entry,XeenWorld &candidate,const XeenRestoreGuard &prepared,
		const XeenBarrierCandidate &rule,const XeenGameplayContext &context,std::uint64_t now,bool dailyReset) {
	if(!current(entry) || !journeyEvent() || _busy || !rule.done || !rule.handled || &candidate!=&prepared.w ||
		!candidate.detachedEventCandidate() || now<_lastTime)throw std::logic_error("Barrier publication authority is absent or stale");
	_journeyPreimage->check();prepared.check();
	// Retain a narrow time/economy capability. Canonical rollover replaces the
	// old boundary bans; it does not grant arbitrary party/economy mutation.
	auto expectedContext=*_party.encounterContext;bool expectedReset=false, regeneration=false;
	const auto chargeTime=[&](unsigned charge) {
		const auto before=expectedContext;const auto time=xeenPrepareTime(before,charge);
		expectedContext=time.context;
		regeneration=regeneration || xeenServiceDayRegenerates(before.day,expectedContext.day,charge);
		if(time.dailyProcessing) {
			expectedReset=true;xeenResetPartyTemps(expectedContext);expectedContext.rested=false;expectedContext.newDay=false;
		}
	};
	if(rule.bash)chargeTime(_camera.mapId==XeenMapIdentity(28)?1:10);
	if(rule.moved)chargeTime(1);
	if(!(context==expectedContext) || dailyReset!=expectedReset || prepared.food!=_party.food || !prepared.economy)
		throw std::logic_error("Barrier time/food capability mismatch");
	if(!regeneration) {
		if(prepared.economy!=_party.serviceEconomy)throw std::logic_error("Barrier changed economy without restocking time");
	} else {
		xeenValidateServiceEconomy(*prepared.economy);
		if(prepared.economy->bank!=xeenPrepareBankInterest(_party.serviceEconomy->bank))
			throw std::logic_error("Barrier restocking interest mismatch");
	}
	// Only these two barrier paths own injuries, unlock XP, charged time and
	// sparse walls. Ordinary Event publication retains its stricter capability.
	for(unsigned n=0;n<6;++n) {
		const auto owner=rule.characters[n].rosterId;
		auto expected=_party.roster.at(owner);expected.currentHp=rule.characters[n].currentHp;
		expected.conditions=rule.characters[n].conditions;expected.armor=rule.characters[n].armor;
		auto input=*_party.roster.combatInputs(owner);input.experience=rule.inputs[n].experience;
  if(dailyReset) {
   xeenResetCharacterTemps(expected,input);
  }
		if(!xeen_state::sameCharacter(expected,rule.characters[n]) || !xeen_state::sameInputs(input,rule.inputs[n]))
			throw std::logic_error("Barrier candidate changed an unowned character field");
	}
	// Allocate continuation and retained preimage before the callback-free stores.
	std::unique_ptr<XeenRegionalActionCandidate> opportunity;
	if(rule.bash) {
		opportunity=std::make_unique<XeenRegionalActionCandidate>();auto &c=*opportunity;
		c.camera=rule.camera;c.context=context;c.actors=candidate.sessionState().regionalActors(_camera.mapId);
		for(unsigned n=0;n<6;++n)c.owners[n]=rule.characters[n].rosterId;
		c.characters=rule.characters;c.inputs=rule.inputs;c.random=rule.random;
		// The Flow serviced the shared cursor; its continuation is supplied below.
		// chargeStep leaves three draws; the scene draw after perform consumes
		// its first beat, matching the existing action->pulse scheduling.
		c.revision=_state.revision();c.pending=2;c.remaining=rule.moved?1:0;c.classify=true;
		c.result.outcome=XeenEncounterOutcome::Accepted;
		if(rule.moved) {
			c.context.ctr24=(c.context.ctr24+1)%24;
			const auto cell=candidate.sampleCell(rule.camera.mapId,rule.camera.x,rule.camera.y);
			c.result.automaticEvent=cell && (cell->cell->rawAttributes&kXeenAutomaticEventFlag) && xeenRegionalEvent(_events,rule.camera).has_value();
		}
	}
	auto barriers=candidate._sessionState._barriers;
	const bool indoor=_camera.mapId==XeenMapIdentity(28);
	std::vector<XeenActor> actors=candidate.sessionState().regionalActors(_camera.mapId);
	const auto beforeActors=_world.sessionState().regionalActors(_camera.mapId);
	if(actors.size()!=beforeActors.size())throw std::logic_error("Barrier actor collection changed");
	for(unsigned n=0;n<actors.size();++n) {
		auto expected=beforeActors[n];expected.activated=actors[n].activated;
		if(!xeen_state::sameActor(expected,actors[n]) || (beforeActors[n].activated && !actors[n].activated))
			throw std::logic_error("Barrier changed an unowned actor field");
	}
	const auto arrival=indoor?XeenIndoorScene().classifyActors(candidate,rule.camera,actors):XeenActorApproach::classify(actors,rule.camera);
	// The detached World retained original resources, including tiles loaded
	// during preparation. Bind their values without borrowing its owner identity.
	auto maps=_journeyPreimage->maps;auto objects=_journeyPreimage->objects;
	for(const auto &entry:prepared.maps) {
		const auto found=maps.find(entry.first);
		if(found!=maps.end() && !xeen_state::sameMap(found->second,entry.second))
			throw std::logic_error("Barrier map resource changed at publication");
		maps.emplace(entry);
	}
	for(const auto &entry:prepared.objects) {
		const auto found=objects.find(entry.first);
		if(found!=objects.end() && !xeen_state::sameObjectFile(found->second,entry.second))
			throw std::logic_error("Barrier object resource changed at publication");
		objects.emplace(entry);
	}
	auto retained=std::make_shared<XeenRestoreGuard>(_world,_party,_camera,_flags);
	retained->retainResources(*_journeyPreimage);
	retained->maps.swap(maps);retained->objects.swap(objects);
	retained->s._barriers=barriers;
	if(indoor)retained->s._vertigoActors=actors;else retained->s._actors=actors;
	retained->s._journeyRandom=rule.random.continuation();
	retained->context=context;retained->cameraValue=rule.camera;retained->economy=prepared.economy;
	for(unsigned n=0;n<6;++n) {
		retained->characters[rule.characters[n].rosterId]=rule.characters[n];
		retained->inputs[rule.characters[n].rosterId]=rule.inputs[n];
	}
	retained->prepareMutationRanges();
	_journeyPreimage->check();prepared.check();
	XeenMutationWatch::write(&_world);_world._sessionState._barriers.swap(barriers);
	if(indoor)_world._sessionState._vertigoActors->swap(actors);else _world._sessionState._actors.swap(actors);
	_world._sessionState._journeyRandom=rule.random.continuation();
	_lastTime=now;
	for(unsigned n=0;n<6;++n) {
		auto &live=_party.roster.at(rule.characters[n].rosterId);const auto &value=rule.characters[n];
		live=value;
		_party.roster._combatInputs[rule.characters[n].rosterId]=rule.inputs[n];
		if((rule.portraitMask&(1u<<n)) && !rule.damagePauseAcknowledged) {
			_world.scenePresentation().portraitDamage(live.rosterId,rule.portraitFrame,_lastTime);
			if(rule.bash)_world.scenePresentation().portraits[live.rosterId].damageDeadline=_lastTime+100;
		}
	}
	_camera=rule.camera;_party.encounterContext=context;_party.serviceEconomy=prepared.economy;
	if(opportunity) {_regionalWork.swap(opportunity);_castingSettlement=true;schedule(_lastTime);}
	_journeyPreimage.swap(retained);_journeyPreimage->adoptMutationBoundary();
	if(!rule.bash) {_result.view=arrival;publishArrival(arrival);}
}
}

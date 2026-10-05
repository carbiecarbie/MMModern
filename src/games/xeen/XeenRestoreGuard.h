#ifndef MMODERN_XEEN_RESTORE_GUARD_H
#define MMODERN_XEEN_RESTORE_GUARD_H
#include "games/xeen/XeenStateEquality.h"
#include "games/xeen/XeenGameFlags.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenLearnedSpellRules.h"
#include <mutex>
namespace mmodern {
// Retained callback preimages, never a gameplay owner or a publication capability.
class XeenRestoreGuard {
public:
	// Watches actual nested map/MOB callbacks and admits only checked detached
	// provider values. Retained cache preimages are never replaced by cache hits.
	class Providers;
	class EventProviders;
	XeenRestoreGuard(const XeenWorld &world, const XeenPartyState &party,
		const XeenCamera &camera, const XeenGameFlags &flags, bool exactCaches = false) :
		w(world), p(party), c(camera), f(flags), worldId(w._incarnation),
		worldRevision(w._ownerRevision),
		partyId(p._incarnation), rosterId(p.roster._incarnation),
		partyReplacement(p._replacement), rosterReplacement(p.roster._replacement),
		s(w._sessionState), characters(p.roster.characters()), inputs(p.roster._combatInputs),
		marked(p.roster.combatMarked()), membership(p.party.activeRosterIds()),
		quests(p.questItems.counts()), questFlags(p.questFlags.values()), recovery(p.regionalRecovery), context(p.encounterContext), treasure(p.monsterTreasure), economy(p.serviceEconomy),
		first(p.firstSerializedCount), effective(p.effectiveSerializedCount), diagnostics(p.diagnostics),
		cameraValue(c), flagValues(f.values()), combatCheck(bool(w._combatCheck)), combatAuthorized(bool(w._combatAuthorized)),
		maps(w._maps), objects(w._objects), cityStatistics(w._cityStatistics), cityOriginalCount(w._cityOriginalActorCount), cacheRevision(w._cacheRevision), exactCaches(exactCaches),
		borrowOwners{&w._gameplayBorrow, &p._gameplayBorrow, &p.roster._gameplayBorrow, &c.gameplayBorrow, &f._gameplayBorrow} {
		for (unsigned i = 0; i < borrowOwners.size(); ++i) {
			borrowStates[i] = borrowOwners[i]->retain();
			borrowRevisions[i] = borrowStates[i]->revision;
		}
		prepareMutationRanges();
		adoptMutationBoundary();
		std::lock_guard<std::mutex> lock(resourceMutex());
		resourceObservers().push_back(this);
	}
	~XeenRestoreGuard() {
		std::lock_guard<std::mutex> lock(resourceMutex());
		auto &observers=resourceObservers();
		observers.erase(std::find(observers.begin(),observers.end(),this));
	}
	XeenRestoreGuard(XeenRestoreGuard &&other) :
		w(other.w), p(other.p), c(other.c),
		f(other.f), worldId(std::move(other.worldId)), worldRevision(std::move(other.worldRevision)),
		partyId(std::move(other.partyId)), rosterId(std::move(other.rosterId)), partyReplacement(std::move(other.partyReplacement)),
		rosterReplacement(std::move(other.rosterReplacement)), s(std::move(other.s)), characters(std::move(other.characters)),
		inputs(std::move(other.inputs)), marked(std::move(other.marked)), membership(std::move(other.membership)),
		quests(std::move(other.quests)), questFlags(std::move(other.questFlags)), recovery(std::move(other.recovery)),
		context(std::move(other.context)), treasure(std::move(other.treasure)), economy(std::move(other.economy)),
		first(std::move(other.first)), effective(std::move(other.effective)), diagnostics(std::move(other.diagnostics)),
		cameraValue(std::move(other.cameraValue)), flagValues(std::move(other.flagValues)), combatCheck(std::move(other.combatCheck)),
		combatAuthorized(std::move(other.combatAuthorized)), maps(std::move(other.maps)), objects(std::move(other.objects)),
		cityStatistics(std::move(other.cityStatistics)), cityOriginalCount(other.cityOriginalCount), regionalText(std::move(other.regionalText)), vertigoText(std::move(other.vertigoText)),
		learnedNames(std::move(other.learnedNames)), cacheRevision(std::move(other.cacheRevision)), exactCaches(std::move(other.exactCaches)),
		borrowOwners(std::move(other.borrowOwners)), borrowStates(other.borrowStates), borrowRevisions(std::move(other.borrowRevisions)),
		failed(std::move(other.failed)), mutations(std::move(other.mutations)) {
		std::lock_guard<std::mutex> lock(resourceMutex());
		resourceObservers().push_back(this);
	}
	XeenRestoreGuard(const XeenRestoreGuard &)=delete;
	XeenRestoreGuard &operator=(const XeenRestoreGuard &)=delete;
	bool current() const noexcept {
		using namespace xeen_state;
		if (failed || !mutations.current()) return false;
		for (const auto &state : borrowStates) if (!state->alive) return false;
		for (unsigned i = 0; i < borrowOwners.size(); ++i)
			if (borrowOwners[i]->state != borrowStates[i] || borrowStates[i]->revision != borrowRevisions[i]) return false;
		if (w._incarnation != worldId || w._ownerRevision != worldRevision ||
			p._incarnation != partyId || p.roster._incarnation != rosterId ||
			p._replacement != partyReplacement || p.roster._replacement != rosterReplacement ||
			p.roster.combatMarked() != marked || p.party.activeRosterIds() != membership ||
			p.questItems.counts() != quests || p.questFlags.values() != questFlags || p.regionalRecovery != recovery ||
			!(p.encounterContext == context) || p.monsterTreasure != treasure || p.serviceEconomy != economy || p.firstSerializedCount != first ||
			p.effectiveSerializedCount != effective || p.diagnostics != diagnostics ||
			!sameCamera(c, cameraValue) || f.values() != flagValues ||
			bool(w._combatCheck) != combatCheck || bool(w._combatAuthorized) != combatAuthorized ||
			w._cityStatistics.size()!=cityStatistics.size() || w._cityOriginalActorCount!=cityOriginalCount) return false;
		for(unsigned i=0;i<cityStatistics.size();++i) if(w._cityStatistics[i].raw!=cityStatistics[i].raw) return false;
		const auto &live = w._sessionState;
		if (live._journeyActivity != s._journeyActivity || live._journeyOwner != s._journeyOwner ||
			live._journeyGeneration != s._journeyGeneration || live._skeletonSeed != s._skeletonSeed ||
			live._journeyRandom != s._journeyRandom ||
			live._accountedMonsters != s._accountedMonsters) return false;
		if (live._combatOwner != s._combatOwner || live._combatApproachState != s._combatApproachState ||
			live._combatEntered != s._combatEntered ||
			live._entry != s._entry || live._encounterMarked != s._encounterMarked ||
			live._encounterInitialized != s._encounterInitialized || live._encounterTerminal != s._encounterTerminal ||
			live._encounterRevision != s._encounterRevision || live._objects != s._objects || live._events != s._events ||
			live._actors.size() != s._actors.size() ||
			bool(live._vertigoActors) != bool(s._vertigoActors)) return false;
		for (std::size_t i = 0; i < characters.size(); ++i) {
			if (!sameCharacter(p.roster.characters()[i], characters[i]) ||
				bool(p.roster.combatInputs(i)) != bool(inputs[i])) return false;
			if (inputs[i] && !sameInputs(*p.roster.combatInputs(i), *inputs[i])) return false;
		}
		for (std::size_t i = 0; i < s._actors.size(); ++i)
			if (!sameActor(live._actors[i], s._actors[i])) return false;
		if (s._vertigoActors) {
			if (live._vertigoActors->size() != s._vertigoActors->size()) return false;
			for (std::size_t i=0; i<s._vertigoActors->size(); ++i)
				if (!sameActor((*live._vertigoActors)[i], (*s._vertigoActors)[i])) return false;
		}
		if(!cachesCurrent())return false;
		refreshMutationRanges();
		return mutations.current();
	}
	// Combat owns its gameplay preimage separately; retain the admitted resource
	// values across its publications, allowing matching cache reconstruction.
	bool mutationHistoryCurrent() const noexcept {
		if (!mutations.current()) failed = true;
		return !failed;
	}
	bool cachesCurrent() const noexcept {
		using namespace xeen_state;
		if (failed || !worldAlive()) return false;
		if (exactCaches && (w._cacheRevision != cacheRevision || maps.size() != w._maps.size() || objects.size() != w._objects.size())) return false;
		for (const auto &entry : w._maps) {
			const auto found = maps.find(entry.first);
			if (found == maps.end() || !sameMap(entry.second, found->second)) return false;
		}
		for (const auto &entry : w._objects) {
			const auto found = objects.find(entry.first);
			if (found == objects.end() || !sameObjectFile(entry.second, found->second)) return false;
		}
		return true;
	}
	bool worldAlive() const noexcept { return borrowStates[0]->alive; }
	bool ownersAlive() const noexcept {
		for (const auto &state : borrowStates) if (!state->alive) return false;
		return true;
	}
	void check() const {
		if (!current()) {
			failed = true;
			throw std::logic_error("preparation owner preimage changed");
		}
	}
	// Called only with detached provider results after both callback guards pass,
	// before XeenWorld inserts them. Cache hits never establish a new preimage.
	void admitMap(XeenMapIdentity id, const XeenMap &value) {
		check();
		const auto found = maps.find(id);
		if (value.identity() != id) {
			// A retained key binds internal identity as well as resource contents.
			if (found != maps.end()) failed = true;
			throw std::invalid_argument("prepared map identity mismatch");
		}
		if (found != maps.end() && !xeen_state::sameMap(found->second, value)) {
			failed = true;
			throw std::invalid_argument("prepared map resource changed during reconstruction");
		}
		maps.emplace(id, value);
		prepareMutationRanges();
	}
	void admitObjects(XeenMapIdentity id, const XeenObjectFile &value) {
		check();
		const auto found = objects.find(id);
		if (value.mapId != id) {
			if (found != objects.end()) failed = true;
			throw std::invalid_argument("prepared object identity mismatch");
		}
		if (found != objects.end() && !xeen_state::sameObjectFile(found->second, value)) {
			failed = true;
			throw std::invalid_argument("prepared object resource changed during reconstruction");
		}
		objects.emplace(id, value);
		prepareMutationRanges();
	}
	void admitRegionalText(const XeenEventTextFile &value) {
		check();
		if (value.mapId!=XeenMapIdentity(23) || value.resourceName!="aaze0023.txt" ||
			!value.resourcePresent || value.strings.size()<=32) {
			if (regionalText) failed=true;
			throw std::invalid_argument("Regional event text is missing or incomplete");
		}
		if (regionalText && (regionalText->mapId!=value.mapId || regionalText->resourceName!=value.resourceName ||
			regionalText->resourcePresent!=value.resourcePresent || regionalText->strings!=value.strings)) {
			failed=true; throw std::logic_error("Regional event text changed");
		}
		if (!regionalText) regionalText=value;
	}
	void admitVertigoText(const XeenEventTextFile &value) {
		check();
		if (value.mapId!=XeenMapIdentity(28) || value.resourceName!="aaze0028.txt" ||
			!value.resourcePresent || value.strings.size()!=64) {
			if (vertigoText) failed=true;
			throw std::invalid_argument("Vertigo event text is missing or incomplete");
		}
		if (vertigoText && (vertigoText->mapId!=value.mapId || vertigoText->resourceName!=value.resourceName ||
			vertigoText->resourcePresent!=value.resourcePresent || vertigoText->strings!=value.strings)) {
			failed=true;throw std::logic_error("Vertigo event text changed");
		}
		if (!vertigoText) vertigoText=value;
	}
	void admitLearnedSpellNames(const XeenLearnedSpellNames &value) {
		check();
		verifyLearnedSpellNames(value);
		if (!learnedNames) learnedNames=value;
	}
	// Combat checks a fresh live guard, but a changed resource must also poison
	// the retained exploration preimage so a later equal callback cannot reopen it.
	void verifyLearnedSpellNames(const XeenLearnedSpellNames &value) {
		if (learnedNames && !(*learnedNames==value)) {
			failed=true;throw std::logic_error("Learned spell names changed");
		}
	}
private:
	friend class XeenWorld;
	// World calls this only for detached provider results. Validate identity
	// and retained contents after callback authorization, before insertion. Never learn a
	// preimage from a cache hit. Retained combat guards use this same path.
	template<class Value, class Select, class Equal>
	static void admitLoadedResource(const XeenWorld &world, XeenMapIdentity id,
			const Value &value, bool identityMatches, Select select, Equal equal) {
		std::lock_guard<std::mutex> lock(resourceMutex());
		bool mismatch=false;
		for (auto *guard:resourceObservers()) {
			if (&guard->w!=&world || guard->worldId!=world._incarnation || !guard->worldAlive()) continue;
			auto &values=select(*guard);
			const auto found=values.find(id);
			if (found!=values.end() && !equal(found->second,value)) {
				guard->failed=true;
				mismatch=true;
			}
		}
		if (mismatch) throw std::logic_error("immutable resource changed during cache admission");
		if (!identityMatches) throw std::runtime_error("resource identity differs from requested map");
		for (auto *guard:resourceObservers()) {
			if (&guard->w!=&world || guard->worldId!=world._incarnation || !guard->worldAlive()) continue;
			select(*guard).emplace(id,value);
			guard->prepareMutationRanges();
		}
	}
	// Advance only the revision for this successful insertion. An earlier
	// eviction/replacement still invalidates an exact-cache destination guard.
	static void loadedResourceInserted(const XeenWorld &world) noexcept {
		std::lock_guard<std::mutex> lock(resourceMutex());
		for (auto *guard:resourceObservers())
			if (&guard->w==&world && guard->worldId==world._incarnation && guard->worldAlive() &&
				guard->cacheRevision==world._cacheRevision-1) ++guard->cacheRevision;
	}
	static std::mutex &resourceMutex() { static std::mutex mutex;return mutex; }
	static std::vector<XeenRestoreGuard *> &resourceObservers() {
		static std::vector<XeenRestoreGuard *> observers;return observers;
	}
	friend class XeenEventPublication;
	friend class XeenEncounterFlow;
	friend struct XeenTrainingTestAccess;
	friend struct XeenPurchaseTestAccess;
	friend class XeenCombat;
	friend class XeenEventFlow;
	friend class XeenSaveState;
	// Authorized publication renews mutable values, not immutable compatibility.
	// Carry the union of admitted preimages even when disposable caches are empty.
	// The old mutable preimage is intentionally not checked after publication.
	void retainResources(const XeenRestoreGuard &previous) {
		if (previous.failed || !previous.worldAlive() || &w != &previous.w || worldId != previous.worldId) {
			failed = true;
			throw std::logic_error("cannot renew invalid resource authority");
		}
		for (const auto &entry : previous.maps) {
			const auto found = maps.find(entry.first);
			if (found != maps.end() && !xeen_state::sameMap(found->second,entry.second)) {
				failed = previous.failed = true;
				throw std::logic_error("map resource changed at guard renewal");
			}
			maps.emplace(entry);
		}
		for (const auto &entry : previous.objects) {
			const auto found = objects.find(entry.first);
			if (found != objects.end() && !xeen_state::sameObjectFile(found->second,entry.second)) {
				failed = previous.failed = true;
				throw std::logic_error("object resource changed at guard renewal");
			}
			objects.emplace(entry);
		}
		if (previous.regionalText) admitRegionalText(*previous.regionalText);
		if (previous.vertigoText) admitVertigoText(*previous.vertigoText);
		if (previous.learnedNames) admitLearnedSpellNames(*previous.learnedNames);
		prepareMutationRanges();
	}
	// Fresh initialization keeps the existing owner addresses/replacement
	// revisions. Copy its exact detached successor before irreversible stores;
	// resource retention and all range allocation already precede this method.
	void prepareFreshJourneyPublication(const XeenPartyState &candidate,
		const std::vector<XeenActor> &actors,
		const std::optional<XeenJourneyRandomState> &random) {
		check();
		characters=candidate.roster.characters();
		for(unsigned owner=0;owner<inputs.size();++owner)inputs[owner]=candidate.roster.combatInputs(owner);
		marked=candidate.roster.combatMarked();membership=candidate.party.activeRosterIds();
		quests=candidate.questItems.counts();questFlags=candidate.questFlags.values();
		recovery=candidate.regionalRecovery;context=candidate.encounterContext;
		treasure=candidate.monsterTreasure;economy=candidate.serviceEconomy;
		first=candidate.firstSerializedCount;effective=candidate.effectiveSerializedCount;diagnostics=candidate.diagnostics;
		s._actors=actors;s._entry=XeenEncounterEntry::Journey;
		s._encounterMarked=s._encounterInitialized=true;s._encounterRevision=1;
		s._skeletonSeed=0;s._journeyRandom=random;
		prepareMutationRanges();
	}
	// Prepare a final-destination preimage before publication. Only the private
	// SaveState swaps below are anticipated; no callback mutation is adopted.
	void prepareJourneyPublication(const XeenRestoreGuard &candidate) {
		s = candidate.s; characters = candidate.characters; inputs = candidate.inputs;
		marked = candidate.marked; membership = candidate.membership;
		quests = candidate.quests; questFlags = candidate.questFlags; recovery = candidate.recovery; context = candidate.context; treasure = candidate.treasure;
		economy = candidate.economy;
		first = candidate.first; effective = candidate.effective; diagnostics = candidate.diagnostics;
		cameraValue = candidate.cameraValue; flagValues = candidate.flagValues;
		maps = candidate.maps; objects = candidate.objects;
		cityStatistics = candidate.cityStatistics;
		cityOriginalCount = candidate.cityOriginalCount;
		if (regionalText && candidate.regionalText &&
			(regionalText->mapId!=candidate.regionalText->mapId ||
			 regionalText->resourceName!=candidate.regionalText->resourceName ||
			 regionalText->resourcePresent!=candidate.regionalText->resourcePresent ||
			 regionalText->strings!=candidate.regionalText->strings)) {
			failed=true;
			throw std::logic_error("Regional text changed at restore publication");
		}
		if (candidate.regionalText) regionalText=candidate.regionalText;
		if (vertigoText && candidate.vertigoText &&
			(vertigoText->mapId!=candidate.vertigoText->mapId || vertigoText->strings!=candidate.vertigoText->strings)) {
			failed=true;throw std::logic_error("Vertigo text changed at restore publication");
		}
		if (candidate.vertigoText) vertigoText=candidate.vertigoText;
		if (learnedNames && candidate.learnedNames && !(*learnedNames==*candidate.learnedNames)) {
			failed=true;throw std::logic_error("Learned spell names changed at restore publication");
		}
		if (candidate.learnedNames) learnedNames=candidate.learnedNames;
		prepareMutationRanges();
		++worldRevision; ++partyReplacement; ++rosterReplacement;
	}
	void prepareVertigoPrelude(const XeenGameFlags &after) {
		check();
		flagValues=after.values();
	}
	void prepareVertigoPublication(const XeenRestoreGuard &candidate) {
		check();candidate.check();
		// Event capabilities currently permit quest flags, camera/game flags and
		// World overlays/Spawn. Every other party input must remain its preimage.
		if(candidate.membership!=membership || candidate.quests!=quests || candidate.marked!=marked ||
			candidate.recovery!=recovery || !(candidate.context==context) || candidate.treasure!=treasure ||
			candidate.economy!=economy || candidate.first!=first || candidate.effective!=effective ||
			candidate.diagnostics!=diagnostics || candidate.s._journeyRandom!=s._journeyRandom) {
			failed=true;throw std::logic_error("Event candidate changed an unowned party or RNG field");
		}
		for(unsigned owner=0;owner<characters.size();++owner) {
			if(!xeen_state::sameCharacter(candidate.characters[owner],characters[owner]) ||
				bool(candidate.inputs[owner])!=bool(inputs[owner]) ||
				(inputs[owner] && !xeen_state::sameInputs(*candidate.inputs[owner],*inputs[owner]))) {
				failed=true;throw std::logic_error("Event candidate changed character inputs without a capability");
			}
		}
		// Allocate every anticipated value before changing the retained source
		// preimage. Allocation failure remains a recoverable pre-commit failure.
		auto nextMaps=maps;auto nextObjects=objects;
		auto actors=candidate.s._actors;auto city=candidate.s._vertigoActors;
		auto accounted=candidate.s._accountedMonsters;
		auto events=candidate.s._events;auto disabledObjects=candidate.s._objects;
		auto statistics=candidate.cityStatistics;
		for (const auto &entry:candidate.maps) {
			const auto found=maps.find(entry.first);
			if (found!=maps.end() && !xeen_state::sameMap(entry.second,found->second)) {
				failed=true;throw std::logic_error("Vertigo map preimage changed");
			}
			nextMaps.emplace(entry);
		}
		for (const auto &entry:candidate.objects) {
			const auto found=objects.find(entry.first);
			if (found!=objects.end() && !xeen_state::sameObjectFile(entry.second,found->second)) {
				failed=true;throw std::logic_error("Vertigo object preimage changed");
			}
			nextObjects.emplace(entry);
		}
		mutations.prepare(mutationRangeCapacity(nextMaps,nextObjects));
		check();candidate.check();
		s._actors.swap(actors);s._vertigoActors.swap(city);
		s._accountedMonsters.swap(accounted);
		s._events.swap(events);s._objects.swap(disabledObjects);
		maps.swap(nextMaps);objects.swap(nextObjects);cityStatistics.swap(statistics);
		cityOriginalCount=candidate.cityOriginalCount;
		cameraValue=candidate.cameraValue;flagValues=candidate.flagValues;++worldRevision;
		questFlags=candidate.questFlags;
	}
	// Only the coordinator's checked, callback-free authority transitions may adopt these fields.
	// Gameplay values, identity controls and cache preimages remain retained.
	void adoptJourneyCoordination() noexcept {
		const auto &live = w._sessionState;
		s._journeyActivity = live._journeyActivity; s._journeyOwner = live._journeyOwner;
		s._journeyGeneration = live._journeyGeneration; s._combatOwner = live._combatOwner;
		s._combatApproachState = live._combatApproachState; s._combatEntered = live._combatEntered;
		s._encounterTerminal = live._encounterTerminal; s._encounterRevision = live._encounterRevision;
		combatCheck = bool(w._combatCheck); combatAuthorized = bool(w._combatAuthorized);
		adoptMutationBoundary();
	}
	// Only after the coordinator's checked, callback-free owned stores. Other
	// guards observing the same owners remain invalidated by those writes.
	void adoptMutationBoundary() noexcept {
		mutations.renew();
		refreshMutationRanges();
	}
	static std::size_t mutationRangeCapacity(const std::map<XeenMapIdentity,XeenMap> &mapValues,
			const std::map<XeenMapIdentity,XeenObjectFile> &objectValues) {
		std::size_t count=8+4*objectValues.size();
		for(const auto &entry:mapValues) count+=5+entry.second.instructions.size();
		return count;
	}
	void prepareMutationRanges() { mutations.prepare(mutationRangeCapacity(maps,objects)); }
	void refreshMutationRanges() const noexcept {
		mutations.clearRanges();
		mutations.add(&w,sizeof(w));mutations.add(&p,sizeof(p));
		mutations.add(&c,sizeof(c));mutations.add(&f,sizeof(f));
		const auto &members=p.party.activeRosterIds();
		mutations.add(members.data(),members.size()*sizeof(XeenMutable<std::uint8_t>));
		mutations.add(w._sessionState._actors.data(),w._sessionState._actors.size()*sizeof(XeenActor));
		if(w._sessionState._vertigoActors) mutations.add(w._sessionState._vertigoActors->data(),w._sessionState._vertigoActors->size()*sizeof(XeenActor));
		mutations.add(w._cityStatistics.data(),w._cityStatistics.size()*sizeof(XeenMonsterRecord));
		watchResourceMutations();
	}
	void watchResourceMutations() const noexcept {
		auto entities=[&](const XeenMapEntities &e) {
			mutations.add(e.objects.data(),e.objects.size()*sizeof(XeenMapEntity));
			mutations.add(e.monsters.data(),e.monsters.size()*sizeof(XeenMapEntity));
			mutations.add(e.wallItems.data(),e.wallItems.size()*sizeof(XeenMapEntity));
		};
		for(const auto &entry:w._maps){
			mutations.add(&entry.second,sizeof(entry.second));entities(entry.second.entities);
			mutations.add(entry.second.instructions.data(),entry.second.instructions.size()*sizeof(XeenEventInstruction));
			for(const auto &instruction:entry.second.instructions)
				mutations.add(instruction.parameters.data(),instruction.parameters.size()*sizeof(XeenMutable<std::uint8_t>));
		}
		for(const auto &entry:w._objects){mutations.add(&entry.second,sizeof(entry.second));entities(entry.second.entities);}
	}
	void adoptJourneyBorrowRelease() noexcept { for (auto &revision : borrowRevisions) ++revision; }
	const XeenWorld &w; const XeenPartyState &p; const XeenCamera &c; const XeenGameFlags &f;
	std::uint64_t worldId, worldRevision;
	std::uint64_t partyId, rosterId, partyReplacement, rosterReplacement;
	XeenSessionWorldState s;
	std::array<XeenCharacter, 30> characters;
	std::array<XeenMutableOptional<XeenCombatInputs>, 30> inputs;
	bool marked;
	std::vector<std::uint8_t> membership;
	XeenCloudsQuestItems::Counts quests;
	XeenCloudsQuestFlags::Values questFlags;
	std::optional<XeenRegionalRecoveryState> recovery;
	std::optional<XeenGameplayContext> context;
	std::optional<XeenMonsterTreasure> treasure;
	std::optional<XeenServiceEconomy> economy;
	std::uint8_t first, effective;
	std::vector<std::string> diagnostics;
	XeenCamera cameraValue;
	XeenGameFlags::Storage flagValues;
	bool combatCheck, combatAuthorized;
	std::map<XeenMapIdentity, XeenMap> maps;
	std::map<XeenMapIdentity, XeenObjectFile> objects;
	std::vector<XeenMonsterRecord> cityStatistics;
	std::uint16_t cityOriginalCount=0;
	std::optional<XeenEventTextFile> regionalText;
	std::optional<XeenEventTextFile> vertigoText;
	std::optional<XeenLearnedSpellNames> learnedNames;
	std::uint64_t cacheRevision;
	bool exactCaches;
	std::array<const XeenGameplayBorrowOwner *, 5> borrowOwners;
	std::array<std::shared_ptr<XeenGameplayBorrowOwner::State>, 5> borrowStates;
	std::array<std::uint64_t, 5> borrowRevisions;
	mutable bool failed = false;
	mutable XeenMutationWatch mutations;
};
class XeenRestoreGuard::Providers {
public:
	Providers(XeenRestoreGuard &guard, XeenWorld &world, std::function<void()> authorization = {}) :
		g(guard), w(world), maps(w._loader), objects(w._objectLoader), authorization(std::move(authorization)) {
		check();
		XeenWorld::MapLoader map = [this](XeenMapIdentity id) {
			check();
			try { auto value = maps(id); check(); g.admitMap(id, value); return value; }
			catch (...) { check(); throw; }
		};
		XeenWorld::ObjectLoader object;
		if (objects) object = [this](XeenMapIdentity id) {
			check();
			try { auto value = objects(id); check(); g.admitObjects(id, value); return value; }
			catch (...) { check(); throw; }
		};
		w._loader.swap(map); w._objectLoader.swap(object);
	}
	~Providers() { if (g.worldAlive()) { w._loader.swap(maps); w._objectLoader.swap(objects); } }
	Providers(const Providers &) = delete;
	Providers &operator=(const Providers &) = delete;
private:
	XeenRestoreGuard &g; XeenWorld &w;
	XeenWorld::MapLoader maps; XeenWorld::ObjectLoader objects;
	std::function<void()> authorization;
	void check() const { if (authorization) authorization(); g.check(); }
};
// Event execution owns intentional candidate writes between provider calls.
// Each external provider call gets a fresh guard over those current candidate
// values; these wrappers exist only for this invocation and are never copied
// into another retained world.
class XeenRestoreGuard::EventProviders {
public:
	EventProviders(XeenWorld &world, const XeenPartyState &party, const XeenCamera &camera,
			const XeenGameFlags &flags, const XeenRestoreGuard &retained,
			std::function<void()> authorization, std::function<void()> integrity) :
		w(world), p(party), c(camera), f(flags), maps(w._loader), objects(w._objectLoader),
		authorization(std::move(authorization)), integrity(std::move(integrity)), retained(retained), lifetime(w,p,c,f) {
		XeenWorld::MapLoader map=[this](XeenMapIdentity id) {
			XeenRestoreGuard guard(w,p,c,f);
			guard.retainResources(this->retained);
			check();
			try {auto value=maps(id);check();guard.check();guard.admitMap(id,value);return value;}
			catch (const std::logic_error &) {this->integrity();throw;}
			catch (...) {check();checkCandidate(guard);throw;}
		};
		XeenWorld::ObjectLoader object;
		if (objects) object=[this](XeenMapIdentity id) {
			XeenRestoreGuard guard(w,p,c,f);
			guard.retainResources(this->retained);
			check();
			try {auto value=objects(id);check();guard.check();guard.admitObjects(id,value);return value;}
			catch (const std::logic_error &) {this->integrity();throw;}
			catch (...) {check();checkCandidate(guard);throw;}
		};
		w._loader.swap(map);w._objectLoader.swap(object);
	}
	~EventProviders() {if(lifetime.worldAlive()){w._loader.swap(maps);w._objectLoader.swap(objects);}}
	EventProviders(const EventProviders &)=delete;
	EventProviders &operator=(const EventProviders &)=delete;
private:
	XeenWorld &w;const XeenPartyState &p;const XeenCamera &c;const XeenGameFlags &f;
	XeenWorld::MapLoader maps;XeenWorld::ObjectLoader objects;
	std::function<void()> authorization;
	std::function<void()> integrity;
	const XeenRestoreGuard &retained;
	XeenRestoreGuard lifetime;
	void checkCandidate(const XeenRestoreGuard &guard) const {
		try {guard.check();}catch (...) {integrity();throw;}
	}
	void check() const {if (authorization) authorization();}
};
}
#endif

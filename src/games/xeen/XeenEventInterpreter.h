#ifndef MMODERN_GAMES_XEEN_XEEN_EVENT_INTERPRETER_H
#define MMODERN_GAMES_XEEN_XEEN_EVENT_INTERPRETER_H

#include "games/xeen/XeenEventDecoder.h"
#include "games/xeen/XeenDialogView.h"
#include "games/xeen/XeenEventScript.h"
#include "games/xeen/XeenEventTextLoader.h"
#include "games/xeen/XeenEventValues.h"
#include "games/xeen/XeenGameFlags.h"
#include "games/xeen/XeenNavigation.h"
#include "games/xeen/XeenParty.h"
#include "games/xeen/XeenItemRewards.h"
#include "games/xeen/XeenItemGeneration.h"
#include "games/xeen/XeenCombatRules.h"
#include "games/xeen/XeenRecordIdentity.h"
#include "games/xeen/XeenArenaRules.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace mmodern {

class XeenWorld;

struct XeenEventExecutionAddress {
	XeenMapIdentity mapId = 0;
	int x = 0;
	int y = 0;
	int line = 0;
};

struct XeenEventExecutionCompleted {
	XeenCamera finalCamera;
	XeenGameFlags finalGameFlags;
	std::size_t instructionCount = 0;
};

enum class XeenPresentationKind {
	CenteredMessage,
	SceneLabelReduced,
	SceneLabelNormal,
	SceneLabelSign,
	SceneLabelSignReduced,
	BottomWindowMessage,
	BottomWindowTwoLines,
	MainWindowMessage,
	Confirmation,
	NpcAcknowledgment,
	CharacterSelection,
	RewardWarning,
	RewardReceipt,
	ArmorRepairService,
	TrainingService,
	TempleService,
	TavernService,
	BankService,
	GuildService,
	DeferredAudio,
	RewardGeneration,
	PortraitEffect,
	WordInput,
    EntranceScene,
    ArenaNumber,
    ArenaPreparation
};

enum class XeenPresentationResponseRequirement {
	Presented,
	Acknowledgment,
	YesNo,
	CharacterSelection,
	WordInput
};

struct SelectedCharacter { std::size_t partyIndex; };
struct CharacterSelectionCancelled {};
struct XeenWordInput { std::string text; };

struct XeenPresentationResponse {
	enum Signal {
		Presented,
		Acknowledged,
		Yes,
		No
	};
	std::variant<Signal, SelectedCharacter, CharacterSelectionCancelled, XeenWordInput> value;
	XeenPresentationResponse(Signal signal) : value(signal) {}
	XeenPresentationResponse(SelectedCharacter selection) : value(selection) {}
	XeenPresentationResponse(CharacterSelectionCancelled cancel) : value(cancel) {}
	XeenPresentationResponse(XeenWordInput input) : value(std::move(input)) {}
	bool operator==(Signal signal) const {
		const auto *actual = std::get_if<Signal>(&value);
		return actual && *actual == signal;
	}
	bool operator!=(Signal signal) const { return !(*this == signal); }
};

bool xeenResponseMatches(XeenPresentationResponseRequirement requirement,
	XeenPresentationResponse response);

struct XeenCharacterSelectionMember {
	std::size_t partyIndex;
	std::uint8_t rosterId;
	std::string name;
	bool eligible;
};

enum class XeenEventPortraitStage { Sparkle, DamageOverlay, DamageHold };
struct XeenEventPortraitEffect {
    XeenEventPortraitStage stage=XeenEventPortraitStage::Sparkle;
    unsigned owner=0,frame=0;
    bool operator==(const XeenEventPortraitEffect &other) const noexcept {return stage==other.stage && owner==other.owner && frame==other.frame;}
};
struct XeenEventPortraitProgress {
    enum class Phase { Recipients, DamageApply, DamageDone };
    unsigned mask=0,next=0;
    Phase phase=Phase::Recipients;
    bool operator==(const XeenEventPortraitProgress &other) const noexcept {return mask==other.mask && next==other.next && phase==other.phase;}
};

struct XeenPresentationRequest {
    std::optional<XeenArenaPrompt> arena;
    std::optional<std::uint8_t> entranceAction;
	XeenPresentationKind kind = XeenPresentationKind::CenteredMessage;
	XeenPresentationResponseRequirement response =
		XeenPresentationResponseRequirement::Presented;
	XeenMapIdentity mapId = 0;
	std::optional<std::uint8_t> textIndex;
	std::string text;
	std::optional<std::uint8_t> layoutValue;
	XeenEventSourceLocation source;
	std::vector<XeenCharacterSelectionMember> members;
	std::optional<std::uint8_t> verbIndex;
	std::string refusal;
	std::optional<XeenEventNpc> npc;
	std::optional<XeenEventCdSpeech> cdSpeech;
	bool npcBodyHidden=false;
	std::string speechNotice;
	std::string title;
	std::optional<XeenTreasureDisplay> treasure;
	unsigned rewardWaitTicks=0;
    std::optional<XeenEventPortraitEffect> portrait;
	// Party::subtract's refusal (0x1e8bb): consumable 0 gold, 1 gems, 2 food.
	std::optional<std::uint8_t> notEnough;
	// giveExt's lock message (0x236b3, 0x236e9) for the member named in title.
	std::optional<bool> lockPicked;
	std::optional<XeenEventConfirmWord> word;
	std::string alternate;
	std::vector<XeenMirrorDestination> mirrors;
};

enum class XeenEventMissingInstructionPolicy {
	NaturalCompletion,
	ExplicitJump,
	ExplicitCall
};

enum class XeenEventPendingContinuation {
	Advance,
	Terminate,
	ConditionalAction44,
	WhoWill,
	RewardFinalization,
	GiveMulti,
	PortraitEffect,
	NpcChoice,
	ConfirmWord
};

struct XeenEventPendingPresentation {
	XeenPresentationRequest request;
	XeenEventPendingContinuation continuation = XeenEventPendingContinuation::Advance;
	std::optional<XeenEventConditional> conditional;
};

struct XeenEventCallFrame {
	XeenEventExecutionAddress returnAddress;
};

// giveExt's lock picking (0x235ac..0x23726): trap, then the attempt and its message.
struct XeenEventLockProgress {
	enum class Phase { Trap, Attempt, Message };
	Phase phase = Phase::Trap;
	std::size_t member = 0;
	XeenObjectIdentity object;
	bool picked = false;
	bool operator==(const XeenEventLockProgress &o) const noexcept {
		return phase==o.phase && member==o.member && object==o.object && picked==o.picked;
	}
};

// A TakeOrGive member loop (0x275f4..0x2790d) suspended by a member's sparkle.
struct XeenEventGiveTakeProgress {
	enum class Phase { Member, BeforeHp, AfterMember };
	Phase phase = Phase::Member;
	unsigned member = 0;
	std::uint8_t takeMode = 0;  // the loop clears it after mode 8 and 82 (0x27877, 0x278b4)
	bool flag = false;           // giveTake's result for the current member
	bool operator==(const XeenEventGiveTakeProgress &o) const noexcept {
		return phase==o.phase && member==o.member && takeMode==o.takeMode && flag==o.flag;
	}
};

struct XeenEventExecutionState {
    std::optional<XeenEventPortraitProgress> portraitProgress;
    std::optional<XeenTypedDamageCandidate> portraitDamage;
    std::optional<XeenCombatRandom> portraitRandom;
    // Original per-member sparkle latch: cleared only when an Event run starts or ends.
    std::uint32_t sparkledOwners=0;
	std::optional<XeenEventCdSpeech> cdSpeech;
	std::optional<XeenCombatRandom> rewardRandom;
	std::optional<XeenItemCandidate> rewardItem;
	unsigned rewardPair=0;
	bool generatingReward=false;
	// Event processing (0x26774) stashes the pending treasure when its currency or
	// readiness is set and clears only those; Event treasure stages into the same
	// arrays, is delivered after the Events and the stash is restored over it.
	XeenTreasureStaging treasure, stash;
	bool stashed=false;
	// The run's entry clear (0x26797) is part of the base; WhoWill selections
	// are checked apart from the arrays, so they never count as treasure.
	XeenTreasureStaging treasureBase() const { auto base=stash;base.clearPending();base.whoWill=0;return base; }
	bool treasureChanged() const { auto live=treasure;live.whoWill=0;return live!=treasureBase(); }
	XeenTreasureDelivery reward;
	XeenEventExecutionAddress logicalAddress;
	XeenDirection lookupDirection = XeenDirection::North;
	XeenCamera workingCamera;
	bool continuedAtDestination=false; // DOS redraws before restarting destination Events.
	std::optional<XeenObjectIdentity> selectedObject;
	// Temporary active-party index; independent of roster IDs and call frames.
	std::optional<std::size_t> activeCharacterIndex;
	bool entireParty=false; // checkEvents starts at member 1; SetChar(0) selects all.
	XeenEventScriptValues scriptValues;
	std::optional<XeenMirrorDestination> mirrorDestination;
	std::optional<XeenEventGiveTakeProgress> giveTake;
	std::optional<XeenEventLockProgress> lock;
	XeenGameFlags workingGameFlags;
	std::optional<XeenEventScript> currentScript;
	std::vector<XeenEventCallFrame> callStack;
	std::size_t instructionCount = 0;
	XeenEventMissingInstructionPolicy missingInstructionPolicy =
		XeenEventMissingInstructionPolicy::NaturalCompletion;
	std::optional<XeenEventSourceLocation> pendingTransferSource;
	std::optional<XeenEventPendingPresentation> pendingPresentation;
};

struct XeenEventExecutionSuspended {
	XeenEventExecutionState state;
	XeenPresentationRequest request;
};

enum class XeenEventExecutionErrorKind {
	InvalidInitialCamera,
	ScriptLoadFailed,
	ScriptMapMismatch,
	MalformedInstruction,
	UnsupportedOpcode,
	UnsupportedOperand,
	LineOverflow,
	UnsupportedConditionAction,
	UnsupportedOperationMode,
	QuestItemOverflow,
	QuestItemUnderflow,
	EmptyParty,
	InvalidFlagIndex,
	InvalidJumpTarget,
	InvalidCallTarget,
	InvalidReturn,
	CallStackOverflow,
	UnsupportedTeleportDestination,
	MapLoadFailed,
	ObjectLoadFailed,
	InvalidRemoveContext,
	UnsupportedExecutionContext,
	InstructionLimitExceeded,
	MissingTextResource,
	TextMapMismatch,
	InvalidTextIndex,
	InvalidPresentationResponse,
	PresentationRequired,
	PresentationFailed
};

struct XeenEventExecutionError {
	XeenEventExecutionErrorKind kind =
		XeenEventExecutionErrorKind::InvalidInitialCamera;
	std::string message;
	std::size_t instructionCount = 0;
	XeenEventExecutionAddress logicalAddress;
	std::optional<XeenEventSourceLocation> source;
	std::optional<XeenEventExecutionAddress> requestedTarget;
	XeenRewardReceipt rewards;
};

using XeenEventExecutionResult = std::variant<
	XeenEventExecutionCompleted,
	XeenEventExecutionError>;

using XeenEventExecutionStepResult = std::variant<
	XeenEventExecutionCompleted,
	XeenEventExecutionSuspended,
	XeenEventExecutionError>;

class XeenEventPublication;
class XeenEventInterpreter {
public:
    std::function<void(unsigned)> fmEffect; // Optional presentation sink for detached Event execution.
    std::function<void(unsigned)> eventVoice;
	static constexpr std::size_t kMaximumInstructions = 1024;
	static constexpr std::size_t kMaximumCallDepth = 64;

	using ScriptProvider = std::function<XeenEventScript(XeenMapIdentity mapId)>;
	using TextProvider = std::function<XeenEventTextFile(XeenMapIdentity mapId)>;

	XeenEventExecutionResult execute(const XeenCamera &initialCamera,
		XeenPartyState &partyState, const XeenGameFlags &gameFlags,
		XeenWorld &world, const ScriptProvider &scriptProvider) const;

	// Integration checkpoints may specify a line; all gameplay callers use zero.
	XeenEventExecutionStepResult begin(const XeenCamera &initialCamera,
		XeenPartyState &partyState, const XeenGameFlags &gameFlags,
		XeenWorld &world, const ScriptProvider &scriptProvider,
		const TextProvider &textProvider, std::uint8_t initialLine = 0, const XeenEventPublication *publication = nullptr) const;

	XeenEventExecutionStepResult resume(XeenEventExecutionState state,
		XeenPresentationResponse response, XeenPartyState &partyState,
		XeenWorld &world, const ScriptProvider &scriptProvider,
		const TextProvider &textProvider, const XeenEventPublication *publication = nullptr) const;

private:
    void emitFmEffect(const XeenEventPublication *,unsigned) const noexcept;
	XeenEventExecutionStepResult runInstructions(XeenEventExecutionState &state,
		std::optional<XeenPresentationResponse> response,
		XeenPartyState &partyState, XeenWorld &world,
		const ScriptProvider &scriptProvider, const TextProvider &textProvider, const XeenEventPublication *publication = nullptr) const;
	XeenEventExecutionStepResult run(XeenEventExecutionState state,
		std::optional<XeenPresentationResponse> response,
		XeenPartyState &partyState, XeenWorld &world,
		const ScriptProvider &scriptProvider, const TextProvider &textProvider, const XeenEventPublication *publication = nullptr) const;
};

} // namespace mmodern

#endif

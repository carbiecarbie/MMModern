#include "app/XeenEventFlow.h"
#include "games/xeen/XeenCharacterRules.h"

#include <algorithm>
#include <sstream>

namespace mmodern {
namespace {
std::vector<std::size_t> learnedRows(const XeenCharacter &character) {
	std::vector<std::size_t> rows;
	for (std::size_t slot=0;slot<39;++slot)
		if (XeenLearnedSpellRules::known(character,slot)) rows.push_back(slot);
	return rows;
}
}

bool XeenEventFlow::castingCasterEligible(std::size_t active) const {
	if (active>=_party.party.size()) return false;
	const auto &character=_party.party.member(_party.roster,active);
	const auto category=XeenLearnedSpellRules::categoryForClass(character.characterClass);
	if (!character.hasSpells || !character.learnedSpells || !category || !character.canAct() || character.currentSp<1)
		return false;
	for (std::size_t slot=0;slot<39;++slot)
		if (XeenLearnedSpellRules::eligible(_party,active,slot)) return true;
	return false;
}

std::string XeenEventFlow::castingText() const {
	if (!_castingUi) return {};
	const auto &ui=*_castingUi;
	std::ostringstream out;
	switch (ui.phase) {
	case CastingUi::Phase::ChooseCaster:
		out<<"Cast: F1-F6 choose caster\n";
		for (std::size_t index=0;index<_party.party.size();++index) {
			const auto &c=_party.party.member(_party.roster,index);
			out<<"F"<<index+1<<' '<<c.name<<" SP"<<c.currentSp;
			if (!castingCasterEligible(index)) {
				if (!c.hasSpells || !c.learnedSpells) out<<" No book";
				else if (!c.canAct()) out<<" Unable";
				else if (c.currentSp<1) out<<" No SP";
				else out<<" No supported spell";
			}
			out<<'\n';
		}
		out<<"Esc closes";
		break;
	case CastingUi::Phase::BrowseLearned: {
		const auto &c=_party.party.member(_party.roster,ui.caster);
		const auto category=XeenLearnedSpellRules::categoryForClass(c.characterClass);
		const auto rows=learnedRows(c);
		out<<c.name<<" spells Up/Down\n";
		for (std::size_t index=ui.scroll;index<rows.size() && index<ui.scroll+6;++index) {
			const auto slot=rows[index];
			const auto id=category ? XeenLearnedSpellRules::spellForSlot(*category,slot) : std::nullopt;
			out<<(slot==ui.slot?">":" ")<<slot<<' ';
			if (id && _encounter->journeySavePreimage().learnedNames)
				out<<_encounter->journeySavePreimage().learnedNames->names[*id];
			else out<<"Unknown";
			if (!id || !XeenLearnedSpellRules::supported(*id)) out<<" Not supported";
			out<<'\n';
		}
		out<<"Enter confirms; F1-F6 switch; Esc";
		break;
	}
	case CastingUi::Phase::ConfirmCast: {
		const auto &c=_party.party.member(_party.roster,ui.caster);
		const auto category=XeenLearnedSpellRules::categoryForClass(c.characterClass);
		const auto id=category ? XeenLearnedSpellRules::spellForSlot(*category,ui.slot) : std::nullopt;
		out<<"Confirm "<<c.name<<' ';
		if (id && _encounter->journeySavePreimage().learnedNames)
			out<<_encounter->journeySavePreimage().learnedNames->names[*id];
		out<<"\nCost 1 SP / 0 gems\n";
		out<<(id && *id==26 ? "Single member" : "Whole party")<<"; 10 minutes\n";
		if (id && *id==26) out<<"Target Esc refunds SP; time spent\n";
		out<<"Enter casts; Esc returns";
		break;
	}
	case CastingUi::Phase::ChooseTarget:
		out<<"First Aid: F1-F6 target\n";
		for (std::size_t index=0;index<_party.party.size();++index) {
			const auto &c=_party.party.member(_party.roster,index);
			out<<"F"<<index+1<<' '<<c.name<<" HP"<<c.currentHp<<'/'<<
				XeenCharacterRules::maxHp(c,{_party.encounterContext->year});
			if (c.conditions[13] || c.conditions[14] || c.conditions[15]) out<<" Fails";
			else if (c.conditions[8]) out<<" Sleep";
			out<<'\n';
		}
		out<<"Esc refunds SP; spends time";
		break;
	case CastingUi::Phase::Settling:
		out<<_encounter->castingResult()<<"\nTen minutes owed; actor work pending; Esc waits";
		break;
	}
	if (!ui.refusal.empty()) out<<'\n'<<ui.refusal;
	return out.str();
}

IndexedFrame XeenEventFlow::handleCasting(const PlayerAction &action, std::uint64_t input) {
	if (!_castingUi || !_encounter->castingFrameCurrent(input,_frame.presentation())) return frameCopy();
	auto &ui=*_castingUi;
	if (ui.phase==CastingUi::Phase::Settling) return frameCopy();
	const auto *member=std::get_if<SelectMemberAction>(&action);
	const auto *navigation=std::get_if<NavigationAction>(&action);
	const bool enter=std::holds_alternative<AcknowledgeAction>(action);
	const bool escape=std::holds_alternative<CancelInteractionAction>(action);
	if (ui.phase==CastingUi::Phase::ChooseCaster) {
		if (escape) {
			if (!_encounter->cancelCasting(_encounter->ticket(),input,_frame.presentation())) return frameCopy();
			_castingUi.reset();return renderEncounter();
		}
		if (!member || member->partyIndex>=_party.party.size()) return frameCopy();
		if (!castingCasterEligible(member->partyIndex)) { ui.refusal="Caster unavailable";return renderEncounter(); }
		ui.caster=member->partyIndex;
		const auto &c=_party.party.member(_party.roster,ui.caster);
		ui.slot=learnedRows(c).front();ui.scroll=0;ui.refusal.clear();
		ui.phase=CastingUi::Phase::BrowseLearned;return renderEncounter();
	}
	if (ui.phase==CastingUi::Phase::BrowseLearned) {
		if (escape) { ui.phase=CastingUi::Phase::ChooseCaster;ui.refusal.clear();return renderEncounter(); }
		if (member) {
			if (!castingCasterEligible(member->partyIndex)) { ui.refusal="Caster unavailable";return renderEncounter(); }
			ui.caster=member->partyIndex;ui.slot=learnedRows(_party.party.member(_party.roster,ui.caster)).front();
			ui.scroll=0;ui.refusal.clear();return renderEncounter();
		}
		const auto &rows=learnedRows(_party.party.member(_party.roster,ui.caster));
		const auto found=std::find(rows.begin(),rows.end(),ui.slot);
		if (found==rows.end()) { closeGameplay();throw std::logic_error("Casting book selection changed"); }
		const auto position=static_cast<std::size_t>(found-rows.begin());
		if (navigation && (*navigation==NavigationAction::MoveForward || *navigation==NavigationAction::MoveBackward)) {
			const auto next=*navigation==NavigationAction::MoveForward ? (position?position-1:position) :
				std::min(position+1,rows.size()-1);
			if (next==position) return frameCopy();
			ui.slot=rows[next];
			if (next<ui.scroll) ui.scroll=next;
			else if (next>=ui.scroll+6) ui.scroll=next-5;
			ui.refusal.clear();return renderEncounter();
		}
		if (enter) {
			if (!XeenLearnedSpellRules::eligible(_party,ui.caster,ui.slot)) {
				ui.refusal="Not supported or insufficient SP";return renderEncounter();
			}
			ui.phase=CastingUi::Phase::ConfirmCast;ui.refusal.clear();return renderEncounter();
		}
		return frameCopy();
	}
	if (ui.phase==CastingUi::Phase::ConfirmCast) {
		if (escape) { ui.phase=CastingUi::Phase::BrowseLearned;return renderEncounter(); }
		if (!enter) return frameCopy();
		if (!_encounter->confirmCasting(_encounter->ticket(),ui.caster,ui.slot,input,_frame.presentation())) {
			ui.refusal="Cast refused: eligibility, SP or time";return renderEncounter();
		}
		const auto &c=_party.party.member(_party.roster,ui.caster);
		const auto category=XeenLearnedSpellRules::categoryForClass(c.characterClass);
		const auto id=category ? XeenLearnedSpellRules::spellForSlot(*category,ui.slot) : std::nullopt;
		if (!id) { closeGameplay();throw std::logic_error("Committed spell identity disappeared"); }
		if (*id==1) {
			if (!_encounter->publishAwaken(_encounter->ticket())) {closeGameplay();throw std::logic_error("Awaken publication failed");}
			ui.phase=CastingUi::Phase::Settling;
		} else ui.phase=CastingUi::Phase::ChooseTarget;
		ui.refusal.clear();return renderEncounter();
	}
	if (ui.phase==CastingUi::Phase::ChooseTarget) {
		if (!escape && (!member || member->partyIndex>=_party.party.size())) return frameCopy();
		if (!_encounter->respondCastingTarget(_encounter->ticket(),escape ? std::nullopt :
			std::optional<std::size_t>{member->partyIndex},input,_frame.presentation())) return frameCopy();
		ui.phase=CastingUi::Phase::Settling;return renderEncounter();
	}
	return frameCopy();
}

} // namespace mmodern

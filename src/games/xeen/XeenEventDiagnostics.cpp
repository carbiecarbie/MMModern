#include "games/xeen/XeenEventDiagnostics.h"

#include "games/xeen/XeenEventScript.h"

#include <array>
#include <iomanip>
#include <sstream>

namespace mmodern {
namespace {

constexpr std::array<const char *, 0x3d> kOpcodeNames{{
	"None", "Display0x01", "DoorTextSml", "DoorTextLrg", "SignText",
	"NPC", "PlayFX", "TeleportAndExit", "If1", "If2", "If3", "MoveObj",
	"TakeOrGive", "NoAction", "Remove", "SetChar", "Spawn", "DoTownEvent",
	"Exit", "AfterMap", "GiveMulti", "ConfirmWord", "Damage", "JumpRnd",
	"AfterEvent", "CallEvent", "Return", "SetVar", "TakeOrGive_2",
	"TakeOrGive_3", "CutsceneEndClouds", "TeleportAndContinue", "WhoWill",
	"RndDamage", "MoveWallObj", "AlterCellFlag", "AlterHed", "DisplayStat",
	"TakeOrGive_4", "SeatTextSml", "PlayEventVoc", "DisplayBottom", "IfMapFlag",
	"SelectRandomChar", "GiveEnchanted", "ItemType", "MakeNothingHere",
	"NoAction_2", "ChooseNumeric", "DisplayBottomTwoLines", "DisplayLarge",
	"ExchObj", "FallToMap", "DisplayMain", "Goto", "ConfirmWord_2",
	"GotoRandom", "CutsceneEndDarkside", "CutsceneEdWorld", "FlipWorld", "PlayCD"
}};

std::string hexByte(std::uint8_t value) {
	std::ostringstream output;
	output << std::uppercase << std::hex << std::setfill('0') << std::setw(2)
		<< static_cast<unsigned>(value);
	return output.str();
}

void writeRecord(std::ostringstream &output, const XeenEventRecord &record) {
	output << '\n' << '@' << record.fileOffset
		<< " length=" << static_cast<unsigned>(record.lengthField)
		<< " X=" << static_cast<unsigned>(record.x)
		<< " Y=" << static_cast<unsigned>(record.y)
		<< " dir=" << XeenEventDiagnostics::directionName(record.direction);
	if (record.direction <= kXeenEventDirectionAll)
		output << '(' << static_cast<unsigned>(record.direction) << ')';
	output << " line=" << static_cast<unsigned>(record.line) << '\n'
		<< "opcode=0x" << hexByte(record.opcode) << ' '
		<< XeenEventDiagnostics::opcodeName(record.opcode) << '\n'
		<< "params=";
	if (record.parameters.empty()) {
		output << "[]";
	} else {
		for (std::size_t i = 0; i < record.parameters.size(); ++i) {
			if (i)
				output << ' ';
			output << hexByte(record.parameters[i]);
		}
	}
	output << '\n';
}

bool matchesFilter(const XeenEventRecord &record,
		const XeenEventDiagnosticFilter &filter) {
	if (record.x != filter.x || record.y != filter.y)
		return false;
	switch (filter.directionFilter) {
	case XeenEventDirectionFilter::Any:
		return true;
	case XeenEventDirectionFilter::Physical:
		return record.direction == static_cast<std::uint8_t>(filter.direction) ||
			record.direction == kXeenEventDirectionAll;
	case XeenEventDirectionFilter::AllOnly:
		return record.direction == kXeenEventDirectionAll;
	}
	return false;
}

void writeDuplicates(std::ostringstream &output, const XeenEventScript &script) {
	const auto duplicates = script.duplicateKeys();
	output << "\nDuplicatas exatas: " << duplicates.size() << '\n';
	for (const XeenEventDuplicateKey &duplicate : duplicates) {
		output << "X=" << static_cast<unsigned>(duplicate.x)
			<< " Y=" << static_cast<unsigned>(duplicate.y)
			<< " dir=" << XeenEventDiagnostics::directionName(duplicate.direction)
			<< " line=" << static_cast<unsigned>(duplicate.line)
			<< " first=@" << duplicate.firstOffset
			<< " duplicate=@" << duplicate.duplicateOffset << '\n';
	}
}

} // namespace

std::string XeenEventDiagnostics::opcodeName(std::uint8_t opcode) {
	if (opcode < kOpcodeNames.size())
		return kOpcodeNames[opcode];
	return "Unknown(0x" + hexByte(opcode) + ")";
}

std::string XeenEventDiagnostics::directionName(std::uint8_t direction) {
	static constexpr std::array<const char *, 5> names{{
		"North", "East", "South", "West", "All"
	}};
	if (direction < names.size())
		return names[direction];
	return "Unknown(" + std::to_string(static_cast<unsigned>(direction)) + ")";
}

std::string XeenEventDiagnostics::format(const XeenEventFile &eventFile) {
	std::ostringstream output;
	output << "Mapa: " << eventFile.mapId << '\n'
		<< "Recurso: " << eventFile.resourceName << '\n'
		<< "Presente: " << (eventFile.resourcePresent ? "sim" : "nao") << '\n'
		<< "Registros: " << eventFile.records.size() << '\n';

	for (const XeenEventRecord &record : eventFile.records)
		writeRecord(output, record);

	return output.str();
}

std::string XeenEventDiagnostics::format(const XeenEventScript &script,
		const std::optional<XeenEventDiagnosticFilter> &filter) {
	if (!filter) {
		std::ostringstream output;
		output << format(script.file());
		writeDuplicates(output, script);
		return output.str();
	}

	const XeenEventFile &eventFile = script.file();
	std::ostringstream output;
	output << "Mapa: " << eventFile.mapId << '\n'
		<< "Recurso: " << eventFile.resourceName << '\n'
		<< "Presente: " << (eventFile.resourcePresent ? "sim" : "nao") << '\n'
		<< "Registros totais: " << eventFile.records.size() << '\n'
		<< "\nPosition: X=" << static_cast<unsigned>(filter->x)
		<< " Y=" << static_cast<unsigned>(filter->y) << '\n';

	switch (filter->directionFilter) {
	case XeenEventDirectionFilter::Any:
		output << "Direction: not supplied\n";
		break;
	case XeenEventDirectionFilter::Physical:
		output << "Direction: "
			<< directionName(static_cast<std::uint8_t>(filter->direction)) << '\n';
		break;
	case XeenEventDirectionFilter::AllOnly:
		output << "Direction: All records only\n";
		break;
	}
	if (filter->automatic)
		output << "Automatic: " << (*filter->automatic ? "yes" : "no") << '\n';
	else
		output << "Automatic: n/a\nLogical address: yes\n";

	std::size_t displayed = 0;
	for (const XeenEventRecord &record : eventFile.records) {
		if (matchesFilter(record, *filter))
			++displayed;
	}
	output << "Registros exibidos: " << displayed << '\n';
	for (const XeenEventRecord &record : eventFile.records) {
		if (matchesFilter(record, *filter))
			writeRecord(output, record);
	}

	output << "\nSelected line 0:";
	if (filter->directionFilter != XeenEventDirectionFilter::Physical) {
		output << " n/a - physical direction not supplied\n";
	} else if (const XeenEventRecord *selected = script.findInstruction(
			filter->x, filter->y, filter->direction, 0)) {
		writeRecord(output, *selected);
	} else {
		output << " none\n";
	}
	writeDuplicates(output, script);
	return output.str();
}

} // namespace mmodern

#include "games/xeen/XeenEventDecoder.h"

#include <sstream>

namespace mmodern {
namespace {

class ParameterReader {
public:
	explicit ParameterReader(const std::vector<std::uint8_t> &parameters) :
		_parameters(parameters) {}

	bool readUint8(std::uint8_t &value) {
		if (_position >= _parameters.size())
			return false;
		value = _parameters[_position++];
		return true;
	}

	bool readSignedByte(int &value) {
		std::uint8_t byte = 0;
		if (!readUint8(byte))
			return false;
		value = byte < 0x80 ? static_cast<int>(byte) : static_cast<int>(byte) - 0x100;
		return true;
	}

	bool readUint16LE(std::uint16_t &value) {
		if (_parameters.size() - _position < 2)
			return false;
		value = static_cast<std::uint16_t>(_parameters[_position]) |
			(static_cast<std::uint16_t>(_parameters[_position + 1]) << 8);
		_position += 2;
		return true;
	}

	bool readUint32LE(std::uint32_t &value) {
		if (_parameters.size() - _position < 4)
			return false;
		value = static_cast<std::uint32_t>(_parameters[_position]) |
			(static_cast<std::uint32_t>(_parameters[_position + 1]) << 8) |
			(static_cast<std::uint32_t>(_parameters[_position + 2]) << 16) |
			(static_cast<std::uint32_t>(_parameters[_position + 3]) << 24);
		_position += 4;
		return true;
	}

	bool atEnd() const { return _position == _parameters.size(); }

private:
	const std::vector<std::uint8_t> &_parameters;
	std::size_t _position = 0;
};

XeenEventSourceLocation makeSource(const XeenEventRecord &record,
		const XeenEventDecodeContext &context) {
	return {context.mapId, context.resourceName, record.fileOffset, record.x,
		record.y, record.direction, record.line, record.opcode, context.recordIndex};
}

XeenEventDecodeError makeError(const XeenEventRecord &record,
		const XeenEventDecodeContext &context, XeenEventDecodeErrorKind kind,
		std::string message, std::optional<std::size_t> expected = std::nullopt) {
	return {kind, makeSource(record, context), std::move(message), expected,
		record.parameters.size()};
}

XeenEventDecodeError wrongSize(const XeenEventRecord &record,
		const XeenEventDecodeContext &context, std::size_t expected) {
	std::ostringstream message;
	message << "expected " << expected << " parameter bytes, found "
		<< record.parameters.size();
	return makeError(record, context,
		XeenEventDecodeErrorKind::MalformedInstruction, message.str(), expected);
}

XeenDecodedEventInstruction instruction(const XeenEventRecord &record,
		const XeenEventDecodeContext &context, XeenDecodedEventOperation operation) {
	return {makeSource(record, context), std::move(operation)};
}

bool usesUint32(std::uint8_t action) {
	return action == 16 || action == 34 || action == 100;
}

bool usesUint16(std::uint8_t action) {
	return action == 25 || action == 35 || action == 101 || action == 106;
}

bool readUnsignedValue(ParameterReader &reader, std::uint8_t mode,
		std::uint32_t &value) {
	if (usesUint32(mode))
		return reader.readUint32LE(value);
	if (usesUint16(mode)) {
		std::uint16_t value16 = 0;
		if (!reader.readUint16LE(value16))
			return false;
		value = value16;
		return true;
	}
	std::uint8_t value8 = 0;
	if (!reader.readUint8(value8))
		return false;
	value = value8;
	return true;
}

XeenEventDecodeResult decodeEmpty(const XeenEventRecord &record,
		const XeenEventDecodeContext &context, XeenDecodedEventOperation operation) {
	if (!record.parameters.empty())
		return wrongSize(record, context, 0);
	return instruction(record, context, std::move(operation));
}

XeenEventDecodeResult decodeDisplay(const XeenEventRecord &record,
		const XeenEventDecodeContext &context, XeenEventDisplayKind kind) {
	const std::size_t expected = kind == XeenEventDisplayKind::BottomWindowTwoLines ? 2 : 1;
	if (record.parameters.size() != expected)
		return wrongSize(record, context, expected);
	XeenEventDisplay display;
	display.kind = kind;
	if (expected == 2) {
		display.layoutValue = record.parameters[0];
		display.textIndex = record.parameters[1];
	} else {
		display.textIndex = record.parameters[0];
	}
	return instruction(record, context, display);
}

XeenEventDecodeResult decodeTeleport(const XeenEventRecord &record,
		const XeenEventDecodeContext &context, bool exits) {
	if (record.parameters.empty())
		return wrongSize(record, context, 1);

	ParameterReader reader(record.parameters);
	std::uint8_t mapId = 0;
	reader.readUint8(mapId);
	if (mapId == 0) {
		return makeError(record, context,
			XeenEventDecodeErrorKind::UnsupportedOperand,
			"mirror teleport is outside the supported decoder subset");
	}
	if (record.parameters.size() != 3)
		return wrongSize(record, context, 3);

	int x = 0;
	int y = 0;
	if (!reader.readSignedByte(x) || !reader.readSignedByte(y))
		return wrongSize(record, context, 3);
	if (exits)
		return instruction(record, context, XeenEventTeleportAndExit{mapId, x, y});
	return instruction(record, context, XeenEventTeleportAndContinue{mapId, x, y});
}

XeenEventDecodeResult decodeCallEvent(const XeenEventRecord &record,
		const XeenEventDecodeContext &context) {
	if (record.parameters.size() != 3)
		return wrongSize(record, context, 3);
	ParameterReader reader(record.parameters);
	int x = 0;
	int y = 0;
	std::uint8_t line = 0;
	if (!reader.readSignedByte(x) || !reader.readSignedByte(y) || !reader.readUint8(line))
		return wrongSize(record, context, 3);
	// Signed call targets remain values only. Converting them to the raw lookup
	// byte domain belongs to the future interpreter, not this decoder.
	return instruction(record, context, XeenEventCallEvent{x, y, line});
}

XeenEventDecodeResult decodeConditional(const XeenEventRecord &record,
		const XeenEventDecodeContext &context, XeenEventComparison comparison) {
	if (record.parameters.empty())
		return wrongSize(record, context, 1);

	const std::uint8_t action = record.parameters[0];
	const std::size_t expected = usesUint32(action) ? 6 :
		(usesUint16(action) ? 4 : 3);
	if (record.parameters.size() != expected)
		return wrongSize(record, context, expected);

	ParameterReader reader(record.parameters);
	std::uint8_t decodedAction = 0;
	std::uint32_t value = 0;
	std::uint8_t targetLine = 0;
	reader.readUint8(decodedAction);
	if (usesUint32(action)) {
		if (!reader.readUint32LE(value))
			return wrongSize(record, context, expected);
	} else if (usesUint16(action)) {
		std::uint16_t value16 = 0;
		if (!reader.readUint16LE(value16))
			return wrongSize(record, context, expected);
		value = value16;
	} else {
		std::uint8_t value8 = 0;
		if (!reader.readUint8(value8))
			return wrongSize(record, context, expected);
		value = value8;
	}
	if (!reader.readUint8(targetLine))
		return wrongSize(record, context, expected);
	return instruction(record, context,
		XeenEventConditional{comparison, decodedAction, value, targetLine});
}

XeenEventDecodeResult decodeTakeOrGive(const XeenEventRecord &record,
		const XeenEventDecodeContext &context) {
	ParameterReader reader(record.parameters);
	XeenEventTakeOrGive result;
	XeenEventTakeOrGivePair *pairs[] = {
		&result.first, &result.second, &result.third
	};

	for (std::size_t index = 0; index < 3; ++index) {
		if (reader.atEnd())
			break;
		if (!reader.readUint8(pairs[index]->mode) ||
				!readUnsignedValue(reader, pairs[index]->mode, pairs[index]->value)) {
			std::ostringstream message;
			message << "truncated TakeOrGive pair " << (index + 1)
				<< ", found " << record.parameters.size() << " parameter bytes";
			return makeError(record, context,
				XeenEventDecodeErrorKind::MalformedInstruction, message.str());
		}
	}

	if (!reader.atEnd()) {
		std::ostringstream message;
		message << "extra bytes after third TakeOrGive pair, found "
			<< record.parameters.size() << " parameter bytes";
		return makeError(record, context,
			XeenEventDecodeErrorKind::MalformedInstruction, message.str());
	}
	return instruction(record, context, result);
}

} // namespace

XeenEventDecodeResult XeenEventDecoder::decode(const XeenEventRecord &record,
		XeenEventDecodeContext context) {
	switch (record.opcode) {
	case 0x05:
		if (record.parameters.size() != 5) return wrongSize(record, context, 5);
		return instruction(record, context, XeenEventNpc{record.parameters[0],
			record.parameters[1], record.parameters[2], record.parameters[3], record.parameters[4]});
	case 0x20:
		if (record.parameters.size() != 2)
			return wrongSize(record, context, 2);
		return instruction(record, context,
			XeenEventWhoWill{record.parameters[0], record.parameters[1]});
	case 0x00:
		// None retains old operands when an event is disabled by Remove.
		return instruction(record, context, XeenEventNone{});
	case 0x0e:
		return decodeEmpty(record, context, XeenEventRemove{});
	case 0x01:
		return decodeDisplay(record, context, XeenEventDisplayKind::Centered);
	case 0x02:
		return decodeDisplay(record, context, XeenEventDisplayKind::DoorLabelReduced);
	case 0x03:
		return decodeDisplay(record, context, XeenEventDisplayKind::DoorLabelNormal);
	case 0x04:
		return decodeDisplay(record, context, XeenEventDisplayKind::SignLabel);
	case 0x07:
		return decodeTeleport(record, context, true);
	case 0x08:
		return decodeConditional(record, context, XeenEventComparison::GreaterOrEqual);
	case 0x09:
		return decodeConditional(record, context, XeenEventComparison::Equal);
	case 0x0a:
		return decodeConditional(record, context, XeenEventComparison::LessOrEqual);
	case 0x0c:
		return decodeTakeOrGive(record, context);
	case 0x12:
		return decodeEmpty(record, context, XeenEventExit{});
	case 0x19:
		return decodeCallEvent(record, context);
	case 0x1a:
		return decodeEmpty(record, context, XeenEventReturn{});
	case 0x1f:
		return decodeTeleport(record, context, false);
	case 0x29:
		return decodeDisplay(record, context, XeenEventDisplayKind::BottomWindow);
	case 0x31:
		return decodeDisplay(record, context, XeenEventDisplayKind::BottomWindowTwoLines);
	case 0x35:
		return decodeDisplay(record, context, XeenEventDisplayKind::MainWindow);
	default: {
		std::ostringstream message;
		message << "opcode " << static_cast<unsigned int>(record.opcode)
			<< " is outside the supported decoder subset";
		return makeError(record, context,
			XeenEventDecodeErrorKind::UnsupportedOpcode, message.str());
	}
	}
}

} // namespace mmodern

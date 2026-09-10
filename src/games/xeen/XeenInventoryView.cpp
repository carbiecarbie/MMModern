#include "games/xeen/XeenInventoryView.h"
#include "games/xeen/XeenCharacterRules.h"
#include "games/xeen/XeenPartyLoader.h"
#include <stdexcept>

namespace mmodern {
namespace {
std::string literal(std::string text) {
	for (char &c : text) if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) c = '?';
	return text;
}
}
std::vector<XeenInventoryLine> xeenInventoryLayout(const XeenFontFormat &font,
		const XeenItemCatalog &catalog, const XeenPartyState &party,
		const XeenInventorySelection &selection, const char *feedback) {
	XeenTextRenderer renderer(font);
	std::vector<XeenInventoryLine> lines;
	const auto line = [&](int x, int right, int y, std::string text, bool elide = false) {
		text = literal(std::move(text));
		// Advance width is not glyph extent: reserve the final eight-pixel glyph.
		const int width = right - x - 7;
		if (renderer.textWidth(text, XeenFontSize::Reduced) > width) {
			if (!elide) throw std::runtime_error("Inventory numeric/identity field does not fit");
			while (!text.empty() && renderer.textWidth(text + "...", XeenFontSize::Reduced) > width) text.pop_back();
			text += "...";
		}
		lines.push_back({{x,y,right,y+9}, std::move(text)});
	};
	const auto selectedName = [&](const std::string &displayName) {
		constexpr XeenTextRect area{154,44,310,62};
		constexpr int lineHeight = 9;
		constexpr int lineBudget = (area.bottom - area.top) / lineHeight;
		const auto text = literal(displayName);
		// Reuse the renderer's word wrapping/source offsets on one-line pages.
		// The scratch width reserves glyph extent just like the visible rows;
		// only the offsets are used, never its clipped raster or page spacing.
		IndexedFrame scratch;
		scratch.width = area.right - area.left - 7;
		scratch.height = lineHeight;
		scratch.pixels.assign(scratch.width * scratch.height, 0);
		XeenTextRenderOptions options;
		options.bounds = {0,0,scratch.width,scratch.height};
		options.size = XeenFontSize::Reduced;
		options.paginate = true;
		const auto wrapped = renderer.render(scratch,text,options);
		std::size_t start = 0;
		for (int i = 0; i < lineBudget && static_cast<std::size_t>(i) < wrapped.pageSourceEnds.size(); ++i) {
			const auto end = wrapped.pageSourceEnds[i];
			auto part = text.substr(start,end-start);
			while (!part.empty() && part.back() == ' ') part.pop_back();
			if (i == lineBudget-1 && end < text.size()) {
				while (!part.empty() && renderer.textWidth(part + "...",XeenFontSize::Reduced) > scratch.width) part.pop_back();
				part += "...";
			}
			line(area.left,area.right,area.top+i*lineHeight,std::move(part));
			start = end;
		}
	};
	const auto &ids = party.party.activeRosterIds();
	const XeenCharacter *character = nullptr;
	if (selection.source < ids.size() && selection.sourceOwner == ids[selection.source])
		character = &party.roster.at(ids[selection.source]);
	if (character) {
		const auto &c = *character;
		XeenCharacterRules::validateForUse(c, {kCloudsInitialYear});
		line(10,166,8,"F" + std::to_string(selection.source+1) + " [owner " + std::to_string(ids[selection.source]) + "] " + c.name,true);
		line(170,310,8,std::string("Condition: ") + xeenConditionName(c.worstCondition()));
		line(10,310,17,"HP " + std::to_string(c.currentHp) + " / " + std::to_string(XeenCharacterRules::maxHp(c,{kCloudsInitialYear})));
		line(10,310,26,"SP " + std::to_string(c.currentSp) + " / " + std::to_string(XeenCharacterRules::maxSp(c,{kCloudsInitialYear})));
	} else line(10,310,8,"No active characters");
	const char *categories[]{"Weapons","Armor","Accessories","Miscellaneous"};
	const auto category = static_cast<unsigned>(selection.category);
	if (category >= 4) throw std::invalid_argument("Invalid inventory view category");
	const auto *items = character ? xeenInventoryItems(*character,selection.category) : nullptr;
	const bool selected = items && selection.slot && *selection.slot < 9;
	line(10,150,35,std::string(categories[category]) + (selected ? " - Slot " + std::to_string(*selection.slot+1) : ""));
	for (std::size_t slot=0;slot<9;++slot) {
		const auto description = catalog.describe(selection.category,items ? (*items)[slot] : XeenItem{});
		std::string prefix = selection.slot == slot ? ">" : " ";
		prefix += std::to_string(slot+1) + " ";
		if (description.equipped) prefix += "E ";
		if (description.broken) prefix += "B ";
		if (description.cursed) prefix += "C ";
		line(10,150,44+static_cast<int>(slot)*9,prefix+description.displayName,true);
	}
	if (selected) {
		const auto d = catalog.describe(selection.category,(*items)[*selection.slot]);
		selectedName(d.displayName);
		line(154,310,62,d.catalogAvailability == XeenCatalogAvailability::Unavailable ? "Catalog unavailable" :
			!d.unsupportedFields.empty() ? "Unknown fields" :
			d.materialAvailability != XeenMaterialAvailability::Ready ? "Material fallback" : "");
		line(154,310,71,d.empty ? "Empty" : selection.category == XeenInventoryCategory::Miscellaneous ? "Miscellaneous" : d.equipped ? "Equipped" : "Unequipped");
		line(154,310,80,std::string(d.broken ? "Broken" : "Unbroken") + (d.cursed ? " / Cursed" : " / Uncursed"));
		line(154,310,89,std::string(d.counterKind == XeenItemCounterKind::Charges ? "Charges " : "Counter ") + std::to_string(d.counter));
		line(154,310,98,"M="+std::to_string(d.raw.material)+" ID="+std::to_string(d.raw.id));
		line(154,310,107,"S="+std::to_string(d.raw.state)+" F="+std::to_string(d.raw.frame));
	} else line(154,310,44,"Select slot 1-9");
	if (selection.destination && *selection.destination < ids.size() && selection.destinationOwner == ids[*selection.destination]) {
		line(154,310,35,"To F"+std::to_string(*selection.destination+1)+" [owner "+std::to_string(*selection.destinationOwner)+"]");
		line(154,310,116,party.roster.at(*selection.destinationOwner).name,true);
	} else if (selection.mode == XeenInventoryMode::ChooseDestination) line(154,310,35,"To: choose F1-F6");
	line(10,310,126,feedback ? feedback : "",true);
	line(10,310,137,selection.mode == XeenInventoryMode::Browse ? "F1-F6 owner; arrows/1-9 browse; T move; Esc/I close" :
		selection.mode == XeenInventoryMode::Confirm ? "Enter confirms; F1-F6 changes recipient; Esc/N cancels" : "F1-F6 recipient; Escape to cancel",true);
	return lines;
}
IndexedFrame drawXeenInventory(const IndexedFrame &base, const XeenFontFormat &font,
		const XeenItemCatalog &catalog, const XeenPartyState &party,
		const XeenInventorySelection &selection, const char *feedback) {
	XeenTextRenderer renderer(font);
	XeenTextRenderOptions options;
	options.bounds = options.windowBounds = {4,4,316,149};
	options.x=4; options.y=4; options.drawWindow=true;
	auto frame = renderer.render(base,"",options).pages.front();
	for (const auto &line : xeenInventoryLayout(font,catalog,party,selection,feedback)) {
		options.bounds=line.bounds; options.x=line.bounds.left; options.y=line.bounds.top;
		options.drawWindow=false; options.size=XeenFontSize::Reduced;
		frame=renderer.render(frame,line.text,options).pages.front();
	}
	return frame;
}
}

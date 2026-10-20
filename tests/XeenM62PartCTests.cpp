#include "XeenTestInstallation.h"
#include "XeenTrainingTestSupport.h"
#include "XeenPurchaseTestSupport.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenObjectVisual.h"
#include "games/xeen/XeenServiceAnimation.h"
#include <algorithm>
#include <array>
#include <iostream>

// M62 Part C: service animation resources and scenery end frames, on original data.
using namespace training_test;
namespace {
using Art = XeenServiceArt;
struct Shop { Art art; const char *name; unsigned modulus, repeat; std::vector<unsigned> firstCycle; };

std::vector<unsigned> range(unsigned first, unsigned last) {
	std::vector<unsigned> result;
	for (unsigned value = first; value <= last; ++value) result.push_back(value);
	return result;
}

// Pure frame selection against the DOS tables (0x508b6, 0x1c45e..0x1c499).
void cycles(Inputs &in) {
	const Shop shops[] = {
		{Art::Bank, "bnkr", 23, 0, range(0, 22)}, {Art::Smith, "blck", 13, 0, range(0, 12)},
		{Art::Guild, "gild", 32, 17, range(0, 31)}, {Art::Tavern, "tvrn", 16, 0, range(0, 15)},
		{Art::Temple, "tmpl", 26, 20, range(0, 25)}, {Art::Training, "trng", 16, 0, range(0, 15)},
	};
	for (const auto &shop : shops) {
		check(XeenServiceAnimation::modulus(shop.art) == shop.modulus && XeenServiceAnimation::firstRepeat(shop.art) == shop.repeat,
			"service modulus/repeat");
		// Initial cycle from 0, then the repeat range, twice.
		std::vector<unsigned> expected = shop.firstCycle;
		for (unsigned round = 0; round < 2; ++round) for (unsigned value = shop.repeat; value < shop.modulus; ++value) expected.push_back(value);
		std::vector<unsigned> visited;
		unsigned index = 0;
		for (std::size_t step = 0; step < expected.size(); ++step) { visited.push_back(index); index = XeenServiceAnimation::next(shop.art, index); }
		check(visited == expected, "service logical index sequence");
		unsigned files = 0;
		for (unsigned i = 0; i < shop.modulus; ++i) {
			const auto base = XeenServiceAnimation::base(shop.art, i);
			check(base.resource == std::string(shop.name) + std::to_string(i / 8 + 1) + ".twn" && base.frame == i % 8, "service file/frame split");
			check(in.assets.spriteFrameCount(base.resource) > base.frame, "service base frame exists in the original resource");
			files = std::max(files, i / 8 + 1);
		}
		check(files == (shop.modulus + 7) / 8, "service file count");
		bool rejected = false;
		try { XeenServiceAnimation::base(shop.art, shop.modulus); } catch (const std::invalid_argument &) { rejected = true; }
		check(rejected, "service index past modulus");
	}
	// Spot checks on the file boundaries named by the plan.
	check(XeenServiceAnimation::base(Art::Bank, 22) == XeenServiceSprite{"bnkr3.twn", 6} &&
		XeenServiceAnimation::base(Art::Smith, 12) == XeenServiceSprite{"blck2.twn", 4} &&
		XeenServiceAnimation::base(Art::Guild, 31) == XeenServiceSprite{"gild4.twn", 7} &&
		XeenServiceAnimation::base(Art::Tavern, 15) == XeenServiceSprite{"tvrn2.twn", 7} &&
		XeenServiceAnimation::base(Art::Temple, 25) == XeenServiceSprite{"tmpl4.twn", 1} &&
		XeenServiceAnimation::base(Art::Training, 8) == XeenServiceSprite{"trng2.twn", 0}, "service boundary frames");
	check(XeenServiceAnimation::next(Art::Guild, 31) == 17 && XeenServiceAnimation::next(Art::Temple, 25) == 20 &&
		XeenServiceAnimation::next(Art::Bank, 22) == 0, "service wrap targets");
}

// Draw a composed list the way the Flow does, over an existing frame.
IndexedFrame drawn(Inputs &in, IndexedFrame frame, const std::vector<XeenServiceSprite> &sprites) {
	for (const auto &sprite : sprites)
		in.assets.drawDialogSprite(frame, sprite.resource.c_str(), sprite.frame, XeenServiceAnimation::kX, XeenServiceAnimation::kY);
	return frame;
}

// DOS speaking frames are composed and tested, not activated (gates/choices are M63).
void speaking(Inputs &in) {
	struct Case { Art art; const char *resource; unsigned first, last, frames; bool replaces; };
	const Case cases[] = {
		{Art::Bank, "bnkr3.twn", 7, 11, 12, false}, {Art::Temple, "tmpl4.twn", 2, 4, 5, false},
		{Art::Training, "trng2.twn", 8, 12, 13, false}, {Art::Smith, "blck2.twn", 5, 7, 8, true},
	};
	for (const auto &c : cases) {
		const auto overlay = XeenServiceAnimation::speaking(c.art);
		check(overlay && overlay->resource == c.resource && overlay->first == c.first && overlay->last == c.last &&
			overlay->replacesBase == c.replaces, "speaking overlay table");
		check(in.assets.spriteFrameCount(c.resource) == c.frames, "speaking resource frame count");
		for (unsigned index = 0; index < XeenServiceAnimation::modulus(c.art); ++index) {
			auto marker = frame(); std::fill(marker.pixels.begin(), marker.pixels.end(), 77);
			const auto base = XeenServiceAnimation::compose(c.art, index);
			check(base.size() == 1 && base.front() == XeenServiceAnimation::base(c.art, index), "no speaking frame draws the base only");
			const auto baseOnly = drawn(in, marker, base);
			for (unsigned f = c.first; f <= c.last; ++f) {
				const auto list = XeenServiceAnimation::compose(c.art, index, f);
				if (c.replaces) check(list.size() == 1 && list.front() == XeenServiceSprite{c.resource, f}, "Smith speaking replaces the base");
				else check(list.size() == 2 && list[0] == base.front() && list[1] == XeenServiceSprite{c.resource, f}, "overlay follows the base");
				const auto composed = drawn(in, marker, list);
				// The speaking frame must be visible against the plain base.
				check(composed.pixels != baseOnly.pixels, "speaking frame is invisible");
			}
			for (unsigned f : {c.first - 1, c.last + 1}) {
				bool rejected = false;
				try { XeenServiceAnimation::compose(c.art, index, f); } catch (const std::invalid_argument &) { rejected = true; }
				check(rejected, "speaking frame outside the DOS range");
			}
		}
	}
	for (Art art : {Art::Guild, Art::Tavern}) {
		check(!XeenServiceAnimation::speaking(art), "Guild/Tavern have no Clouds speaking overlay");
		bool rejected = false;
		try { XeenServiceAnimation::compose(art, 0, 0u); } catch (const std::invalid_argument &) { rejected = true; }
		check(rejected, "Guild/Tavern reject a speaking frame");
	}
}

// Live base-cycle playback on the existing cosmetic pulse (150 ms, no gameplay work).
struct Shop6 : Fixture {
	Shop6(Inputs &in, const XeenSaveSnapshot &s) : Fixture(in, s) {
		flow->drawSmithArt = [&in](auto &image) { in.assets.drawSmith(image); };
		flow->drawTempleArt = [&in](auto &image) { in.assets.drawTemple(image); };
		flow->drawTavernArt = [&in](auto &image) { in.assets.drawTavern(image); };
		flow->drawBankArt = [&in](auto &image) { in.assets.drawBank(image); };
		flow->drawGuildArt = [&in](auto &image) { in.assets.drawGuild(image); };
		flow->tavernFoodOffer = [&in](unsigned index) { return in.assets.tavernFoodOffer(index); };
		flow->loadRestDream = [&in] { return in.assets.restDreamImage(); };
	}
	bool admitted() const {
		const auto dialog = flow->inputContext(flow->frame().presentation()).dialog;
		return w.sessionState().journeyActivity() == XeenJourneyActivity::Service && dialog && !dialog->hits.empty();
	}
	void open() {
		act(InteractionAction{});
		for (unsigned n = 0; n < 1000 && !admitted(); ++n) {
			flow->beginCycle(++cycle);
			if (const auto next = flow->updatePresentation()) present(*next);
		}
		check(admitted(), "service was not admitted");
	}
	std::optional<IndexedFrame> tick(std::uint64_t at) {
		now = at; flow->beginCycle(++cycle);
		auto next = flow->updatePresentation();
		if (next) present(*next);
		return next;
	}
};

void artRegionMatches(const IndexedFrame &flowFrame, const IndexedFrame &oracle, const char *message) {
	// Rows 8..100 and columns 8..224 are never covered by the location dialog or its buttons.
	for (int y = 8; y < 100; ++y) for (int x = 8; x < 224; ++x)
		if (flowFrame.pixels[y * 320 + x] != oracle.pixels[y * 320 + x])
			throw std::runtime_error(std::string(message) + " at " + std::to_string(x) + "," + std::to_string(y) + " flow=" +
				std::to_string(flowFrame.pixels[y * 320 + x]) + " oracle=" + std::to_string(oracle.pixels[y * 320 + x]));
}

void playback(Inputs &in, const XeenSaveSnapshot &source, Art art, const char *label) {
	Shop6 shop(in, source);
	shop.open();
	check(!shop.flow->canSave(), "service did not retain its visit");
	IndexedFrame oracle = frame();
	oracle = drawn(in, oracle, XeenServiceAnimation::compose(art, 0));
	artRegionMatches(shop.flow->frame(), oracle, label);
	const auto gold = shop.p.monsterTreasure->gold;
	const auto day = shop.p.encounterContext->day;
	const auto minutes = shop.p.encounterContext->minutes;
	// The fixture clock stays at 0 while the visit opens, so the first call at 0 can only arm
	// (or find armed) the 150 ms deadline.
	std::uint64_t now = 0;
	check(!shop.tick(now), "first pulse call only arms the deadline");
	unsigned index = 0;
	const unsigned steps = XeenServiceAnimation::modulus(art) + (XeenServiceAnimation::modulus(art) - XeenServiceAnimation::firstRepeat(art)) + 2;
	for (unsigned step = 0; step < steps; ++step) {
		check(!shop.tick(PresentationClock::afterMilliseconds(now,3)-1), "base frame advanced before its 150 ms pulse");
		const auto due=PresentationClock::afterMilliseconds(now,3);
        const auto next = shop.tick(due);
		check(next.has_value(), "base frame did not advance on its pulse");
		now = due;
		index = XeenServiceAnimation::next(art, index);
		oracle = drawn(in, oracle, XeenServiceAnimation::compose(art, index));
		artRegionMatches(*next, oracle, label);
		check(next->presentation() == shop.flow->frame().presentation(), "pulse frame is the flow frame");
		check(!shop.flow->canSave(), "pulse released the service visit");
	}
	// A stalled host does not trigger catch-up redraws.
	check(shop.tick(now + 100000).has_value() && !shop.tick(now + 100001), "pulse caught up after a stall");
	check(shop.p.monsterTreasure->gold == gold && shop.p.encounterContext->day == day && shop.p.encounterContext->minutes == minutes,
		"base-frame pulse changed gold or service-day accounting");
	std::cout << label << " base cycle passed\n";
}

// The character sheet selection blink keeps its original four-counter-increment cadence and frames 48/49.
void sheetBlink(Inputs &in) {
	Shop6 f(in, in.service());
	f.act(SelectMemberAction{0});
	check(f.flow->inventoryOpen(), "character sheet did not open");
	const auto first = f.flow->frame().pixels;
	check(!f.tick(219), "sheet blink advanced before four BIOS increments");
	const auto blinked = f.tick(220);
	check(blinked.has_value() && blinked->pixels != first, "sheet blink did not toggle at four BIOS increments");
	for (int y = 0; y < 200; ++y) for (int x = 0; x < 320; ++x)
		if (blinked->pixels[y * 320 + x] != first[y * 320 + x]) check(x < 60 && y < 60, "sheet blink changed more than the selection marker");
	check(!f.tick(439), "sheet blink retimed");
	const auto restored = f.tick(440);
	check(restored.has_value() && restored->pixels == first, "sheet blink did not return to frame 48");
	std::cout << "Sheet blink passed\n";
}

// Inclusive end frame of the DOS scenery routine (0x10d21..0x10d55).
unsigned dosFrame(unsigned start, unsigned end, std::uint64_t phase) {
	unsigned frame = start;
	for (std::uint64_t i = 0; i < phase; ++i) { ++frame; if (!(end >= frame)) frame = start; }
	return frame;
}

void scenery(Inputs &in) {
	struct Resource { unsigned id; unsigned frames; std::array<unsigned, 4> start, end; };
	const Resource resources[] = {
		{8, 10, {0, 8, 9, 8}, {7, 8, 9, 8}}, {9, 8, {0, 4, 4, 4}, {3, 7, 7, 7}},
		{102, 12, {0, 8, 4, 8}, {3, 11, 7, 11}}, {105, 9, {0, 8, 8, 8}, {7, 8, 8, 8}},
		{109, 8, {0, 0, 0, 0}, {7, 7, 7, 7}},
	};
	const auto resolver = XeenObjectVisualResolver::load(in.assets);
	const auto metadata = XeenCloudsVisualMetadata::parse(*in.assets.readCloudsVisualMetadataFromDarkArchive());
	for (const auto &r : resources) {
		const auto &entry = metadata.at(r.id);
		for (unsigned d = 0; d < 4; ++d) check(entry.initialFrames[d] == r.start[d] && entry.frameLimits[d] == r.end[d], "scenery metadata matches the indexed ranges");
		check(in.assets.spriteFrameCount(xeenObjectSpriteName(r.id)) == r.frames, "scenery resource frame count");
		for (unsigned d = 0; d < 4; ++d) check(r.end[d] < r.frames, "inclusive end frame exists");
	}
	struct Placement { unsigned map, id; int x, y; unsigned direction; };
	const Placement placements[] = {
		{28, 8, 14, 10, 3}, {28, 9, 14, 5, 3}, {28, 9, 25, 26, 0}, {28, 9, 16, 11, 1},
		{23, 9, 9, 11, 3}, {23, 9, 9, 6, 1}, {23, 102, 0, 1, 0}, {23, 102, 7, 7, 0}, {23, 102, 12, 12, 0},
		{23, 105, 8, 10, 0}, {23, 109, 10, 4, 0}, {23, 109, 14, 4, 0},
	};
	for (const auto &placement : placements) {
		const auto objects = XeenMapLoader().loadObjects(in.assets, placement.map);
		std::size_t found = objects.entities.objects.size();
		for (std::size_t i = 0; i < objects.entities.objects.size(); ++i) {
			const auto &o = objects.entities.objects[i];
			if (o.resourceId == int(placement.id) && o.x == placement.x && o.y == placement.y && o.direction == placement.direction) found = i;
		}
		check(found < objects.entities.objects.size(), "indexed scenery placement is present in the original map");
		const Resource *resource = nullptr;
		for (const auto &r : resources) if (r.id == placement.id) resource = &r;
		for (unsigned camera = 0; camera < 4; ++camera) {
			const unsigned relative = (camera + 4 - placement.direction) % 4;
			const auto &entry = metadata.at(placement.id);
			const unsigned start = resource->start[relative], end = resource->end[relative];
			const bool animated = end > start;
			const unsigned length = animated ? end - start + 1 : 1;
			check(resolver.resolve(objects, found, static_cast<XeenDirection>(camera)).status ==
				(animated ? XeenObjectVisualStatus::UnsupportedAnimation : XeenObjectVisualStatus::SupportedStatic), "static classification");
			for (std::uint64_t phase = 0; phase < 3u * length + 2; ++phase) {
				const auto visual = resolver.resolve(objects, found, static_cast<XeenDirection>(camera), phase);
				check(visual.frame == dosFrame(start, end, phase) && visual.horizontalFlip == (entry.flipFlags[relative] != 0) &&
					visual.status == (animated ? XeenObjectVisualStatus::SupportedAnimated : XeenObjectVisualStatus::SupportedStatic),
					"scenery frame follows the DOS inclusive cycle with the directional flip");
				if (animated && phase == length - 1) check(visual.frame == end, "end frame is displayed");
			}
		}
	}
	std::cout << "Scenery inclusive end frames and facing flips passed\n";
}
}

int main(int argc, char **argv) {
	try {
		check(argc == 2, "usage: m62-part-c <original-installation>");
		const auto installation = xeenTestInstallationDetector().detect(argv[1]);
		check(bool(installation), "installation unavailable");
		Inputs in(*installation);
		cycles(in);
		speaking(in);
		scenery(in);
		sheetBlink(in);
		const auto source = in.service();
		playback(in, source, Art::Training, "Training");
		{
			auto s = purchase_test::service(in);
			playback(in, s, Art::Smith, "Smith");
		}
		{
			auto s = in.service(8); s.journey->content = 14; s.camera = {28, 15, 28, XeenDirection::North};
			playback(in, s, Art::Temple, "Temple");
		}
		const auto city = in.city;
		const auto entrance = [&](unsigned action) {
			const int x = action == 3 ? 24 : 26, y = action == 3 ? 5 : 17;
			for (const auto &record : city.records) if (record.x == x && record.y == y && record.line == 0)
				return XeenCamera{28, x, y, record.direction == 4 ? XeenDirection::North : XeenDirection(record.direction)};
			throw std::runtime_error("Original Tavern/Bank entry is absent");
		};
		{ auto s = in.service(8); s.camera = entrance(3); playback(in, s, Art::Tavern, "Tavern"); }
		{ auto s = in.service(8); s.camera = entrance(4); playback(in, s, Art::Bank, "Bank"); }
		{ auto s = in.service(10); s.camera = {28, 20, 13, XeenDirection::North}; playback(in, s, Art::Guild, "Guild"); }
		std::cout << "M62 Part C service animation and scenery passed\n";
		return 0;
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}

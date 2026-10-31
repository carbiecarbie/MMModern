#include "games/xeen/XeenServiceAnimation.h"
#include <stdexcept>

namespace mmodern {
namespace {
struct Entry {
	const char *prefix;
	unsigned modulus, repeat;
};
// Index by XeenServiceArt. Prefix pointers DAT 0x5088c..0x508a7.
constexpr Entry kEntries[] = {
	{"bnkr", 23, 0}, {"blck", 13, 0}, {"gild", 32, 17},
	{"tvrn", 16, 0}, {"tmpl", 26, 20}, {"trng", 16, 0},
};
const Entry &entry(XeenServiceArt art, XeenSide side=XeenSide::Clouds) {
	const auto index = static_cast<unsigned>(art);
	if (index >= sizeof(kEntries) / sizeof(kEntries[0])) throw std::invalid_argument("unknown service art");
	static constexpr Entry dark[]={{"bnkr",26,0},{"blck",19,0},{"gild",48,0},
		{"tvrn",27,0},{"tmpl",26,0},{"trng",37,0}};
	return side==XeenSide::Darkside?dark[index]:kEntries[index];
}
std::string resource(XeenServiceArt art, unsigned file) {
	return std::string(entry(art).prefix) + std::to_string(file) + ".twn";
}
}

const char *XeenServiceAnimation::greeting(XeenServiceArt art, XeenSide side) {
	static constexpr const char *names[]={"banker.voc","whaddayo.voc","guild10.voc","hello.voc","maywe2.voc","training.voc"};
	static constexpr const char *dark[]={"bank1.voc","see2.voc","parrot1.voc","hello1.voc","help2.voc","youtrn1.voc"};
	(void)entry(art);return (side==XeenSide::Darkside?dark:names)[static_cast<unsigned>(art)];
}

const char *XeenServiceAnimation::voice(XeenServiceArt art,XeenServiceVoice action,XeenSide side) {
	const bool dark=side==XeenSide::Darkside;
	switch(action) {
	case XeenServiceVoice::Greeting:return greeting(art,side);
	case XeenServiceVoice::Purchase:
		return art==XeenServiceArt::Smith?(dark?"choice2.voc":nullptr):
			art==XeenServiceArt::Guild?(dark?"parrot2.voc":"guild12.voc"):(dark?"prtygd.voc":"trainin2.voc");
	case XeenServiceVoice::Refusal:return dark?"skull1.voc":"guild11.voc";
	case XeenServiceVoice::TrainingCap:return dark?"gtlost.voc":"trainin1.voc";
	case XeenServiceVoice::TrainingExperience:return dark?"gtlost.voc":"trainin0.voc";
	case XeenServiceVoice::TrainingGold:return dark?"gtlost.voc":nullptr;
	case XeenServiceVoice::Thanks:return dark?"thanks2.voc":"thankyou.voc";
	case XeenServiceVoice::Farewell:return art==XeenServiceArt::Tavern?(dark?"gdluck1.voc":"goodbye.voc"):
		art==XeenServiceArt::Smith && dark?"come1.voc":nullptr;
	}
	throw std::invalid_argument("unknown service voice");
}
const char *XeenServiceAnimation::darksideSong(unsigned action) {
	static constexpr const char *names[]={"bank.m","sf09.m","guild.m","tavern.m","temple.m","smith.m"};
	if(action>=6)throw std::invalid_argument("unknown Darkside service song");return names[action];
}

unsigned XeenServiceAnimation::modulus(XeenServiceArt art,XeenSide side) { return entry(art,side).modulus; }
unsigned XeenServiceAnimation::firstRepeat(XeenServiceArt art,XeenSide side) { return entry(art,side).repeat; }

unsigned XeenServiceAnimation::next(XeenServiceArt art, unsigned index,XeenSide side) {
	const auto &e = entry(art,side);
	if (index >= e.modulus) throw std::invalid_argument("service art index out of range");
	return index + 1 >= e.modulus ? e.repeat : index + 1;
}

XeenServiceSprite XeenServiceAnimation::base(XeenServiceArt art, unsigned index,XeenSide side) {
	if (index >= entry(art,side).modulus) throw std::invalid_argument("service art index out of range");
	return {resource(art, index / 8 + 1), index % 8};
}

XeenServiceAnimation::DarksideFrame XeenServiceAnimation::darkside(XeenServiceArt art,
		unsigned index,bool busy,unsigned selector,const std::function<unsigned(unsigned,unsigned)> &draw) {
	DarksideFrame result;result.selector=selector;
	const auto add=[&](unsigned file,unsigned frame,int x,int y) {result.sprites.push_back({{resource(art,file),frame},x,y});};
	if(art==XeenServiceArt::Training && busy) {
		add(1,++result.selector%8,8,8);add(6,draw(0,5),61,74);
		return result;
	}
	result.sprites.push_back({base(art,index,XeenSide::Darkside),8,8});
	switch(art) {
	case XeenServiceArt::Bank:
		if(busy || selector==1)add(5,draw(13,18),8,30);
		else if(selector>1) {add(5,13-selector,8,30);if(++result.selector>13)result.selector=0;}
		break;
	case XeenServiceArt::Smith:
		add(3,draw(0,11)==1?9:10,34,33);if(busy)add(3,draw(3,8),34,54);break;
	case XeenServiceArt::Guild:
		if(busy) {if(selector) {result.selector^=1;add(7,result.selector,8,106);}else add(7,draw(0,3),161,48);}
		break;
	case XeenServiceArt::Tavern:if(busy)add(5,draw(0,7),153,49);break;
	case XeenServiceArt::Temple:if(busy)add(4,draw(2,4),8,8);break;
	case XeenServiceArt::Training:break;
	}
	return result;
}
XeenServiceAnimation::DarksideFrame XeenServiceAnimation::darksideEntry(XeenServiceArt art,
		const std::function<unsigned(unsigned,unsigned)> &draw) {
	DarksideFrame result;
	if(art==XeenServiceArt::Bank) {
		// drawBackground's overlay precedes the initial drawAnim and greeting.
		result.sprites.push_back({{"bnkr5.twn",draw(13,18)},8,30});
		auto second=darkside(art,0,false,1,draw);
		result.sprites.insert(result.sprites.end(),second.sprites.begin(),second.sprites.end());
		result.selector=2;
	} else result=darkside(art,0,false,0,draw);
	return result;
}

std::optional<XeenServiceAnimation::Speaking> XeenServiceAnimation::speaking(XeenServiceArt art) {
	switch (art) {
	case XeenServiceArt::Bank: return Speaking{resource(art, 3), 7, 11, false};
	case XeenServiceArt::Smith: return Speaking{resource(art, 2), 5, 7, true};
	case XeenServiceArt::Temple: return Speaking{resource(art, 4), 2, 4, false};
	case XeenServiceArt::Training: return Speaking{resource(art, 2), 8, 12, false};
	case XeenServiceArt::Guild: case XeenServiceArt::Tavern: break;
	}
	return std::nullopt;
}

std::vector<XeenServiceSprite> XeenServiceAnimation::compose(XeenServiceArt art, unsigned index,
		std::optional<unsigned> speakingFrame) {
	auto sprite = base(art, index);
	if (!speakingFrame) return {std::move(sprite)};
	const auto overlay = speaking(art);
	if (!overlay || *speakingFrame < overlay->first || *speakingFrame > overlay->last)
		throw std::invalid_argument("service speaking frame is outside the original range");
	XeenServiceSprite speech{overlay->resource, *speakingFrame};
	if (overlay->replacesBase) return {std::move(speech)};
	return {std::move(sprite), std::move(speech)};
}

} // namespace mmodern

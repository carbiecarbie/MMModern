#ifndef MMODERN_DOS_EXPORT_TEST_SUPPORT_H
#define MMODERN_DOS_EXPORT_TEST_SUPPORT_H
// Shared by the exporter tests and the existing earned world-Event route.
#include "tools/XeenDosExport.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenGameFlagsFormat.h"
#include "formats/xeen/XeenQuestFlagFormat.h"
#include "formats/xeen/XeenQuestItemFormat.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "formats/xeen/XeenMapFormat.h"
#include "formats/xeen/XeenEventFormat.h"
#include "games/xeen/XeenStateEquality.h"
#include <algorithm>
#include <stdexcept>
namespace dos_export_test {
using namespace mmodern;
using namespace mmodern::developer;
inline void check(bool b,const std::string &why) {if(!b)throw std::runtime_error(why);}
inline const DosBytes &bytes(const DosMembers &m,const std::string &name) {
	for(const auto &entry:m) if(entry.name==name)return entry.bytes;
	throw std::runtime_error("Missing test member: "+name);
}
inline void compare(const DosExportPair &pair,const DosExportPair &base,const XeenSaveSnapshot &s) {
	const auto &pty=bytes(pair.darkside,"maze.pty"),&chr=bytes(pair.darkside,"maze.chr");
	const auto header=XeenCharacterFormat::parsePartyHeader(pty);
	check(header.firstCount==s.activeRosterIds.size() && header.effectiveCount==s.activeRosterIds.size(),"Party count");
	for(unsigned i=0;i<s.activeRosterIds.size();++i)check(header.rosterIds[i]==s.activeRosterIds[i],"Active order");
	check(xeen_state::sameCamera(XeenCharacterFormat::parsePartyLocation(pty),s.camera),"Camera");
	check(XeenGameplayContextFormat::parse(pty)==*s.journey->context,"Calendar and effects");
	const auto purse=XeenCharacterFormat::parseMonsterPurse(pty);
	check(purse.gold==s.journey->purse->gold && purse.gems==s.journey->purse->gems,"Purse");
	const DosBytes bankPrefix(pty.begin(),pty.begin()+812);
	check(XeenCharacterFormat::parseBankBalances(bankPrefix)==s.journey->serviceEconomy->bank,"Both bank balances");
        const auto clouds=XeenGameFlagsFormat::parseClouds(pty);
        check(std::equal(clouds.begin(),clouds.end(),s.gameFlags.begin()),"Clouds flags");
	check(XeenQuestFlagFormat::parseClouds(pty).values()==s.questFlags,"Quest flags");
	check(XeenQuestFlagFormat::parseWorldFlags(pty)==*s.journey->worldFlags,"World flags");
	check(XeenQuestItemFormat::parseClouds(pty).counts()==s.questItems,"Quest items");
	check(pty[14]==s.soundOptions.efx && pty[15]==s.soundOptions.music && pty[1528]==!s.soundOptions.text && pty[1529]==s.soundOptions.speech,"Sound options");
	const auto word=[&](unsigned off){return unsigned(pty[off])|(unsigned(pty[off+1])<<8);};
	const auto dword=[&](unsigned off){return std::uint32_t(word(off))|(std::uint32_t(word(off+2))<<16);};
	check(word(618)==s.food && word(604)==s.journey->cloudsCompleted && dword(654)==s.journey->playTimeSeconds,"Food, completion, play time");
	for(unsigned side=0;side<2;++side)for(unsigned shop=0;shop<4;++shop)for(unsigned category=0;category<4;++category)for(unsigned item=0;item<9;++item) {
		const unsigned at=(side?832:28)+((category*9+item)*4+shop)*4;
		const auto &v=s.journey->serviceEconomy->wares[side][shop][category][item];
		check(pty[at]==v.material && pty[at+1]==v.id && pty[at+2]==v.state && pty[at+3]==v.frame,"Both merchant banks and every item byte");
	}
	auto roster=XeenCharacterFormat::parseRoster(chr);
	for(unsigned i=0;i<30;++i) {
		roster.at(i).learnedSpells=XeenCharacterFormat::parseLearnedSpells(chr,i);
		check(xeen_state::sameCharacter(roster.at(i),s.characters[i]),"CHR record "+std::to_string(i));
		check(xeen_state::sameInputs(XeenCharacterFormat::parseCombatInputs(chr,i,true,true,true),s.journey->supplements[i].inputs),"CHR supplement "+std::to_string(i));
		for(unsigned at=s.characters[i].name.size()+1;at<16;++at)
			check(chr[i*354+at]==bytes(base.darkside,"maze.chr")[i*354+at],"CHR name padding retains New Game");
	}
	const auto &basePty=bytes(base.darkside,"maze.pty");
	for(unsigned i=2+s.activeRosterIds.size();i<10;++i)check(pty[i]==basePty[i],"Unused party ID suffix retains New Game");
	for(const auto span: {std::pair{16,18},std::pair{606,610},std::pair{632,638},std::pair{691,723},std::pair{782,832},std::pair{1408,1528}})
		check(std::equal(pty.begin()+span.first,pty.begin()+span.second,basePty.begin()+span.first),"Unmodeled PTY fields retain New Game");
	check((pty[742]&3)==(basePty[742]&3) && std::equal(pty.begin()+743,pty.begin()+747,basePty.begin()+743),"Unused quest flags retain New Game");
	for(const auto &m:base.clouds) if(m.name=="maze.chr" || m.name=="maze.pty")check(bytes(pair.clouds,m.name)==m.bytes,"Stale XEEN replica");
	for(const auto &m:base.darkside)if(m.name!="maze.nam" && m.name!="maze.chr" && m.name!="maze.pty")check(bytes(pair.darkside,m.name)==m.bytes,"DARK map remains New Game");
	for(const auto &m:s.maps) {
		const auto &dat=bytes(pair.clouds,dosMapName(m.id.number,"dat"));
		const auto &old=bytes(base.clouds,dosMapName(m.id.number,"dat"));
		const auto geometry=XeenMapFormat::parseDat(dat);
		check(std::equal(dat.begin()+828,dat.begin()+860,old.begin()+828),"DAT seen retains New Game");
		for(const auto &c:m.cells)check(geometry.cells[c.cell].rawWord==c.word && geometry.cells[c.cell].rawAttributes==c.attributes,"Physical DAT word and attribute");
		if(m.explored)for(unsigned i=0;i<256;++i)check(geometry.cells[i].stepped==bool((*m.explored)[i/8]&(1u<<(i%8))),"Explored reversal");
		if(m.tavernTips)check(geometry.tavernTips==*m.tavernTips,"Tavern tip index");
		if(m.monsters || !m.objects.empty() || !m.wallItems.empty()) {
			const auto mob=XeenMapFormat::parseMob(bytes(pair.clouds,dosMapName(m.id.number,"mob")));
			for(const auto &o:m.objects)check(mob.objects.at(o.record).x==o.x && mob.objects.at(o.record).y==o.y && mob.objects.at(o.record).tableIndex==o.tableIndex,"Object position and picture-table index");
			for(const auto &w:m.wallItems)check(mob.wallItems.at(w.record).x==w.x && mob.wallItems.at(w.record).y==w.y,"Wall item position");
			if(m.monsters)for(unsigned i=0;i<m.monsters->originalCount;++i) {
				const auto &v=m.monsters->slots.at(i);const bool gone=v.lifecycle==XeenActorLifecycle::Defeated;
				check(mob.monsters.at(i).x==(gone?-128:v.x) && mob.monsters.at(i).y==(gone?-128:v.y),"Monster original-count positions/removals");
			}
		}
		if(!m.events.empty()) {
			const auto evt=XeenEventFormat::parse(bytes(pair.clouds,dosMapName(m.id.number,"evt")));
			for(const auto &e:m.events)check(evt.at(e.record).opcode==e.opcode,"Event opcode");
		}
	}
}
}
#endif

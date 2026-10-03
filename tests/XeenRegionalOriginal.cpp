// Opt-in, read-only original-resource rule evidence. Not SDL/runtime acceptance.
#include "games/xeen/XeenRegionalRules.h"
#include "games/xeen/XeenMovement.h"
#include "games/xeen/XeenInstallationDetector.h"
#include "games/xeen/XeenMapLoader.h"
#include "games/xeen/XeenEventLoader.h"
#include "formats/xeen/XeenAssetSource.h"
#include "formats/xeen/XeenGameplayContextFormat.h"
#include "formats/xeen/XeenCharacterFormat.h"
#include "formats/xeen/XeenSaveFormat.h"
#include "games/xeen/XeenPartyLoader.h"
#include "games/xeen/XeenSaveState.h"
#include "app/XeenEncounterFlow.h"
#include "XeenRegionalOracle.h"
#include "XeenJourneyResourceTestSupport.h"
#include "XeenTrainingTestSupport.h"
#include <iostream>
#include "platform/XeenSaveFile.h"
#include <stdexcept>
using namespace mmodern;
int main(int argc,char **argv) {
	try {
		if (argc!=2) throw std::invalid_argument("usage: mmodern_regional_original <installation> [legacy-3-save]");
		const auto installation=XeenInstallationDetector().detect(argv[1]);
		if (!installation) throw std::runtime_error("Installation not found");
		XeenAssetSource assets(*installation);XeenMapLoader maps;
		const auto map=maps.loadGeometryMap(assets,23);
		const auto mob=maps.loadObjects(assets,23);
		XeenEventLoader loader([&](const std::string &name)->std::optional<std::vector<std::uint8_t>> {
			if (!assets.hasInitialResource(name)) return {};return assets.readInitialResource(name);
		});
		const auto events=loader.load(23);
		const auto mon=assets.readCloudsMonsterStatisticsFromDarkArchive();
		if (!mon) throw std::runtime_error("MON missing");
		const auto statistics=XeenMonsterFormat::parse(*mon);
		xeenValidateRegionalManifest(map,mob,events,statistics,assets.readInitialResource("maze0023.dat"),
			assets.readInitialResource("maze0023.mob"),assets.readInitialResource("maze0023.evt"));
		const auto component=XeenMovement::component(map,9,11,{});
		std::set<std::pair<int,int>> addresses;
		unsigned automaticCells=0;
		for (int y=0;y<16;++y) for(int x=0;x<16;++x) if(component[y*16+x]) {
			if(map.geometry.cells[y*16+x].rawAttributes&0x10) {++automaticCells;if(x!=5 || y!=9)throw std::runtime_error("Unknown mainland automatic trigger");}
			for(unsigned d=0;d<4;++d) {
				const XeenCamera c{23,x,y,static_cast<XeenDirection>(d)};
				const auto found=xeenRegionalEvent(events,c);
				if(found)addresses.insert({x,y});
				if(xeenRegionalSign(events,c)!=(x==5 && y==9 && d==0))throw std::runtime_error("Original event/facing admission mismatch");
			}
		}
		if(automaticCells!=1 || addresses!=std::set<std::pair<int,int>>{{0,1},{4,5},{5,9},{5,13},{7,7},{8,2},{8,10},{9,11},{10,13},{12,12}})
			throw std::runtime_error("Original mainland event addresses changed");
		if (component.count()!=121 || !component[2*16+8] || map.geometry.runX!=10 || map.geometry.runY!=12)
			throw std::runtime_error("Original mainland/Run metadata mismatch");
		const auto actors=XeenActorApproach::actorsFromResources(mob,statistics);
		if (actors.size()!=19) throw std::runtime_error("Original actor count mismatch");
		for (const auto &a:actors) {
			assets.validateNormalMonster(a.statistics->image());
			const auto closure=xeenActorClosure(map,a);
			for(int y=0;y<16;++y)for(int x=0;x<16;++x)
				if((xeenRegionalActorTerrain(map,a,x,y)==XeenMonsterTerrain::Allowed)!=regional_test::terrain(map,x,y))throw std::runtime_error("Original actor terrain/reference mismatch");
			std::cout << "Actor " << a.id.recordIndex << " type=" << a.original.resourceId << " spawn=" << a.x << ',' << a.y
				<< " HP=" << a.hp << " closure=" << closure.count() << '\n';
		}
		std::cout << "Original regional manifest, 121-cell mainland, Run (10,12), 19 actors and normal sprites passed\n";
		const auto chr=assets.readInitialResource("maze.chr");
		const auto pty=assets.readInitialResource("maze.pty");
		const auto context=XeenGameplayContextFormat::parse(pty);
		const auto purse=XeenCharacterFormat::parseMonsterPurse(pty);
		if (purse.gold!=800 || purse.gems!=10 || purse.pending()) throw std::runtime_error("Original purse mismatch");
		constexpr std::array<unsigned,6> activeResistances{7,10,2,5,7,0};
		for (unsigned owner=0;owner<30;++owner) {
			const auto input=XeenCharacterFormat::parseCombatInputs(chr,owner,true,true);
			if (!input.resistances || !input.luck) throw std::runtime_error("Missing original consequence input");
			const auto &r=*input.resistances;
			const auto pos=std::find(kXeenCombatOwners.begin(),kXeenCombatOwners.end(),owner);
			if (pos!=kXeenCombatOwners.end()) {
				const auto expected=activeResistances[pos-kXeenCombatOwners.begin()];
				if (r.coldPermanent!=expected || r.electricalPermanent!=expected || r.coldTemporary || r.electricalTemporary)
					throw std::runtime_error("Original active resistances mismatch");
			}
			if (r.coldPermanent!=chr[354*owner+313] || r.coldTemporary!=chr[354*owner+314] ||
				r.electricalPermanent!=chr[354*owner+315] || r.electricalTemporary!=chr[354*owner+316])
				throw std::runtime_error("Original complete resistance inputs mismatch");
		}
		std::cout << "Original M33 purse and thirty resistance records passed (resource readers only)\n";
		const XeenRegionalManifest manifest=[&](const auto &m,const auto &o,const auto &e,const auto &s) {
			xeenValidateRegionalManifest(m,o,e,s,assets.readInitialResource("maze0023.dat"),assets.readInitialResource("maze0023.mob"),assets.readInitialResource("maze0023.evt"));
		};

        training_test::Inputs current(*installation);
        journey_resources_test::run([&]{return XeenPartyLoader().loadInitialCloudsParty(current.assets);},current.setup(),current.mapLoader(),current.objectLoader(),current.signature,current.resources());
        std::cout<<"Original regional terrain, profile and current resource renewal controls passed\n";
        return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
std::string trim(std::string s) {
    const auto a=s.find_first_not_of(" \r\n"),b=s.find_last_not_of(" \r\n");
    return a==std::string::npos?"":s.substr(a,b-a+1);
}
bool documentedGap(const std::string &dimension,const std::string &mechanic,const std::string &status,const std::string &reason) {
    // Expectations only, never classification inputs: audio and deferred Sign In.
    if(dimension=="Events") {
        const std::map<std::string,std::string> audio{{"6 PlayFX","Sound effects not supported yet"},
            {"40 PlayEventVoc","Voice audio not supported yet"},{"60 PlayCD","CD speech not supported yet"}};
        if(const auto i=audio.find(mechanic);i!=audio.end())return status=="partial" && reason.find(i->second)!=std::string::npos;
    }
    const std::set<std::string> actions{"Sign In"};
    return dimension=="Services" && actions.count(mechanic) && status=="unsupported" &&
        reason.find(mechanic+": not supported yet")!=std::string::npos;
}
bool unresolvedSubroutine(const std::string &mechanic,const std::string &reason) {
    const std::set<std::string> unused{"26 Return","1 Display0x01 layout=default","9 If2 mode=44 operand=1","32 WhoWill"};
    return unused.count(mechanic) && reason.find("Off-map subroutine has no physical caller in this resource")!=std::string::npos;
}
void require(bool b,const std::string &message){if(!b)throw std::runtime_error(message);}
}
int main(int argc,char **argv) {
    try {
        require(argc==2,"usage: coverage-calibration <coverage.md>");
        require(!documentedGap("Events","16 Spawn slot=0","partial","Invalid Spawn operands or owner"),"Missing probe owner must fail calibration");
        require(!documentedGap("Events","17 DoTownEvent action=0","unsupported","opcode 17 is outside the supported decoder subset"),"Missing service handoff must fail calibration");
        std::ifstream input(argv[1]);require(bool(input),"Cannot read coverage report");
        std::string line,dimension;bool clouds=false,darkside=false;unsigned gaps=0,unused=0,waterDragon=0,vultureRoc=0,movement=0;
        std::set<std::string> supported,supportedServices,skillWalls,lightingRows;
        std::array<unsigned,6> townSites{};
        while(std::getline(input,line)) {
            if(line=="## Clouds detail")clouds=true;
            if(line=="## Darkside detail") {clouds=false;darkside=true;}
            if(!clouds && !darkside)continue;
            if(line.rfind("### ",0)==0)dimension=line.substr(4);
            std::stringstream stream(line);std::vector<std::string> columns;std::string column;
            while(std::getline(stream,column,'|'))columns.push_back(trim(column));
            if(columns.size()!=7)continue;
            const auto &mechanic=columns[1],&status=columns[2],&reason=columns[6];
            if(status!="supported" && status!="partial" && status!="unsupported" && status!="unclassified")continue;
            // M68 Part B: every live monster moves (walkers, flyers, species 59).
            // The one exception is the zero-HP, zero-use dark.mon 0 record.
            const bool emptyRecord=darkside && reason.rfind("dark.mon 0 ;",0)==0;
            if(dimension=="Monsters" && mechanic.rfind("Movement ",0)==0) {
                require(status=="supported" || (emptyRecord && columns[3]=="0"),"Monster movement gap: "+mechanic+" / "+status+" / "+reason);++movement;
                if(mechanic.find("species=59")!=std::string::npos)++(clouds?waterDragon:vultureRoc);
            }
            if(dimension=="Monster species") {
                require(reason.find("xeenMonsterMovementSupported rejects")==std::string::npos || (darkside && mechanic=="dark.mon 0"),
                    "Monster species movement refusal: "+mechanic+" / "+reason);
                if(clouds && mechanic.rfind("xeen.mon 59 ",0)==0) {require(status=="supported","Water Dragon species is not supported: "+reason);++waterDragon;}
                if(darkside && mechanic.rfind("dark.mon 59 ",0)==0) {require(status=="supported","Vulture Roc species is not supported: "+reason);++vultureRoc;}
            }
            if(!clouds)continue;
            if(dimension=="Map features") {
                if(mechanic=="Dark physical DAT tile" || mechanic=="Dark primary map" || mechanic=="Light-drain cell") {
                    require(status=="supported","Lighting consumer probe is not supported: "+mechanic);
                    const auto count=mechanic=="Dark physical DAT tile"?"36":mechanic=="Dark primary map"?"20":"0";
                    require(columns[3]==count,"Clouds lighting resource inventory changed: "+mechanic);
                    lightingRows.insert(mechanic);
                }
                if(mechanic.rfind("Mountaineer wall=",0)==0 || mechanic.rfind("Pathfinder wall=",0)==0) {
                    const std::map<std::string,unsigned> uses{{"Mountaineer wall=1",422},{"Mountaineer wall=7",40},
                        {"Mountaineer wall=9",0},{"Mountaineer wall=10",110},{"Mountaineer wall=12",0},
                        {"Pathfinder wall=3",516},{"Pathfinder wall=6",0}};
                    require(status=="supported" && uses.count(mechanic) && std::stoul(columns[3])==uses.at(mechanic),
                        "Traversal skill probe or original Clouds inventory changed: "+mechanic);
                    skillWalls.insert(mechanic);
                }
            }
            if(dimension=="Events" && status!="supported") {
                const auto opcode=mechanic.substr(0,mechanic.find(' '));
                const bool excluded=opcode=="6" || opcode=="40" || opcode=="60" || opcode=="17" || opcode=="30";
                const bool admission=status=="partial" && (opcode=="7" || opcode=="31") &&
                    (reason.find("Destination map is not admitted yet")!=std::string::npos ||
                     reason.find("Destination is outside the admitted mainland")!=std::string::npos);
                const bool limit=status=="unclassified" &&
                    (reason.find("Off-map subroutine has no physical caller in this resource")!=std::string::npos ||
                     reason.find("Census limit: mirror transfer is a dead suffix after terminal DoTownEvent")!=std::string::npos ||
                     reason.find("Census limit: isolated original self-target;")!=std::string::npos);
                require(excluded || admission || limit,"Unresolved in-scope Clouds Event: "+mechanic+" / "+reason);
            }
            std::set<unsigned> maps;std::stringstream ids(columns[5]);
            while(std::getline(ids,column,','))try {maps.insert(std::stoul(trim(column)));}catch(...){}
            if(dimension=="Events" && mechanic.rfind("17 DoTownEvent action=",0)==0) {
                const auto action=std::stoul(mechanic.substr(std::string("17 DoTownEvent action=").size()));
                if(action<=5) {
                    require(status=="supported","Unexpected Clouds service gap: "+mechanic+" / "+reason);
                    townSites[action]+=std::stoul(columns[3]);
                }
            }
            if(maps.count(28)) {
                if(status=="partial" || status=="unsupported") {
                    require(documentedGap(dimension,mechanic,status,reason),"Unexpected Vertigo gap: "+dimension+" / "+mechanic+" / "+reason);++gaps;
                }
                if(dimension=="Events" && status=="unclassified") {
                    require(unresolvedSubroutine(mechanic,reason),"Unclassified accepted Vertigo Event: "+mechanic+" / "+reason);++unused;
                }
                if(dimension=="Events" && status=="supported")supported.insert(mechanic);
                if(dimension=="Services" && status=="supported")supportedServices.insert(mechanic);
            }
            if(maps.count(23) && dimension=="Events" && (mechanic=="14 Remove" || mechanic=="12 TakeOrGive modes=0,8,0" ||
                    mechanic=="12 TakeOrGive modes=21,0,0 operand1=99" || mechanic=="12 TakeOrGive modes=21,0,0 operand1=100"))
                require(status=="supported","Accepted mainland operation misclassified: "+mechanic+" / "+reason);
        }
        for(unsigned action=0;action<6;++action)require(supported.count("17 DoTownEvent action="+std::to_string(action)),"Vertigo service missing from supported set");
        require(supported.count("22 Damage type=5"),"Vertigo well damage not supported");
        for(const auto prefix:{"12 TakeOrGive modes=0,16,0","12 TakeOrGive modes=0,15,0","16 Spawn slot="}) {
            bool found=false;for(const auto &name:supported)found=found || name.rfind(prefix,0)==0;
            require(found,std::string("Accepted Vertigo mechanic missing: ")+prefix);
        }
        for(const auto action:{"Drink","Tip","Rumors","Donation","Uncurse"})
            require(supportedServices.count(action),std::string("Implemented Vertigo service missing: ")+action);
        require(townSites==std::array<unsigned,6>{3,4,6,3,4,3},"Clouds service site inventory changed");
        require(gaps==4 && unused==4,"Vertigo documented gap/subroutine inventory changed; explain and recalibrate");
        require(waterDragon==2 && vultureRoc==2,"Species 59 species and movement rows are missing from the census");
        require(movement>=4,"Monster movement rows are missing from the census");
        require(lightingRows.size()==3 && skillWalls.size()==7,"Lighting/traversal consumer census rows are missing");
        std::cout<<"Vertigo calibration: "<<gaps<<" documented gap rows; "<<unused<<" unresolved-subroutine exceptions; accepted effects and services supported\n";
        return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

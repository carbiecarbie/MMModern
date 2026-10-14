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
    // Expectations only, never classification inputs. docs/project-status.md:
    // magic mirror, silent audio, Tavern/Temple actions. ConfirmWord belongs
    // to the mirror's destination-name input, not an accepted town interaction.
    if(dimension=="Events") {
        const std::map<std::string,std::string> audio{{"6 PlayFX","Sound effects not supported yet"},
            {"40 PlayEventVoc","Voice audio not supported yet"},{"60 PlayCD","CD speech not supported yet"}};
        if(const auto i=audio.find(mechanic);i!=audio.end())return status=="partial" && reason.find(i->second)!=std::string::npos;
        if(mechanic=="7 TeleportAndExit destination=0")return status=="unsupported" && reason.find("mirror teleport")!=std::string::npos;
        if(mechanic=="21 ConfirmWord inputType=0")return status=="unsupported" && reason.find("opcode 21")!=std::string::npos;
    }
    const std::set<std::string> actions{"Drink","Tip","Rumors","Sign In","Donation","Uncurse"};
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
        std::string line,dimension;bool clouds=false;unsigned gaps=0,unused=0;
        std::set<std::string> supported;
        while(std::getline(input,line)) {
            if(line=="## Clouds detail")clouds=true;
            if(line=="## Darkside detail")break;
            if(!clouds)continue;
            if(line.rfind("### ",0)==0)dimension=line.substr(4);
            std::stringstream stream(line);std::vector<std::string> columns;std::string column;
            while(std::getline(stream,column,'|'))columns.push_back(trim(column));
            if(columns.size()!=7)continue;
            const auto &mechanic=columns[1],&status=columns[2],&reason=columns[6];
            std::set<unsigned> maps;std::stringstream ids(columns[5]);
            while(std::getline(ids,column,','))try {maps.insert(std::stoul(trim(column)));}catch(...){}
            if(maps.count(28)) {
                if(status=="partial" || status=="unsupported") {
                    require(documentedGap(dimension,mechanic,status,reason),"Unexpected Vertigo gap: "+dimension+" / "+mechanic+" / "+reason);++gaps;
                }
                if(dimension=="Events" && status=="unclassified") {
                    require(unresolvedSubroutine(mechanic,reason),"Unclassified accepted Vertigo Event: "+mechanic+" / "+reason);++unused;
                }
                if(dimension=="Events" && status=="supported")supported.insert(mechanic);
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
        require(gaps==11 && unused==4,"Vertigo documented gap/subroutine inventory changed; explain and recalibrate");
        std::cout<<"Vertigo calibration: "<<gaps<<" documented gap rows; "<<unused<<" unresolved-subroutine exceptions; accepted effects and services supported\n";
        return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

#ifndef MMODERN_TEST_M40_EVIDENCE_H
#define MMODERN_TEST_M40_EVIDENCE_H
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include "games/xeen/XeenSaveSnapshot.h"
#include "games/xeen/XeenStateEquality.h"
namespace m40_test {
using namespace mmodern;
inline void check(bool value,const char *message) { if(!value)throw std::runtime_error(message); }
inline std::vector<std::uint8_t> diskBytes(const std::filesystem::path &path) {
    std::ifstream input(path,std::ios::binary);check(bool(input),"M40 previous disk read failed");
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
inline std::string sha256(const std::vector<std::uint8_t> &bytes) {
    BCRYPT_ALG_HANDLE algorithm=nullptr; BCRYPT_HASH_HANDLE hash=nullptr;
    DWORD objectLength=0,hashLength=0,received=0;
    const auto cleanup=[&]{if(hash)BCryptDestroyHash(hash);if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);};
    std::vector<std::uint8_t> object,digest;
    try {
        check(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0,"M40 SHA-256 provider");
        check(BCryptGetProperty(algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&objectLength),sizeof(objectLength),&received,0)>=0,"M40 hash object length");
        check(BCryptGetProperty(algorithm,BCRYPT_HASH_LENGTH,reinterpret_cast<PUCHAR>(&hashLength),sizeof(hashLength),&received,0)>=0,"M40 digest length");
        object.resize(objectLength);digest.resize(hashLength);
        check(BCryptCreateHash(algorithm,&hash,object.data(),objectLength,nullptr,0,0)>=0,"M40 create hash");
        check(BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0)>=0,"M40 hash bytes");
        check(BCryptFinishHash(hash,digest.data(),hashLength,0)>=0,"M40 finish hash");cleanup();
    }catch(...){cleanup();throw;}
    std::ostringstream result;result<<std::hex<<std::setfill('0');
    for(auto value:digest)result<<std::setw(2)<<unsigned(value);
    return result.str();
}
inline std::vector<std::uint8_t> stockBytes(const XeenServiceEconomy &economy) {
    std::vector<std::uint8_t> bytes;bytes.reserve(1152);
    for(const auto &side:economy.wares.records)for(const auto &shop:side)for(const auto &category:shop)for(const auto &item:category)
        for(auto value:{std::uint8_t(item.material),std::uint8_t(item.id),std::uint8_t(item.state),std::uint8_t(item.frame)})bytes.push_back(value);
    return bytes;
}
inline void equalFields(const XeenSaveSnapshot &a,const XeenSaveSnapshot &b) {
    check(a.journey && b.journey,"M40 Journey presence differs");const auto &x=*a.journey,&y=*b.journey;
    for(unsigned owner=0;owner<30;++owner)check(xeen_state::sameCharacter(a.characters[owner],b.characters[owner]) &&
        x.supplements[owner].owner==y.supplements[owner].owner && xeen_state::sameInputs(x.supplements[owner].inputs,y.supplements[owner].inputs),"M40 character/books/raw items/supplement differs");
    const auto actors=[](const auto &left,const auto &right) {
        check(left.size()==right.size(),"M40 actor count differs");
        for(std::size_t i=0;i<left.size();++i){const auto &l=left[i],&r=right[i];
            check(l.id==r.id && l.x==r.x && l.y==r.y && l.hp==r.hp && l.activated==r.activated &&
                l.lifecycle==r.lifecycle && l.status==r.status && l.accounted==r.accounted,"M40 actor field differs");}
    };
    actors(x.actors,y.actors);check(x.vertigoActors.has_value()==y.vertigoActors.has_value(),"M40 city presence differs");
    if(x.vertigoActors)actors(*x.vertigoActors,*y.vertigoActors);
    check(a.resources==b.resources && a.activeRosterIds==b.activeRosterIds && a.camera.mapId==b.camera.mapId &&
        a.camera.x==b.camera.x && a.camera.y==b.camera.y && a.camera.direction==b.camera.direction &&
        a.questItems==b.questItems && a.questFlags==b.questFlags && a.gameFlags==b.gameFlags &&
        a.disabledEvents==b.disabledEvents && a.disabledObjects==b.disabledObjects &&
        x.entry==y.entry && x.schema==y.schema && x.contract==y.contract && x.initializedMap==y.initializedMap &&
        x.originalActorCount==y.originalActorCount && x.skeletonSeed==y.skeletonSeed && x.context==y.context &&
        x.random==y.random && x.treasure==y.treasure && x.regionalRecovery==y.regionalRecovery,"M40 domain/context/purse/recovery/flags differs");
    check(x.serviceEconomy.has_value()==y.serviceEconomy.has_value(),"M40 economy presence differs");
    if(x.serviceEconomy)check(stockBytes(*x.serviceEconomy)==stockBytes(*y.serviceEconomy) &&
        x.serviceEconomy->bank.gold==y.serviceEconomy->bank.gold && x.serviceEconomy->bank.gems==y.serviceEconomy->bank.gems,"M40 stock/bank field differs");
}
}
#endif

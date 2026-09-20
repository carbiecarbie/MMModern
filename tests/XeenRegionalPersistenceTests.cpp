#include "XeenSaveTestSupport.h"
#include <iostream>
using namespace save_test;
int main() {
	try {
		XeenSaveSnapshot s;s.resources.clouds={1,2};s.resources.darkside=XeenArchiveFingerprint{3,4};s.camera={23,8,11,XeenDirection::West};
		s.journey.emplace();auto &j=*s.journey;j.schema=j.contract=3;j.initializedMap=23;j.originalActorCount=19;j.skeletonSeed=0;
		j.context=XeenGameplayContext{};j.context->day=42;j.context->year=611;j.context->minutes=1000;j.context->ctr24=23;
		j.random=XeenJourneyRandomState{1,0xf1234567,0x123456789ULL};
		for (unsigned i=0;i<30;++i) {j.supplements[i].owner=i;j.supplements[i].inputs.luck=XeenAttributeValue{17,3};}
		for (unsigned i=0;i<19;++i) j.actors.push_back({{23,i},int(i%16),int(i/16),int(i+1),bool(i%2),XeenActorLifecycle::Present,XeenActorStatus::Physical,false});
		const auto bytes=XeenSaveFormat::encode(s);auto base=s;base.journey.reset();const auto offset=XeenSaveFormat::encode(base).size();
		check(bytes.size()-offset==1651,"Schema-3 exact suffix length");
		check(bytes[offset]==3 && bytes[offset+1]==3 && bytes[offset+3]==3 && bytes[offset+5]==1 && bytes[offset+39]==30,"Schema-3 literal prefix");
		check(bytes[offset+1270]==1 && bytes[offset+1271]==0x67 && bytes[offset+1274]==0xf1 && bytes[offset+1275]==0x89,"Schema-3 literal random fields");
		check(bytes[offset+1283]==0 && bytes[offset+1284]==23 && bytes[offset+1286]==19 && bytes[offset+1288]==19,"Schema-3 literal map and counts");
		for(unsigned i=0;i<19;++i)check(bytes[offset+1290+19*i]==0 && bytes[offset+1291+19*i]==23 && bytes[offset+1293+19*i]==i,"Complete ordered wire identities");
		check(XeenSaveFormat::encode(XeenSaveFormat::decode(bytes))==bytes,"Complete representation roundtrip");
		const auto badByte=[&](std::size_t at,unsigned value){auto b=bytes;b[offset+at]=value;fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});};
		for(unsigned value:{0U,1U,2U,4U,255U}) {badByte(1,value);badByte(3,value);}
		for(unsigned value:{0U,1U,18U,20U,107U,108U,255U}) {badByte(1286,value);badByte(1288,value);}
		for(auto at:{5U,37U,38U})badByte(at,2);
		badByte(6,1);badByte(7,2);badByte(39,29);badByte(1270,2);badByte(1283,1);badByte(1284,20);
		for(unsigned i=0;i<30;++i) {badByte(40+41*i,31);badByte(40+41*i+34,1);}
		for(unsigned i=0;i<19;++i) {
			const auto at=1290+19*i;
			badByte(at,1);badByte(at+1,20);badByte(at+3,19);
			badByte(at+15,2);badByte(at+16,4);badByte(at+17,2);badByte(at+18,2);
		}
		for(std::size_t size=offset;size<bytes.size();++size) {auto b=bytes;b.resize(size);fixIndependentEnvelope(b);rejects([&]{XeenSaveFormat::decode(b);});}
		auto extra=bytes;extra.push_back(0);fixIndependentEnvelope(extra);rejects([&]{XeenSaveFormat::decode(extra);});
		auto wrong=s;wrong.journey->skeletonSeed=1;rejects([&]{XeenSaveFormat::encode(wrong);});
		wrong=s;wrong.journey->supplements[0].inputs.luck.reset();rejects([&]{XeenSaveFormat::encode(wrong);});
		std::cout << "Schema-3 exact layout, full coverage and malformed-wire controls passed\n";
		return 0;
	} catch(const std::exception &e) {std::cerr << e.what() << '\n';return 1;}
}

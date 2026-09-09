#include "games/xeen/XeenOutdoorScene.h"
#include "games/xeen/XeenWorld.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>

using namespace mmodern;
namespace {
void check(bool v, const char *s) { if (!v) throw std::runtime_error(s); }
XeenMap flat(XeenMapIdentity id) {
	XeenMap m; m.side=id.side; m.geometry.id=id.number; m.geometry.flags2=0x8000;
	for(auto &c:m.geometry.cells) c.geometry=XeenOutdoorLayers{};
	return m;
}
XeenObjectFile objects(XeenMapIdentity id, std::vector<XeenMapEntity> records) {
	return {id,"synthetic.mob",true,{ {},{},{},std::move(records),{},{} }};
}
std::vector<XeenOutdoorDrawCommand> objectCommands(const std::vector<XeenOutdoorDrawCommand> &all) {
	std::vector<XeenOutdoorDrawCommand> result;
	for(const auto &c:all) if(c.object()) result.push_back(c);
	return result;
}
XeenObjectVisualResolver resolver(bool animated = false) {
	std::vector<std::uint8_t> b(1452);
	for(int id:{111,113}) for(int d=0;d<4;++d) {
		b[id*12+d]=d; b[id*12+4+d]=d%2; b[id*12+8+d]=d+(animated?3:0);
	}
	b[110*12+8]=4; // Temporal when facing North.
	return XeenObjectVisualResolver(XeenCloudsVisualMetadata::parse(b));
}
void projection() {
	const int depths[]={0,1,1,1,2,2,2,3,3,3,3,3};
	const int laterals[]={0,-1,0,1,-1,0,1,-2,-1,0,1,2};
	const int samples[]={2,5,7,9,12,14,16,23,25,27,29,31};
	const int orders[]={111,88,87,89,67,66,68,40,38,37,39,41};
	const int scales[]={0,7,7,7,12,12,12,14,14,14,14,14};
	const int xs[2][12]={{-5,-112,-7,98,-77,-8,61,-74,-43,-9,25,56},
		{-35,-142,-35,68,-95,-35,19,-98,-62,-35,-24,16}};
	const int ys[2][12]={{2,25,25,25,50,50,50,58,58,58,58,58},
		{-65,-6,-6,-6,36,36,36,54,54,54,54,54}};
	const int fx[]={0,1,0,-1}, fy[]={1,0,-1,0}, rx[]={1,0,-1,0}, ry[]={0,-1,0,1};
	for(bool animated:{false,true}) for(const auto phase: {std::optional<std::uint64_t>{},std::optional<std::uint64_t>{0},std::optional<std::uint64_t>{1},std::optional<std::uint64_t>{3},std::optional<std::uint64_t>{5}}) {
	if(animated && !phase) continue; // Omitted animated precedence is checked separately.
	const auto r=resolver(animated);
	for(int row=0;row<2;++row) for(int direction=0;direction<4;++direction) {
		std::vector<XeenMapEntity> records;
		for(int p=0;p<12;++p) records.push_back({8+fx[direction]*depths[p]+rx[direction]*laterals[p],
			8+fy[direction]*depths[p]+ry[direction]*laterals[p],0,0,row?113:111});
		records.push_back({1,1,0,0,111}); // Outside all twelve view positions.
		int loads=0;
		XeenWorld world([](auto id){return flat(id);},[&](auto id){++loads;return objects(id,records);});
		const auto all=XeenOutdoorScene().build(world,{23,8,8,static_cast<XeenDirection>(direction)},&r,nullptr,phase);
		const auto commands=objectCommands(all);
		check(commands.size()==12 && loads==1,"12-position object load/count");
		check(std::is_sorted(all.begin(),all.end(),[](const auto &a,const auto &b){return a.originalOrder<b.originalOrder;}),"shared sort");
		for(const auto &c:commands) {
			const auto &v=c.object()->visual;const auto p=v.identity.recordIndex;
			check(v.identity.mapId==XeenMapIdentity{23} && p<12,"stable identity");
			check(c.sourceX==records[p].x && c.sourceY==records[p].y && c.sourceMapId==23,"raw rotation");
			check(c.sampleIndex==samples[p] && c.originalOrder==orders[p] && c.x==xs[row][p] && c.y==ys[row][p],"projection/table row");
			const auto o=c.drawOptions();
			check(o.scaleIndex==scales[p] && o.sceneClipped && o.bottomClipped==(p==0) && !o.enlarge,"draw options");
			check(v.frame==static_cast<unsigned>(direction)+(animated?*phase%3:0) && v.horizontalFlip==(direction%2!=0) && o.horizontalFlip==v.horizontalFlip,"authoritative direction and phase");
			check(v.status==(animated?XeenObjectVisualStatus::SupportedAnimated:XeenObjectVisualStatus::SupportedStatic),"placement status");
		}
	}
	}
}
void stateAndPrecedence() {
	const auto r=resolver();
	const XeenCamera camera{23,8,8,XeenDirection::North};
	for(int firstId:{111,110,121}) {
		XeenWorld world([](auto id){return flat(id);},[&](auto id){return objects(id,{{8,8,0,0,firstId},{8,8,0,0,111}});});
		std::vector<XeenObjectVisual> diagnostics;
		const auto c=objectCommands(XeenOutdoorScene().build(world,camera,&r,&diagnostics));
		if(firstId==111) check(c.size()==1 && c[0].object()->visual.identity.recordIndex==0 && diagnostics.empty(),"first applicable record");
		else check(c.empty() && diagnostics.size()==1 && diagnostics[0].identity.recordIndex==0 &&
			diagnostics[0].status==(firstId==110?XeenObjectVisualStatus::UnsupportedAnimation:XeenObjectVisualStatus::Invalid),"unsupported first promoted overlap");
		check(world.selectObject(camera)==XeenObjectIdentity{23,0},"rendering changed interaction selection");
		for(std::uint64_t phase:{0,3,5}) {
			const auto explicitCommands=objectCommands(XeenOutdoorScene().build(world,camera,&r,&diagnostics,phase));
			if(firstId==121) check(explicitCommands.empty() && diagnostics.size()==1,"invalid first no longer blocks overlap");
			else check(explicitCommands.size()==1 && explicitCommands[0].object()->visual.identity.recordIndex==0 &&
				explicitCommands[0].object()->visual.frame==(firstId==110?phase%4:0),"animated first precedence");
		}
		world.disableObject({23,0});
		const auto next=objectCommands(XeenOutdoorScene().build(world,camera,&r));
		check(next.size()==1 && next[0].object()->visual.identity.recordIndex==1,"disabled first blocks eligible overlap");
		world.discardMapCache();
		for(std::uint64_t phase:{0,2,9}) {
			const auto rebuilt=objectCommands(XeenOutdoorScene().build(world,camera,&r,nullptr,phase));
			check(rebuilt.size()==1 && rebuilt[0].object()->visual.identity.recordIndex==1,"phase/reload revived disabled first");
		}
	}
	int mapLoads=0, mobLoads=0;
	XeenWorld world([&](auto id){++mapLoads;return flat(id);},[&](auto id){++mobLoads;return objects(id,
		{{8,-128,0,0,111},{8,8,0,0,-1},{8,8,0,0,255},{8,8,0,0,111},{8,9,0,0,111}});});
	world.disableObject({24,3});world.disableObject({{XeenSide::Darkside,23},3});
	auto c=objectCommands(XeenOutdoorScene().build(world,camera,&r));
	check(c.size()==2,"ineligible records or unrelated mutations suppressed valid objects");
	world.disableObject({23,3});
	c=objectCommands(XeenOutdoorScene().build(world,camera,&r));
	check(c.size()==1 && c[0].object()->visual.identity==XeenObjectIdentity{23,4},"shared resource identity conflated");
	const auto beforeMap=mapLoads,beforeMob=mobLoads;
	world.discardMapCache();
	c=objectCommands(XeenOutdoorScene().build(world,camera,&r));
	check(c.size()==1 && c[0].object()->visual.identity==XeenObjectIdentity{23,4} && mapLoads==beforeMap+1 && mobLoads==beforeMob+1,"cache reload revived disabled object");
	check(world.objectFile(23).entities.objects.size()==5,"rendering mutated base objects");
}
void boundary() {
	const auto r=resolver();std::map<XeenMapIdentity,int> mapLoads,mobLoads;
	XeenWorld world([&](auto id){++mapLoads[id];auto m=flat(id);if(id==23)m.geometry.neighbors[0]=24;return m;},
		[&](auto id){++mobLoads[id];return objects(id,id==23?std::vector<XeenMapEntity>{{8,16,0,0,111}}:std::vector<XeenMapEntity>{{8,0,0,0,111}});});
	const auto all=XeenOutdoorScene().build(world,{23,8,15,XeenDirection::North},&r);
	const auto c=objectCommands(all);
	check(mapLoads[24]==1 && mobLoads[23]==1 && mobLoads[24]==0,"neighbor MOB was loaded");
	check(c.size()==1 && c[0].sourceY==16 && c[0].sourceMapId==23 && c[0].sampleIndex==7,"raw out-of-grid object not matched");
	check(std::any_of(all.begin(),all.end(),[](const auto &v){return !v.object() && v.sourceMapId==24;}),"terrain neighbor sampling lost");
}
void sharedPhase() {
	std::vector<std::uint8_t> bytes(1452);
	bytes[110*12]=4;bytes[110*12+8]=7;
	bytes[111*12]=8;bytes[111*12+8]=13;
	const XeenObjectVisualResolver r(XeenCloudsVisualMetadata::parse(bytes));
	int maps=0,mobs=0;
	XeenWorld world([&](auto id){++maps;return flat(id);},[&](auto id){++mobs;return objects(id,
		{{8,8,0,0,110},{8,9,0,0,111},{8,10,0,0,110},{1,1,0,0,110}});});
	const XeenCamera camera{23,8,8,XeenDirection::North};
	for(std::uint64_t phase:{7,0,4,7}) {
		const auto c=objectCommands(XeenOutdoorScene().build(world,camera,&r,nullptr,phase));
		check(c.size()==3,"visible shared phase objects");
		for(const auto &cmd:c) {
			const auto &v=cmd.object()->visual;
			check(v.frame==(v.identity.recordIndex==1?8+phase%5:4+phase%3),"independent cycle reduction");
		}
	}
	const auto appeared=objectCommands(XeenOutdoorScene().build(world,{23,1,1,XeenDirection::North},&r,nullptr,7));
	check(appeared.size()==1 && appeared[0].object()->visual.identity==XeenObjectIdentity{23,3} && appeared[0].object()->visual.frame==5,"offscreen record started a new cycle");
	world.disableObject({23,0});
	const auto previousMaps=maps,previousMobs=mobs;world.discardMapCache();
	for(std::uint64_t phase:{0,2,7}) {
		const auto c=objectCommands(XeenOutdoorScene().build(world,camera,&r,nullptr,phase));
		check(c.size()==2 && std::none_of(c.begin(),c.end(),[](const auto &v){return v.object()->visual.identity.recordIndex==0;}),"shared resource removal");
	}
	check(maps==previousMaps+1 && mobs==previousMobs+1 && world.sessionState().disabledObjectCount()==1 && world.objectFile(23).entities.objects.size()==4,"cache/state reconstruction");
	check(objectCommands(XeenOutdoorScene().build(world,camera,nullptr,nullptr,7)).empty(),"null resolver emitted objects");
}
}
int main(){try{projection();stateAndPrecedence();boundary();sharedPhase();std::cout<<"Outdoor object commands passed\n";return 0;}
	catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}

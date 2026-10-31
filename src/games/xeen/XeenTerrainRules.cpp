#include "games/xeen/XeenTerrainRules.h"
#include <stdexcept>
namespace mmodern {
XeenCamera xeenTerrainFallDestination(const XeenCamera &source) {
 auto c=source;
 if(c.mapId.side==XeenSide::Darkside) {
  // DOS Darkside dispatch 0x0cb70..0x0cc91, table 0x0cd22.
  const auto id=c.mapId.number;
  if(id>88 && id<113) {c.mapId={XeenSide::Darkside,std::uint16_t(id-88)};return c;}
  unsigned to=29;int x=25,y=21;bool north=false;
  if(id==25) {to=89;x=2;y=14;}
  else if(id==26) {to=109;x=13;y=14;}
  else if(id==27) {to=112;x=13;y=3;}
  else if(id==28) {to=92;x=2;y=3;}
  else if(id==37 || id==38) {to=14;x=10;y=2;}
  else if(id>=41 && id<=43) {to=4;x=5;y=14;}
  else if(id>=45 && id<=47) {to=21;x=9;y=11;}
  else if(id>=49 && id<=51) {to=1;x=10;y=4;}
  else if(id>=53 && id<=56) {to=26;x=12;y=10;}
  else if(id>=57 && id<=60) {to=3;x=4;y=9;}
  else if(id>=61 && id<=64) {to=16;x=2;y=7;}
  else if(id>=65 && id<=68) {to=23;x=10;y=9;}
  else if(id>=69 && id<=72) {to=13;x=2;y=10;}
  else if(id==128 || id==129) {c.mapId={XeenSide::Clouds,8};c.x=11;c.y=15;c.direction=XeenDirection::North;return c;}
  else north=true;
  c.mapId={XeenSide::Darkside,std::uint16_t(to)};c.x=x;c.y=y;if(north)c.direction=XeenDirection::North;return c;
 }
 // DOS Clouds table 0x0cdf4 and destination writes 0x0cad0..0x0cb6d.
 const auto id=c.mapId.number;
 if(id==25 || (id>=51 && id<=54)) {c.mapId=24;c.x=11;c.y=9;}
 else if(id==26 || (id>=55 && id<=58)) {c.mapId=12;c.x=6;c.y=15;}
 else if(id==27 || (id>=59 && id<=62) || (id>=76 && id<=78)) {c.mapId=15;c.x=4;c.y=12;c.direction=XeenDirection::South;}
 else if(id==65 || id==66) {c.mapId=14;c.x=8;c.y=3;c.direction=XeenDirection::South;}
 else if(id==69 || id==70) {c.mapId=1;c.x=8;c.y=7;c.direction=XeenDirection::North;}
 else if(id==74) {c.mapId=12;c.x=11;c.y=13;c.direction=XeenDirection::South;}
 else if(id>=82 && id<=84) {c.mapId=5;c.x=12;c.y=7;c.direction=XeenDirection::North;}
 else if(id==85) {c.mapId=6;c.x=12;c.y=3;c.direction=XeenDirection::North;}
 else {c.mapId=23;c.x=12;c.y=10;c.direction=XeenDirection::North;}
 return c;
}
XeenTerrainArrival xeenTerrainArrival(unsigned surface,bool outdoor,bool navigator,bool levitating,const XeenCamera &camera,bool darksideFlag118) {
 if(surface>15)throw std::invalid_argument("Invalid arrival surface type");
 XeenTerrainArrival r;r.surface=surface;r.transition={XeenTransitionResult::Status::Available,camera,{}};
 if(surface==15)r.dead=true;
 else if(surface==5) {r.damage=100;r.type=XeenDamageType::Fire;}
 else if(surface==10 || (surface==13 && !levitating)) {r.damage=100;r.falling=true;r.transition.destination=xeenTerrainFallDestination(camera);}
 else if(surface==6 && outdoor && !navigator)r.minutes=170;
 // DOS 0x0c9af..0x0c9ce skips the entire fall/damage suffix on Darkside.
 if(camera.mapId.side==XeenSide::Darkside && darksideFlag118) {
  r.damage=0;r.falling=false;r.transition.destination=camera;
 }
 return r;
}
XeenTerrainArrivalCandidate::XeenTerrainArrivalCandidate(const XeenTerrainArrival &r,
 const XeenConsequenceCharacters &c,const XeenConsequenceInputs &i,const XeenGameplayContext &ctx,const XeenServiceEconomy *e):
 rule(r),characters(c),inputs(i),context(ctx) {
 if(!r)throw std::invalid_argument("Terrain arrival destination is unavailable");
 if(e)economy=*e;
}
bool XeenTerrainArrivalCandidate::service(XeenConsequenceDraw &draw) {
 if(!timeDone) {
  if(rule.minutes) {
   if(!time)time.emplace(context,rule.minutes,characters,inputs,economy?&*economy:nullptr,XeenTimeMode::Interactive,XeenTimeCall::Add);
   if(!time->service(draw))return false;
   characters=time->characters;inputs=time->inputs;context=time->context;economy=time->economy;needsRest=time->needsRest;resetTemps=time->resetTemps;
  }
  timeDone=true;
 }
 while(rule.damage && member<characters.size()) {
  if(!injury) {
   XeenDamageProtection protection;
   for(unsigned n=0;n<4;++n)protection.resistances[n]=context.lightAndResistances[n+2];
   protection.powerShield=context.effects[XeenPartyEffect::PowerShield];protection.blessed=context.effects[XeenPartyEffect::Bless];
   injury.emplace(characters,inputs,int(rule.damage),rule.type,context.year,1u<<member,protection);
  }
  if(!injury->service(draw))return false;
  characters=injury->characters;injury.reset();++member;
 }
 return true;
}
}

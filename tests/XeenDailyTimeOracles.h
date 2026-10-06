// Part A oracles, adapted from pinned ScummVM 6814ee9b party.cpp.
// Weak sentinel comparisons are provisional, authorized 2026-10-06; not DOS-confirmed.
void dailyTimeOracles() {
 auto c=characters();auto in=inputs();XeenGameplayContext t;t.day=99;t.year=610;t.minutes=1439;
 auto next=xeenPrepareTime(t,1);
 check(next.context.day==0 && next.context.year==611 && next.context.minutes==0 && next.context.newDay,
  "100-day calendar rollover must retain pending dawn");
 t.day=8;t.minutes=480;
 check(xeenPrepareTime(t,2000).processing480==1,"One long changeTime call has one eight-hour block");
 // Terminal fixtures bypass effective-stat death, and nonzero Poison/Disease skip their draws.
 for(auto &v:c){v.conditions[13]=255;v.conditions[3]=v.conditions[4]=1;}
 const unsigned values[]{0,1,127,128,254,255};
 for(unsigned n=0;n<6;++n){c[n].conditions[2]=values[n];c[n].conditions[7]=3;c[n].conditions[5]=255;}
 XeenCombatRandom none(std::vector<XeenCombatRandom::Draw>{});
 XeenConditionTimeCandidate block(t,480,c,in);finish(block,none,1);
 for(unsigned n=0;n<6;++n) {
  check(block.characters[n].conditions[2]==(values[n]==255?255:3) &&
   block.characters[n].conditions[7]==(values[n]==255?3:0),"Provisional FF sentinel must guard replacement and Drunk clearing");
  check(block.characters[n].conditions[13]==255 && block.characters[n].conditions[5]==0,
   "Terminal counters saturate; Insane wraps");
 }
 t.minutes=299;t.newDay=true;
 XeenConditionTimeCandidate dawn(t,1,c,in);finish(dawn,none,1);
 for(unsigned n=0;n<6;++n)check(dawn.characters[n].conditions[2]==(values[n]==255?255:values[n]+1),
  "Dawn increments positive high bytes but preserves FF sentinel");
 check(!dawn.context.newDay,"Dawn is consumed once");
 t.minutes=1259;t.newDay=false;
 XeenConditionTimeCandidate dusk(t,1,c,in);finish(dusk,none,1);
 check(dusk.context.minutes==1260 && !none.position(),"Dusk changes sky time without condition RNG");
 c=characters();c[0].conditions[10]=2;c[0].conditions[11]=2;
 // Character physical save: statBonus(Luck 15) + level 3 + 20, no equipment bonus.
 t.minutes=500;
 XeenCombatRandom confusion(std::vector<XeenCombatRandom::Draw>{{0,2,1},{1,25,1},{0,4,1}});
 XeenConditionTimeCandidate each(t,1,c,in);finish(each,confusion,1);
 check(each.characters[0].conditions[10]==0 && each.characters[0].conditions[11]==1 && confusion.position()==3,
  "Confused and Paralyzed process on every changeTime in active order");
 finish(each,confusion,1);check(confusion.position()==3,"Completed time never replays RNG");

 // Same-call transitions: HeartBroken->Depressed then Depressed advances;
 // InLove->HeartBroken runs after HeartBroken's own branch.
 c=characters();for(auto &v:c){v.conditions[3]=v.conditions[4]=1;}
 c[0].conditions[1]=10;c[0].conditions[6]=10;c[1].conditions[1]=255;c[1].conditions[6]=255;
 t.minutes=479;
 XeenConditionTimeCandidate ordered(t,1,c,in);finish(ordered,none,1);
 check(ordered.characters[0].conditions[1]==1 && ordered.characters[0].conditions[6]==0 &&
  ordered.characters[0].conditions[9]==2 && !ordered.characters[1].conditions[1] && !ordered.characters[1].conditions[6],
  "Transition order and unguarded HeartBroken/InLove wrap");
 t.minutes=299;t.newDay=true;
 in[0].resistances->fireTemporary=19;in[0].resistances->magicTemporary=21;
 in[0].resistances->energyTemporary=23;in[0].resistances->coldTemporary=17;
 in[0].resistances->electricalTemporary=15;in[0].poisonResistance=XeenAttributeValue{11,13};
 in[0].might.temporary=9;in[0].speed.temporary=8;in[0].accuracy.temporary=7;
 in[0].luck->temporary=6;in[0].temporaryAc=5;c[0].temporaryAge=4;
 c[0].intellect.temporary=3;c[0].personality.temporary=2;c[0].endurance.temporary=1;c[0].temporaryLevel=9;
 t.effects[0]=2;t.effects[1]=1;t.effects[2]=1;t.effects[3]=1;t.effects[4]=1;
 for(unsigned n=5;n<9;++n)t.effects[n]=n;
 for(unsigned n=0;n<6;++n)t.lightAndResistances[n]=n+1;
 for(auto mode:{XeenTimeMode::Interactive,XeenTimeMode::Sleeping,XeenTimeMode::Script,XeenTimeMode::Interactive7}) {
  XeenConditionTimeCandidate daily(t,1,c,in,nullptr,mode,XeenTimeCall::Add);finish(daily,none,1);
  const bool suppressed=mode==XeenTimeMode::Script || mode==XeenTimeMode::Interactive7;
  check(!daily.context.newDay && daily.resetTemps==!suppressed && daily.needsRest==(mode==XeenTimeMode::Interactive),
   "Mode-specific dawn consumption, reset and message");
  check(daily.characters[0].temporaryAge==4 && daily.context.effects[1]==1 && daily.context.lightAndResistances[1]==2,
   "Age, automap and torch count survive reset");
  check(daily.inputs[0].resistances->fireTemporary==(suppressed?19:0) &&
   daily.inputs[0].resistances->energyTemporary==(suppressed?23:0) &&
   daily.inputs[0].resistances->magicTemporary==(suppressed?21:0) &&
   daily.characters[0].temporaryLevel==(suppressed?9:0) && daily.inputs[0].temporaryAc==(suppressed?5:0),
   "Live missing resistance pairs and character bonuses reset only in daily modes");
 }
 // Long calls stock once, then interest, then dawn; use an independent existing
 // stock candidate as the RNG/wares oracle, with no replay at publication.
 XeenCombatRandom initial(7);XeenMerchantStockCandidate stocked;finish(stocked,initial,1);
 XeenServiceEconomy economy;economy.wares=stocked.wares();economy.bank={1000,200};
 XeenCombatRandom oracle(initial.continuation());XeenMerchantStockCandidate newStock;finish(newStock,oracle,1);
 c=characters();in=inputs();for(auto &v:c){v.conditions[3]=v.conditions[4]=1;v.conditions[5]=1;}
 t=XeenGameplayContext{};t.year=610;t.day=8;t.minutes=480;
 XeenConditionTimeCandidate longCall(t,2000,c,in,&economy);auto copy=longCall;
 XeenCombatRandom charged(initial.continuation());finish(longCall,charged,1);
 check(longCall.context.day==9 && longCall.context.minutes==1040 && longCall.characters[0].conditions[5]==2 &&
  longCall.economy->wares==newStock.wares() && longCall.economy->bank==XeenBankBalances{1010,202} &&
  charged.continuation()==oracle.continuation(),"Long call condition block, stock/interest and single shared cursor");
 XeenCombatRandom copied(initial.continuation());finish(copy,copied,1);
 check(copy.economy==longCall.economy && copied.continuation()==charged.continuation(),"Copied time stock continuation is independent");
 XeenConditionTimeCandidate wholeYear(t,100*1440,c,in,&economy);finish(wholeYear,none,1);
 check(wholeYear.context.year==611 && wholeYear.context.day==8 && !wholeYear.context.newDay &&
  wholeYear.economy==economy && !wholeYear.needsRest,"Same ending day after whole years skips stock and dawn");

 // The stat-death block uses the old year; age changes only after addTime.
 c=characters();in=inputs();c[0].birthYear=575;in[0].might.permanent=1;
 t=XeenGameplayContext{};t.year=610;t.day=99;t.minutes=1439;
 std::vector<XeenCombatRandom::Draw> cleanTick;
 for(unsigned n=0;n<6;++n)cleanTick.insert(cleanTick.end(),{{1,10,2},{0,9,0}});
 XeenCombatRandom oldYear(cleanTick);XeenConditionTimeCandidate rollover(t,1,c,in);finish(rollover,oldYear,1);
 check(!rollover.characters[0].conditions[13] && rollover.context.year==611 &&
  XeenCharacterRules::effectivePhysical(rollover.characters[0],rollover.inputs[0],XeenCharacterRules::PhysicalAttribute::Might,{611})==0,
  "Rollover death check precedes the new age bracket");
 XeenCombatRandom nextTick(cleanTick);XeenConditionTimeCandidate aged(rollover.context,480,rollover.characters,rollover.inputs);
 finish(aged,nextTick,1);check(aged.characters[0].conditions[13]==2,"Next block sees the new year without HP refill or birthYear rewrite");
 check(aged.characters[0].birthYear==575 && aged.characters[0].currentHp==50,"Aging preserves birth year and stored HP");

 // Provisional sentinel comparisons must not change after a quiet codec round trip.
 for(unsigned weak:{0u,1u,127u,128u,254u,255u})for(unsigned drunk:{0u,3u,255u}) {
  c=characters();in=inputs();for(auto &v:c){v.conditions[13]=255;v.conditions[3]=v.conditions[4]=1;}
  c[0].conditions[2]=weak;c[0].conditions[7]=drunk;c[1].conditions[14]=254;c[2].conditions[15]=255;
  auto wire=save_test::currentWireSnapshot();wire.characters[29].conditions=c[0].conditions;
  const auto bytes=XeenSaveFormat::encode(wire);const auto decoded=XeenSaveFormat::decode(bytes);
  check(XeenSaveFormat::encode(decoded)==bytes,"Sentinel bytes round-trip exactly");
  auto loaded=c;loaded[0].conditions=decoded.characters[29].conditions;
  t=XeenGameplayContext{};t.year=610;t.day=8;t.minutes=480;
  XeenConditionTimeCandidate original(t,480,c,in),restored(t,480,loaded,in);
  finish(original,none,1);finish(restored,none,1);
  check(original.characters[0].conditions==restored.characters[0].conditions &&
   original.characters[0].conditions[2]==(weak==255?255:drunk) &&
   original.characters[0].conditions[7]==(weak==255?drunk:0),"Weak/Drunk sentinel mapping is stable across save/load");
  check(original.characters[1].conditions[14]==255 && original.characters[2].conditions[15]==255,
   "Stoned 254->255 and Eradicated 255->255 saturate");
 }
}

#include "games/xeen/XeenMerchantGeneration.h"
#include "games/xeen/XeenServiceDay.h"
#include "games/xeen/XeenArmorRepair.h"
#include "XeenM40Evidence.h"
#include <iostream>
#include <limits>
using namespace mmodern;
namespace {
using D=XeenCombatRandom::Draw;
void check(bool value,const char *message) { if(!value)throw std::runtime_error(message); }
bool same(const XeenItem &a,const XeenItem &b) { return a.material==b.material && a.id==b.id && a.state==b.state && a.frame==b.frame; }
template<class F> void rejects(F f) { bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}check(rejected,"invalid merchant candidate accepted"); }
void itemTape(unsigned level,unsigned category,XeenItem expected,std::vector<D> tape) {
	const auto attempts=tape.size();XeenCombatRandom random(std::move(tape));XeenMerchantItemCandidate candidate(level);
	XeenConsequenceDraw zero{random,0,{}};check(!candidate.service(zero) && random.position()==0,"zero budget drew an item");
	while(!candidate.complete()) {const auto before=random.position();XeenConsequenceDraw draw{random,1,{}};candidate.service(draw);check(random.position()-before<=1,"item ignored budget");}
	check(random.position()==attempts && candidate.category()==category && same(candidate.item(),expected),"literal item/request oracle mismatch");
	check(xeenPossibleMerchantItem(level,category,expected),"finite support rejected literal generated item");
	XeenConsequenceDraw repeat{random,64,{}};check(candidate.service(repeat) && random.position()==attempts,"completed item drew twice");
}
void branchTapes() {
	itemTape(1,0,{0,1,0,0},{{0,100,0},{0,100,0},{1,6,1},{1,100,100}});
	itemTape(2,1,{40,8,0,0},{{0,100,36},{0,100,71},{1,100,70},{1,100,70},{1,4,4}});
	itemTape(2,0,{34,29,6,0},{{0,100,35},{0,100,85},{18,29,29},{1,100,98},{1,100,100},{1,1,1},{0,20,10},{1,6,6}});
	itemTape(6,2,{130,10,0,0},{{0,100,100},{0,80,80},{8,10,10},{1,100,100},{1,100,100},{7,10,10}});
	itemTape(5,3,{9,60,8,0},{{0,100,100},{0,100,100},{1,9,9},{1,100,1},{51,60,60},{1,8,8}});
	// Independent interval/offset literals from the accepted generation tables.
	constexpr unsigned material[2][5][2]={{{1,4},{3,7},{4,8},{5,9},{8,9}},{{1,4},{2,6},{4,7},{6,10},{9,13}}};
	constexpr unsigned element[6][5][2]={{{1,3},{2,5},{3,6},{4,7},{5,8}},{{1,3},{2,5},{3,6},{4,7},{6,7}},
		{{1,2},{1,3},{2,4},{3,5},{4,5}},{{1,2},{1,3},{2,4},{3,4},{4,5}},{{1,3},{2,5},{3,6},{4,7},{5,8}},{{1,1},{1,1},{1,2},{2,2},{2,3}}};
	constexpr unsigned et[]={25,45,60,75,95,100},eo[]={0,8,15,20,25,33};
	constexpr unsigned attribute[10][5][2]={{{1,4},{2,5},{3,6},{4,7},{6,10}},{{1,3},{2,5},{3,6},{4,7},{5,8}},
		{{1,3},{2,5},{3,6},{4,7},{5,8}},{{1,3},{2,5},{3,6},{4,7},{5,8}},{{1,2},{1,3},{2,4},{3,5},{4,6}},
		{{1,2},{2,3},{3,4},{4,5},{5,6}},{{1,2},{1,3},{2,4},{3,4},{4,5}},{{1,2},{1,3},{2,4},{3,5},{4,6}},
		{{1,2},{1,3},{2,4},{3,4},{4,5}},{{1,2},{1,4},{3,6},{5,8},{7,10}}};
	constexpr unsigned at[]={15,25,35,50,65,80,85,90,95,100},ao[]={0,10,18,26,34,40,46,51,57,62};
	for(unsigned level=2;level<=6;++level)for(unsigned endpoint=0;endpoint<2;++endpoint) {
		for(unsigned row=0;row<2;++row) {
			const auto lo=material[row][level-2][0],hi=material[row][level-2][1],v=material[row][level-2][endpoint];
			itemTape(level,1,{std::uint8_t(v+36+9*row),8,0,0},{{0,100,36},{0,level==6?80u:100u,71},{1,100,70},{1,100,row?(endpoint?100u:71u):(endpoint?70u:1u)},{lo,hi,v}});
		}
		for(unsigned row=0;row<6;++row) {
			const auto lo=element[row][level-2][0],hi=element[row][level-2][1],v=element[row][level-2][endpoint];
			itemTape(level,1,{std::uint8_t(v+eo[row]),8,0,0},{{0,100,60},{0,level==6?80u:100u,80},{1,100,98},{1,100,endpoint?et[row]:row?et[row-1]+1:1},{lo,hi,v}});
		}
		for(unsigned row=0;row<10;++row) {
			const auto lo=attribute[row][level-2][0],hi=attribute[row][level-2][1],v=attribute[row][level-2][endpoint];
			itemTape(level,2,{std::uint8_t(v+58+ao[row]),1,0,0},{{0,100,61},{0,level==6?80u:100u,35},{1,100,endpoint?100u:61u},{1,100,endpoint?at[row]:row?at[row-1]+1:1},{lo,hi,v}});
		}
	}
	constexpr unsigned ws[]={0,30,31,60,61,85,86,100};
	constexpr unsigned wl[]={1,1,7,7,18,18,30,30},wh[]={6,6,17,17,29,29,33,33};
	for(unsigned i=0;i<8;++i)for(unsigned endpoint=0;endpoint<2;++endpoint) {
		const auto id=endpoint?wh[i]:wl[i];
		itemTape(1,0,{0,std::uint8_t(id),0,0},{{0,100,40},{0,100,ws[i]},{wl[i],wh[i],id},{1,100,1}});
	}
	for(unsigned c:{41u,85u})itemTape(1,1,{0,7,0,0},{{0,100,c},{0,100,100},{1,7,7},{1,100,100}});
	constexpr unsigned sl[][2]={{1,15},{16,30},{31,40},{41,50},{51,60}};
	for(unsigned level=1;level<=5;++level)for(unsigned endpoint=0;endpoint<2;++endpoint) {
		const auto special=sl[level-1][endpoint],charges=endpoint?8u:1u;
		itemTape(level,3,{std::uint8_t(endpoint?9:1),std::uint8_t(special),std::uint8_t(charges),0},
			{{0,100,level==1?86u:61u},{0,100,endpoint?100u:81u},{1,9,endpoint?9u:1u},{1,100,endpoint?100u:1u},{sl[level-1][0],sl[level-1][1],special},{1,8,unsigned(charges)}});
	}
	// All higher-level S branches, both sides of every boundary; constant IDs
	// deliberately omit a request and thus independently lock the request count.
	constexpr unsigned sub[]={0,10,11,20,21,35,36,45,46,55,56,65,66,75,76,80};
	constexpr unsigned category[]={1,1,1,1,2,2,1,1,1,1,2,2,2,2,2,2};
	constexpr unsigned ids[]={9,9,13,13,1,1,10,10,11,12,2,2,3,7,8,10};
	for(unsigned i=0;i<16;++i) {
		std::vector<D> tape{{0,100,100},{0,100,sub[i]}};
		if(i==8 || i==9)tape.push_back({11,12,ids[i]});
		if(i==12 || i==13)tape.push_back({3,7,ids[i]});
		if(i==14 || i==15)tape.push_back({8,10,ids[i]});
		tape.insert(tape.end(),{{1,100,20},{1,100,1},{1,4,1}});
		itemTape(2,category[i],{37,std::uint8_t(ids[i]),0,0},std::move(tape));
	}
	itemTape(2,1,{37,7,0,0},{{0,100,36},{0,100,70},{1,7,7},{1,100,1},{1,100,1},{1,4,1}});
	rejects([]{XeenMerchantItemCandidate c(0);});rejects([]{XeenMerchantItemCandidate c(7);});
}
XeenServiceEconomy seeded(std::uint32_t seed,unsigned budget,std::uint32_t state,std::uint64_t count,const char *hash) {
	XeenCombatRandom random(seed);XeenMerchantStockCandidate candidate;
	XeenConsequenceDraw zero{random,0,{}};check(!candidate.service(zero) && random.position()==0,"zero stock budget consumed RNG");
	unsigned observedShops=0;
	while(!candidate.complete()) {
		const auto before=random.position();XeenConsequenceDraw draw{random,budget,{}};candidate.service(draw);
		check(random.position()-before<=std::min(budget,64u),"stock exceeded raw servicing budget");
		if(budget==1 && candidate.generatedItems()/20>observedShops && candidate.generatedItems()%20==0) {
			constexpr unsigned freshCumulative[]={95,208,317,427,530,638,758,886};
			constexpr unsigned restockCumulative[]={97,212,323,436,542,653,779,906};
			const auto index=candidate.generatedItems()/20-1;
			if(seed==3626689381u)check(random.position()==freshCumulative[index],"fresh independent shop raw-draw count mismatch");
			if(seed==2732157854u)check(random.position()==restockCumulative[index],"restock independent shop raw-draw count mismatch");
			observedShops=index+1;
		}
	}
	check(candidate.generatedItems()==160 && random.state()==state && random.position()==count,"complete generation cursor/call oracle mismatch");
	XeenServiceEconomy economy;economy.wares=candidate.wares();xeenValidateServiceEconomy(economy);
	check(m40_test::sha256(m40_test::stockBytes(economy))==hash,"complete independent stock hash mismatch");
	XeenConsequenceDraw repeat{random,64,{}};check(candidate.service(repeat) && random.position()==count,"completed stock consumed again");return economy;
}
void seededVectors() {
	seeded(1,64,2477276124u,878,"bd3799d8b453a2877656c87d4add0d513914ca7679afac20e9b87dae6e031d84");
	seeded(7,63,1652828136u,901,"4abf1666f71af84fbdd0a8acb10749dcf78350b8b746d946a7cf3f5b88253b65");
	const auto fresh=seeded(3626689381u,1,7,886,"39cbe3234d1701fc7859afbb31a5e48f7d41407c75b4fa2364f3ee87c9143b18");
	const auto second=seeded(3626689381u,64,7,886,"39cbe3234d1701fc7859afbb31a5e48f7d41407c75b4fa2364f3ee87c9143b18");
	check(fresh==second,"yield cadence changed generated bytes");
	const auto restock=seeded(2732157854u,64,3686439625u,906,"b2d744b92079134a10dc16c73ef4ec90e738a5b7d46b8e90229c5f240b9d6cc9");
	seeded(2732157854u,1,3686439625u,906,"b2d744b92079134a10dc16c73ef4ec90e738a5b7d46b8e90229c5f240b9d6cc9");
	constexpr unsigned fc[8][4]={{8,5,1,2},{8,8,2,1},{5,8,3,1},{8,6,3,1},{7,8,1,3},{5,8,2,2},{7,8,1,2},{8,4,3,1}};
	constexpr unsigned rc[8][4]={{4,8,0,3},{8,8,1,3},{7,8,3,2},{6,8,3,2},{8,7,1,1},{8,8,1,1},{8,8,2,0},{8,6,1,1}};
	for(unsigned side=0;side<2;++side)for(unsigned shop=0;shop<4;++shop)for(unsigned category=0;category<4;++category) {
		unsigned f=0,r=0;for(unsigned slot=0;slot<9;++slot){f+=fresh.wares[side][shop][category][slot].id!=0;r+=restock.wares[side][shop][category][slot].id!=0;}
		check(f==fc[side*4+shop][category] && r==rc[side*4+shop][category],"independent per-shop stored counts mismatch");
		check(same(fresh.wares[side][shop][category][8],{}) && same(restock.wares[side][shop][category][8],{}),"ninth stock slot generated");
	}
}
void capacityDiscard() {
	constexpr unsigned calls[2][4][4]={{{15,5,5,5},{5,10,5,5},{0,5,10,5},{0,0,0,5}},{{10,5,0,5},{10,5,5,5},{0,5,5,10},{0,5,10,0}}};
	constexpr unsigned commonMin[]={0,1,3,4,5,8},commonMax[]={0,4,7,8,9,9};
	std::vector<D> tape;
	for(unsigned side=0;side<2;++side)for(unsigned shop=0;shop<4;++shop)for(unsigned band=0;band<4;++band) {
		const unsigned level=band+(side==1 && shop>=2?3:1);
		for(unsigned call=0;call<calls[side][band][shop];++call) {
			if(level==1)tape.insert(tape.end(),{{0,100,41},{0,100,100},{1,7,1},{1,100,100}});
			else tape.insert(tape.end(),{{0,100,36},{0,level==6?80u:100u,71},{1,100,70},{1,100,1},{commonMin[level-1],commonMax[level-1],commonMin[level-1]}});
		}
	}
	check(tape.size()==755,"independent capacity tape count");XeenCombatRandom random(std::move(tape));XeenMerchantStockCandidate stock;
	while(!stock.complete()){XeenConsequenceDraw draw{random,64,{}};stock.service(draw);}
	check(stock.generatedItems()==160 && stock.discardedItems()==96 && random.position()==755,"full category skipped complete generated item requests");
	xeenValidateMerchantWares(stock.wares());
	for(unsigned side=0;side<2;++side)for(unsigned shop=0;shop<4;++shop)for(unsigned category=0;category<4;++category)
	for(unsigned slot=0;slot<9;++slot)check(bool(stock.wares()[side][shop][category][slot].id)==(category==1 && slot<8),"capacity inserted ninth slot or changed category");
}
void rejectionAndExhaustion() {
	// Span 101 rejects 67 and accepts 68; >64 rejected attempts retain the
	// same pending request and never advance the accepted item state machine.
	std::vector<D> tape(130,D{0,100,67,true});
	tape.insert(tape.end(),{{0,100,68,true},{0,100,100},{1,7,7},{1,100,100}});
	XeenCombatRandom random(tape);XeenMerchantItemCandidate item(1);
	for(unsigned n=0;n<2;++n){XeenConsequenceDraw draw{random,64,{}};check(!item.service(draw) && random.position()==(n+1)*64,"raw rejection failed budget/pending request");}
	XeenConsequenceDraw draw{random,64,{}};check(item.service(draw) && random.position()==134 && item.category()==1 && same(item.item(),{0,7,0,0}),"rejected-prefix continuation wrong");
	XeenCombatRandom final(XeenJourneyRandomState{1,1,std::numeric_limits<std::uint64_t>::max()-1});XeenMerchantItemCandidate incomplete(1);
	XeenConsequenceDraw last{final,1,{}};check(!incomplete.service(last) && final.position()==std::numeric_limits<std::uint64_t>::max(),"last available raw cursor refused");
	const auto retained=final.continuation();rejects([&]{XeenConsequenceDraw beyond{final,1,{}};incomplete.service(beyond);});
	check(final.continuation()==retained && !incomplete.complete(),"cursor exhaustion wrapped or published prefix");
	unsigned checked=0;XeenCombatRandom guarded(7);XeenMerchantStockCandidate candidate;
	rejects([&]{XeenConsequenceDraw fault{guarded,64,[&]{if(++checked==3)throw std::runtime_error("injected extension fault");}};candidate.service(fault);});
	check(guarded.position()==3 && !candidate.complete(),"guard fault escaped retained preparation");
	// Retry starts from the unchanged owner preimage, not the failed detached cursor.
	seeded(7,64,1652828136u,901,"4abf1666f71af84fbdd0a8acb10749dcf78350b8b746d946a7cf3f5b88253b65");
	// Reject immediately before every reachable request whose conversion has a
	// nonzero threshold. Singleton/power-of-two spans consume without rejection.
	const std::vector<D> completeTape{{0,100,35},{0,100,85},{18,29,29},{1,100,98},{1,100,100},{1,1,1},{0,20,10},{1,6,6}};
	for(std::size_t phase=0;phase<completeTape.size();++phase) {
		auto rejected=completeTape;const auto request=completeTape[phase];const unsigned span=request.hi-request.lo+1;
		if((0u-span)%span) {rejected.insert(rejected.begin()+phase,{request.lo,request.hi,0,true});itemTape(2,0,{34,29,6,0},std::move(rejected));}
	}
	const std::vector<D> usableTape{{0,100,100},{0,100,100},{1,9,9},{1,100,1},{51,60,60},{1,8,8}};
	for(std::size_t phase=0;phase<usableTape.size();++phase) {
		auto rejected=usableTape;const auto request=usableTape[phase];const unsigned span=request.hi-request.lo+1;
		if((0u-span)%span){rejected.insert(rejected.begin()+phase,{request.lo,request.hi,0,true});itemTape(5,3,{9,60,8,0},std::move(rejected));}
	}
}
XeenMerchantWares literalValid() {
	// Twenty armor calls per shop: only first eight survive. Literal earliest
	// level schedules independently establish the material at each prefix slot.
	constexpr unsigned materials[8][8]={{0,0,0,0,0,0,0,0},{0,0,0,0,0,37,37,37},{0,0,0,0,0,37,37,37},{0,0,0,0,0,37,37,37},
		{0,0,0,0,0,0,0,0},{0,0,0,0,0,37,37,37},{40,40,40,40,40,41,41,41},{39,39,39,39,39,40,40,40}};
	XeenMerchantWares result;for(unsigned s=0;s<2;++s)for(unsigned p=0;p<4;++p)for(unsigned slot=0;slot<8;++slot)
		result[s][p][1][slot]={std::uint8_t(materials[s*4+p][slot]),1,0,0};return result;
}
void canonicalAndMutation() {
	const auto literal=literalValid();xeenValidateMerchantWares(literal);
	rejects([]{xeenValidateMerchantWares({});});
	for(unsigned side=0;side<2;++side)for(unsigned shop=0;shop<4;++shop)for(unsigned mode=0;mode<9;++mode) {
		auto broken=literal;auto &item=broken[side][shop][1][mode==8?8:0];
		if(mode==0)item={};else if(mode==1)item.frame=1;else if(mode==2)item.state=64;else if(mode==3)item.state=128;
		else if(mode==4)item.id=14;else if(mode==5)item.material=255;else if(mode==6)broken[side][shop][0][8].material=1;
		else if(mode==7)item.state=1;else item={0,1,0,0};
		rejects([&]{xeenValidateMerchantWares(broken);});
	}
	check(!xeenPossibleMerchantItem(1,2,{0,1,0,0}) && !xeenPossibleMerchantItem(1,1,{0,8,0,0}) &&
		!xeenPossibleMerchantItem(6,0,{44,30,0,0}) && !xeenPossibleMerchantItem(6,3,{1,61,1,0}) &&
		!xeenPossibleMerchantItem(2,3,{1,15,1,0}) && !xeenPossibleMerchantItem(2,3,{1,16,0,0}),"impossible finite item support admitted");
	// Broad byte bounds alone cannot satisfy the twenty-call schedule.
	auto wrongOrder=literal;wrongOrder[0][0][1][0].material=37;rejects([&]{xeenValidateMerchantWares(wrongOrder);});
	auto impossibleCalls=literal;
	for(unsigned i=0;i<6;++i)impossibleCalls[0][0][2][i]={37,1,0,0};
	rejects([&]{xeenValidateMerchantWares(impossibleCalls);}); // Only five L2 calls admit accessories.
	auto prematureDiscard=literal;prematureDiscard[0][0][1][7]={};
	rejects([&]{xeenValidateMerchantWares(prematureDiscard);}); // Twenty calls cannot discard below eight.
	XeenServiceEconomy economy;economy.wares=literal;
	for(unsigned side=0;side<2;++side)for(unsigned shop=0;shop<4;++shop)for(unsigned category=0;category<4;++category)
	for(unsigned slot=0;slot<9;++slot)for(unsigned field=0;field<4;++field) {
		XeenMutationWatch watch;watch.add(&economy,sizeof(economy));auto &item=economy.wares[side][shop][category][slot];
		auto &value=field==0?item.material:field==1?item.id:field==2?item.state:item.frame;const std::uint8_t before=value;
		value=before^1;value=before;check(!watch.current(),"merchant mutation/reversion escaped observation");
	}
	for(unsigned field=0;field<2;++field){XeenMutationWatch watch;watch.add(&economy,sizeof(economy));auto &value=field?economy.bank.gems:economy.bank.gold;value=1;value=0;check(!watch.current(),"bank mutation/reversion escaped observation");}
	XeenMutableOptional<XeenServiceEconomy> optional;
	XeenMutationWatch inserted;inserted.add(&optional,sizeof(optional));optional.emplace(economy);
	check(!inserted.current(),"economy insertion escaped owner observation");
	XeenMutationWatch reconstructed;reconstructed.add(&optional,sizeof(optional));
	optional->wares[1][3][3][8].frame=1;optional->wares[1][3][3][8].frame=0;
	check(!reconstructed.current(),"newly reconstructed stock reference escaped mutation observation");
	XeenMutationWatch replaced;replaced.add(&optional,sizeof(optional));optional.reset();optional.emplace(economy);
	check(!replaced.current(),"equal economy reset/reinsert cleared integrity violation");
}
void interestAndDates() {
	const auto maximum=std::numeric_limits<std::uint64_t>::max();
	for(unsigned remaining:{0u,1u,2u,3u,6u,9u,10u}) {
		check(xeenSmithAuthorityRoom(maximum-remaining,remaining),"reserved Smith authority rejected exact boundary");
		if(remaining)check(!xeenSmithAuthorityRoom(maximum-remaining+1,remaining),"Smith reservation admitted insufficient authority");
	}
	// Lowest admitted content-11 entry: preparation, admission and first
	// service presentation retain a warning frame plus the complete departure.
	const auto entryGeneration=maximum-10;
	check(xeenSmithAuthorityRoom(entryGeneration,10) && xeenSmithAuthorityRoom(entryGeneration+1,9) &&
		xeenSmithAuthorityRoom(entryGeneration+4,6),"Smith preparation consumed reserved departure revisions");
	check(xeenSmithAuthorityRoom(maximum-10,10) && xeenSmithAuthorityRoom(maximum-8,8) &&
		!xeenSmithAuthorityRoom(maximum-9,10),"repair reservation failed to protect result/warning/departure frames");
	check(xeenSmithAuthorityRoom(maximum-3,3) && xeenSmithAuthorityRoom(maximum-2,2),"Service/Event lease retirement reserve lost");
	check(!xeenServiceDayRegenerates(11,11,1440) && !xeenServiceDayRegenerates(11,11,2880) &&
		!xeenServiceDayRegenerates(8,9,1440) && !xeenServiceDayRegenerates(9,10,1440) &&
		xeenServiceDayRegenerates(8,10,2880) && xeenServiceDayRegenerates(10,11,1440),"reference changed-day/single-charge boundary mismatch");
	constexpr std::uint32_t input[]={0,1,99,100,199,200,4252442867u,4252442868u,4252442869u,4294967295u};
	constexpr std::uint32_t output[]={0,1,99,101,200,202,4294967295u,0,1,42949671};
	for(unsigned gold=0;gold<10;++gold)for(unsigned gems=0;gems<10;++gems) {
		XeenBankBalances before{input[gold],input[gems]};const auto after=xeenPrepareBankInterest(before);
		check(after.gold==output[gold] && after.gems==output[gems] && before.gold==input[gold] && before.gems==input[gems],"unsigned bank/independence oracle mismatch");
	}
	check(xeenBankInterest(xeenBankInterest(199))==202,"repeated bank interest wrong");
	XeenServiceEconomy economy;economy.wares=literalValid();economy.bank={199,4252442868u};
	for(unsigned day=8;day<=98;++day)for(unsigned minute:{300u,1259u})for(unsigned ctr:{0u,23u}) {
		XeenGameplayContext before;before.day=day;before.year=610;before.minutes=minute;before.ctr24=ctr;
		const XeenJourneyRandomState cursor{1,2732157854u,1203};XeenServiceDayCandidate candidate(before,economy,cursor);
		const bool trigger=(day+1)%10==1;check(candidate.triggered()==trigger,"incorrect destination regeneration trigger");
		while(!candidate.complete())candidate.service(64);
		auto expected=before;expected.day=day+1;check(candidate.context()==expected && before.day==day,"service changed ordinary time/context");
		check(candidate.beforeContext()==before && candidate.beforeEconomy()==economy && candidate.beforeRandom()==cursor,"candidate lost exact original preimages");
		if(trigger) {
			check(candidate.continuation()==XeenJourneyRandomState{1,3686439625u,2109} && candidate.economy().bank==XeenBankBalances{200,0},"service replacement/interest cursor wrong");
			check(m40_test::sha256(m40_test::stockBytes(candidate.economy()))=="b2d744b92079134a10dc16c73ef4ec90e738a5b7d46b8e90229c5f240b9d6cc9","service did not replace complete prior stock");
		}else check(candidate.continuation()==cursor && candidate.economy()==economy,"non-generating service changed economy/RNG");
		const auto final=candidate.continuation();const auto end=candidate.economy();check(candidate.service() && candidate.continuation()==final && candidate.economy()==end,"completed day repeated stock/interest/RNG");
	}
	XeenGameplayContext c;c.year=610;c.day=99;c.minutes=300;rejects([&]{XeenServiceDayCandidate candidate(c,economy,{1,7,0});});
	c.day=8;for(unsigned content:{1u,8u,9u,10u,12u})rejects([&]{XeenServiceDayCandidate candidate(c,economy,{1,7,0},content);});
	for(unsigned day:{0u,7u,100u,65535u}){c.day=day;rejects([&]{XeenServiceDayCandidate candidate(c,economy,{1,7,0});});}
	c.day=10;XeenServiceDayCandidate yielded(c,economy,{1,2732157854u,1203});check(!yielded.service(0) && yielded.continuation()==XeenJourneyRandomState{1,2732157854u,1203},"zero service budget changed detached cursor");
	while(!yielded.complete())yielded.service(1);check(yielded.continuation()==XeenJourneyRandomState{1,3686439625u,2109},"one-raw service cadence changed result");
	XeenServiceDayCandidate fault(c,economy,{1,2732157854u,1203});unsigned stockHooks=0,checks=0;
	rejects([&]{while(!fault.complete())fault.service(64,[&]{++checks;},[&]{++stockHooks;throw std::runtime_error("stock boundary failure before bank");});});
	check(stockHooks==1 && checks>0 && !fault.complete() && fault.continuation()==XeenJourneyRandomState{1,3686439625u,2109} && fault.economy()==economy,"post-stock fault prepared bank or repeated RNG");
	check(fault.service(64,[&]{++checks;},[&]{++stockHooks;}) && stockHooks==2 && fault.economy().bank==XeenBankBalances{200,0} && fault.continuation()==XeenJourneyRandomState{1,3686439625u,2109},"post-stock retry repeated generation or missed bank");
	// One 1440 charge only; this pure service candidate exposes no multi-day,
	// interactive mode, rest or actor catch-up admission API.
}
void legacyDropInterleaving() {
	XeenCombatRandom random(XeenJourneyRandomState{1,3686439625u,2109});XeenConsequenceDraw draw{random,64,{}};
	XeenMagicArrowCandidate arrow(6,0,0,6);check(arrow.service(draw) && arrow.damage==8 && random.continuation()==XeenJourneyRandomState{1,2493262264u,2110},"synthetic stock->Arrow continuation mismatch");
	XeenMonsterDropCandidate drop({},9,11);check(drop.service(draw) && drop.outcome==XeenMonsterDropOutcome::Item && drop.armor &&
		same(drop.generated.item,{0,3,0,0}) && drop.treasure.pendingGold==10 && drop.treasure.pendingMask==(1u<<9) &&
		random.continuation()==XeenJourneyRandomState{1,1051044860u,2115},"synthetic stock->Arrow->legacy Orc drop semantics changed");
	XeenCombatRandom raw(std::vector<D>{{1,56,2493262264u,true},{1,100,617549005u,true},
		{0,100,1871643302u,true},{0,100,1957375019u,true},{1,7,887542588u,true},{1,100,1051044860u,true}});
	XeenConsequenceDraw trace{raw,64,{}};XeenMagicArrowCandidate tracedArrow(6,0,0,6);XeenMonsterDropCandidate tracedDrop({},9,11);
	check(tracedArrow.service(trace) && tracedDrop.service(trace) && raw.position()==6 && tracedDrop.armor &&
		same(tracedDrop.generated.item,{0,3,0,0}),"independent interleaved exact request/raw tape mismatch");
}
}
int main() {
	try {branchTapes();seededVectors();capacityDiscard();rejectionAndExhaustion();canonicalAndMutation();interestAndDates();legacyDropInterleaving();
		std::cout<<"M40 exact merchant generation, finite canonical validator, bank and service-day candidates passed\n";return 0;
	}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

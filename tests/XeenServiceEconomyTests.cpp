#include "games/xeen/XeenMerchantGeneration.h"
#include "games/xeen/XeenServiceDay.h"
#include "games/xeen/XeenServiceAnimation.h"
#include "games/xeen/XeenMerchantTables.h"
#include "games/xeen/XeenArmorRepair.h"
#include "XeenM40Evidence.h"
#include "XeenM42Evidence.h"
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
void sharedGeneration() {
    for(const auto &tape:std::vector<std::vector<D>>{
      {{1,100,35},{1,80,1},{1,6,1},{1,100,1},{1,100,1},{13,13,13},{1,20,1}},
      {{1,100,35},{1,80,1},{1,6,1},{1,100,71},{1,100,25},{8,8,8},{1,20,1}}}) {
      XeenCombatRandom random(tape);XeenItemCandidate item(7);
      while(!item.complete()){XeenConsequenceDraw draw{random,1,{}};item.service(draw);}
      check(item.item().material==(tape[3].value==1?58:8) && item.item().id==1 && random.position()==tape.size(),"Shared level-7 conditional draw tape");
    }
}
// Mutations: shift category/subcategory boundaries, skip constant-ID draws,
// use a wrong level column, omit level-7 selector, or skip ItemType's first draws.
void exhaustiveMapping() {
    const auto compare=[](unsigned level,unsigned c,unsigned sub,std::optional<int> type) {
        m42_test::StockOracle oracle{1,0};unsigned at=0;
        oracle.scripted=[&](unsigned lo,unsigned hi) {++at;return at==1?c:at==2?sub:hi;};
        const auto expected=oracle.item(level,type);
        XeenCombatRandom random(oracle.requests);XeenItemCandidate actual(level,type);
        while(!actual.complete()) {XeenConsequenceDraw draw{random,1,{}};actual.service(draw);}
        check(actual.category()==expected.first && same(actual.item(),expected.second) && random.position()==oracle.requests.size(),
            "M71 exhaustive independent category/item/request mapping differs");
    };
    for(unsigned level=1;level<=7;++level)
        for(unsigned c=1;c<=100;++c)for(unsigned sub=1;sub<=(level>=6?80u:100u);++sub)compare(level,c,sub,std::nullopt);
    for(unsigned level=1;level<=7;++level)for(int type:{1,34,35,48,49,59,60,69})
        compare(level,100,level>=6?80:100,type);
    for(unsigned level=1;level<=7;++level)for(unsigned invalid:{0u,101u}) {
        rejects([&]{XeenCombatRandom r(std::vector<D>{{1,100,invalid}});XeenItemCandidate i(level);XeenConsequenceDraw d{r};i.service(d);});
        rejects([&]{XeenCombatRandom r(std::vector<D>{{1,100,1},{1,level>=6?80u:100u,invalid}});XeenItemCandidate i(level);XeenConsequenceDraw d{r};i.service(d);});
    }
}
void branchTapes() {
	// DOS 0x25cbc..0x25cdb: zero is unavailable in either initial draw;
	// level 6 still admits category 100 but cannot draw subcategory 100.
	rejects([]{XeenCombatRandom random(std::vector<D>{{1,100,0}});XeenItemCandidate item(1);XeenConsequenceDraw draw{random};item.service(draw);});
	rejects([]{XeenCombatRandom random(std::vector<D>{{1,100,100},{1,80,100}});XeenItemCandidate item(6);XeenConsequenceDraw draw{random};item.service(draw);});
	itemTape(1,0,{0,1,0,0},{{1,100,1},{1,100,1},{1,6,1},{1,100,100}});
	itemTape(2,1,{40,8,0,0},{{1,100,36},{1,100,71},{1,100,70},{1,100,70},{1,4,4}});
	itemTape(2,0,{34,29,6,0},{{1,100,35},{1,100,85},{18,29,29},{1,100,98},{1,100,100},{1,1,1},{1,20,10},{1,6,6}});
	itemTape(6,2,{130,10,0,0},{{1,100,100},{1,80,80},{8,10,10},{1,100,100},{1,100,100},{7,10,10}});
	itemTape(5,3,{9,60,10,0},{{1,100,100},{1,100,100},{1,9,9},{1,100,1},{51,60,60},{1,8,8}});
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
			itemTape(level,1,{std::uint8_t(v+36+9*row),8,0,0},{{1,100,36},{1,level==6?80u:100u,71},{1,100,70},{1,100,row?(endpoint?100u:71u):(endpoint?70u:1u)},{lo,hi,v}});
		}
		for(unsigned row=0;row<6;++row) {
			const auto lo=element[row][level-2][0],hi=element[row][level-2][1],v=element[row][level-2][endpoint];
			itemTape(level,1,{std::uint8_t(v+eo[row]),8,0,0},{{1,100,60},{1,level==6?80u:100u,80},{1,100,98},{1,100,endpoint?et[row]:row?et[row-1]+1:1},{lo,hi,v}});
		}
		for(unsigned row=0;row<10;++row) {
			const auto lo=attribute[row][level-2][0],hi=attribute[row][level-2][1],v=attribute[row][level-2][endpoint];
			itemTape(level,2,{std::uint8_t(v+58+ao[row]),1,0,0},{{1,100,61},{1,level==6?80u:100u,35},{1,100,endpoint?100u:61u},{1,100,endpoint?at[row]:row?at[row-1]+1:1},{lo,hi,v}});
		}
	}
	constexpr unsigned ws[]={1,30,31,60,61,85,86,100};
	constexpr unsigned wl[]={1,1,7,7,18,18,30,30},wh[]={6,6,17,17,29,29,33,33};
	for(unsigned i=0;i<8;++i)for(unsigned endpoint=0;endpoint<2;++endpoint) {
		const auto id=endpoint?wh[i]:wl[i];
		itemTape(1,0,{0,std::uint8_t(id),0,0},{{1,100,40},{1,100,ws[i]},{wl[i],wh[i],id},{1,100,1}});
	}
	for(unsigned c:{41u,85u})itemTape(1,1,{0,7,0,0},{{1,100,c},{1,100,100},{1,7,7},{1,100,100}});
	constexpr unsigned sl[][2]={{1,15},{16,30},{31,40},{41,50},{51,60}};
	for(unsigned level=1;level<=5;++level)for(unsigned endpoint=0;endpoint<2;++endpoint) {
		const auto special=sl[level-1][endpoint],charges=endpoint?8u:1u;
		itemTape(level,3,{std::uint8_t(endpoint?9:1),std::uint8_t(special),std::uint8_t(charges+2),0},
			{{1,100,level==1?86u:61u},{1,100,endpoint?100u:81u},{1,9,endpoint?9u:1u},{1,100,endpoint?100u:1u},{sl[level-1][0],sl[level-1][1],special},{1,8,unsigned(charges)}});
	}
	// All higher-level S branches, both sides of every boundary; constant IDs
	// deliberately omit a request and thus independently lock the request count.
	constexpr unsigned sub[]={1,10,11,20,21,35,36,45,46,55,56,65,66,75,76,80};
	constexpr unsigned category[]={1,1,1,1,2,2,1,1,1,1,2,2,2,2,2,2};
	constexpr unsigned ids[]={9,9,13,13,1,1,10,10,11,12,2,2,3,7,8,10};
	for(unsigned i=0;i<16;++i) {
		std::vector<D> tape{{1,100,100},{1,100,sub[i]}};
		if(i==8 || i==9)tape.push_back({11,12,ids[i]});
		if(i==12 || i==13)tape.push_back({3,7,ids[i]});
		if(i==14 || i==15)tape.push_back({8,10,ids[i]});
		tape.insert(tape.end(),{{1,100,20},{1,100,1},{1,4,1}});
		itemTape(2,category[i],{37,std::uint8_t(ids[i]),0,0},std::move(tape));
	}
	itemTape(2,1,{37,7,0,0},{{1,100,36},{1,100,70},{1,7,7},{1,100,1},{1,100,1},{1,4,1}});
	rejects([]{XeenMerchantItemCandidate c(0);});rejects([]{XeenMerchantItemCandidate c(8);});
}
XeenServiceEconomy seeded(std::uint32_t seed,unsigned budget,std::uint32_t state,std::uint64_t count,const char *hash) {
	XeenCombatRandom random(seed);XeenMerchantStockCandidate candidate;
	XeenConsequenceDraw zero{random,0,{}};check(!candidate.service(zero) && random.position()==0,"zero stock budget consumed RNG");
	unsigned observedShops=0;
	while(!candidate.complete()) {
		const auto before=random.position();XeenConsequenceDraw draw{random,budget,{}};candidate.service(draw);
		check(random.position()-before<=std::min(budget,64u),"stock exceeded raw servicing budget");
		if(budget==1 && candidate.generatedItems()/20>observedShops && candidate.generatedItems()%20==0) {
			constexpr unsigned freshCumulative[]={94,201,309,421,521,639,762,885};
			constexpr unsigned restockCumulative[]={96,213,323,437,539,657,773,895};
			const auto index=candidate.generatedItems()/20-1;
			if(seed==3626689381u)check(random.position()==freshCumulative[index],"fresh independent shop raw-draw count mismatch");
			if(seed==2732157854u)check(random.position()==restockCumulative[index],"restock independent shop raw-draw count mismatch");
			observedShops=index+1;
		}
	}
	check(candidate.generatedItems()==160 && random.state()==state && random.position()==count,"complete generation cursor/call oracle mismatch");
	XeenServiceEconomy economy;economy.wares=candidate.wares();xeenValidateServiceEconomy(economy);
	m42_test::StockOracle oracle{seed,0};const auto expected=oracle.generate();
	check(economy==expected && random.continuation()==XeenJourneyRandomState{1,oracle.state,oracle.count},
		"independent complete stock/draw continuation differs");
	check(m40_test::sha256(m40_test::stockBytes(economy))==hash,"complete independent stock hash mismatch");
	XeenConsequenceDraw repeat{random,64,{}};check(candidate.service(repeat) && random.position()==count,"completed stock consumed again");return economy;
}
void seededVectors() {
	seeded(1,64,795614091u,893,"7983393dfd830dbe9eae696e6798025c477af13d4d9e64de085fbf0f94b40da2");
	seeded(7,63,2000247898u,875,"57da748654a13215fe4e4f4cbb8c4719567b43a137030ff5c8f8a3e4539668c2");
	const auto fresh=seeded(3626689381u,1,3372760030u,885,"c5dd4613660e1bb560d8d592974f96853d7368f0c52af6b65318e66747b4ad22");
	const auto second=seeded(3626689381u,64,3372760030u,885,"c5dd4613660e1bb560d8d592974f96853d7368f0c52af6b65318e66747b4ad22");
	check(fresh==second,"yield cadence changed generated bytes");
	const auto restock=seeded(2732157854u,64,3856430767u,895,"bc3674445219431acd366b7b6eaedef64ba1af05aecb1ce96b80ac0e96432646");
	seeded(2732157854u,1,3856430767u,895,"bc3674445219431acd366b7b6eaedef64ba1af05aecb1ce96b80ac0e96432646");
	constexpr unsigned fc[8][4]={{5,8,2,3},{4,8,3,2},{4,8,3,4},{6,8,1,3},{4,8,0,2},{8,8,0,2},{8,8,2,2},{8,6,3,2}};
	constexpr unsigned rc[8][4]={{8,7,1,3},{8,5,2,4},{6,8,5,1},{6,8,3,3},{5,8,3,1},{8,7,1,0},{4,8,2,0},{7,8,3,2}};
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
			if(level==1)tape.insert(tape.end(),{{1,100,41},{1,100,100},{1,7,1},{1,100,100}});
			else tape.insert(tape.end(),{{1,100,36},{1,level==6?80u:100u,71},{1,100,70},{1,100,1},{commonMin[level-1],commonMax[level-1],commonMin[level-1]}});
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
	// Span 100 rejects raw 95 and accepts raw 140 (category 41); >64 attempts retain the
	// same pending request and never advance the accepted item state machine.
	std::vector<D> tape(130,D{1,100,95,true});
	tape.insert(tape.end(),{{1,100,140,true},{1,100,50},{1,7,7},{1,100,100}});
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
	seeded(7,64,2000247898u,875,"57da748654a13215fe4e4f4cbb8c4719567b43a137030ff5c8f8a3e4539668c2");
	// Reject immediately before every reachable request whose conversion has a
	// nonzero threshold. Singleton/power-of-two spans consume without rejection.
	const std::vector<D> completeTape{{1,100,35},{1,100,85},{18,29,29},{1,100,98},{1,100,100},{1,1,1},{1,20,10},{1,6,6}};
	for(std::size_t phase=0;phase<completeTape.size();++phase) {
		auto rejected=completeTape;const auto request=completeTape[phase];const unsigned span=request.hi-request.lo+1;
		if((0u-span)%span) {rejected.insert(rejected.begin()+phase,{request.lo,request.hi,0,true});itemTape(2,0,{34,29,6,0},std::move(rejected));}
	}
	const std::vector<D> usableTape{{1,100,100},{1,100,100},{1,9,9},{1,100,1},{51,60,60},{1,8,8}};
	for(std::size_t phase=0;phase<usableTape.size();++phase) {
		auto rejected=usableTape;const auto request=usableTape[phase];const unsigned span=request.hi-request.lo+1;
		if((0u-span)%span){rejected.insert(rejected.begin()+phase,{request.lo,request.hi,0,true});itemTape(5,3,{9,60,10,0},std::move(rejected));}
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
// Independent aggregate-band oracle for a literal palette with disjoint
// support: plain W/A and Misc ID1 are L1; material37 W/A/X and Misc ID16
// are L2. All categories can now be purchased, so missing band draws can be
// deleted. The literal capacities leave room to fill both original bands.
bool literalBandOracle(const std::array<unsigned,4> &plain,const std::array<unsigned,4> &modified) {
    return plain[0]+plain[1]+plain[3]<=15 &&
        modified[0]+modified[1]+modified[2]+modified[3]<=5;
}
void independentDepletionOracle(const XeenServiceEconomy &baseline) {
	std::uint32_t cursor=0x42f13a7u;unsigned accepted=0,rejected=0;
	const auto bounded=[&](unsigned span){cursor=cursor*1664525u+1013904223u;return (cursor>>8)%span;};
	for(unsigned test=0;test<128;++test) {
		std::array<unsigned,4> plain{},modified{};
		plain[0]=bounded(9);plain[1]=bounded(9);plain[3]=bounded(9);
		modified[0]=bounded(9-plain[0]);modified[1]=bounded(9-plain[1]);
		modified[2]=bounded(7);modified[3]=bounded(9-plain[3]);
		auto current=baseline;auto &shop=current.wares[0][0];shop={};
		for(unsigned category=0;category<4;++category) {
			unsigned slot=0;
			for(unsigned n=0;n<plain[category];++n)shop[category][slot++]=category==3?XeenItem{1,1,3,0}:XeenItem{0,1,0,0};
			for(unsigned n=0;n<modified[category];++n)shop[category][slot++]=category==0?XeenItem{37,1,0,0}:
				category==1?XeenItem{37,8,0,0}:category==2?XeenItem{37,1,0,0}:XeenItem{1,16,3,0};
		}
		const bool expected=literalBandOracle(plain,modified);bool actual=true;
		try{xeenValidateCurrentServiceEconomy(current);}catch(const std::invalid_argument &){actual=false;}
		check(actual==expected,"depletion per-call proof differs from independent literal band/capacity oracle");
		accepted+=expected;rejected+=!expected;
	}
	check(accepted && rejected,"independent depletion oracle did not cover both verdicts");
}
void currentDepletion() {
	XeenServiceEconomy literal;literal.wares=literalValid();
	independentDepletionOracle(literal);
	xeenValidateCurrentServiceEconomy(literal);
	// Independent twenty-Armor-call schedule: eight L1 entries inserted,
	// seven remaining L1 and five L2 entries discarded at original capacity.
	// Every deletion subset, including all eight, has a literal complete source.
	for(unsigned mask=0;mask<256;++mask) {
		auto current=literal;auto &armor=current.wares[0][0][1];armor={};unsigned retained=0;
		for(unsigned slot=0;slot<8;++slot)if(!(mask&(1u<<slot)))armor[retained++]={0,1,0,0};
		xeenValidateCurrentServiceEconomy(current);
	}
	// Independent source: eight Weapons followed by seven Armor at L1,
	// then five Weapons discarded at original full capacity at L2. Purchasing
	// all fifteen inserted plain records gives one completely empty shop.
	auto emptyShop=literal;emptyShop.wares[0][0]={};xeenValidateCurrentServiceEconomy(emptyShop);
	rejects([&]{xeenValidateServiceEconomy(emptyShop);});
	XeenServiceEconomy allEmpty;rejects([&]{xeenValidateCurrentServiceEconomy(allEmpty);});
	// A full modified Weapon category requires eight L2 insertions, but shop
	// 0/0 has only five L2 calls. Deleted L1 capacity cannot be reused to refill.
	auto refill=emptyShop;for(unsigned i=0;i<8;++i)refill.wares[0][0][0][i]={37,1,0,0};
	rejects([&]{xeenValidateCurrentServiceEconomy(refill);});
	for(unsigned side=0;side<2;++side)for(unsigned shop=0;shop<4;++shop) {
		auto depleted=literal;depleted.wares[side][shop][1][7]={};
		if(!side)xeenValidateCurrentServiceEconomy(depleted);
		else rejects([&]{xeenValidateCurrentServiceEconomy(depleted);});
		rejects([&]{xeenValidateServiceEconomy(depleted);});
	}
	for(unsigned category:{2u,3u}) {
		// Fifteen L1 Armor calls fill A to eight. Five L2 calls each insert
		// the chosen X/M category. Every purchase subset retains a valid
		// predecessor, including a completely depleted modified category.
		auto stock=literal;for(unsigned i=0;i<5;++i)stock.wares[0][0][category][i]=
			category==2?XeenItem{37,1,0,0}:XeenItem{1,16,3,0};
		xeenValidateServiceEconomy(stock);xeenValidateCurrentServiceEconomy(stock);
        for(unsigned mask=0;mask<32;++mask) {
            auto depleted=stock;auto &items=depleted.wares[0][0][category];items={};unsigned kept=0;
            for(unsigned slot=0;slot<5;++slot)if(!(mask&(1u<<slot)))items[kept++]=stock.wares[0][0][category][slot];
            xeenValidateCurrentServiceEconomy(depleted);
        }
		// Make four L2 entries plus a retained L2 Weapon. This different
		// complete schedule is valid; exact runtime deletion is tested by Buy.
		stock.wares[0][0][category][4]={};stock.wares[0][0][0][0]={37,1,0,0};
		xeenValidateCurrentServiceEconomy(stock);
		for(unsigned i=0;i<6;++i)stock.wares[0][0][category][i]=
			category==2?XeenItem{37,1,0,0}:XeenItem{1,16,3,0};
		rejects([&]{xeenValidateCurrentServiceEconomy(stock);});
	}
    auto reversed=emptyShop;reversed.wares[0][0][0][0]={37,1,0,0};reversed.wares[0][0][0][1]={0,1,0,0};
    rejects([&]{xeenValidateCurrentServiceEconomy(reversed);}); // L1 cannot follow retained L2.
    auto excessL1=emptyShop;
    for(unsigned slot=0;slot<6;++slot)for(unsigned category:{0u,1u})excessL1.wares[0][0][category][slot]={0,1,0,0};
    for(unsigned slot=0;slot<4;++slot)excessL1.wares[0][0][3][slot]={1,1,3,0};
    rejects([&]{xeenValidateCurrentServiceEconomy(excessL1);}); // Sixteen retained L1 records.
    auto excessL2=emptyShop;
    for(unsigned slot=0;slot<3;++slot)for(unsigned category:{1u,2u})excessL2.wares[0][0][category][slot]={37,1,0,0};
    rejects([&]{xeenValidateCurrentServiceEconomy(excessL2);}); // Six aggregate L2 records.
	for(unsigned field=0;field<4;++field) {
		auto broken=emptyShop;auto &tail=broken.wares[0][0][0][8];
		if(field==0)tail.material=1;else if(field==1)tail.id=1;else if(field==2)tail.state=1;else tail.frame=1;
		rejects([&]{xeenValidateCurrentServiceEconomy(broken);});
	}
	for(unsigned mutation=0;mutation<7;++mutation) {
		auto broken=literal;auto &item=broken.wares[0][0][1][0];
		if(mutation==0)item={};else if(mutation==1)item.frame=1;else if(mutation==2)item.state=64;
		else if(mutation==3)item.state=128;else if(mutation==4)item.material=255;
		else if(mutation==5)item.id=14;else {item.material=37;broken.wares[0][0][1][1].material=0;}
		rejects([&]{xeenValidateCurrentServiceEconomy(broken);});
	}
	// Production seed-7 stock has independently recorded literal identity/order.
	const auto original=seeded(7,64,2000247898u,875,"57da748654a13215fe4e4f4cbb8c4719567b43a137030ff5c8f8a3e4539668c2");
	const XeenItem armorBefore[8]={{0,6,0,0},{0,3,0,0},{0,5,0,0},{0,7,0,0},{0,1,0,0},{0,1,0,0},{0,5,0,0},{9,5,0,0}};
	for(unsigned slot=0;slot<8;++slot)check(same(original.wares[0][0][1][slot],armorBefore[slot]),"seed-7 literal Armor source mismatch");
	for(unsigned category=0;category<2;++category)for(unsigned slot=0;slot<9;++slot) {
		const auto &offer=original.wares[0][0][category][slot];
		if(!offer.id || offer.material || offer.state || offer.frame)continue;
		auto depleted=original;auto &records=depleted.wares[0][0][category];
		for(unsigned i=slot;i<8;++i)records[i]=records[i+1];records[8]={};
		xeenValidateCurrentServiceEconomy(depleted);
	}
	auto depleted=original;
	for(unsigned category=0;category<2;++category) {
		auto &records=depleted.wares[0][0][category];XeenItemCategory retained{};unsigned n=0;
		for(const auto &item:records)if(item.id && (item.material || item.state || item.frame))retained[n++]=item;
		records=retained;
	}
	xeenValidateCurrentServiceEconomy(depleted);
	// Snapshots prove possible current values, never actual historical deletion.
	// These unsupported bytes cannot even participate in any permitted source.
	depleted.wares[0][0][0][0].state=7;rejects([&]{xeenValidateCurrentServiceEconomy(depleted);});
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
			check(candidate.continuation()==XeenJourneyRandomState{1,3856430767u,2098} && candidate.economy().bank==XeenBankBalances{200,0},"service replacement/interest cursor wrong");
			check(m40_test::sha256(m40_test::stockBytes(candidate.economy()))=="bc3674445219431acd366b7b6eaedef64ba1af05aecb1ce96b80ac0e96432646","service did not replace complete prior stock");
		}else check(candidate.continuation()==cursor && candidate.economy()==economy,"non-generating service changed economy/RNG");
		const auto final=candidate.continuation();const auto end=candidate.economy();check(candidate.service() && candidate.continuation()==final && candidate.economy()==end,"completed day repeated stock/interest/RNG");
	}
	XeenGameplayContext c;c.year=610;c.day=99;c.minutes=300;XeenServiceDayCandidate rollover(c,economy,{1,7,0});check(rollover.context().day==0 && rollover.context().year==611,"Script addTime year rollover");
	for(unsigned day:{100u,65535u}){c.day=day;rejects([&]{XeenServiceDayCandidate candidate(c,economy,{1,7,0});});}
	c.day=10;XeenServiceDayCandidate yielded(c,economy,{1,2732157854u,1203});check(!yielded.service(0) && yielded.continuation()==XeenJourneyRandomState{1,2732157854u,1203},"zero service budget changed detached cursor");
	while(!yielded.complete())yielded.service(1);check(yielded.continuation()==XeenJourneyRandomState{1,3856430767u,2098},"one-raw service cadence changed result");
	XeenServiceDayCandidate fault(c,economy,{1,2732157854u,1203});unsigned stockHooks=0,checks=0;
	rejects([&]{while(!fault.complete())fault.service(64,[&]{++checks;},[&]{++stockHooks;throw std::runtime_error("stock boundary failure before bank");});});
	check(stockHooks==1 && checks>0 && !fault.complete() && fault.continuation()==XeenJourneyRandomState{1,3856430767u,2098} && fault.economy()==economy,"post-stock fault prepared bank or repeated RNG");
	check(fault.service(64,[&]{++checks;},[&]{++stockHooks;}) && stockHooks==2 && fault.economy().bank==XeenBankBalances{200,0} && fault.continuation()==XeenJourneyRandomState{1,3856430767u,2098},"post-stock retry repeated generation or missed bank");
	// One 1440 charge only; this pure service candidate exposes no multi-day,
	// interactive mode, rest or actor catch-up admission API.
}
void deferredServiceRandom() {
    XeenServiceEconomy economy;economy.wares=literalValid();economy.bank={199,99};
    XeenGameplayContext context;context.year=610;context.day=10;context.minutes=300;
    const XeenJourneyRandomState before{1,2732157854u,1203};
    XeenServiceDayCandidate reserved(context,economy,before,XeenScriptServiceCharge::OneDay,false,true);
    check(reserved.reservationReady() && !reserved.complete() && reserved.pendingGeneration() && reserved.economy()==economy && reserved.continuation()==before,
        "A reserved future service day consumed stock/RNG before departure");
    XeenCombatRandom speech(before);while(!speech.draw(7,11)){}
    auto rebound=reserved.rebindRandom(speech.continuation());
    XeenServiceDayCandidate expected(context,economy,speech.continuation());
    while(!expected.complete())expected.service(64);
    check(!rebound.service(1),"One raw draw unexpectedly completed merchant generation");
    rejects([&]{rebound.rebindRandom(before);});
    while(!rebound.service(64)){}
    check(rebound.economy()==expected.economy() && rebound.continuation()==expected.continuation(),
        "Departure generation did not follow speaking RNG in causal order");
    unsigned total=0;for(const auto &side:merchant::counts)for(const auto &band:side)for(auto count:band)total+=count;
    check(total==160,"Deferred RNG reserve must follow the merchant item count");
    for(std::uint32_t raw=1;raw<=100;++raw) {
        auto next=raw;next^=next<<13;next^=next>>17;next^=next<<5;
        check(next>100,"Merchant rejection reserve bound changed with RNG algorithm");
    }
    check(!XeenServiceDayCandidate::randomRoom({1,7,UINT64_MAX-2559}) &&
        XeenServiceDayCandidate::randomRoom({1,7,UINT64_MAX-2560}),"Deferred merchant RNG suffix bound differs");
    rejects([&]{XeenServiceDayCandidate noDebt(context,economy,{1,7,UINT64_MAX-1},XeenScriptServiceCharge::OneDay,false,true);});
    XeenSilentSampleInterval sample;
    sample.start(100,300);check(sample.playing(399) && !sample.playing(400),"Silent sample completion boundary differs");
    sample.start(200,50);check(sample.playing(249) && !sample.playing(250),"Sample replacement retained the older interval");
    sample.stop();check(!sample.playing(0),"Sample stop retained busy status");
}

void sharedDropInterleaving() {
	// Mutation: using the old duplicate generator changes this interleaved item and request tape.
	XeenCombatRandom random(XeenJourneyRandomState{1,3686439625u,2109});XeenConsequenceDraw draw{random,64,{}};
	XeenMagicArrowCandidate arrow(6,0,0,6);check(arrow.service(draw) && arrow.damage==8 && random.continuation()==XeenJourneyRandomState{1,2493262264u,2110},"synthetic Arrow continuation mismatch");
	XeenMonsterDropCandidate drop(XeenTreasureStaging{});check(drop.service(draw) && drop.outcome==XeenMonsterDropOutcome::Item && drop.category==XeenRewardCategory::Weapons &&
		same(drop.generated,{0,5,0,0}) && drop.treasure.gold==10 && drop.treasure.ready && same(drop.treasure.weapons[0],{0,5,0,0}) &&
		random.continuation()==XeenJourneyRandomState{1,1051044860u,2115},"synthetic Arrow->shared Orc drop semantics changed");
	XeenCombatRandom raw(std::vector<D>{{1,56,2493262264u,true},{1,100,617549005u,true},
		{1,100,1871643302u,true},{1,100,1957375019u,true},{1,6,887542588u,true},{1,100,1051044860u,true}});
	XeenConsequenceDraw trace{raw,64,{}};XeenMagicArrowCandidate tracedArrow(6,0,0,6);XeenMonsterDropCandidate tracedDrop(XeenTreasureStaging{});
	check(tracedArrow.service(trace) && tracedDrop.service(trace) && raw.position()==6 && tracedDrop.category==XeenRewardCategory::Weapons &&
		same(tracedDrop.generated,{0,5,0,0}),"independent interleaved exact request/raw tape mismatch");
}
}
int main() {
	try {branchTapes();sharedGeneration();exhaustiveMapping();seededVectors();capacityDiscard();rejectionAndExhaustion();canonicalAndMutation();currentDepletion();interestAndDates();deferredServiceRandom();sharedDropInterleaving();
		std::cout<<"M40 exact merchant generation, finite canonical validator, bank and service-day candidates passed\n";return 0;
	}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}

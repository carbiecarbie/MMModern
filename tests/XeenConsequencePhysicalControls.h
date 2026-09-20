// Additional original-resource production presentation controls. The sign
// positioning fixture is labeled; fresh empty Shoot uses unchanged entry state.
void physicalPresentationControls(Source &source,const std::filesystem::path &game) {
 Domain original(source);const auto fresh=original.save();
 const auto &sign=source.evt.records.at(56);
 check(sign.fileOffset==495 && sign.lengthField==6 && sign.x==5 && sign.y==9 && sign.direction==0 && sign.line==0 && sign.opcode==4 && sign.parameters==std::vector<std::uint8_t>{16},"Original map23 SignLabel record");
 const auto path=std::filesystem::temp_directory_path()/"mmodern-m33-physical-controls.mms";
 combat_gameplay_test::Harness h(game);
 for(unsigned mode=0;mode<5;++mode){
  auto saved=fresh;if(mode){saved.camera={23,5,9,XeenDirection::North};for(auto &a:saved.journey->actors)a.activated=true;}
  XeenSaveFile::write(path,saved);auto services=h.services();services.resources.regionalManifest=source.manifest();
  XeenEventTextLoader texts([&](const std::string &name)->std::optional<std::vector<std::uint8_t>>{if(!h.assets->hasArchiveResource(name))return {};return h.assets->readArchiveResource(name);});
  services.texts=[&](auto id){return texts.load(id);};
  const auto compose=services.composeEncounter;std::vector<std::pair<unsigned,unsigned>> visuals;
  services.composeEncounter=[&](auto &w,const auto &p,const auto &c,auto phase,auto appearance){
   if(const auto shot=appearance.projectile;shot&&!shot->enemy){check(!shot->source,"Empty volley has no invented monster identity");const auto value=std::make_pair(shot->lane,shot->row);if(visuals.empty()||visuals.back()!=value)visuals.push_back(value);}
   return compose(w,p,c,phase,appearance); // Real original POW/font/terrain renderer.
  };
  services.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
   handler.framePresented(h.flow->frame().presentation());const auto press=[&](const PlayerAction &a){handler.beginCycle(++h.cycle);handler.withDisplayedInput(a,*handler.displayedInput());handler.framePresented(h.flow->frame().presentation());};
   if(mode){
    press(InteractionAction{});
    check(!h.flow->presentationGeneration()&&!h.flow->presenter().blocksGameplay()&&h.flow->canSave(),"Original SignLabel completes without acknowledgment and remains nonmodal");
    bool ink=false;for(unsigned y=80;y<110;++y)for(unsigned x=8;x<222;++x)ink|=h.flow->frame().pixels[y*320+x]!=h.base.pixels[y*320+x];check(ink,"Original sign label rendered");
    const auto bytes=XeenSaveFormat::encode(XeenSaveState::capture(h.signature,*h.party,*h.camera,*h.flags,*h.world));
    if(mode==1){press(SaveGameAction{});check(XeenSaveFormat::encode(XeenSaveFile::read(path))==bytes,"F9 with passive sign preserves exact state");}
    if(mode==2){press(InspectInventoryAction{});check(h.flow->inventoryOpen(),"Passive sign permits inventory");press(InspectInventoryAction{});check(h.flow->canSave(),"Inventory returns to Quiet");}
    if(mode==3){press(NavigationAction::TurnRight);check(h.camera->direction==XeenDirection::East,"Passive sign permits fresh movement");}
    if(mode<4)return true;
   }
   press(ShootAction{});
   for(unsigned n=0;n<80 && h.party->encounterContext->minutes==480;++n){h.now+=100;handler.beginCycle(++h.cycle);idle();handler.framePresented(h.flow->frame().presentation());}
   check(!visuals.empty()&&h.party->encounterContext->minutes==490,"Typed eligible Shoot fires and charges after passive sign/empty ray");
   if(!mode){check(visuals==std::vector<std::pair<unsigned,unsigned>>{{2,0},{2,1},{2,2},{2,3}},"Fresh empty Shoot renders all four outward rows through production composition");check(h.world->sessionState().journeyRandom()->count==0,"Empty Shoot visual introduces no random work");}
   for(unsigned n=0;n<80&&!h.flow->canSave();++n){h.now+=100;handler.beginCycle(++h.cycle);idle();handler.framePresented(h.flow->frame().presentation());}
   check(h.flow->canSave(),"Pending movement/ranged/projectile work returns Quiet");const auto previous=h.camera->direction;press(NavigationAction::TurnRight);check(h.camera->direction!=previous,"Fresh command after projectile/pending work accepted");
   return true;
  };
  check(Application().playGameplay(services,{},path,true)==0,"Original production presentation control");
 }
 std::filesystem::remove(path);
 std::cout<<"PHYSICAL controls: original SignLabel nonmodal (ARTIFICIAL positioning); fresh empty Shoot POW rows0..3 and next Quiet input PASS\n";
}

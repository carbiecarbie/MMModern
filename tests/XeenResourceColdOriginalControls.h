// Original archives and production composition; only the starting checkpoint is artificial.
void coldVertigo(Source &source) {
 auto saved=source.base();saved.camera={23,10,13,XeenDirection::North};
 for(auto &actor:saved.journey->actors)actor.activated=true;
 const auto path=std::filesystem::temp_directory_path()/"mmodern-m45-cold-vertigo.mms";
 XeenSaveFile::write(path,saved);combat_gameplay_test::Harness h;
 auto services=disengagementServices(source,h);bool completed=false;
 unsigned cityLoads=0;
 const auto maps=services.maps;services.maps=[&](auto id){
  if(id==XeenMapIdentity(28) || id.number>=109)++cityLoads;
  return maps(id);
 };
 services.show=[&](const auto &,const auto &handler,const auto &,const auto &idle,const auto &){
  const auto present=[&]{handler.framePresented(h.flow->frame().presentation());};
  const auto input=[&](PlayerAction action){present();handler.beginCycle(++h.cycle);
   handler.withDisplayedInput(action,*handler.displayedInput());present();};
  const auto settle=[&]{for(unsigned n=0;n<1000;++n){
   check(h.world->sessionState().journeyActivity()!=XeenJourneyActivity::Failed,"Cold transition integrity failure");
   if(h.flow->canSave())return;
   h.now+=100;handler.beginCycle(++h.cycle);idle();present();
  }throw std::runtime_error("Cold transition failed to settle");};
  present();settle();
  // Restore validation reads original resources, but does not prewarm a scene.
  // Drop disposable validation caches before ordinary input to prove misses.
  h.world->discardMapCache();cityLoads=0;
  input(InteractionAction{});input(YesAction{});settle();
  check(h.camera->mapId==XeenMapIdentity(28) && h.camera->x==15 && h.camera->y==0,
   "Cold Vertigo entry did not publish original entrance");
  check(cityLoads>0,"City resources were not loaded on demand");
  input(NavigationAction::TurnRight);settle();input(NavigationAction::TurnRight);settle();
  h.world->discardMapCache();
  input(InteractionAction{});input(YesAction{});settle();
  check(h.camera->mapId==XeenMapIdentity(23) && h.camera->x==10 && h.camera->y==12,
   "Cold Vertigo exit did not publish mainland destination");
  completed=true;return true;
 };
 check(Application().playGameplay(services,{},path,true)==0 && completed,"Cold Vertigo production entry/exit");
 std::filesystem::remove(path);
 std::cout<<"Cold Vertigo entry/exit with original composition PASS\n";
}


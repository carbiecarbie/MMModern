// One whole-game route and cross-process continuation, sharing the existing
// purchase process harness and native CLI witness.
void freshStartProcesses(const fs::path &executable, const fs::path &installation, const fs::path &directory) {
    const auto run = [&](const std::string &name, const std::wstring &stage, std::vector<std::wstring> args) {
        SetEnvironmentVariableW(L"MMODERN_M42_STAGE",stage.c_str());
        const auto result = child_test::launch(executable,args,directory/(name+".log"),false,false,180000);
        check(result.exit == 0 && result.output.find("M52 PUBLIC GAMEPLAY PASSED") != std::string::npos,
            "M52 public original gameplay failed; inspect process log");
    };
    for (bool warrior : {false,true}) {
        const std::string name = warrior ? "warrior" : "adventurer";
        const auto save = directory/(name+".mmsave");
        std::vector<std::wstring> args;
        // Exercise explicit and bare entry through the same whole-game route.
        if (!warrior) args.push_back(L"--new-game");
        args.push_back(installation.wstring());
        if (warrior) { args.push_back(L"--difficulty"); args.push_back(L"warrior"); }
        args.push_back(L"--save-file"); args.push_back(save.wstring());
        run(name,L"m52-play",args);
        const auto initial = XeenSaveFile::read(directory/(name+"-initial.mmsave"));
        const auto played = XeenSaveFile::read(directory/(name+"-played.mmsave"));
        check(initial.journey->context->difficulty == (warrior ? XeenDifficulty::Warrior : XeenDifficulty::Adventurer) &&
            played.journey->context->difficulty == initial.journey->context->difficulty,
            "M52 default/explicit difficulty retained through gameplay");
        const auto restoreInitial = directory/(name+"-restore-initial.mmsave");
        fs::copy_file(directory/(name+"-initial.mmsave"),restoreInitial);
        run(name+"-restore-initial",L"m52-play",{L"--load-game",installation.wstring(),restoreInitial.wstring()});
        for (const auto *label : {"initial","played","continued"})
            equal(directory/(name+"-"+label+".mmsave"),directory/(name+"-restore-initial-"+label+".mmsave"));
        const auto restorePlayed = directory/(name+"-restore-played.mmsave");
        fs::copy_file(directory/(name+"-played.mmsave"),restorePlayed);
        run(name+"-restore-played",L"m52-after",{L"--load-game",installation.wstring(),restorePlayed.wstring()});
        equal(directory/(name+"-continued.mmsave"),directory/(name+"-restore-played-continued.mmsave"));
        // Cover the other entry spelling with an explicit difficulty and F9 on
        // the very first frame, including Unicode/space save paths.
        const auto spellingSave = directory/fs::path(warrior ? L"explicit warrior \u6e38.mmsave" : L"bare adventurer \u00e7.mmsave");
        args.clear(); if (warrior) args.push_back(L"--new-game");
        args.push_back(installation.wstring()); args.push_back(L"--difficulty");
        args.push_back(warrior ? L"warrior" : L"adventurer");
        args.push_back(L"--save-file"); args.push_back(spellingSave.wstring());
        run(name+"-other-spelling",L"m52-initial",args);
        equal(directory/(name+"-initial.mmsave"),spellingSave);
        args.clear(); if (warrior) args.push_back(L"--new-game");
        args.push_back(installation.wstring());
        if (warrior) { args.push_back(L"--difficulty"); args.push_back(L"warrior"); }
        run(name+"-no-target",L"m52-initial",args);
    }
    SetEnvironmentVariableW(L"MMODERN_M42_STAGE",nullptr);
    std::cout << "M52 both public forms/difficulties, immediate F9, East Event, walk/combat/Rest/paid service, "
        "refusal, mainland exit/re-entry, exact initial/played restores and continuation passed; evidence " << directory.u8string() << '\n';
}

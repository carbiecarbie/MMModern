#include "XeenPortableTestSupport.h"
// One whole-game route and cross-process continuation, sharing the existing
// purchase process harness and native CLI witness.
void freshStartProcesses(const fs::path &executable, const fs::path &installation, const fs::path &directory) {
    const auto run = [&](const std::string &name, const std::string &stage, std::vector<std::string> args) {
        portable_test::setEnvironment("MMODERN_M42_STAGE",stage.c_str());
        const auto result = child_test::launch(executable,args,directory/(name+".log"),false,false,180000);
        check(result.exit == 0 && result.output.find("M52 PUBLIC GAMEPLAY PASSED") != std::string::npos,
            "M52 public original gameplay failed; inspect process log");
    };
    for (bool warrior : {false,true}) {
        const std::string name = warrior ? "warrior" : "adventurer";
        const auto save = directory/(name+".mmsave");
        std::vector<std::string> args;
        // Gameplay witnesses use an explicit developer entry. Bare launch tests cover title.
        args.push_back("--new-game");
        args.push_back(installation.u8string());
        if (warrior) { args.push_back("--difficulty"); args.push_back("warrior"); }
        args.push_back("--save-file"); args.push_back(save.u8string());
        run(name,"m52-play",args);
        const auto initial = XeenSaveFile::read(directory/(name+"-initial.mmsave"));
        const auto played = XeenSaveFile::read(directory/(name+"-played.mmsave"));
        check(initial.journey->context->difficulty == (warrior ? XeenDifficulty::Warrior : XeenDifficulty::Adventurer) &&
            played.journey->context->difficulty == initial.journey->context->difficulty,
            "M52 default/explicit difficulty retained through gameplay");
        const auto restoreInitial = directory/(name+"-restore-initial.mmsave");
        fs::copy_file(directory/(name+"-initial.mmsave"),restoreInitial);
        run(name+"-restore-initial","m52-play",{"--load-game",installation.u8string(),restoreInitial.u8string()});
        for (const auto *label : {"initial","played","continued"})
            equal(directory/(name+"-"+label+".mmsave"),directory/(name+"-restore-initial-"+label+".mmsave"));
        const auto restorePlayed = directory/(name+"-restore-played.mmsave");
        fs::copy_file(directory/(name+"-played.mmsave"),restorePlayed);
        run(name+"-restore-played","m52-after",{"--load-game",installation.u8string(),restorePlayed.u8string()});
        equal(directory/(name+"-continued.mmsave"),directory/(name+"-restore-played-continued.mmsave"));
        // Cover the explicit entry with an explicit difficulty and F9 on
        // the very first frame, including Unicode/space save paths.
        const auto spellingSave = directory/fs::u8path(warrior ? u8"explicit warrior \u6e38.mmsave" : u8"explicit adventurer \u00e7.mmsave");
        args.clear(); args.push_back("--new-game");
        args.push_back(installation.u8string()); args.push_back("--difficulty");
        args.push_back(warrior ? "warrior" : "adventurer");
        args.push_back("--save-file"); args.push_back(spellingSave.u8string());
        run(name+"-other-spelling","m52-initial",args);
        equal(directory/(name+"-initial.mmsave"),spellingSave);
        args.clear(); args.push_back("--new-game");
        args.push_back(installation.u8string());
        if (warrior) { args.push_back("--difficulty"); args.push_back("warrior"); }
        run(name+"-no-target","m52-initial",args);
    }
    portable_test::setEnvironment("MMODERN_M42_STAGE",nullptr);
    std::cout << "M52 explicit entry/both difficulties, immediate F9, East Event, walk/combat/Rest/paid service, "
        "refusal, mainland exit/re-entry, exact initial/played restores and continuation passed; evidence " << directory.u8string() << '\n';
}

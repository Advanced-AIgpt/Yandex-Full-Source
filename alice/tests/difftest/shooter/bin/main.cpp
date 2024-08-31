#include <alice/joker/library/log/log.h>
#include <alice/tests/difftest/shooter/library/core/context.h>
#include <alice/tests/difftest/shooter/library/core/engine.h>
#include <alice/tests/difftest/shooter/library/diff2html/diff2html.h>
#include <alice/tests/difftest/shooter/library/perfdiff/perfdiff.h>
#include <alice/tests/difftest/shooter/library/ydb/ydb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/getopt/small/modchooser.h>
#include <library/cpp/sighandler/async_signals_handler.h>

#include <util/generic/maybe.h>
#include <util/generic/yexception.h>
#include <util/system/fs.h>
#include <util/thread/pool.h>

using namespace NAlice::NShooter;

namespace {

int Run(int argc, const char** argv) {
    NLastGetopt::TOpts opts;
    opts.SetFreeArgsNum(1);
    opts.SetFreeArgTitle(0, "<config.json>", "configuration file in json");

    // tokens are top secrets, can't have then in configs
    TTokens tokens;

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("yav-token").
        Help("Token for Yav secrets").
        StoreResult(&tokens.YavToken));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("test-user-token").
        Help("Token for test user").
        StoreResult(&tokens.TestUserToken));

    NLastGetopt::TOptsParseResult res{&opts, argc, argv};

    TString configFileName{res.GetFreeArgs()[0]};
    TContext context{std::move(configFileName), std::move(tokens)};

    const auto& config = context.Config();
    LOG(INFO) << "Config: " << config.ToJson() << Endl;

    auto runsSettings = config.RunsSettings();
    Y_ASSERT(runsSettings.Size() > 0);

    TVector<THolder<TEngine>> engines;
    engines.reserve(runsSettings.Size());
    for (size_t i = 0; i < runsSettings.Size(); ++i) {
        engines.emplace_back(MakeHolder<TEngine>(context, runsSettings[i]));
    }

    auto shutdown = [&engines](int) {
        for (auto& engine : engines) {
            engine->ForceShutdown();
        }
        exit(EXIT_SUCCESS);
    };

    SetAsyncSignalFunction(SIGTERM, shutdown);
    SetAsyncSignalFunction(SIGINT, shutdown);

    auto threadPool = CreateThreadPool(engines.size());

    for (auto& engine : engines) {
        threadPool->SafeAddFunc([&engine]() {
            TLogging::InitTlsUniqId();
            try {
                engine->Run();
            } catch (...) {
                LOG(ERROR) << CurrentExceptionMessage() << Endl;
            }
        });
    }
    threadPool->Stop();

    SetAsyncSignalFunction(SIGTERM, nullptr);
    SetAsyncSignalFunction(SIGINT, nullptr);

    return EXIT_SUCCESS;
}

int Diff(int argc, const char** argv) {
    int diffsPerFile;
    TString mode;
    TString oldResponsesPath;
    TString newResponsesPath;
    TString outputPath;
    TString statsPath;
    int threads;

    NLastGetopt::TOpts opts;
    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("diffs-per-file").
        Help("Diff count per 1 HTML file").
        Required().
        StoreResult(&diffsPerFile));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("mode").
        Help("'megamind', 'hollywood', 'hollywood_bass'").
        DefaultValue("megamind").
        StoreResult(&mode));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("old-path").
        Help("Path to folder with old responses").
        Required().
        StoreResult(&oldResponsesPath));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("new-path").
        Help("Path to folder with new responses").
        Required().
        StoreResult(&newResponsesPath));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("output-path").
        Help("Output path to diff").
        Required().
        StoreResult(&outputPath));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("threads").
        Help("Working threads").
        DefaultValue(5).
        StoreResult(&threads));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("stats-path").
        Help("Stats in JSON format (for Sandbox task)").
        StoreResult(&statsPath));

    NLastGetopt::TOptsParseResult{&opts, argc, argv};

    TMaybe<TFsPath> statsFsPath;
    if (!statsPath.empty()) {
        statsFsPath = statsPath;
    }
    TDiff2Html{threads, diffsPerFile, mode, oldResponsesPath, newResponsesPath, outputPath, statsFsPath}.ConstructDiff();
    return EXIT_SUCCESS;
}

int PerfDiff(int argc, const char** argv) {
    TString oldResponsesPath;
    TString newResponsesPath;
    TString outputPath;

    NLastGetopt::TOpts opts;
    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("old-path").
        Help("Path to folder with old responses").
        Required().
        StoreResult(&oldResponsesPath));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("new-path").
        Help("Path to folder with new responses").
        Required().
        StoreResult(&newResponsesPath));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("output-path").
        Help("Output path to diff").
        Required().
        StoreResult(&outputPath));

    NLastGetopt::TOptsParseResult{&opts, argc, argv};

    TPerfDiff{oldResponsesPath, newResponsesPath, outputPath}.ConstructDiff();
    return EXIT_SUCCESS;
}

int DownloadConfig(int argc, const char** argv) {
    TString endpoint;
    TString database;
    TString ydbToken;

    TString table;
    TString id;

    TString savePath;

    NLastGetopt::TOpts opts;
    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("endpoint").
        Help("YDB Endpoint").
        Required().
        StoreResult(&endpoint));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("database").
        Help("YDB Database").
        Required().
        StoreResult(&database));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("ydb-token").
        Help("YDB Token").
        Required().
        StoreResult(&ydbToken));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("table").
        Help("YDB table to take the config").
        Required().
        StoreResult(&table));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("id").
        Help("ID of the config in the YDB table").
        Required().
        StoreResult(&id));

    opts.AddOption(NLastGetopt::TOpt().
        AddLongName("save-path").
        Help("Path to save the config").
        Required().
        StoreResult(&savePath));

    NLastGetopt::TOptsParseResult{&opts, argc, argv};

    TYdb ydb{endpoint, database, ydbToken};
    TString config = ydb.ObtainConfig(table, id);
    TFileOutput{savePath}.Write(config);

    return EXIT_SUCCESS;
}

} // namespace

int main(int argc, const char** argv) {
    TLogging::InitTlsUniqId();

    TModChooser modChooser;
    modChooser.SetDescription("Tool for running Megamind + VINS + BASS from VINS_PACKAGE and shooting.");
    modChooser.AddMode("run", Run, "run and shoot apps");
    modChooser.AddMode("diff", Diff, "diff two responses packs");
    modChooser.AddMode("perfdiff", PerfDiff, "perf-diff two responses packs");
    modChooser.AddMode("download-config", DownloadConfig, "download config from YDB");

    return modChooser.Run(argc, argv);
}

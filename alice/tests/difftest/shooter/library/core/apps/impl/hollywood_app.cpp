#include "hollywood_app.h"

#include <alice/joker/library/log/log.h>
#include <alice/tests/difftest/shooter/library/core/context.h>
#include <alice/tests/difftest/shooter/library/core/engine.h>
#include <alice/tests/difftest/shooter/library/core/run_settings.h>

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/printf.h>
#include <util/system/fs.h>

namespace NAlice::NShooter {

namespace {

static inline constexpr TStringBuf NAME = "hollywood";

static inline const TString CONFIG_TEMPLATE = R"(
# We use default value for AppHost

AppHostConfig {
    Port: %d
    Threads: %d
}

RTLog {
    Async: true
    Filename: "%s"
    FlushPeriodSecs: 1
    ServiceName: "hollywood"
    FileStatCheckPeriodSecs: 1
}

Scenarios: [
    "fast_command",
    "game_suggest",
    "general_conversation",
    "general_conversation_proactivity",
    "hardcoded_music",
    "hardcoded_response",
    "movie_akinator",
    "movie_suggest",
    "market_orders_status",
    "music",
    "random_number",
    "search",
    "sssss",
    "video_rater"
]

FastDataPath: "fast_data_stable"
ScenarioResourcesPath: "resources"
)";

void SetupHollywoodConfig(TFsPath workdir, ui16 port, ui16 threads) {
    LOG(INFO) << "Make hollywood config" << Endl;
    auto configPath = workdir / "fixed-hollywood.pb.txt";

    TFile file(configPath, OpenAlways | WrOnly);
    TFileOutput fs(file);

    TString config = Sprintf(CONFIG_TEMPLATE.c_str(), port, threads, TString{workdir / "current-hollywood-rtlog"}.c_str());
    fs << config << Endl;
}

void CreateSvnInfoFile(TFsPath workdir) {
    TFileOutput fout(workdir / "fast_data_stable" / ".svninfo");
    fout << "fast_data_version: 1";
}

} // namespace

THollywoodApp::THollywoodApp(const IEngine& engine)
    : TLocalDecoratedApp{engine}
{
}

THollywoodApp::THollywoodApp(const IEngine& engine, TIntrusivePtr<IApp> wrappee)
    : TLocalDecoratedApp{engine, wrappee}
{
}

TStringBuf THollywoodApp::Name() const {
    return NAME;
}

TStatus THollywoodApp::ExtraRun() {
    // Obtain free port
    auto& ports = Engine_.Ports();
    ui16 port = ports->Add(Name());

    // Setup redis config file
    TFsPath workdir(Engine_.RunSettings().PackagePath);
    SetupHollywoodConfig(workdir, port, 90);
    CreateSvnInfoFile(workdir);

    // Update handler commands
    ShellCommand_ = MakeHolder<TShellCommand>(/* cmd = */ "", ShellCommandOptions_, /* workdir = */ TString{Engine_.RunSettings().PackagePath});
    *ShellCommand_ << TString(workdir / "hollywood_server");
    *ShellCommand_ << "-c" << TString(workdir / "fixed-hollywood.pb.txt");
    ShellCommand_->Run();

    return Success();
}

TStatus THollywoodApp::ExtraPing() {
    const auto& ports = Engine_.Ports();
    NNeh::TResponseRef r = AppRequest(Name(), {"localhost", ports->Get(Name()), "/ping"});
    if (r && r->Data == "pong") {
        return Success();
    }
    return TError();
}

} // namespace NAlice::NShooter

#include "redis_app.h"

#include <alice/joker/library/log/log.h>
#include <alice/tests/difftest/shooter/library/core/context.h>
#include <alice/tests/difftest/shooter/library/core/engine.h>
#include <alice/tests/difftest/shooter/library/core/run_settings.h>

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/system/fs.h>

namespace NAlice::NShooter {

namespace {

static inline constexpr TStringBuf NAME = "redis";

// copy 'redis.conf.base' -> 'redis.conf' and write the port at the end
void SetupRedisConfig(TFsPath workdir, ui16 port) {
    LOG(INFO) << "Make redis config" << Endl;

    auto baseConfigPath = workdir / "redis.conf.base";
    auto configPath = workdir / "redis.conf";
    NFs::Copy(baseConfigPath, configPath);

    TFile file(configPath, OpenExisting | WrOnly | ForAppend);
    TFileOutput fs(file);
    fs << "port " << port << Endl;
}

} // namespace

TRedisApp::TRedisApp(const IEngine& engine)
    : TLocalDecoratedApp{engine}
{
}

TRedisApp::TRedisApp(const IEngine& engine, TIntrusivePtr<IApp> wrappee)
    : TLocalDecoratedApp{engine, wrappee}
{
}

TStringBuf TRedisApp::Name() const {
    return NAME;
}

TStatus TRedisApp::ExtraRun() {
    // Obtain free port
    auto& ports = Engine_.Ports();
    ui16 port = ports->Add(Name());

    // Setup redis config file
    TFsPath workdir(Engine_.RunSettings().PackagePath);
    SetupRedisConfig(workdir, port);

    // Update handler commands
    ShellCommand_ = MakeHolder<TShellCommand>(/* cmd = */ "", ShellCommandOptions_, /* workdir = */ TString{Engine_.RunSettings().PackagePath});
    *ShellCommand_ << TString(workdir / "redis-server");
    *ShellCommand_ << "redis.conf";
    ShellCommand_->Run();

    return Success();
}

TStatus TRedisApp::ExtraPing() {
    return Success();
}

} // namespace NAlice::NShooter

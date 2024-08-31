#include "vins_app.h"

#include <alice/joker/library/log/log.h>
#include <alice/tests/difftest/shooter/library/core/context.h>
#include <alice/tests/difftest/shooter/library/core/util.h>
#include <alice/tests/difftest/shooter/library/core/engine.h>

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/system/fs.h>

#include <library/cpp/json/json_writer.h>

namespace NAlice::NShooter {

namespace {

static inline constexpr TStringBuf NAME = "vins";

} // namespace

TVinsApp::TVinsApp(const IContext& ctx, const IEngine& engine)
    : TLocalDecoratedApp{engine}
    , Ctx_{ctx}
{
}

TVinsApp::TVinsApp(const IContext& ctx, const IEngine& engine, TIntrusivePtr<IApp> wrappee)
    : TLocalDecoratedApp{engine, wrappee}
    , Ctx_{ctx}
{
}

TStringBuf TVinsApp::Name() const {
    return NAME;
}

TStatus TVinsApp::ExtraRun() {
    // Shell environment
    auto& env = ShellCommandOptions_.Environment;
    env = *Ctx_.Yav();
    env["MONGO_PASSWORD"] = env["VINS_MONGO_PASSWORD"];
    env["BASS_AUTH_TOKEN"] = Ctx_.Tokens().YavToken;

    const auto& jokerSettings = Ctx_.JokerServerSettings();
    const auto& config = Ctx_.Config();
    const auto& runSettings = Engine_.RunSettings();
    auto& ports = Engine_.Ports();
    TFsPath logsPath{runSettings.LogsPath};

    if (jokerSettings) {
        env["VINS_PROXY_URL"] = MakeUrl(jokerSettings->Host, jokerSettings->Port);
        env["VINS_PROXY_SKIP"] = "http://localhost"; // don't proxy all local services

        THashMap<TString, TString> headers = MakeProxyHeaders(Ctx_);
        NJson::TJsonValue headersJson;
        for (const auto& p : headers) {
            headersJson[p.first] = p.second;
        }
        TStringStream ss;
        NJson::TJsonWriter{&ss, /* formatOutput = */ false}.Write(headersJson);
        env["VINS_PROXY_DEFAULT_HEADERS"] = NJson::WriteJson(headersJson, /* formatOutput = */ false);

        //env["VINS_NOW_TIMESTAMP"] = ToString(config.ServersSettings().VinsCurrentTimestamp());
    }
    env["VINS_WORKERS_COUNT"] = ToString(config.ServersSettings().VinsWorkers());
    env["VINS_DEV_BASS_API_URL"] = MakeUrl("localhost", ports->Get("bass"));
    env["VINS_REDIS_PORT"] = ToString(ports->Get("redis"));
    env["VINS_RTLOG_FILE"] = TString{logsPath / "vins-rtlog"};
    env["VINS_LOG_FILE"] = TString{logsPath / "vins-log-file"};

    // Obtain free port
    ui16 port = ports->Add(Name());

    TFsPath packagePath{Engine_.RunSettings().PackagePath};

    // Vmtouch resources
    if (Ctx_.Config().Vmtouch()) {
        auto resourcesPath{packagePath / "resources"};
        TVector<TFsPath> resPaths;
        resourcesPath.List(resPaths);
        TShellCommand vmtouchCmd(TString{resourcesPath / "vmtouch"}, {"-l", "-v", "-f"});
        for (const auto& p : resPaths) {
            vmtouchCmd << TString{p};
        }
        vmtouchCmd.Run().Wait();
    }

    // Update handler commands
    ShellCommand_ = MakeHolder<TShellCommand>(/* cmd = */ "", ShellCommandOptions_, /* workdir = */ TString{runSettings.PackagePath});
    *ShellCommand_ << TString{packagePath / "run-vins.py"};
    *ShellCommand_ << "-p" << ToString(port);
    *ShellCommand_ << "--conf-dir" << "cit_configs";

    *ShellCommand_ << "--env" << "stable";
    *ShellCommand_ << "--component" << "speechkit-api-pa";
    *ShellCommand_ << "-L";

    ShellCommand_->Run();

    return Success();
}

TStatus TVinsApp::ExtraPing() {
    const auto& ports = Engine_.Ports();
    NNeh::TResponseRef r = AppRequest(Name(), {"localhost", ports->Get(Name()), "/ping"});
    if (r && r->Data == "Ok") {
        return Success();
    }
    return TError();
}

//TMaybe<TString> TVinsApp::SensorsData() const {
    //NNeh::TResponseRef r = AppRequest(Name(), {"localhost", Engine_.Ports()->Get(NAME), "/solomon"});
    //if (!r || r->IsError()) {
        //LOG(ERROR) << "Can't get sensors data from VINS" << Endl;
        //return Nothing();
    //}
    //return BeautifyJson(std::move(r->Data));
//}

} // namespace NAlice::NShooter

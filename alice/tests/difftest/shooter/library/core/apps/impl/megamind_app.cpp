#include "megamind_app.h"

#include <alice/joker/library/log/log.h>
#include <alice/tests/difftest/shooter/library/core/context.h>
#include <alice/tests/difftest/shooter/library/core/util.h>
#include <alice/tests/difftest/shooter/library/core/engine.h>

#include <contrib/libs/re2/re2/re2.h>

#include <util/folder/path.h>
#include <util/generic/guid.h>
#include <util/folder/filelist.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/system/fs.h>

#include <library/cpp/json/json_writer.h>

namespace NAlice::NShooter {

namespace {

static inline constexpr TStringBuf NAME = "megamind";
static inline constexpr TStringBuf SENSORS_NAME = "megamind_sensors";

} // namespace

TMegamindApp::TMegamindApp(const IContext& ctx, const IEngine& engine)
    : TLocalDecoratedApp{engine}
    , Ctx_{ctx}
{
}

TMegamindApp::TMegamindApp(const IContext& ctx, const IEngine& engine, TIntrusivePtr<IApp> wrappee)
    : TLocalDecoratedApp{engine, wrappee}
    , Ctx_{ctx}
{
}

TStringBuf TMegamindApp::Name() const {
    return NAME;
}

TStatus TMegamindApp::ExtraRun() {
    // Shell environment
    auto& env = ShellCommandOptions_.Environment;
    env = *Ctx_.Yav();
    env["MONGO_PASSWORD"] = env["VINS_MONGO_PASSWORD"];
    env["BASS_AUTH_TOKEN"] = Ctx_.Tokens().YavToken;

    // Obtain free port for Megamind and its sensors
    auto& ports = Engine_.Ports();
    ui16 sensorsPort = ports->Add(SENSORS_NAME);
    ui16 port = ports->Add(NAME);

    const auto& jokerSettings = Ctx_.JokerServerSettings();
    const auto& runSettings = Engine_.RunSettings();
    TFsPath logsPath{runSettings.LogsPath};
    TFsPath packagePath{runSettings.PackagePath};

    TString vinsUrl{MakeUrl("localhost", ports->Get("vins"))};
    TString vinsHostPort{MakeHostPort("localhost", ports->Get("vins"))};
    TString bassUrl{MakeUrl("localhost", ports->Get("bass"))};
    TString bassHostPort{MakeHostPort("localhost", ports->Get("bass"))};

    // Make new configs
    const auto changeConfig = [&](TString& value) {
        if (runSettings.IncreaseTimeouts) {
            re2::RE2::GlobalReplace(&value, "TimeoutMs:\\s*(\\S+)", "TimeoutMs: \\100"); // timeout x100
            re2::RE2::GlobalReplace(&value, "RetryPeriodMs:\\s*(\\S+)", "RetryPeriodMs: \\100"); // retry period x100
        }
        re2::RE2::GlobalReplace(&value, "/logs/", "");
        re2::RE2::GlobalReplace(&value, "Tvm2ClientId: \"222\"", "Tvm2ClientId: \"239\"");
        re2::RE2::GlobalReplace(&value, "http://vins.hamster.alice.yandex.net/vins/", "http://" + vinsHostPort + "/");
        re2::RE2::GlobalReplace(&value, "https://blackbox.yandex.net/blackbox", "https://blackbox-mimino.yandex.net/blackbox");

        re2::RE2::GlobalReplace(&value, "megamind-rc.alice.yandex.net/vins", vinsHostPort);
        re2::RE2::GlobalReplace(&value, "vins.alice.yandex.net/vins", vinsHostPort);
        re2::RE2::GlobalReplace(&value, "vins.hamster.alice.yandex.net/vins/", vinsHostPort);

        re2::RE2::GlobalReplace(&value, "localhost:86", bassHostPort);
        re2::RE2::GlobalReplace(&value, "bass-prod.yandex.net", bassHostPort);
        re2::RE2::GlobalReplace(&value, "bass.hamster.alice.yandex.net", bassHostPort);
        re2::RE2::GlobalReplace(&value, "megamind-rc.alice.yandex.net/bass", bassHostPort);
    };

    TFsPath updatedConfigPath{logsPath / "fixed_megamind_config"};
    CopyDir(packagePath / "megamind_configs" / runSettings.Config, updatedConfigPath);
    CopyDir(packagePath / "megamind_configs" / "common", logsPath / "common");
    TString configValue = TFileInput{updatedConfigPath / "megamind.pb.txt"}.ReadAll();
    changeConfig(configValue);
    TFileOutput{updatedConfigPath / "megamind.pb.txt"}.Write(configValue);

    TFileEntitiesList fl(TFileEntitiesList::EM_FILES);
    fl.Fill(updatedConfigPath / "scenarios", TStringBuf(), TStringBuf(), /* depth = */ 100);
    while (const char * filename = fl.Next()) {
        TFsPath path{updatedConfigPath / "scenarios" / filename};
        TString value = TFileInput{path}.ReadAll();
        changeConfig(value);
        TFileOutput{path}.Write(value);
    }

    // Update handler commands
    ShellCommand_ = MakeHolder<TShellCommand>(/* cmd = */ "", ShellCommandOptions_, /* workdir = */ TString{runSettings.PackagePath});
    *ShellCommand_ << TString{packagePath / "bin" / "megamind_server"};
    *ShellCommand_ << "-c" << TString{updatedConfigPath / "megamind.pb.txt"};
    *ShellCommand_ << "-p" << ToString(port);

    if (jokerSettings) {
        for (const auto& pair : MakeProxyHeaders(Ctx_)) {
            *ShellCommand_ << "--via-proxy-headers" << TString::Join(pair.first, ":", pair.second);
        }
        *ShellCommand_ << "--via-proxy-host" << jokerSettings->Host;
        *ShellCommand_ << "--via-proxy-port" << ToString(jokerSettings->Port);
    }

    *ShellCommand_ << "--mon-service-port" << ToString(sensorsPort);

    *ShellCommand_ << "--service-sources-vins-url" << vinsUrl;

    *ShellCommand_ << "--scenarios-sources-bass-url" << bassUrl;
    *ShellCommand_ << "--scenarios-sources-bass-apply-url" << TString::Join(bassUrl, "/megamind/apply");
    *ShellCommand_ << "--scenarios-sources-bass-run-url" << TString::Join(bassUrl, "/megamind/prepare");

    *ShellCommand_ << "--rtlog-filename" << TString{logsPath / "megamind-rtlog"};
    *ShellCommand_ << "--vins-like-log-file" << TString{logsPath / "megamind-vins-like-log"};
    *ShellCommand_ << "--geobase-path" << TString{packagePath / "geodata6.bin"};

    //*ShellCommand_ << "--test-mode";
    *ShellCommand_ << "--http-server-threads" << "30";
    *ShellCommand_ << "--app-host-worker-threads" << "30";
    *ShellCommand_ << "--app-host-http-port" << ToString(ports->Add("apphost-http"));
    *ShellCommand_ << "--app-host-grpc-port" << ToString(ports->Add("apphost-grpc"));

    ShellCommand_->Run();

    return Success();
}

TStatus TMegamindApp::ExtraPing() {
    const auto& ports = Engine_.Ports();
    NNeh::TResponseRef r = AppRequest(Name(), {"localhost", ports->Get(Name()), "/ping"});
    if (r && r->Data == "pong") {
        return Success();
    }
    return TError();
}

//TMaybe<TString> TMegamindApp::SensorsData() const {
    //NNeh::TResponseRef r = AppRequest(Name(), {"localhost", Engine_.Ports()->Get(SENSORS_NAME), "/counters/json"});
    //if (!r || r->IsError()) {
        //LOG(ERROR) << "Can't get sensors data from Megamind" << Endl;
        //return Nothing();
    //}
    //return BeautifyJson(std::move(r->Data));
//}

} // namespace NAlice::NShooter

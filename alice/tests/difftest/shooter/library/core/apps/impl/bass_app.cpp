#include "bass_app.h"

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

static inline constexpr TStringBuf NAME = "bass";

void SetValueRecursively(NJson::TJsonValue& json, const TString& key, const TString& value) {
    if (json.Has(key)) {
        json[key] = value;
    }
    if (json.IsMap()) {
        for (auto& p : json.GetMapSafe()) {
            SetValueRecursively(p.second, key, value);
        }
    } else if (json.IsArray()) {
        for (auto& p : json.GetArraySafe()) {
            SetValueRecursively(p, key, value);
        }
    }
};

} // namespace

TBassApp::TBassApp(const IContext& ctx, const IEngine& engine, bool isHollywood)
    : TLocalDecoratedApp{engine}
    , Ctx_{ctx}
    , IsHollywood_{isHollywood}
{
}

TBassApp::TBassApp(const IContext& ctx, const IEngine& engine, bool isHollywood, TIntrusivePtr<IApp> wrappee)
    : TLocalDecoratedApp{engine, wrappee}
    , Ctx_{ctx}
    , IsHollywood_{isHollywood}
{
}

TStringBuf TBassApp::Name() const {
    return NAME;
}

TStatus TBassApp::ExtraRun() {
    // Shell environment
    auto& env = ShellCommandOptions_.Environment;
    env = *Ctx_.Yav();
    env["MONGO_PASSWORD"] = env["VINS_MONGO_PASSWORD"];
    env["BASS_AUTH_TOKEN"] = Ctx_.Tokens().YavToken;

    // Obtain free port
    auto& ports = Engine_.Ports();
    ui16 port = ports->Add(Name());

    // Setup bass config file
    const auto& config = Ctx_.Config();
    const auto& runSettings = Engine_.RunSettings();
    TFsPath packagePath{runSettings.PackagePath};
    TFileInput bassConfigFile{packagePath / "bass_configs" / TString::Join(runSettings.Config, "_config.json")};
    NJson::TJsonValue bassConfig = NJson::ReadJsonTree(&bassConfigFile);

    const auto& jokerSettings = Ctx_.JokerServerSettings();
    if (jokerSettings) {
        auto& fetcherProxy = bassConfig["FetcherProxy"];
        fetcherProxy["HostPort"] = MakeHostPort(jokerSettings->Host, jokerSettings->Port);

        THashMap<TString, TString> headers = MakeProxyHeaders(Ctx_);
        NJson::TJsonValue headersArray;
        for (const auto& p : headers) {
            NJson::TJsonValue headerPair;
            headerPair["Name"] = p.first;
            headerPair["Value"] = p.second;
            headersArray.AppendValue(std::move(headerPair));
        }
        fetcherProxy["Headers"] = headersArray;
    }

    if (config.HasYdb()) {
        auto& ydb = bassConfig["YDb"];
        ydb["Endpoint"] = TString{config.Ydb().Endpoint()};
        ydb["DataBase"] = TString{config.Ydb().Database()};
    }

    int bassThreads = static_cast<int>(config.ServersSettings().BassThreads());
    bassConfig["HttpThreads"] = bassThreads;
    bassConfig["SearchThreads"] = bassThreads;
    bassConfig["SetupThreads"] = bassThreads;
    if (runSettings.IncreaseTimeouts) {
        SetValueRecursively(bassConfig, "Timeout", "30s");
    }
    bassConfig["Vins"]["BlackBox"]["Host"] = "https://blackbox-mimino.yandex.net/blackbox";
    bassConfig["Vins"]["BlackBox"]["Tvm2ClientId"] = "239";
    bassConfig["Vins"]["Tvm2"]["BassTvm2ClientId"] = "2000860";

    // Save bass config file
    TFsPath logsPath{runSettings.LogsPath};
    TFsPath newConfigPath{logsPath / "new_bass.json"};
    TFileOutput fs{TFile{newConfigPath, OpenAlways | WrOnly}};
    NJson::TJsonWriter{&fs, /* formatOutput = */ true}.Write(bassConfig);

    // Update handler commands
    ShellCommand_ = MakeHolder<TShellCommand>(/* cmd = */ "", ShellCommandOptions_, /* workdir = */ TString{runSettings.PackagePath});
    if (IsHollywood_) {
        *ShellCommand_ << TString{packagePath / "bin" / "bass" / "bass_server"};
    } else {
        *ShellCommand_ << TString{packagePath / "bin" / "bass_server"};
    }
    *ShellCommand_ << TString{newConfigPath};
    *ShellCommand_ << "--port" << ToString(port);
    *ShellCommand_ << "-V" << TString::Join("EventLogFile=", TString{logsPath / "bass-rtlog"});
    if (IsHollywood_) {
        *ShellCommand_ << "-V" << TString::Join("ENV_GEOBASE_PATH=", TString{packagePath / "common_resources" / "geodata6.bin"});
    } else {
        *ShellCommand_ << "-V" << TString::Join("ENV_GEOBASE_PATH=", TString{packagePath / "geodata6.bin"});
    }
    *ShellCommand_ << "--logdir" << TString{logsPath};
    ShellCommand_->Run();

    return Success();
}

TStatus TBassApp::ExtraPing() {
    const auto& ports = Engine_.Ports();
    NNeh::TResponseRef r = AppRequest(Name(), {"localhost", ports->Get(Name()), "/ping"});
    if (r && r->Data == "pong") {
        return Success();
    }
    return TError();
}

//TMaybe<TString> TBassApp::SensorsData() const {
    //NNeh::TResponseRef r = AppRequest(Name(), {"localhost", static_cast<ui16>(Engine_.Ports()->Get(NAME) + 1), "/counters/json"});
    //if (!r || r->IsError()) {
        //LOG(ERROR) << "Can't get sensors data from BASS" << Endl;
        //return Nothing();
    //}
    //return BeautifyJson(std::move(r->Data));
//}

} // namespace NAlice::NShooter

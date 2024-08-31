#include "megamind_factory.h"

#include <alice/joker/library/log/log.h>
#include <alice/rtlog/evloganalize/split_by_reqid/library/splitter.h>
#include <alice/tests/difftest/shooter/library/core/apps/impl/bass_app.h>
#include <alice/tests/difftest/shooter/library/core/apps/impl/hollywood_app.h>
#include <alice/tests/difftest/shooter/library/core/apps/impl/joker_app.h>
#include <alice/tests/difftest/shooter/library/core/apps/impl/megamind_app.h>
#include <alice/tests/difftest/shooter/library/core/apps/impl/redis_app.h>
#include <alice/tests/difftest/shooter/library/core/apps/impl/vins_app.h>
#include <alice/tests/difftest/shooter/library/core/engine.h>

#include <library/cpp/json/json_writer.h>

namespace NAlice::NShooter {

namespace {

void SplitRtLogs(const TFsPath& logsPath, const TFsPath& responsesPath) {
    LOG(INFO) << "Saving logs from Megamind, VINS and BASS" << Endl;
    NSplitter::TSplitter{responsesPath, {
        logsPath / "megamind-rtlog",
        logsPath / "vins-rtlog",
        logsPath / "bass-rtlog"
    }}.Split();
}

//void SaveSensors(const TIntrusivePtr<TBassApp>& bassApp,
                 //const TIntrusivePtr<TVinsApp>& vinsApp,
                 //const TIntrusivePtr<TMegamindApp>& megamindApp,
                 //const TFsPath& responsesPath) {
    //LOG(INFO) << "Saving sensors from MM+VINS+BASS started" << Endl;

    //TMaybe<TString> sensors = bassApp->SensorsData();
    //if (sensors) {
        //TFileOutput{responsesPath / "bass_sensors.json"}.Write(sensors.GetRef());
    //}

    //sensors = vinsApp->SensorsData();
    //if (sensors) {
        //TFileOutput{responsesPath / "vins_sensors.json"}.Write(sensors.GetRef());
    //}

    //sensors = megamindApp->SensorsData();
    //if (sensors) {
        //TFileOutput{responsesPath / "megamind_sensors.json"}.Write(sensors.GetRef());
    //}

    //LOG(INFO) << "Saving sensors from MM+VINS+BASS ended" << Endl;
//}

void SaveIntents(const TFsPath& responsesPath) {
    LOG(INFO) << "Saving intents from Megamind started" << Endl;

    THashMap<TString, ui32> intentsMap;

    TVector<TFsPath> children;
    responsesPath.List(children);
    for (const auto& c : children) {
        if (c.IsDirectory()) {
            TFsPath respPath{c / "response.json"};
            if (respPath.Exists()) {
                TFileInput fin(respPath);

                NJson::TJsonValue value = NJson::ReadJsonTree(&fin, /* throwOnError = */ true);
                if (value.Has("megamind_analytics_info")) {
                    value = value["megamind_analytics_info"];

                    if (value.Has("analytics_info")) {
                        value = value["analytics_info"];

                        if (value.IsMap()) {
                            value = value.GetMapSafe().begin()->second;
                            const TString& intent = value["scenario_analytics_info"]["intent"].GetString();
                            ++intentsMap[intent];
                        }
                    }
                }
            }
        }
    }

    NJson::TJsonValue intentsMapJson;
    for (const auto& p : intentsMap) {
        intentsMapJson[p.first] = p.second;
    }

    TFileOutput{responsesPath / "intents.json"}.Write(
        NJson::WriteJson(intentsMapJson, /* formatOutput = */ true, /* sortKeys = */ true)
    );
    LOG(INFO) << "Saving intents from Megamind ended" << Endl;
}

} // namespace

// TMegamindFactory
TMegamindFactory::TMegamindFactory(const IContext& ctx, const IEngine& engine)
    : Ctx_{ctx}
    , Engine_{engine}
{
}

TIntrusivePtr<IApp> TMegamindFactory::MakeApp() {
    TIntrusivePtr<IApp> app = nullptr;

    // set Joker
    if (Ctx_.Config().HasJokerConfig()) {
        app = MakeIntrusive<TJokerApp>(Ctx_);
    }

    // set Redis
    if (app == nullptr) {
        app = MakeIntrusive<TRedisApp>(Engine_);
    } else {
        app = MakeIntrusive<TRedisApp>(Engine_, app);
    }

    // set Bass+Vins+Megamind
    app = MakeIntrusive<TBassApp>(Ctx_, Engine_, /*isHollywood = */ false, app);
    app = MakeIntrusive<TVinsApp>(Ctx_, Engine_, app);
    app = MakeIntrusive<TMegamindApp>(Ctx_, Engine_, app);

    return app;
}

THolder<IRequester> TMegamindFactory::MakeRequester() {
    return MakeHolder<TMegamindRequester>(Ctx_, Engine_);
}

TMegamindFactory::~TMegamindFactory() {
    // write ports to file
    const auto& ports = Engine_.Ports();
    NJson::TJsonValue value;
    for (const auto& appName : {"joker", "redis", "bass", "vins", "megamind", "megamind_sensors"}) {
        if (ports->Has(appName)) {
            value[appName] = ports->Get(appName);
        }
    }
    TFileOutput fout{"ports.json"};
    NJson::WriteJson(&fout, &value, /* formatOutput = */ true, /* sortKeys = */ true);

    const auto& runSettings = Engine_.RunSettings();
    if (runSettings.ResponsesPath.Empty()) {
        return;
    }
    TFsPath logsPath{runSettings.LogsPath};
    TFsPath responsesPath{runSettings.ResponsesPath};
    if (!runSettings.EnablePerfMode) {
        SplitRtLogs(logsPath, responsesPath);
        //SaveSensors(BassApp_, VinsApp_, MegamindApp_, responsesPath);
        SaveIntents(responsesPath);
    }
}

} // namespace NAlice::NShooter

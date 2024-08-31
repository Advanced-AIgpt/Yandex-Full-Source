#include <library/cpp/testing/benchmark/bench.h>

#include <alice/cuttlefish/library/metrics/aggregation.h>
#include <alice/cuttlefish/library/metrics/dummy.h>
#include <alice/cuttlefish/library/metrics/engine.h>


using NVoice::NMetrics::TAggregationRules;
using NVoice::NMetrics::ELabel;
using NVoice::NMetrics::TLabels;
using NVoice::NMetrics::TClientInfo;

void InitData(TAggregationRules& f, TClientInfo& info);


Y_CPU_BENCHMARK(LabelFilter, iface) {
    TAggregationRules f;
    TClientInfo info;
    TLabels lbls;

    InitData(f, info);

    for (size_t i = 0; i < iface.Iterations(); ++i) {
        Y_DO_NOT_OPTIMIZE_AWAY(lbls = f.Process(info));
    }
}

using NVoice::NMetrics::TDummyBackend;
using NVoice::NMetrics::TMetricsEngine;
using NVoice::NMetrics::TScopeMetrics;
using NVoice::NMetrics::TDummyBackend;
using NVoice::NMetrics::TSolomonBackend;
using NVoice::NMetrics::TAggregationRules;
using NVoice::NMetrics::ELabel;
using NVoice::NMetrics::EMetricsBackend;
using NVoice::NMetrics::TClientInfo;


THolder<TSolomonBackend> MakeBackend() {
    NMonitoring::TBucketBounds bounds;
    TAggregationRules rules;
    rules.AddMap(ELabel::Application, "ru.yandex.quasar.services", "quasar");
    rules.AddMap(ELabel::Application, "aliced", "quasar");
    rules.AddMap(ELabel::Device, "yandex-station", "station");
    rules.AddFilter(ELabel::Application, "quasar");
    rules.SetDefaultLabel(ELabel::Application, "other");

    return MakeHolder<TSolomonBackend>(std::move(rules), bounds, "proxy", false);
}

TIntrusivePtr<TMetricsEngine> MakeEngine() {
    THolder<TSolomonBackend> back = MakeBackend();
    TIntrusivePtr<TMetricsEngine> engine = MakeIntrusive<TMetricsEngine>();
    engine->SetBackend(EMetricsBackend::Dummy, std::move(back));
    return engine;
}

TClientInfo MakeInfo(bool quasar) {
    TClientInfo ret;

    if (quasar) {
        ret.DeviceName = "yandex-station";
        ret.AppId = "ru.yandex.quasar.services";
        ret.GroupName = "quasar";
        ret.SubgroupName = "all";
        ret.ClientType = NVoice::NMetrics::EClientType::User;
    } else {
        ret.DeviceName = "samsung";
        ret.AppId = "ru.yandex.searchplugin.beta";
        ret.GroupName = "pp";
        ret.SubgroupName = "beta";
        ret.ClientType = NVoice::NMetrics::EClientType::User;
    }

    return ret;
}


Y_CPU_BENCHMARK(EngineAndScope, iface) {
    TIntrusivePtr<TMetricsEngine> engine = MakeEngine();
    TClientInfo info = MakeInfo(true);

    for (size_t i = 0; i < iface.Iterations(); ++i) {
        TScopeMetrics scope = engine->BeginScope(info, EMetricsBackend::Dummy);
        scope.OnScopeStarted("session");
        scope.OnPartialResult(1, "dialog", "asrgpu", "eou");
        scope.OnScopeCompleted("ok");
    }
}


void InitData(TAggregationRules& f, TClientInfo& info) {
    f.AddMap(ELabel::Device, "yandex-station", "station");
    f.AddMap(ELabel::Device, "yandex-station2", "station");
    f.AddMap(ELabel::Device, "samsung", "mobile");
    f.AddMap(ELabel::Device, "xiaomi", "mobile");
    f.AddMap(ELabel::Device, "huawei", "mobile");
    f.AddMap(ELabel::Device, "honor", "mobile");
    f.AddMap(ELabel::Application, "ru.yandex.quasar.services", "quasar");
    f.AddMap(ELabel::Application, "ru.yandex.searchplugin", "ru.yandex.searchplugin");
    f.AddMap(ELabel::Application, "ru.yandex.searchplugin.beta", "ru.yandex.searchplugin");
    f.AddMap(ELabel::Application, "ru.yandex.searchplugin.dev", "ru.yandex.searchplugin");
    f.AddMap(ELabel::Application, "aliced", "quasar");
    f.AddFilter(ELabel::Device, "station");
    f.AddFilter(ELabel::Device, "mobile");
    f.AddFilter(ELabel::SubgroupName, "prod");
    f.SetDefaultLabel(ELabel::Device, "other");

    info.DeviceName = "yandex-station";
    info.GroupName = "quasar";
    info.SubgroupName = "prod";
    info.AppId = "ru.yandex.quasar.services";
}

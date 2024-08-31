#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/guid.h>

#include "engine.h"
#include "dummy.h"


Y_UNIT_TEST_SUITE(Engine) {
    using NVoice::NMetrics::TMetricsEngine;
    using NVoice::NMetrics::TScopeMetrics;
    using NVoice::NMetrics::TDummyBackend;
    using NVoice::NMetrics::TAggregationRules;
    using NVoice::NMetrics::ELabel;
    using NVoice::NMetrics::EMetricsBackend;
    using NVoice::NMetrics::TClientInfo;

    THolder<TDummyBackend> MakeBackend() {
        NMonitoring::TBucketBounds bounds;
        TAggregationRules rules;
        rules.AddMap(ELabel::Application, "ru.yandex.quasar.services", "quasar");
        rules.AddMap(ELabel::Application, "aliced", "quasar");
        rules.AddMap(ELabel::Device, "yandex-station", "station");
        rules.AddFilter(ELabel::Application, "quasar");
        rules.SetDefaultLabel(ELabel::Application, "other");

        return MakeHolder<TDummyBackend>(std::move(rules), bounds);
    }

    TIntrusivePtr<TMetricsEngine> MakeEngine() {
        THolder<TDummyBackend> back = MakeBackend();
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

    Y_UNIT_TEST(SingleIncrement) {
        TIntrusivePtr<TMetricsEngine> engine = MakeEngine();

        {
            TClientInfo info = MakeInfo(true);
            TScopeMetrics scope = engine->BeginScope(info, EMetricsBackend::Dummy);
            scope.OnScopeStarted("session");

            {
                TStringStream oss;
                engine->SerializeMetrics(EMetricsBackend::Dummy, oss, NVoice::NMetrics::EOutputFormat::TEXT);
                const TString str = oss.Str();
                UNIT_ASSERT(str.Contains("sensor=session.self.inprogress"));
                UNIT_ASSERT(str.Contains("sensor=session.self.in"));
                UNIT_ASSERT(str.Contains("value=1"));
                UNIT_ASSERT(!str.Contains("value=0"));
            }
            scope.OnPartialResult(1, "dialog", "asrgpu", "eou");
            scope.OnScopeCompleted("ok");
        }

        TStringStream oss;
        engine->SerializeMetrics(EMetricsBackend::Dummy, oss, NVoice::NMetrics::EOutputFormat::TEXT);

        const TString str = oss.Str();
        Cerr << str << Endl;

        UNIT_ASSERT(str.Contains("device=station"));
        UNIT_ASSERT(str.Contains("value=1"));
        UNIT_ASSERT(str.Contains("value=0"));
        UNIT_ASSERT(str.Contains("code=eou"));
        UNIT_ASSERT(str.Contains("code=ok"));
        UNIT_ASSERT(str.Contains("surface=quasar"));
        UNIT_ASSERT(str.Contains("app=quasar"));
        UNIT_ASSERT(str.Contains("sensor=session.asrgpu.dialog"));
        UNIT_ASSERT(str.Contains("sensor=session.self.in"));
        UNIT_ASSERT(str.Contains("sensor=session.self.inprogress"));
        UNIT_ASSERT(str.Contains("sensor=session.self.completed"));
        UNIT_ASSERT(str.Contains("client_type=user"));
    }

    Y_UNIT_TEST(MultiIncrement) {
         TIntrusivePtr<TMetricsEngine> engine = MakeEngine();

        {
            TClientInfo info = MakeInfo(true);
            TScopeMetrics scope = engine->BeginScope(info, EMetricsBackend::Dummy);
            scope.OnScopeStarted("session");
            scope.OnPartialResult(1, "dialog", "asrgpu", "eou");
            scope.OnScopeCompleted("ok");
        }


        {
            TClientInfo info = MakeInfo(true);
            TScopeMetrics scope = engine->BeginScope(info, EMetricsBackend::Dummy);
            scope.OnScopeStarted("session");
            scope.OnPartialResult(1, "dialog", "asrgpu", "eou");
            scope.OnScopeCompleted("ok");
        }

        TStringStream oss;
        engine->SerializeMetrics(EMetricsBackend::Dummy, oss, NVoice::NMetrics::EOutputFormat::TEXT);

        const TString str = oss.Str();
        Cerr << str << Endl;

        UNIT_ASSERT(str.Contains("device=station"));
        UNIT_ASSERT(str.Contains("value=2"));
        UNIT_ASSERT(!str.Contains("value=1"));
        UNIT_ASSERT(str.Contains("value=0"));
        UNIT_ASSERT(str.Contains("code=eou"));
        UNIT_ASSERT(str.Contains("code=ok"));
        UNIT_ASSERT(str.Contains("surface=quasar"));
        UNIT_ASSERT(str.Contains("app=quasar"));
        UNIT_ASSERT(str.Contains("sensor=session.asrgpu.dialog"));
        UNIT_ASSERT(str.Contains("sensor=session.self.in"));
        UNIT_ASSERT(str.Contains("sensor=session.self.inprogress"));
        UNIT_ASSERT(str.Contains("sensor=session.self.completed"));
        UNIT_ASSERT(str.Contains("client_type=user"));
        UNIT_ASSERT(str.Contains("type=gauge"));
        UNIT_ASSERT(str.Contains("type=rate"));
        UNIT_ASSERT(!str.Contains("type=hist"));
    }
}



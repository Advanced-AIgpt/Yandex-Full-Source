#include "metrics.h"
#include "dummy.h"

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>


namespace {

NVoice::NMetrics::TMetrics& InitMetrics() {
    NVoice::NMetrics::TMetrics& metrics = NVoice::NMetrics::TMetrics::Instance();

    metrics.SetBackend(
        NVoice::NMetrics::EMetricsBackend::Solomon,
        MakeHolder<NVoice::NMetrics::TSolomonBackend>(
            NVoice::NMetrics::TAggregationRules(),
            NVoice::NMetrics::MakeMillisBuckets(),
            "proxy",
            true
        )
    );

    return metrics;
}

NJson::TJsonValue GetMetricsJson(NVoice::NMetrics::TMetrics& metrics) {
    TStringStream oss;
    metrics.SerializeMetrics(NVoice::NMetrics::EMetricsBackend::Solomon, oss, NVoice::NMetrics::EOutputFormat::JSON);

    NJson::TJsonValue json;
    NJson::ReadJsonTree(oss.Str(), &json, true);

    return json;
}

void CheckSensor(const NJson::TJsonValue& metricsJson, TStringBuf name, TStringBuf code, ui64 value) {
    for (const auto& sensor : metricsJson["sensors"].GetArray()) {
        if (sensor["labels"]["sensor"] == name && sensor["labels"]["code"] == code) {
            UNIT_ASSERT_VALUES_EQUAL(sensor["value"].GetUInteger(), value);
            return;
        }
    }

    UNIT_ASSERT_C(false, "sensor '" << name << "' with code '" << code << "' not found in " << metricsJson);
}

} // namespace

Y_UNIT_TEST_SUITE(Metrics) {

Y_UNIT_TEST(MetricsInstance) {
    NVoice::NMetrics::TMetrics& metrics = InitMetrics();

    NVoice::NMetrics::TClientInfo clientInfo;

    {
        NVoice::NMetrics::TScopeMetrics sm = metrics.BeginScope(clientInfo, NVoice::NMetrics::EMetricsBackend::Solomon);
        sm.PushRate("hello", "crash");
    }

    {
        CheckSensor(GetMetricsJson(metrics), "service.self.hello", "crash", 1);
        metrics.Reset();
        CheckSensor(GetMetricsJson(metrics), "service.self.hello", "crash", 0);
    }

    {
        NVoice::NMetrics::TScopeMetrics sm = metrics.BeginScope(clientInfo, NVoice::NMetrics::EMetricsBackend::Solomon);
        sm.PushRate("hello", "dolly");
    }

    {
        CheckSensor(GetMetricsJson(metrics), "service.self.hello", "dolly", 1);
        metrics.Reset();
        CheckSensor(GetMetricsJson(metrics), "service.self.hello", "dolly", 0);
    }

    {
        NVoice::NMetrics::TScopeMetrics sm = metrics.BeginScope(clientInfo, NVoice::NMetrics::EMetricsBackend::Solomon);
        sm.PushRate("hello", "crash");
        CheckSensor(GetMetricsJson(metrics), "service.self.hello", "crash", 1);
    }

    {
        CheckSensor(GetMetricsJson(metrics), "service.self.hello", "crash", 1);
        metrics.Reset();
        CheckSensor(GetMetricsJson(metrics), "service.self.hello", "crash", 0);
    }
}

Y_UNIT_TEST(SourceMetricsOk) {
    NVoice::NMetrics::TMetrics& metrics = InitMetrics();

    {
        NVoice::NMetrics::TClientInfo clientInfo;
        NVoice::NMetrics::TSourceMetrics sourceMetrics(metrics, "my_source", clientInfo, NVoice::NMetrics::EMetricsBackend::Solomon);

        sourceMetrics.PushRate("my_sensor", "my_code");
        sourceMetrics.PushRate("other_sensor", "other_code", "other_source");


        NJson::TJsonValue metricsJson = GetMetricsJson(metrics);

        CheckSensor(metricsJson, "my_source.self.in", "", 1);
        CheckSensor(metricsJson, "my_source.self.inprogress", "", 1);

        CheckSensor(metricsJson, "my_source.self.my_sensor", "my_code", 1);
        CheckSensor(metricsJson, "my_source.other_source.other_sensor", "other_code", 1);

    }

    CheckSensor(GetMetricsJson(metrics), "my_source.self.completed", "ok", 1);
}

Y_UNIT_TEST(SourceMetricsWithStatus) {
    NVoice::NMetrics::TMetrics& metrics = InitMetrics();

    {
        NVoice::NMetrics::TClientInfo clientInfo;
        NVoice::NMetrics::TSourceMetrics sourceMetrics(metrics, "my_source", clientInfo, NVoice::NMetrics::EMetricsBackend::Solomon);
        sourceMetrics.SetStatus("my_code");
    }

    CheckSensor(GetMetricsJson(metrics), "my_source.self.completed", "my_code", 1);
}

Y_UNIT_TEST(SourceMetricsWithError) {
    NVoice::NMetrics::TMetrics& metrics = InitMetrics();

    {
        NVoice::NMetrics::TClientInfo clientInfo;
        NVoice::NMetrics::TSourceMetrics sourceMetrics(metrics, "my_source", clientInfo, NVoice::NMetrics::EMetricsBackend::Solomon);
        sourceMetrics.SetError("error");
    }

    CheckSensor(GetMetricsJson(metrics), "my_source.self.completed", "error", 1);
}

Y_UNIT_TEST(SourceMetricsRateHttpCode) {
    NVoice::NMetrics::TMetrics& metrics = InitMetrics();

    NVoice::NMetrics::TClientInfo clientInfo;
    NVoice::NMetrics::TSourceMetrics sourceMetrics(metrics, "my_source", clientInfo, NVoice::NMetrics::EMetricsBackend::Solomon);

    for (int code = 0; code < 1000; ++code) {
        sourceMetrics.RateHttpCode(code, "some_source");
    }

    NJson::TJsonValue metricsJson = GetMetricsJson(metrics);

    // Check all unknown [0; 100) U [600; 1000)
    CheckSensor(metricsJson, "my_source.some_source.response", "unknown", 500);

    // Check "!IsHttpCode(i)" codes
    for (int i = 100; i < 600; ++i) {
        if (!IsHttpCode(i)) {
            CheckSensor(metricsJson, "my_source.some_source.response", ToString(i), 1);
        }
    }

    // Just check some random special codes
    CheckSensor(metricsJson, "my_source.some_source.response", "200_ok", 1);
    CheckSensor(metricsJson, "my_source.some_source.response", "201_created", 1);
    CheckSensor(metricsJson, "my_source.some_source.response", "400_bad_request", 1);
    CheckSensor(metricsJson, "my_source.some_source.response", "503_service_unavailable", 1);
}

}

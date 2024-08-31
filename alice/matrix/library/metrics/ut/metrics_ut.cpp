#include <alice/matrix/library/metrics/metrics.h>

#include <infra/libs/sensors/sensor_registry.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/gtest/gtest.h>

#include <util/string/cast.h>


namespace {

using TLabels = TVector<std::pair<TStringBuf, TStringBuf>>;
using THistSensorValues = TVector<ui64>;

static constexpr TStringBuf DEFAULT_BACKEND = "self";
static constexpr TStringBuf OTHER_BACKEND = "other_backend";


TMaybe<std::reference_wrapper<const NJson::TJsonValue::TMapType>> FindSensor(
    const NJson::TJsonValue& metricsJson,
    const TString& name,
    const TLabels& labels
) {
    for (const auto& sensor : metricsJson["sensors"].GetArray()) {
        if (const auto& sensorLabels = sensor.GetMapSafe().at("labels").GetMapSafe(); labels.size() + 1 == sensorLabels.size() && sensorLabels.at("sensor") == name) {
            bool ok = true;
            for (const auto& [label, value] : labels) {
                if (const auto* actualValue = sensorLabels.FindPtr(label); !actualValue || *actualValue != value) {
                    ok = false;
                    break;
                }
            }
            if (ok) {
                return std::reference_wrapper<const NJson::TJsonValue::TMapType>(sensor.GetMapSafe());
            }
        }
    }

    return Nothing();
}

TString LabelsToString(
    const TLabels& labels
) {
    bool isFirst = true;
    TStringBuilder result;
    result << "{";
    for (const auto& [label, value] : labels) {
        result << TString::Join((isFirst ? "(" : ", ("), label, ", ", value, ')');
        isFirst = false;
    }
    result << "}";
    return TString(result);
}

TString HistSensorValuesToString(
    const THistSensorValues& values
) {
    if (values.empty()) {
        return "{}";
    }

    bool isFirst = true;
    TStringBuilder result;
    result << "{";
    for (size_t i = 0; i < values.size() - 1; ++i) {
        result << TString::Join((isFirst ? "" : ", "), ToString(values[i]));
        isFirst = false;
    }
    result << "}, inf = " << values.back();
    return TString(result);
}

MATCHER_P3(ScalarSensorEq, name, labels, expected, TString::Join("Sensor ", name, " with labels ", LabelsToString(labels), " scalar value ", (negation ? "is not " : "is "), ToString(expected))) {
    const auto sensorMaybe = FindSensor(arg, name, labels);
    if (!sensorMaybe.Defined()) {
        *result_listener << "where sensor not found";
        return false;
    }

    const auto& sensor = sensorMaybe.GetRef().get();
    auto valuePtr = sensor.FindPtr("value");
    if (!valuePtr) {
        *result_listener << "where sensor is not a scalar sensor";
        return false;
    }

    if (auto actualValue = valuePtr->GetUIntegerSafe(); actualValue != (ui64)expected) {
        *result_listener << "where actual sensor value is " << actualValue;
        return false;
    }

    return true;
}

MATCHER_P3(HistSensorEq, name, labels, expected, TString::Join("Sensor ", name, " with labels ", LabelsToString(labels), " hist values ", (negation ? "are not " : "are "), HistSensorValuesToString(expected))) {
    const auto sensorMaybe = FindSensor(arg, name, labels);
    if (!sensorMaybe.Defined()) {
        *result_listener << "where sensor not found";
        return false;
    }

    const auto& sensor = sensorMaybe.GetRef().get();
    auto histPtr = sensor.FindPtr("hist");
    if (!histPtr) {
        *result_listener << "where sensor is not a hist sensor";
        return false;
    }

    const auto& hist = histPtr->GetMapSafe();
    const auto& buckets = hist.at("buckets").GetArraySafe();
    const ui64 inf = hist.at("inf").GetUIntegerSafe();

    if (expected.size() != buckets.size() + 1) {
        *result_listener << "where number of buckets is wrong, actual buckets size is " << buckets.size();
        return false;
    }

    bool ok = true;
    for (size_t i = 0; i < buckets.size(); ++i) {
        if (auto actualValue = buckets[i].GetUIntegerSafe(); actualValue != expected[i]) {
            if (ok) {
                ok = false;
                *result_listener << "where:";
            }
            *result_listener << "\nactual_values[" << i << "] (" << actualValue << ") != expected[" << i << "] (" << expected[i] << ')';
        }
    }
    if (inf != expected.back()) {
        if (ok) {
            ok = false;
            *result_listener << "where:";
        }
        *result_listener << "\nactual_values[inf] (" << inf << ") != expected[inf] (" << expected.back() << ')';
    }

    return ok;
}

NMonitoring::IHistogramCollectorPtr GetHistogramCollector() {
    return NMonitoring::ExplicitHistogram({
          0,
          4,
          8,
          15,
    });
}

NJson::TJsonValue GetMetricsJson() {
    TStringStream outStream;
    NInfra::SensorRegistryPrint(outStream, NInfra::ESensorsSerializationType::JSON);

    NJson::TJsonValue metricsJson;
    NJson::ReadJsonTree(outStream.Str(), &metricsJson, true);

    return metricsJson;
}

TLabels GetLabelsWithCode(TStringBuf code) {
    return {
        {"code", code},
    };
}

class TMatrixMetricsTest : public ::testing::Test {
protected:
    TMatrixMetricsTest()
        : ::testing::Test()
        , SourceName_(::testing::UnitTest::GetInstance()->current_test_info()->name())
    {}

    TString GetSensorName(TStringBuf sensor, TStringBuf backend = DEFAULT_BACKEND) {
        return TString::Join(SourceName_, '.', backend, '.', sensor);
    }

protected:
    TStringBuf SourceName_;
};

} // namespace


TEST_F(TMatrixMetricsTest, TestSourceMetricInInprogressCompletedDefault) {
    for (ui32 i = 0; i < 10; ++i) {
        {
            NMatrix::TSourceMetrics metrics(SourceName_);

            auto metricsJson = GetMetricsJson();

            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("inprogress"), TLabels(), 1));
            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("in"), TLabels(), i + 1));

            if (i != 0) {
                EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("completed"), GetLabelsWithCode("ok"), i));
            }
        }

        {
            auto metricsJson = GetMetricsJson();

            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("inprogress"), TLabels(), 0));
            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("in"), TLabels(), i + 1));
            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("completed"), GetLabelsWithCode("ok"), i + 1));
        }
    }
}

TEST_F(TMatrixMetricsTest, TestSourceMetricInInprogressCompletedCustomStatus) {
    for (ui32 i = 0; i < 10; ++i) {
        {
            NMatrix::TSourceMetrics metrics(SourceName_);
            metrics.SetStatus("some_status");

            auto metricsJson = GetMetricsJson();

            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("inprogress"), TLabels(), 1));
            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("in"), TLabels(), i + 1));

            if (i != 0) {
                EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("completed"), GetLabelsWithCode("some_status"), i));
            }
        }

        {
            auto metricsJson = GetMetricsJson();

            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("inprogress"), TLabels(), 0));
            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("in"), TLabels(), i + 1));
            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("completed"), GetLabelsWithCode("some_status"), i + 1));
        }
    }
}

TEST_F(TMatrixMetricsTest, TestSourceMetricInInprogressCompletedWithEmptyErrror) {
    for (ui32 i = 0; i < 10; ++i) {
        {
            NMatrix::TSourceMetrics metrics(SourceName_);
            metrics.SetError("");

            auto metricsJson = GetMetricsJson();

            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("inprogress"), TLabels(), 1));
            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("in"), TLabels(), i + 1));

            if (i != 0) {
                EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("completed"), GetLabelsWithCode("error"), i));
            }
        }

        {
            auto metricsJson = GetMetricsJson();

            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("inprogress"), TLabels(), 0));
            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("in"), TLabels(), i + 1));
            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("completed"), GetLabelsWithCode("error"), i + 1));
        }
    }
}

TEST_F(TMatrixMetricsTest, TestSourceMetricInInprogressCompletedWithCustomErrror) {
    for (ui32 i = 0; i < 10; ++i) {
        {
            NMatrix::TSourceMetrics metrics(SourceName_);
            metrics.SetError("some_error");

            auto metricsJson = GetMetricsJson();

            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("inprogress"), TLabels(), 1));
            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("in"), TLabels(), i + 1));

            if (i != 0) {
                EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("completed"), GetLabelsWithCode("some_error"), i));
            }
        }

        {
            auto metricsJson = GetMetricsJson();

            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("inprogress"), TLabels(), 0));
            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("in"), TLabels(), i + 1));
            EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("completed"), GetLabelsWithCode("some_error"), i + 1));
        }
    }
}

TEST_F(TMatrixMetricsTest, TestSourceMetricBasicGauge) {
    NMatrix::TSourceMetrics metrics(SourceName_);

    metrics.PushAbs(2, "sensor");
    metrics.PushAbs(4, "sensor", "code_0");
    metrics.IncGauge("sensor_inc", "code_1");
    metrics.DecGauge("sensor", "code_0");

    metrics.PushAbs(5, "sensor", "", OTHER_BACKEND);
    metrics.PushAbs(7, "sensor", "code_2", OTHER_BACKEND);
    metrics.PushAbs(8, "sensor", "code_4", OTHER_BACKEND, {{"label", "value"}});
    metrics.IncGauge("sensor_inc", "code_3", OTHER_BACKEND);
    metrics.DecGauge("sensor", "code_2", OTHER_BACKEND);

    auto metricsJson = GetMetricsJson();
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("sensor"), TLabels(), 2));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("sensor"), GetLabelsWithCode("code_0"), 3));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("sensor_inc"), GetLabelsWithCode("code_1"), 1));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("sensor", OTHER_BACKEND), GetLabelsWithCode("code_2"), 6));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("sensor_inc", OTHER_BACKEND), GetLabelsWithCode("code_3"), 1));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("sensor", OTHER_BACKEND), TLabels({{"code", "code_4"}, {"label", "value"}}), 8));
}

TEST_F(TMatrixMetricsTest, TestSourceMetricBasicRate) {
    NMatrix::TSourceMetrics metrics(SourceName_);

    metrics.PushRate(2, "sensor");
    metrics.PushRate("sensor", "code_0");

    metrics.PushRate(5, "sensor", "", OTHER_BACKEND);
    metrics.PushRate("sensor", "code_1", OTHER_BACKEND);
    metrics.PushRate(7, "sensor", "code_2", OTHER_BACKEND, {{"label", "value"}});

    auto metricsJson = GetMetricsJson();
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("sensor"), TLabels(), 2));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("sensor"), GetLabelsWithCode("code_0"), 1));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("sensor", OTHER_BACKEND), GetLabelsWithCode("code_1"), 1));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("sensor", OTHER_BACKEND), TLabels(), 5));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("sensor", OTHER_BACKEND), TLabels({{"code", "code_2"}, {"label", "value"}}), 7));
}

TEST_F(TMatrixMetricsTest, TestSourceMetricBasicHistInit) {
    NMatrix::TSourceMetrics metrics(SourceName_);

    metrics.InitHist("sensor", GetHistogramCollector());
    metrics.InitHist("sensor", GetHistogramCollector(), "code_0", OTHER_BACKEND);
    metrics.InitHist("sensor", GetHistogramCollector(), "code_1", OTHER_BACKEND, {{"label", "value"}});

    auto metricsJson = GetMetricsJson();
    EXPECT_THAT(metricsJson, HistSensorEq(GetSensorName("sensor"), TLabels(), THistSensorValues({0, 0, 0, 0, 0})));
    EXPECT_THAT(metricsJson, HistSensorEq(GetSensorName("sensor", OTHER_BACKEND), GetLabelsWithCode("code_0"), THistSensorValues({0, 0, 0, 0, 0})));
    EXPECT_THAT(metricsJson, HistSensorEq(GetSensorName("sensor", OTHER_BACKEND), TLabels({{"code", "code_1"}, {"label", "value"}}), THistSensorValues({0, 0, 0, 0, 0})));
}

TEST_F(TMatrixMetricsTest, TestSourceMetricBasicHistPush) {
    NMatrix::TSourceMetrics metrics(SourceName_);

    metrics.PushHist(0, "sensor", GetHistogramCollector());
    metrics.PushHist(2, "sensor", GetHistogramCollector());
    metrics.PushHist(3, "sensor", GetHistogramCollector());
    metrics.PushHist(9, "sensor", GetHistogramCollector());
    metrics.PushHist(21, "sensor", GetHistogramCollector());
    metrics.PushHist(100, "sensor", GetHistogramCollector());

    metrics.PushHist(0, "sensor", GetHistogramCollector(), "code_0", OTHER_BACKEND);
    metrics.PushHist(8, "sensor", GetHistogramCollector(), "code_0", OTHER_BACKEND);
    metrics.PushHist(123, "sensor", GetHistogramCollector(), "code_0", OTHER_BACKEND);
    metrics.PushHist(128, "sensor", GetHistogramCollector(), "code_1", OTHER_BACKEND, {{"label", "value"}});

    auto metricsJson = GetMetricsJson();
    EXPECT_THAT(metricsJson, HistSensorEq(GetSensorName("sensor"), TLabels(), THistSensorValues({1, 2, 0, 1, 2})));
    EXPECT_THAT(metricsJson, HistSensorEq(GetSensorName("sensor", OTHER_BACKEND), GetLabelsWithCode("code_0"), THistSensorValues({1, 0, 1, 0, 1})));
    EXPECT_THAT(metricsJson, HistSensorEq(GetSensorName("sensor", OTHER_BACKEND), TLabels({{"code", "code_1"}, {"label", "value"}}), THistSensorValues({0, 0, 0, 0, 1})));
}

TEST_F(TMatrixMetricsTest, TestSourceMetricBasicDurationHist) {
    NMatrix::TSourceMetrics metrics(SourceName_);

    metrics.PushDurationHist(TDuration::MicroSeconds(0), "sensor", "", DEFAULT_BACKEND, TLabels(), GetHistogramCollector());
    metrics.PushDurationHist(TDuration::MicroSeconds(8), "sensor", "", DEFAULT_BACKEND, TLabels(), GetHistogramCollector());
    metrics.PushDurationHist(TDuration::MicroSeconds(100), "sensor", "", DEFAULT_BACKEND, TLabels(), GetHistogramCollector());

    metrics.PushDurationHist(TDuration::MicroSeconds(1), "sensor", "code_0", OTHER_BACKEND, TLabels(), GetHistogramCollector());
    metrics.PushDurationHist(TDuration::MicroSeconds(9), "sensor", "code_0", OTHER_BACKEND, TLabels(), GetHistogramCollector());
    metrics.PushDurationHist(TDuration::MicroSeconds(100), "sensor", "code_0", OTHER_BACKEND, TLabels(), GetHistogramCollector());
    metrics.PushDurationHist(TDuration::MicroSeconds(107), "sensor", "code_1", OTHER_BACKEND, {{"label", "value"}}, GetHistogramCollector());

    auto metricsJson = GetMetricsJson();
    EXPECT_THAT(metricsJson, HistSensorEq(GetSensorName("sensor"), TLabels(), THistSensorValues({1, 0, 1, 0, 1})));
    EXPECT_THAT(metricsJson, HistSensorEq(GetSensorName("sensor", OTHER_BACKEND), GetLabelsWithCode("code_0"), THistSensorValues({0, 1, 0, 1, 1})));
    EXPECT_THAT(metricsJson, HistSensorEq(GetSensorName("sensor", OTHER_BACKEND), TLabels({{"code", "code_1"}, {"label", "value"}}), THistSensorValues({0, 0, 0, 0, 1})));
}

TEST_F(TMatrixMetricsTest, TestSourceMetricHttpInit) {
    NMatrix::TSourceMetrics metrics(SourceName_);
    metrics.InitHttpCode(200);
    metrics.InitHttpCode(200, OTHER_BACKEND);
    metrics.InitHttpCode(200, OTHER_BACKEND, {{"label", "value"}});

    auto metricsJson = GetMetricsJson();
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response"), GetLabelsWithCode("200_ok"), 0));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response", OTHER_BACKEND), GetLabelsWithCode("200_ok"), 0));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response", OTHER_BACKEND), TLabels({{"code", "200_ok"}, {"label", "value"}}), 0));
}

TEST_F(TMatrixMetricsTest, TestSourceMetricHttpRate) {
    NMatrix::TSourceMetrics metrics(SourceName_);
    metrics.RateHttpCode(1, 200);
    metrics.RateHttpCode(2, 404);
    metrics.RateHttpCode(3, 200, OTHER_BACKEND);
    metrics.RateHttpCode(4, 400, OTHER_BACKEND);
    metrics.RateHttpCode(5, 200, OTHER_BACKEND, {{"label", "value"}});

    auto metricsJson = GetMetricsJson();
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response"), GetLabelsWithCode("200_ok"), 1));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response"), GetLabelsWithCode("404_not_found"), 2));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response", OTHER_BACKEND), GetLabelsWithCode("200_ok"), 3));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response", OTHER_BACKEND), GetLabelsWithCode("400_bad_request"), 4));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response", OTHER_BACKEND), TLabels({{"code", "200_ok"}, {"label", "value"}}), 5));
}

TEST_F(TMatrixMetricsTest, TestSourceMetricAppHostInit) {
    NMatrix::TSourceMetrics metrics(SourceName_);
    metrics.InitAppHostResponseOk();
    metrics.InitAppHostResponseOk(OTHER_BACKEND);
    metrics.InitAppHostResponseOk(OTHER_BACKEND, {{"label", "value"}});

    auto metricsJson = GetMetricsJson();
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response"), GetLabelsWithCode("ok"), 0));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response", OTHER_BACKEND), GetLabelsWithCode("ok"), 0));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response", OTHER_BACKEND), TLabels({{"code", "ok"}, {"label", "value"}}), 0));
}

TEST_F(TMatrixMetricsTest, TestSourceMetricAppHostRate) {
    NMatrix::TSourceMetrics metrics(SourceName_);
    metrics.RateAppHostResponseOk();
    metrics.RateAppHostResponseError();
    metrics.RateAppHostResponseOk(3, OTHER_BACKEND);
    metrics.RateAppHostResponseError(4, OTHER_BACKEND);
    metrics.RateAppHostResponseOk(5, OTHER_BACKEND, {{"label", "value"}});
    metrics.RateAppHostResponseError(6, OTHER_BACKEND, {{"label", "value"}});

    auto metricsJson = GetMetricsJson();
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response"), GetLabelsWithCode("ok"), 1));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response"), GetLabelsWithCode("error"), 1));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response", OTHER_BACKEND), GetLabelsWithCode("ok"), 3));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response", OTHER_BACKEND), GetLabelsWithCode("error"), 4));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response", OTHER_BACKEND), TLabels({{"code", "ok"}, {"label", "value"}}), 5));
    EXPECT_THAT(metricsJson, ScalarSensorEq(GetSensorName("response", OTHER_BACKEND), TLabels({{"code", "error"}, {"label", "value"}}), 6));
}

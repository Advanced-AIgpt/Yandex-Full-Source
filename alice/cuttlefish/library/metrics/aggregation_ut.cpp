#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/guid.h>

#include "aggregation.h"


#define X_GEN_MAP_TEST(name, label) \
    Y_UNIT_TEST(name) { \
        TAggregationRules f; \
        f.AddMap(label, "src1", "target1"); \
        f.AddMap(label, "src2", "target1"); \
        f.AddMap(label, "src3", "target2"); \
        f.AddMap(label, "src4", "target2"); \
        for (int i = 0; i < 1000; ++i) { \
            const TString ret = CreateGuidAsString();\
            UNIT_ASSERT_EQUAL(ret, f.MapLabel(label, ret)); \
        } \
        UNIT_ASSERT_EQUAL("target1", f.MapLabel(label, "src1")); \
        UNIT_ASSERT_EQUAL("target1", f.MapLabel(label, "src2")); \
        UNIT_ASSERT_EQUAL("target2", f.MapLabel(label, "src3")); \
        UNIT_ASSERT_EQUAL("target2", f.MapLabel(label, "src4")); \
    }


#define X_GEN_FILTER_TEST(name, label) \
    Y_UNIT_TEST(name) { \
        TAggregationRules f; \
        f.AddFilter(label, "pass1"); \
        f.AddFilter(label, "pass2"); \
        f.AddFilter(label, "pass3"); \
        for (int i = 0; i < 1000; ++i) { \
            const TString ret = CreateGuidAsString();\
            UNIT_ASSERT_EQUAL(TStringBuf(), f.FilterLabel(label, ret)); \
        } \
        UNIT_ASSERT_EQUAL("pass1", f.FilterLabel(label, "pass1")); \
        UNIT_ASSERT_EQUAL("pass2", f.FilterLabel(label, "pass2")); \
        UNIT_ASSERT_EQUAL("pass3", f.FilterLabel(label, "pass3")); \
    } \
    Y_UNIT_TEST(name ## WithDefault) { \
        TAggregationRules f; \
        f.SetDefaultLabel(label, "hello");\
        f.AddFilter(label, "pass1"); \
        f.AddFilter(label, "pass2"); \
        f.AddFilter(label, "pass3"); \
        for (int i = 0; i < 1000; ++i) { \
            const TString ret = CreateGuidAsString();\
            UNIT_ASSERT_EQUAL("hello", f.FilterLabel(label, ret)); \
        } \
        UNIT_ASSERT_EQUAL("pass1", f.FilterLabel(label, "pass1")); \
        UNIT_ASSERT_EQUAL("pass2", f.FilterLabel(label, "pass2")); \
        UNIT_ASSERT_EQUAL("pass3", f.FilterLabel(label, "pass3")); \
    }


Y_UNIT_TEST_SUITE(Aggregation) {
    using NVoice::NMetrics::TAggregationRules;
    using NVoice::NMetrics::TClientInfo;
    using NVoice::NMetrics::TLabels;
    using NVoice::NMetrics::ELabel;

    X_GEN_MAP_TEST(MapDevice, ELabel::Device)
    X_GEN_MAP_TEST(MapSurface, ELabel::Surface)
    X_GEN_MAP_TEST(MapSubgroupName, ELabel::SubgroupName)
    X_GEN_MAP_TEST(MapAppId, ELabel::Application)

    X_GEN_FILTER_TEST(FilterDevice, ELabel::Device)
    X_GEN_FILTER_TEST(FilterSurface, ELabel::Surface)
    X_GEN_FILTER_TEST(FilterSubgroupName, ELabel::SubgroupName)
    X_GEN_FILTER_TEST(FilterAppId, ELabel::Application)

    Y_UNIT_TEST(MapAndFilter) {
        TAggregationRules f;

        f.AddMap(ELabel::Device, "yandex-station", "station");
        f.AddMap(ELabel::Device, "yandex-station2", "station");
        f.AddMap(ELabel::Device, "samsung", "mobile");
        f.AddMap(ELabel::Device, "xiaomi", "mobile");
        f.AddMap(ELabel::Application, "ru.yandex.quasar.services", "quasar");
        f.AddMap(ELabel::Application, "aliced", "quasar");
        f.AddFilter(ELabel::Device, "station");
        f.AddFilter(ELabel::SubgroupName, "prod");
        f.SetDefaultLabel(ELabel::Device, "other");

        {
            TClientInfo info;
            info.DeviceName = "yandex-station";
            info.GroupName = "quasar";
            info.SubgroupName = "prod";
            info.AppId = "ru.yandex.quasar.services";

            TLabels labels = f.Process(info);

            Cout << "SURFACE: " << labels.GroupName << Endl;

            UNIT_ASSERT_EQUAL("station", labels.DeviceName);
            UNIT_ASSERT_EQUAL("quasar", labels.GroupName);
            UNIT_ASSERT_EQUAL("prod", labels.SubgroupName);
            UNIT_ASSERT_EQUAL("quasar", labels.AppId);
        }

        {
            TClientInfo info;
            info.DeviceName = "samsung";
            info.GroupName = "pp";
            info.SubgroupName = "beta";
            info.AppId = "ru.yandex.searchplugin";

            TLabels labels = f.Process(info);

            UNIT_ASSERT_EQUAL("other", labels.DeviceName);
            UNIT_ASSERT_EQUAL("pp", labels.GroupName);
            UNIT_ASSERT_EQUAL(TStringBuf(), labels.SubgroupName);
            UNIT_ASSERT_EQUAL("ru.yandex.searchplugin", labels.AppId);
        }
    }
}



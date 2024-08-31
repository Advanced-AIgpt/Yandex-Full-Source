#include "config.h"
#include "proto_fields_visitor.h"

#include <google/protobuf/util/message_differencer.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/util/pb_io.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;

Y_UNIT_TEST_SUITE(ConfigTestSuite) {
    Y_UNIT_TEST(ValidateSource) {
        {
            TConfig::TSource goodSource;
            goodSource.SetTimeoutMs(200);
            goodSource.SetMaxAttempts(3);
            goodSource.SetRetryPeriodMs(60);
            TStringStream out;
            UNIT_ASSERT(!ValidateSource("src", goodSource, out));
        }
        {
            TConfig::TSource sourceNoRetries;
            sourceNoRetries.SetTimeoutMs(200);
            TStringStream out;
            UNIT_ASSERT(!ValidateSource("src", sourceNoRetries, out));
        }
        {
            TConfig::TSource sourceNoRetryPeriod;
            sourceNoRetryPeriod.SetTimeoutMs(200);
            sourceNoRetryPeriod.SetMaxAttempts(3);
            TStringStream out;
            UNIT_ASSERT(ValidateSource("src", sourceNoRetryPeriod, out));
            UNIT_ASSERT_STRING_CONTAINS(out.Str(), "Retry period was not set");
        }
        {
            TConfig::TSource sourceLargeRetryPeriod;
            sourceLargeRetryPeriod.SetTimeoutMs(200);
            sourceLargeRetryPeriod.SetMaxAttempts(3);
            sourceLargeRetryPeriod.SetRetryPeriodMs(100);
            TStringStream out;
            UNIT_ASSERT(ValidateSource("src", sourceLargeRetryPeriod, out));
            UNIT_ASSERT_STRING_CONTAINS(out.Str(), "is too large");
        }
    }

    Y_UNIT_TEST(ValidateSources) {
        TConfig::TSource sourceNoRetryPeriod;
        sourceNoRetryPeriod.SetTimeoutMs(200);
        sourceNoRetryPeriod.SetMaxAttempts(3);

        TConfig::TScenarios::TConfig badConfig;
        *badConfig.MutableHandlersConfig()->MutableRun() = sourceNoRetryPeriod;
        {
            TConfig config;
            (*config.MutableScenarios()->MutableConfigs())["Video"] = badConfig;
            TStringStream out;
            UNIT_ASSERT(ValidateSources(config, out));
            UNIT_ASSERT_STRING_CONTAINS(out.Str(), "Retry period was not set for Video/Run retriable source");
        }
        {
            TConfig config;
            *config.MutableScenarios()->MutableDefaultConfig() = badConfig;
            TStringStream out;
            UNIT_ASSERT(ValidateSources(config, out));
            UNIT_ASSERT_STRING_CONTAINS(out.Str(), "Retry period was not set for Default/Run retriable source");
        }
    }
}

} // namespace

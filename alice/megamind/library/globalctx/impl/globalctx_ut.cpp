#include "globalctx.h"

#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NMegamind {

namespace {
constexpr TStringBuf CONFIG_JSON =
        R"(
{
  "Scenarios": {
    "DefaultConfig": {
      "HandlersConfig": {
        "Run": {
          "TimeoutMs": 1000,
          "RetryPeriodMs": 300,
          "MaxAttempts": 3
        },
        "Apply": {
          "TimeoutMs": 1500,
          "MaxAttempts": 1
        }
      },
      "DialogManagerParams": {
        "MaxActivityTurns": 1
      }
    },
    "Configs": {
      "CustomScenario": {
        "HandlersConfig": {
          "Run": {
            "TimeoutMs": 3000,
            "RetryPeriodMs": 1000,
            "MaxAttempts": 3
          }
        }
      }
    }
  }
}
)"sv;

constexpr TStringBuf ACTUAL_CONFIG_JSON =
        R"(
{
  "Scenarios": {
    "DefaultConfig": {
      "HandlersConfig": {
        "Run": {
          "TimeoutMs": 1000,
          "RetryPeriodMs": 300,
          "MaxAttempts": 3
        },
        "Apply": {
          "TimeoutMs": 1500,
          "MaxAttempts": 1
        }
      },
      "DialogManagerParams": {
        "MaxActivityTurns": 1
      }
    },
    "Configs": {
      "CustomScenario": {
        "HandlersConfig": {
          "Run": {
            "TimeoutMs": 3000,
            "RetryPeriodMs": 1000,
            "MaxAttempts": 3
          },
          "Apply": {
            "TimeoutMs": 1500,
            "MaxAttempts": 1
          }
        },
        "DialogManagerParams": {
          "MaxActivityTurns": 1
        }
      }
    }
  }
}
)"sv;
} // namespace

class TGlobalCtxTestSuite : public NUnitTest::TTestBase {
public:
    void CheckScenarioConfigDefaultValues();

    UNIT_TEST_SUITE(TGlobalCtxTestSuite);
        UNIT_TEST(CheckScenarioConfigDefaultValues);
    UNIT_TEST_SUITE_END();
};

UNIT_TEST_SUITE_REGISTRATION(TGlobalCtxTestSuite);

void TGlobalCtxTestSuite::CheckScenarioConfigDefaultValues() {
    TConfig config;
    {
        const auto &status = google::protobuf::util::JsonStringToMessage(TString{CONFIG_JSON}, &config, {});
        UNIT_ASSERT_C(status.ok(), status.ToString());
    }

    TConfig expectedConfig;
    {
        const auto &status = google::protobuf::util::JsonStringToMessage(TString{ACTUAL_CONFIG_JSON}, &expectedConfig,
                                                                         {});
        UNIT_ASSERT_C(status.ok(), status.ToString());
    }

    const auto &actualConfig = TGlobalCtx::MakeConfig(config);
    UNIT_ASSERT_MESSAGES_EQUAL(expectedConfig, actualConfig);
}

} // namespace NAlice::NMegamind

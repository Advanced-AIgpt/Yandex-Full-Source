#include "proto_eval.h"

#include <alice/library/json/json.h>

#include <alice/megamind/protos/common/device_state.pb.h>
#include <alice/megamind/protos/scenarios/notification_state.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <google/protobuf/text_format.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>

using namespace NAlice;

namespace {
    TString FullPath(TStringBuf file) {
        return SRC_(TString::Join("ut/", file, ".pb.txt"));
    }

    NScenarios::TScenarioRunRequest LoadRunRequest() {
        NScenarios::TScenarioRunRequest req;
        UNIT_ASSERT_C(NProtoBuf::TextFormat::ParseFromString(TFileInput{FullPath("run_request")}.ReadAll(), &req),
                      "run_request.pb.txt parse failed");
        return req;
    }

    TProtoEvalExpression MakeExpression(const TString& src) {
        TProtoEvalExpression ex;
        UNIT_ASSERT_C(NProtoBuf::TextFormat::ParseFromString(src, &ex), "TProtoEvalExpression parse failed");
        return ex;
    }
}

Y_UNIT_TEST_SUITE(ProtoEval) {
    Y_UNIT_TEST(Query) {
        auto req = LoadRunRequest();

        TProtoEvaluator evaluator;
        evaluator.SetProtoRef("Req", req);
        evaluator.SetProtoRef("Input", req.GetInput());

        size_t errCount = 0;
        evaluator.SetErrorCallback([&](const TString&) { ++errCount; });

        errCount = 0;
        UNIT_ASSERT_EQUAL(evaluator.EvaluateString(MakeExpression("Expression: \"Input.SemanticFrames[]\"")), "");
        UNIT_ASSERT_EQUAL(errCount, 1);

        errCount = 0;
        UNIT_ASSERT_EQUAL(evaluator.EvaluateString(MakeExpression("Expression: \"Input.SemanticFrames[1].Name\"")), "");
        UNIT_ASSERT_EQUAL(errCount, 1);

        errCount = 0;
        UNIT_ASSERT_EQUAL(evaluator.EvaluateString(MakeExpression("Expression: \"Req.Input[0]\"")), "");
        UNIT_ASSERT_EQUAL(errCount, 1);

        errCount = 0;
        UNIT_ASSERT_EQUAL(evaluator.EvaluateString(MakeExpression("Expression: \"NoSuchProto.Name\"")), "");
        UNIT_ASSERT_EQUAL(errCount, 1);

        errCount = 0;
        UNIT_ASSERT_EQUAL(evaluator.EvaluateString(MakeExpression("Expression: \"Req.Input.SemanticFrames[0].Slots[0].Value.Extra\"")), "");
        UNIT_ASSERT_EQUAL(errCount, 1);

        evaluator.SetErrorCallback([](const TString& msg) { Cout << msg << "\n"; });
        UNIT_ASSERT_EQUAL(evaluator.EvaluateString(MakeExpression("Expression: \"Req.Input.SemanticFrames[0].Slots[0].Value\"")), "москва");
        UNIT_ASSERT_EQUAL(evaluator.EvaluateString(MakeExpression("Expression: \"Input.SemanticFrames[0].Name\"")), "personal_assistant.scenarios.get_news");
        UNIT_ASSERT_EQUAL(evaluator.EvaluateString(MakeExpression("Expression: \"Req.BaseRequest.UserPreferences.FiltrationMode\"")), "Moderate");

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Input.Voice.AsrData"
    Aggregate {
        Reducer: Sum
        Value {
            Parameters {
                Path: "AsrData.Words"
                Aggregate {
                    Reducer: Count
                }
            }
        }
    }
}
)___")),
                          3);

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Input.Voice.AsrData"
    Aggregate {
        Reducer: Sum
        Value {
            Expression: "AsrData.Words.size()"
        }
    }
}
)___")),
                          3);

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Input.Voice.AsrData"
    Aggregate {
        Reducer: First
        Value {
            Parameters {
                Path: "AsrData.Words"
                Aggregate {
                    Reducer: Sum
                    Name: "Word"
                    Value {
                        Expression: "Word.Confidence * WordIndex"
                    }
                }
            }
        }
    }
}
)___")),
                          38);

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Input.Voice.AsrData[0].Words"
    Aggregate {
        Reducer: Avg
        Value {
            Expression: "Words.Confidence"
        }
    }
}
)___")),
                          12);

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Input.Voice.AsrData[1-1].Words"
    Aggregate {
        Reducer: First
        Filter {
            Expression: "Words.Value == \"москва\""
        }
        Value {
            Expression: "WordsIndex"
        }
    }
}
)___")),
                          2);

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Input.Voice.AsrData[0].Words"
    Aggregate {
        Reducer: First
        Filter {
            Expression: "Words.Value =~ \".[о]ск.*\""
        }
        Value {
            Expression: "WordsIndex"
        }
    }
}
)___")),
                          2);

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Req.BaseRequest.Options.RadioStations"
    Aggregate {
        Reducer: Sum
    }
}
)___")),
                          10);

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Req.BaseRequest.Options.RadioStations"
    Aggregate {
        Name: "Station"
        Reducer: Sum
        Value {
            Expression: "2*Station"
        }
    }
}
)___")),
                          20);

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Req.BaseRequest.Options.RadioStations"
    Aggregate {
        Reducer: Sum
        Filter {
            # exclude the first element
            Expression: "RadioStationsIndex >= 1"
        }
    }
}
)___")),
                          8);

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Req.BaseRequest.Options.RadioStations"
    Aggregate {
        Reducer: Max
    }
}
)___")),
                          4);

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Req.BaseRequest.Options.RadioStations"
    Aggregate {
        Reducer: Min
    }
}
)___")),
                          1);

        UNIT_ASSERT_EQUAL(evaluator.EvaluateString(MakeExpression(R"___(
Parameters {
    Path: "Req.DataSources[1].DialogHistory.Phrases"
    Aggregate {
        Reducer: Min
    }
}
)___")),
                          "%%%");

        UNIT_ASSERT_EQUAL(evaluator.EvaluateString(MakeExpression(R"___(
Parameters {
    Path: "Req.DataSources[1].DialogHistory.Phrases"
    Aggregate {
        Reducer: Max
    }
}
)___")),
                          "99");

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Req.DataSources[1].DialogHistory.Phrases"
    Aggregate {
        Reducer: Min
        Filter {
            Expression: "Phrases != \"%%%\""
        }
    }
}
)___")),
                          99);

        TString trace;
        evaluator.SetTraceCallback(TProtoEvaluator::TraceToString(trace));
        evaluator.SetTraceEnabled();

        UNIT_ASSERT_EQUAL(evaluator.Evaluate<int>(MakeExpression(R"___(
Parameters {
    Path: "Req.DataSources[1].DialogHistory.Phrases"
    Aggregate {
        Reducer: Max
        Filter {
            Expression: "Phrases != \"%%%\""
        }
        Value {
            Expression: "Phrases"
            TraceEnabled { value: false }
        }
    }
}
)___")),
                          234);

        Cout << trace;
        UNIT_ASSERT_EQUAL(trace,
R"___(    "Phrases" = "123"
  "Phrases != \"%%%\"" = "1"
    "Phrases" = ""
  "Phrases != \"%%%\"" = "1"
    "Phrases" = "234"
  "Phrases != \"%%%\"" = "1"
    "Phrases" = "%%%"
  "Phrases != \"%%%\"" = "0"
    "Phrases" = "99"
  "Phrases != \"%%%\"" = "1"
"Phrases=Max(Req.DataSources[1].DialogHistory.Phrases)" = "234"
"Phrases" = "234"
)___");
        trace.clear();

        UNIT_ASSERT_EQUAL(evaluator.EvaluateBool(MakeExpression(R"___(
Expression: "Req.DataSources[2].DialogHistory.Phrases.empty()"
)___")),
                          true);

        UNIT_ASSERT_EQUAL(evaluator.EvaluateBool(MakeExpression(R"___(
Expression: "Req.BaseRequest.DeviceState.Music.Player.empty() && Req.BaseRequest.DeviceState.Music.empty() && Req.BaseRequest.DeviceState.size() == 1"
)___")),
                          true);

        evaluator.SetTraceEnabled(false);
        UNIT_ASSERT_EQUAL(evaluator.EvaluateBool(MakeExpression(R"___(
Expression: "Req.BaseRequest.NoSuchField.DeviceState.Music.Player.empty() == \"\" && Req.BaseRequest.NoSuchField.DeviceState.Music.Player.size() == \"\""
TraceEnabled { value: true }
)___")),
                          true);

        Cout << trace;
        UNIT_ASSERT_EQUAL(trace,
R"___(Warning: "2": Index value is not found in map
  "Req.DataSources[2].DialogHistory.Phrases.empty()" = "1"
"Req.DataSources[2].DialogHistory.Phrases.empty()" = "1"
  "Req.BaseRequest.DeviceState.size()" = "1"
  "Req.BaseRequest.DeviceState.Music.empty()" = "1"
  "Req.BaseRequest.DeviceState.Music.Player.empty()" = "1"
"Req.BaseRequest.DeviceState.Music.Player.empty() && Req.BaseRequest.DeviceState.Music.empty() && Req.BaseRequest.DeviceState.size() == 1" = "1"
Error: "NoSuchField": Field is not found
  "Req.BaseRequest.NoSuchField.DeviceState.Music.Player.size()" = ""
Error: "NoSuchField": Field is not found
  "Req.BaseRequest.NoSuchField.DeviceState.Music.Player.empty()" = ""
"Req.BaseRequest.NoSuchField.DeviceState.Music.Player.empty() == \"\" && Req.BaseRequest.NoSuchField.DeviceState.Music.Player.size() == \"\"" = "1"
)___");
        evaluator.SetTraceCallback({});

        UNIT_ASSERT_EQUAL(evaluator.EvaluateBool(MakeExpression("Expression: \"Req.BaseRequest.DeviceState.Music.Player.Pause != \\\"0\\\"\"")), true);
        UNIT_ASSERT_EQUAL(evaluator.EvaluateBool(MakeExpression("Expression: \"Req.BaseRequest.DeviceState.Music.Player.Pause == \\\"\\\"\"")), true);
        UNIT_ASSERT_EQUAL(evaluator.EvaluateBool(MakeExpression("Expression: \"Req.BaseRequest.DeviceState.Video.CurrentlyPlaying.Paused == \\\"1\\\"\"")), true);
        UNIT_ASSERT_EQUAL(evaluator.EvaluateBool(MakeExpression("Expression: \"Req.BaseRequest.DeviceState.Bluetooth.Player.Pause == \\\"0\\\"\"")), true);
    }

    Y_UNIT_TEST(Subscription) {
        constexpr TStringBuf notificationStateJson = "{\"count_archived\":0,\"notifications\":[],\"subscriptions\":[],\"device_subscriptions\":[{\"device_id\":\"04007884c9140c0909cf\",\"subscribed\":false},{\"device_id\":\"24007894c12020170450\",\"subscribed\":false},{\"device_id\":\"280b4000011e3400000f3834524e5050\",\"subscribed\":false},{\"device_id\":\"280b400001332f00000f3834524e5050\",\"subscribed\":false},{\"device_id\":\"74005034440c0819054e\",\"subscribed\":false},{\"device_id\":\"94307894c11c1c290450\",\"subscribed\":false},{\"device_id\":\"FF98F029494B27529D7FD106\",\"subscribed\":true},{\"device_id\":\"FF98F02953FF0CF7A0C4CA1C\",\"subscribed\":false},{\"device_id\":\"FF98F02996739FB2945E88EF\",\"subscribed\":false},{\"device_id\":\"FFB8F004865E6D2340170F05\",\"subscribed\":false},{\"device_id\":\"FFB8F004CB9088956A55103D\",\"subscribed\":false}]}";
        TNotificationState notificationState = JsonToProto<TNotificationState>(JsonFromString(notificationStateJson));
        TDeviceState deviceState;

        TProtoEvaluator evaluator;
        evaluator.SetProtoRef("NotificationState", notificationState);
        evaluator.SetProtoRef("DeviceState", deviceState);

        auto expr = MakeExpression(R"___(
Expression: "Subscribed == \"\" || Subscribed"
Parameters {
    Name: "Subscribed"
    Path: "NotificationState.DeviceSubscriptions"
    Aggregate {
        Reducer: First
        Filter {
            Expression: "DeviceSubscriptions.DeviceId == DeviceState.DeviceId"
        }
        Value {
            Expression: "DeviceSubscriptions.Subscribed"
        }
    }
}
)___");

        deviceState.SetDeviceId("94307894c11c1c290450");
        UNIT_ASSERT_EQUAL(evaluator.EvaluateBool(expr), false);

        deviceState.SetDeviceId("FF98F029494B27529D7FD106");
        UNIT_ASSERT_EQUAL(evaluator.EvaluateBool(expr), true);

        deviceState.SetDeviceId("FF98F029494B2752DEADBEEF");
        UNIT_ASSERT_EQUAL(evaluator.EvaluateBool(expr), true);
    }
}

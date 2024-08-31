#include "talkien.h"

#include <alice/megamind/library/testing/fake_guid_generator.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_responses.h>
#include <alice/megamind/library/testing/mock_session.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/protos/common/environment_state.pb.h>

#include <alice/library/frame/builder.h>
#include <alice/library/json/json.h>
#include <alice/library/metrics/names.h>
#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

using namespace ::testing;
using NAlice::NScenarios::TFrameAction;
using NAlice::NScenarios::TDirective;

namespace NAlice {
namespace {

struct TFixture : public NUnitTest::TBaseFixture {
    TString FakeGuid{"fake-guid"};
    NMegamind::TFakeGuidGenerator GuidGenerator{FakeGuid};
    StrictMock<TMockSession> Session;
    StrictMock<TMockContext> Ctx;
};

NJson::TJsonValue AsJsonResponse(const TDirectiveListResponse& response) {
    auto jsonResponse = JsonFromProto(response.GetProto());
    if (jsonResponse["header"]["dialog_id"] == "") {
        jsonResponse["header"]["dialog_id"].SetType(NJson::JSON_NULL);
    }
    jsonResponse["response"]["meta"].SetType(NJson::JSON_ARRAY);
    jsonResponse["response"]["experiments"].SetType(NJson::JSON_MAP);
    return jsonResponse;
}

THolder<IResponses> MakeResponsesWithRecognizedActionFrames(const TVector<TSemanticFrame>& frames) {
    auto begemotResponse = NBg::NProto::TAlicePolyglotMergeResponseResult();
    auto& aliceParsedFrames = *begemotResponse.MutableAliceResponse()->MutableAliceParsedFrames();
    for (const auto& frame : frames) {
        *aliceParsedFrames.AddFrames() = frame;
        aliceParsedFrames.AddSources("AliceActionRecognizer");
        aliceParsedFrames.AddConfidences(1.0);
    }
    auto responses = MakeHolder<TMockResponses>();
    responses->SetWizardResponse(TWizardResponse(std::move(begemotResponse)));
    return responses;
}

THolder<IResponses> MakeResponsesWithRecognizedActionFrame(const TSemanticFrame& frame) {
    return MakeResponsesWithRecognizedActionFrames({frame});
}

TTestSpeechKitRequest MakeSpeechKitRequest(
    const TString& requestId,
    const TString dialogId,
    const TString& session,
    const TMaybe<TDeviceState>& deviceState = Nothing(),
    const TMaybe<TEnvironmentState>& environmentState = Nothing()
) {
    TSpeechKitRequestProto proto;
    proto.MutableHeader()->SetRequestId(requestId);
    proto.MutableHeader()->SetPrevReqId(requestId + "-prev");
    proto.MutableHeader()->SetDialogId(dialogId);
    proto.SetSession(session);
    if (deviceState.Defined()) {
        *proto.MutableRequest()->MutableDeviceState() = *deviceState;
    }
    if (environmentState.Defined()) {
        *proto.MutableRequest()->MutableEnvironmentState() = *environmentState;
    }
    return TSpeechKitRequestBuilder(JsonFromProto(proto)).Build();
}

TFrameAction BuildScenarioAction(const TString& frameName, const TString& directiveName) {
    TDirective directive;
    directive.MutableCallbackDirective()->SetName(directiveName);

    TFrameAction action;
    *action.MutableDirectives()->AddList() = directive;
    action.MutableNluHint()->SetFrameName(frameName);
    return action;
}

TFrameAction BuildScenarioAction(const TString& nluHintFrameName, const TMaybe<TSemanticFrame>& effectFrame) {
    TFrameAction action;
    action.MutableNluHint()->SetFrameName(nluHintFrameName);
    if (effectFrame.Defined()) {
        *action.MutableFrame() = *effectFrame;
    }
    return action;
}

TDeviceStateAction BuildDeviceAction(const TString& directiveName) {
    TUntypedDirective directive;
    directive.SetName(directiveName);

    TDeviceStateAction action;
    *action.AddDirectives() = directive;
    return action;
}

template <typename K, typename V>
TVector<std::pair<K, V>> AsSortedPairs(const THashMap<K, V>& map) {
    TVector<std::pair<K, V>> result;
    for (const auto& [k, v] : map) {
        result.push_back({k, v});
    }
    Sort(result);
    return result;
}

struct TActiveActionFrameTestData final {
    TString FrameName;
    TSemanticFrame SemanticFrame;
    TSemanticFrameRequestData SemanticFrameRequestData;
    bool ReturnFromBegemot;
};

Y_UNIT_TEST_SUITE_F(TestTalkien, TFixture) {

    Y_UNIT_TEST(SkipScenarioAction) {
        const TString firstFrameName = "frame1";
        const TString secondFrameName = "frame2";
        const TSemanticFrame firstFrame = MakeFrame(firstFrameName);

        const TString directiveName = "directive";
        ::google::protobuf::Map <TString, TFrameAction> actions;
        actions["some_action"] = BuildScenarioAction(secondFrameName, directiveName);
        const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName("some scenario")
            .SetScenarioSession("some scenario", NewScenarioSession(TState{}))
            .SetActions(actions)
            .Build();

        const auto responses = MakeResponsesWithRecognizedActionFrame(firstFrame);
        EXPECT_CALL(Ctx, Responses())
            .WillOnce(ReturnRef(*responses));
        EXPECT_CALL(Ctx, Session())
            .WillOnce(Return(session.Get()));

        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
        const auto actionResponse = TryGetScenarioActionResponse(Ctx, /* iotUserInfo= */ Nothing(), analyticsInfoBuilder, GuidGenerator);
        UNIT_ASSERT(!actionResponse.Defined());
    }

    Y_UNIT_TEST(MakeScenarioAction) {
        const TString frameName = "frame1";
        const TSemanticFrame frame = MakeFrame(frameName);

        const TString directiveName = "directive-name";
        ::google::protobuf::Map <TString, TFrameAction> actions;
        actions["some_action"] = BuildScenarioAction(frameName, directiveName);
        const auto session = MakeSessionBuilder()
            ->SetPreviousScenarioName("some scenario")
            .SetScenarioSession("some scenario", NewScenarioSession(TState{}))
            .SetActions(actions)
            .SetPreviousProductScenarioName("some product scenario name")
            .Build();

        const auto responses = MakeResponsesWithRecognizedActionFrame(frame);
        const TString requestId = "request-x";
        const TString dialogId = "dialog-y";
        const TString serializedSession = "some-session";
        const auto skr = MakeSpeechKitRequest(requestId, dialogId, serializedSession);
        EXPECT_CALL(Ctx, Responses())
            .WillOnce(ReturnRef(*responses));
        EXPECT_CALL(Ctx, Session())
            .WillRepeatedly(Return(session.Get()));
        EXPECT_CALL(Ctx, SpeechKitRequest())
            .WillRepeatedly(Return(skr));
        EXPECT_CALL(Ctx, Logger())
            .WillOnce(ReturnRef(TRTLogger::NullLogger()));

        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
        const auto actionResponse = TryGetScenarioActionResponse(Ctx, /* iotUserInfo= */ Nothing(), analyticsInfoBuilder, GuidGenerator);
        UNIT_ASSERT(actionResponse.Defined());
        const auto* directiveListResponse = std::get_if<TDirectiveListResponse>(actionResponse.Get());
        UNIT_ASSERT(directiveListResponse);
        UNIT_ASSERT_VALUES_EQUAL(JsonFromString(TStringBuilder{} << R"({
            "header": {
                "dialog_id": ")" << dialogId << R"(",
                "request_id": ")" << requestId << R"(",
                "response_id": ")" << FakeGuid << R"("
            },
            "response": {
                "cards": [
                    {
                        "type": "simple_text"
                    }
                ],
                "directives": [
                    {
                        "type": "server_action",
                        "name": ")" << directiveName << R"(",
                        "payload": {
                            "@scenario_name": "some scenario",
                            "@request_id": ")" << requestId << R"("
                        }
                    }
                ],
                "meta": [],
                "experiments": {}
            },
            "sessions": {
                ")" << dialogId << R"(": ")" << serializedSession << R"("
            },
            "voice_response": {
                "should_listen": false
            }
        })"), AsJsonResponse(*directiveListResponse));

        TStringBuf analytics = R"({
            "recognized_action": {
                "parent_request_id": "request-x-prev",
                "action_id": "some_action",
                "parent_product_scenario_name":"some product scenario name"
            }
        })";
        UNIT_ASSERT_VALUES_EQUAL(JsonFromString(analytics), analyticsInfoBuilder.BuildJson());
    }

    Y_UNIT_TEST(MakeDeviceAction) {
        const TString frameName = "frame1";
        const TSemanticFrame frame = MakeFrame(frameName);

        const auto responses = MakeResponsesWithRecognizedActionFrame(frame);
        const TString requestId = "request-x";
        const TString dialogId = "dialog-y";
        const TString serializedSession = "some-session";
        const TString directiveName = "directive-name";
        TDeviceState deviceState;
        (*deviceState.MutableActions())[frameName] = BuildDeviceAction(directiveName);
        const auto skr = MakeSpeechKitRequest(requestId, dialogId, serializedSession, deviceState);
        EXPECT_CALL(Ctx, Responses())
            .WillOnce(ReturnRef(*responses));
        EXPECT_CALL(Ctx, SpeechKitRequest())
            .WillRepeatedly(Return(skr));

        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
        const auto actionResponse = TryGetDeviceActionResponse(Ctx, analyticsInfoBuilder, GuidGenerator);
        UNIT_ASSERT(actionResponse.Defined());
        const auto* directiveListResponse = std::get_if<TDirectiveListResponse>(actionResponse.Get());
        UNIT_ASSERT(directiveListResponse);
        UNIT_ASSERT_VALUES_EQUAL(JsonFromString(TStringBuilder{} << R"({
            "header": {
                "dialog_id": ")" << dialogId << R"(",
                "request_id": ")" << requestId << R"(",
                "response_id": ")" << FakeGuid << R"("
            },
            "response": {
                "cards": [
                    {
                        "type": "simple_text"
                    }
                ],
                "directives": [
                    {
                        "name": ")" << directiveName << R"("
                    }
                ],
                "meta": [],
                "experiments": {}
            },
            "sessions": {
                ")" << dialogId << R"(": ")" << serializedSession << R"("
            },
            "voice_response": {
                "should_listen": false
            }
        })"), AsJsonResponse(*directiveListResponse));

        TStringBuf analytics = R"({
            "recognized_action": {
                "origin": "DeviceState",
                "action_id": "frame1"
            }
        })";
        UNIT_ASSERT_VALUES_EQUAL(JsonFromString(analytics), analyticsInfoBuilder.BuildJson());
    }

    Y_UNIT_TEST(AddDynamicAcceptedFrames) {
        const TString activeScenario = "scenario_3";
        IScenarioRegistry::TFramesToScenarios framesToScenarios{
            {"frame_1", {"scenario_1", "scenario_2"}},
            {"frame_2", {activeScenario}},
            {"frame_3", {"scenario_4"}}
        };
        IScenarioRegistry::TFramesToScenarios expectedFramesToScenarios = framesToScenarios;
        expectedFramesToScenarios["frame_1"].push_back(activeScenario);
        expectedFramesToScenarios["frame_5"].push_back(activeScenario);
        expectedFramesToScenarios["frame_6"].push_back(activeScenario);
        expectedFramesToScenarios["frame_7"].push_back(activeScenario);
        expectedFramesToScenarios["frame_8"].push_back(activeScenario);
        expectedFramesToScenarios["frame_9"].push_back(activeScenario);

        ::google::protobuf::Map<TString, NScenarios::TFrameAction> sessionActions;
        sessionActions[
            "on_unknown_dynamic_hint_frame__"
            "when_effect_is_directive__"
            "extends_accepted_frames_with_hint_frame"
        ] = BuildScenarioAction("frame_7", "some_directive");
        sessionActions[
            "on_unknown_dynamic_hint_frame__"
            "when_no_effect__"
            "extends_accepted_frames_with_hint_frame"
        ] = BuildScenarioAction("frame_5", Nothing());
        sessionActions[
            "on_unknown_dynamic_hint_frame__"
            "when_effect_is_unknown_dynamic_frame__"
            "extends_accepted_frames_with_hint_frame"
        ] = BuildScenarioAction("frame_6", MakeFrame("frame_4"));
        sessionActions[
            "on_unknown_dynamic_hint_frame__"
            "when_effect_is_known_static_frame__"
            "extends_accepted_frames_with_hint_frame"
        ] = BuildScenarioAction("frame_8", MakeFrame("frame_2"));
        sessionActions[
            "on_unknown_dynamic_hint_frame__"
            "when_effect_is_known_dynamic_frame__"
            "extends_accepted_frames_with_hint_frame"
        ] = BuildScenarioAction("frame_9", MakeFrame("frame_1"));
        sessionActions[
            "on_known_dynamic_frame__"
            "when_effect_is_directive__"
            "extends_accepted_frames_with_hint_frame"
        ] = BuildScenarioAction("frame_1", "some_directive");

        auto frameBuilder = TSemanticFrameBuilder("frame_with_requested_slot");
        frameBuilder.AddSlot(
                "not_requested",
                /* acceptedTypes= */ {"string"},
                /* type= */ Nothing(),
                /* value= */ Nothing(),
                /* isRequested= */ false
            );

        EXPECT_CALL(Session, GetPreviousScenarioName())
            .Times(4)
            .WillRepeatedly(ReturnRef(activeScenario));
        EXPECT_CALL(Session, GetActions())
            .Times(2)
            .WillRepeatedly(Return(sessionActions));
        {
            EXPECT_CALL(Session, GetResponseFrame())
                .WillOnce(Return(frameBuilder.Build()));
            UNIT_ASSERT_VALUES_EQUAL(AsSortedPairs(expectedFramesToScenarios),
                                     AsSortedPairs(AddDynamicAcceptedFrames(framesToScenarios, &Session)));        }
        {
            frameBuilder.AddSlot(
                "requested",
                /* acceptedTypes= */ {"string"},
                /* type= */ Nothing(),
                /* value= */ Nothing(),
                /* isRequested= */ true
            );
            expectedFramesToScenarios["frame_with_requested_slot"].push_back(activeScenario);

            EXPECT_CALL(Session, GetResponseFrame())
                .WillOnce(Return(frameBuilder.Build()));
            UNIT_ASSERT_VALUES_EQUAL(expectedFramesToScenarios, AddDynamicAcceptedFrames(framesToScenarios, &Session));
        }
    }

    Y_UNIT_TEST(TryGetActionResponseWithActiveSpaceActions) {
        TVector<TActiveActionFrameTestData> activeActionsTestData;
        for (int i = 0; i < 3; ++i) {
            auto& entry = activeActionsTestData.emplace_back();
            entry.SemanticFrame = MakeFrame(TStringBuilder{} << "frame_" << i);
            entry.SemanticFrameRequestData.MutableTypedSemanticFrame()
                ->MutableSearchSemanticFrame()
                ->MutableQuery()
                ->SetStringValue(TStringBuilder{} << "query_" << i);
            entry.ReturnFromBegemot = true;
        }
        activeActionsTestData.back().ReturnFromBegemot = false;
        const auto responses = MakeResponsesWithRecognizedActionFrames([&activeActionsTestData] {
            TVector<TSemanticFrame> frames;
            for (const auto& entry : activeActionsTestData) {
                if (!entry.ReturnFromBegemot) {
                    continue;
                }
                frames.push_back(entry.SemanticFrame);
            }
            return frames;
        }());
        const TString reqId = "req_id";
        const auto skr = MakeSpeechKitRequest(
            reqId, /* dialogId= */ {}, /* session= */ {}, /* deviceState= */ [&activeActionsTestData] {
                TDeviceState deviceState{};
                for (const auto& entry : activeActionsTestData) {
                    deviceState.MutableActiveActions()->MutableSemanticFrames()->insert(
                        {entry.SemanticFrame.GetName(), entry.SemanticFrameRequestData});
                }
                return deviceState;
            }());
        const auto session = MakeSessionBuilder()
                                 ->SetPreviousScenarioName("some scenario")
                                 .SetScenarioSession("some scenario", NewScenarioSession(TState{}))
                                 .SetPreviousProductScenarioName("some product scenario name")
                                 .Build();
        EXPECT_CALL(Ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));
        EXPECT_CALL(Ctx, Responses()).WillRepeatedly(ReturnRef(*responses));
        EXPECT_CALL(Ctx, Session()).WillRepeatedly(Return(session.Get()));
        NMegamind::TMegamindAnalyticsInfoBuilder analyticsInfoBuilder;
        const auto actionResponse =
            TryGetActionResponse(Ctx, /* iotUserInfo= */ Nothing(), analyticsInfoBuilder, GuidGenerator);
        UNIT_ASSERT(actionResponse.Defined());
        const auto* actionFrames = std::get_if<TVector<TSemanticFrameRequestData>>(actionResponse.Get());
        UNIT_ASSERT(actionFrames && actionFrames->size() + 1 == activeActionsTestData.size());
        for (const auto& actionFrame : *actionFrames) {
            const auto* frame = FindIfPtr(activeActionsTestData, [&actionFrame](const TActiveActionFrameTestData& entry) {
                return TMessageDiff{actionFrame, entry.SemanticFrameRequestData}.AreEqual && entry.ReturnFromBegemot;
            });
            UNIT_ASSERT_C(frame, "Unable to find frame in recognized actions: " << actionFrame);
        }
        constexpr TStringBuf analytics = R"({
            "recognized_actions": [
                {
                    "matched_frame": "frame_0",
                    "origin": "ActiveSpaceAction",
                    "analytics": {}
                },
                {
                    "matched_frame": "frame_1",
                    "origin": "ActiveSpaceAction",
                    "analytics": {}
                }
            ]
        })";
        UNIT_ASSERT_VALUES_EQUAL(JsonFromString(analytics), analyticsInfoBuilder.BuildJson());
    }

    Y_UNIT_TEST(TestTryGetConditionalEffect) {
        const auto testPsn = "test_psn";
        const auto testPurpose = "test_purpose";
        /* testGetFromDeviceState */ {
            const auto skr = MakeSpeechKitRequest(
                "req_id", /* dialogId= */ {}, /* session= */ {}, /* deviceState= */ [&] {
                    TConditionalAction action;
                    action.MutableConditionalSemanticFrame()->MutablePlayerPauseSemanticFrame();
                    action.MutableEffectFrameRequestData()->MutableAnalytics()->SetProductScenario(testPsn);
                    action.MutableEffectFrameRequestData()->MutableAnalytics()->SetPurpose(testPurpose);
                    TDeviceState deviceState{};
                    (*(*deviceState.MutableActiveActions()->MutableScreenConditionalActions())["main"].AddConditionalActions()) = action;
                    return deviceState;
                }());

            EXPECT_CALL(Ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));
            TMockSensors sensors;
            EXPECT_CALL(sensors, IncRate(NSignal::LabelsForActivatedConditionalAction(testPsn, testPurpose)));
            EXPECT_CALL(Ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));
            const TSemanticFrame firstFrame = MakeFrame("personal_assistant.scenarios.player.pause");
            const auto responses = MakeResponsesWithRecognizedActionFrame(firstFrame);
            EXPECT_CALL(Ctx, Responses()).WillOnce(ReturnRef(*responses));
            NMegamind::TMegamindAnalyticsInfoBuilder analytics;
            UNIT_ASSERT(TryGetConditionalEffect(Ctx, analytics));
        }
        /* testGetFromTandemDeviceState */ {
            const auto skr = MakeSpeechKitRequest(
                "req_id", /* dialogId= */ {}, /* session= */ {}, /* deviceState= */ {}, /* environmentState= */ [&] {
                    TConditionalAction action;
                    action.MutableConditionalSemanticFrame()->MutablePlayerPauseSemanticFrame();
                    action.MutableEffectFrameRequestData()->MutableAnalytics()->SetProductScenario(testPsn);
                    action.MutableEffectFrameRequestData()->MutableAnalytics()->SetPurpose(testPurpose);
                    TEnvironmentState environmentState{};
                    auto& deviceState = *environmentState.MutableDevices()->Add()->MutableSpeakerDeviceState();
                    (*(*deviceState.MutableActiveActions()->MutableScreenConditionalActions())["main"].AddConditionalActions()) = action;
                    return environmentState;
                }());

            EXPECT_CALL(Ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));
            TMockSensors sensors;
            EXPECT_CALL(sensors, IncRate(NSignal::LabelsForActivatedConditionalAction(testPsn, testPurpose)));
            EXPECT_CALL(Ctx, Sensors()).WillRepeatedly(ReturnRef(sensors));
            const TSemanticFrame firstFrame = MakeFrame("personal_assistant.scenarios.player.pause");
            const auto responses = MakeResponsesWithRecognizedActionFrame(firstFrame);
            EXPECT_CALL(Ctx, Responses()).WillOnce(ReturnRef(*responses));
            NMegamind::TMegamindAnalyticsInfoBuilder analytics;
            UNIT_ASSERT(TryGetConditionalEffect(Ctx, analytics));
        }
        /* testEmpty */ {
            const auto skr = MakeSpeechKitRequest(
                "req_id", /* dialogId= */ {}, /* session= */ {}, /* deviceState= */ [] {
                    TConditionalAction action;
                    action.MutableConditionalSemanticFrame()->MutablePlayerPauseSemanticFrame();
                    TDeviceState deviceState{};
                    (*(*deviceState.MutableActiveActions()->MutableScreenConditionalActions())["main"].AddConditionalActions()) = action;
                    return deviceState;
                }());

            EXPECT_CALL(Ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));
            const TSemanticFrame firstFrame = MakeFrame("fake_frame");
            const auto responses = MakeResponsesWithRecognizedActionFrame(firstFrame);
            EXPECT_CALL(Ctx, Responses()).WillOnce(ReturnRef(*responses));
            NMegamind::TMegamindAnalyticsInfoBuilder analytics;
            UNIT_ASSERT(!TryGetConditionalEffect(Ctx, analytics));
        }
    }

    Y_UNIT_TEST(TestProcessConditionalAction) {
        /* testEmptyTypedFrame */ {
            TTypedSemanticFrame conditionalFrame;
            TVector<TTypedSemanticFrame> requestFrames;
            UNIT_ASSERT(!ProcessConditionalAction(conditionalFrame, requestFrames));
        }
        /* testSimpleMatch */ {
            TTypedSemanticFrame conditionalFrame;
            conditionalFrame.MutableVideoPlaySemanticFrame()->MutableSeason()->SetNumValue(3);
            conditionalFrame.MutableVideoPlaySemanticFrame()->MutableContentType()->SetVideoContentTypeValue("content_type");

            TVector<TTypedSemanticFrame> requestFrames;
            TTypedSemanticFrame frame1;
            frame1.MutablePlayerPauseSemanticFrame();
            requestFrames.push_back(frame1);
            TTypedSemanticFrame frame2;
            frame2.MutableVideoPlaySemanticFrame()->MutableSeason()->SetNumValue(3);
            frame2.MutableVideoPlaySemanticFrame()->MutableContentType()->SetVideoContentTypeValue("content_type");
            frame2.MutableVideoPlaySemanticFrame()->MutableEpisode()->SetNumValue(345);
            requestFrames.push_back(frame2);

            UNIT_ASSERT(ProcessConditionalAction(conditionalFrame, requestFrames));
        }

        /* testSimpleNoMatch */ {
            TTypedSemanticFrame conditionalFrame;
            conditionalFrame.MutableVideoPlaySemanticFrame()->MutableSeason()->SetNumValue(3);
            conditionalFrame.MutableVideoPlaySemanticFrame()->MutableContentType()->SetVideoContentTypeValue("content_type");

            TVector<TTypedSemanticFrame> requestFrames;
            TTypedSemanticFrame frame1;
            frame1.MutablePlayerPauseSemanticFrame();
            requestFrames.push_back(frame1);
            TTypedSemanticFrame frame2;
            frame2.MutableMusicPlaySemanticFrame();
            requestFrames.push_back(frame2);

            UNIT_ASSERT(!ProcessConditionalAction(conditionalFrame, requestFrames));
        }

        /* testNoMatchDifferentValues */ {
            TTypedSemanticFrame conditionalFrame;
            conditionalFrame.MutableVideoPlaySemanticFrame()->MutableSeason()->SetNumValue(3);
            conditionalFrame.MutableVideoPlaySemanticFrame()->MutableContentType()->SetVideoContentTypeValue("content_type");

            TVector<TTypedSemanticFrame> requestFrames;
            TTypedSemanticFrame frame1;
            frame1.MutablePlayerPauseSemanticFrame();
            requestFrames.push_back(frame1);
            TTypedSemanticFrame frame2;
            frame2.MutableVideoPlaySemanticFrame()->MutableSeason()->SetNumValue(3);
            frame2.MutableVideoPlaySemanticFrame()->MutableContentType()->SetVideoContentTypeValue("contentik_type");
            frame2.MutableVideoPlaySemanticFrame()->MutableEpisode()->SetNumValue(345);
            TTypedSemanticFrame frame3;
            frame3.MutableMusicPlaySemanticFrame();
            requestFrames.push_back(frame3);

            UNIT_ASSERT(!ProcessConditionalAction(conditionalFrame, requestFrames));
        }

        /* testNoMatchEmptyField */ {
            TTypedSemanticFrame conditionalFrame;
            conditionalFrame.MutableVideoPlaySemanticFrame()->MutableSeason()->SetNumValue(3);
            conditionalFrame.MutableVideoPlaySemanticFrame()->MutableContentType()->SetVideoContentTypeValue("content_type");

            TVector<TTypedSemanticFrame> requestFrames;
            TTypedSemanticFrame frame1;
            frame1.MutablePlayerPauseSemanticFrame();
            requestFrames.push_back(frame1);
            TTypedSemanticFrame frame2;
            frame2.MutableVideoPlaySemanticFrame()->MutableContentType()->SetVideoContentTypeValue("content_type");
            TTypedSemanticFrame frame3;
            frame3.MutableMusicPlaySemanticFrame();
            requestFrames.push_back(frame3);

            UNIT_ASSERT(!ProcessConditionalAction(conditionalFrame, requestFrames));
        }

        /* testNoMatchAnotherStringSlot */ {
            TTypedSemanticFrame conditionalFrame;
            conditionalFrame.MutableVideoPlaySemanticFrame()->MutableSeason()->SetNumValue(3);
            conditionalFrame.MutableVideoPlaySemanticFrame()->MutableContentType()->SetVideoContentTypeValue("content_type");

            TVector<TTypedSemanticFrame> requestFrames;
            TTypedSemanticFrame frame1;
            frame1.MutablePlayerPauseSemanticFrame();
            requestFrames.push_back(frame1);
            TTypedSemanticFrame frame2;
            frame2.MutableVideoPlaySemanticFrame()->MutableSeason()->SetNumValue(3);
            frame2.MutableVideoPlaySemanticFrame()->MutableContentType()->SetStringValue("content_type");
            TTypedSemanticFrame frame3;
            frame3.MutableMusicPlaySemanticFrame();
            requestFrames.push_back(frame3);

            UNIT_ASSERT(!ProcessConditionalAction(conditionalFrame, requestFrames));
        }
    }
}

} // namespace
} // namespace NAlice

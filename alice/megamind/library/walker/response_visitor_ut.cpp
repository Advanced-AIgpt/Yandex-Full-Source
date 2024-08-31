#include "response_visitor.h"
#include "walker.h"

#include <alice/megamind/library/scenarios/features/protos/features.pb.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_scenario_wrapper.h>
#include <alice/megamind/library/testing/request_fixture.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/speechkit_api_builder.h>

#include <alice/megamind/protos/common/frame.pb.h>

#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice {

using namespace testing;

const TString VINS_GC_PROACTIVITY_RESPONSE = R"({
  "header": {
    "prev_req_id": "232a6a7f-6eeb-4b5d-a39d-c506df2c58c4",
    "dialog_id": null,
    "response_id": "c2a996b0eabf4ca7a975f59f1285382e",
    "sequence_number": 52,
    "request_id": "938ade7b-e222-4a48-8b16-8f1d20df7164"
  },
  "voice_response": {
    "should_listen": true,
    "output_speech": {
      "text": "Хотите послушать музыку?",
      "type": "simple"
    },
    "directives": []
  },
  "response": {
    "features": {
      "form_info": {
        "is_continuing": false,
        "intent": "personal_assistant.general_conversation.general_conversation"
      }
    },
    "suggest": {
      "items": []
    },
    "experiments": {
      "gc_proactivity": "1",
      "gc_force_proactivity": "1"
    },
    "meta": [
      {
        "source": "proactivity",
        "type": "gc_source"
      },
      {
        "overriden_form": "personal_assistant.general_conversation.general_conversation",
        "type": "form_restored"
      }
    ],
    "directives": [],
    "megamind_actions": {
      "some_action": {
        "nlu_hint": {
          "frame_name": "alice.general_conversation.proactivity_agree"
        },
        "frame": {
          "name": "personal_assistant.scenarios.music_play"
        }
      }
    },
    "cards": [
      {
        "text": "Хотите послушать музыку?",
        "tag": null,
        "type": "simple_text"
      }
    ],
    "card": {
      "text": "Хотите послушать музыку?",
      "tag": null,
      "type": "simple_text"
    }
  },
  "sessions": {
  }
})";

constexpr TStringBuf SPEECHKIT_REQUEST = TStringBuf(R"(
{
    "header":{
        "prev_req_id":"d34df00d-f92a-4fae-9c7e-a5630d2212f1",
        "request_id":"d34df00d-c135-4227-8cf8-386d7d989237",
        "sequence_number":670
    },
    "request":{
        "voice_session":false,
        "event":{
            "name":"",
            "type":"text_input",
            "text":"давай поиграем в города"
        },
        "reset_session":false,
    },
    "session":null,
}
)");

constexpr TStringBuf SPEECHKIT_REQUEST_WITH_WHISPER = TStringBuf(R"(
{
    "header":{
        "prev_req_id":"d34df00d-f92a-4fae-9c7e-a5630d2212f1",
        "request_id":"d34df00d-c135-4227-8cf8-386d7d989237",
        "sequence_number":670
    },
    "request":{
        "voice_session":false,
        "event": {
            "type": "voice_input",
            "asr_whisper": true,
            "asr_result":[
                {
                    "endOfPhrase":true,
                    "normalized":"продолжи песню.",
                    "confidence":1,
                    "words":[
                        {
                            "value":"продолжи",
                            "confidence":1
                        },
                        {
                            "value":"песню",
                            "confidence":1
                        }
                    ],
                    "utterance":"продолжи песню."
                }
            ],
        },
        "reset_session":false,
        "additional_options": {
            "server_time_ms": "12346"
        }
    },
    "session":null,
}
)");

constexpr TStringBuf SPEECHKIT_REQUEST_VOICE = TStringBuf(R"(
{
    "header":{
        "prev_req_id":"d34df00d-f92a-4fae-9c7e-a5630d2212f1",
        "request_id":"d34df00d-c135-4227-8cf8-386d7d989237",
        "sequence_number":670
    },
    "request":{
        "voice_session":false,
        "event": {
            "type": "voice_input",
            "asr_whisper": false,
            "asr_result":[
                {
                    "endOfPhrase":true,
                    "normalized":"продолжи песню.",
                    "confidence":1,
                    "words":[
                        {
                            "value":"продолжи",
                            "confidence":1
                        },
                        {
                            "value":"песню",
                            "confidence":1
                        }
                    ],
                    "utterance":"продолжи песню."
                }
            ],
        },
        "reset_session":false,
        "additional_options": {
            "server_time_ms": "12346"
        }
    },
    "session":null,
}
)");

Y_UNIT_TEST_SUITE(ResponseVisitor) {
    Y_UNIT_TEST_F(VinsResponseWithMegamindActions, NMegamind::TPredefinedRequestFixture) {
        auto mockRequestBuilder = MockRequestBuilder(/* utterance= */ "");
        auto& context = mockRequestBuilder.Context();
        const auto speechKitRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        EXPECT_CALL(context, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(context, SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));

        const auto wrapper = MakeIntrusive<StrictMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*wrapper, ShouldBecomeActiveScenario()).WillRepeatedly(Return(false));
        TState state;
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;
        EXPECT_CALL(*wrapper, GetApplyEnv(_, _))
            .WillRepeatedly(Return(TLightScenarioEnv(context, request, /* semanticFrames= */ {}, state, analyticsInfoBuilder, userInfoBuilder)));
        NMegamind::TModifiersStorage modifiersStorage;
        EXPECT_CALL(*wrapper, GetModifiersStorage()).WillRepeatedly(ReturnRef(modifiersStorage));

        TScenarioInfraConfig scenarioConfig;
        EXPECT_CALL(context, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));

        const TString scenarioName = "scenario.420";
        const TFeatures features;

    }

    Y_UNIT_TEST_F(TestWhisperFromRequest, NMegamind::TPredefinedRequestFixture) {
        const auto speechKitRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST_WITH_WHISPER}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        auto mockRequestBuilder = MockRequestBuilder(/* utterance= */ "");

        TScenarioInfraConfig scenarioConfig;

        auto& ctx = mockRequestBuilder.Context();
        const TString scenarioName = "test_scenario_name";
        google::protobuf::Value value;
        TState state;
        value.set_string_value("old value");
        state.MutableState()->PackFrom(value);
        auto scenarioSession = NewScenarioSession(state);
        const auto& scenarioTimestamp = TInstant::Now().MicroSeconds();
        scenarioSession.SetTimestamp(scenarioTimestamp);

        const auto& session = MakeSessionBuilder()
                                  ->SetPreviousScenarioName(scenarioName)
                                  .SetScenarioSession(scenarioName, scenarioSession)
                                  .Build();

        EXPECT_CALL(ctx, Session()).WillRepeatedly(Return(session.Get()));
        EXPECT_CALL(ctx, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
        TClientInfoProto clientInfoProto;
        TClientInfo clientInfo(clientInfoProto);
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        auto& walkerCtx = mockRequestBuilder.LightWalkerCtx();
        const TFeatures features;
        const auto wrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*wrapper, ShouldBecomeActiveScenario()).WillRepeatedly(Return(false));
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;
        EXPECT_CALL(*wrapper, GetApplyEnv(_, _))
            .WillRepeatedly(Return(TLightScenarioEnv{ctx, request, /* semanticFrames= */ {}, state, analyticsInfoBuilder, userInfoBuilder}));
        TSemanticFrame frame;
        TVector<TSemanticFrame> semanticFrames{frame};
        EXPECT_CALL(*wrapper, GetSemanticFrames()).WillRepeatedly(ReturnRef(semanticFrames));
        NMegamind::TModifiersStorage modifiersStorage;
        EXPECT_CALL(*wrapper, GetModifiersStorage()).WillRepeatedly(ReturnRef(modifiersStorage));
        TResponseFinalizer finalizerVisitor(wrapper, walkerCtx, request, scenarioName, features,
                                            /* requestIsExpected= */ false);

        TResponseBuilderProto proto;
        TResponseBuilder response(speechKitRequest, request, scenarioName, proto);

        const auto status = finalizerVisitor.Finalize(&response);
        UNIT_ASSERT(!status.Defined());
        const auto& serializedSession = proto.GetResponse().GetSessions().at("");
        const auto& newSession = DeserializeSession(serializedSession);

        UNIT_ASSERT_EQUAL(newSession->GetLastWhisperTimeMs(), 12346);
    }

    Y_UNIT_TEST_F(TestWhisperFromOldSession, NMegamind::TPredefinedRequestFixture) {
        const ui64 LAST_WHISPER_TIME = 12347;
        const auto speechKitRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                                           /* iotUserInfo= */ Nothing(),
                                           /* requestSource= */ {},
                                           /* semanticFrames= */ {},
                                           /* recognizedActionEffectFrames= */ {},
                                           /* stackEngineCore= */ {},
                                           /* parameters= */ {},
                                           /* contactsList= */ Nothing(),
                                           /* origin= */ Nothing(), LAST_WHISPER_TIME);
        auto mockRequestBuilder = MockRequestBuilder(/* utterance= */ "");

        TScenarioInfraConfig scenarioConfig;

        auto& ctx = mockRequestBuilder.Context();
        const TString scenarioName = "test_scenario_name";
        google::protobuf::Value value;
        TState state;
        value.set_string_value("old value");
        state.MutableState()->PackFrom(value);
        auto scenarioSession = NewScenarioSession(state);
        const auto& scenarioTimestamp = TInstant::Now().MicroSeconds();
        scenarioSession.SetTimestamp(scenarioTimestamp);

        const auto& session = MakeSessionBuilder()
                                  ->SetPreviousScenarioName(scenarioName)
                                  .SetScenarioSession(scenarioName, scenarioSession)
                                  .Build();

        EXPECT_CALL(ctx, Session()).WillRepeatedly(Return(session.Get()));
        EXPECT_CALL(ctx, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
        TClientInfoProto clientInfoProto;
        TClientInfo clientInfo(clientInfoProto);
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        auto& walkerCtx = mockRequestBuilder.LightWalkerCtx();
        const TFeatures features;
        const auto wrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*wrapper, ShouldBecomeActiveScenario()).WillRepeatedly(Return(false));
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;
        EXPECT_CALL(*wrapper, GetApplyEnv(_, _))
            .WillRepeatedly(Return(TLightScenarioEnv{ctx, request, /* semanticFrames= */ {}, state, analyticsInfoBuilder, userInfoBuilder}));
        TSemanticFrame frame;
        TVector<TSemanticFrame> semanticFrames{frame};
        EXPECT_CALL(*wrapper, GetSemanticFrames()).WillRepeatedly(ReturnRef(semanticFrames));
        NMegamind::TModifiersStorage modifiersStorage;
        EXPECT_CALL(*wrapper, GetModifiersStorage()).WillRepeatedly(ReturnRef(modifiersStorage));
        TResponseFinalizer finalizerVisitor(wrapper, walkerCtx, request, scenarioName, features,
                                            /* requestIsExpected= */ false);

        TResponseBuilderProto proto;
        TResponseBuilder response(speechKitRequest, request, scenarioName, proto);

        const auto status = finalizerVisitor.Finalize(&response);
        UNIT_ASSERT(!status.Defined());
        const auto& serializedSession = proto.GetResponse().GetSessions().at("");
        const auto& newSession = DeserializeSession(serializedSession);

        UNIT_ASSERT_EQUAL(newSession->GetLastWhisperTimeMs(), LAST_WHISPER_TIME);
    }

    Y_UNIT_TEST_F(TestWhisperErasedWithNonWhisperVoiceInput, NMegamind::TPredefinedRequestFixture) {
        const ui64 LAST_WHISPER_TIME = 12347;
        const auto speechKitRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST_VOICE}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        auto mockRequestBuilder = MockRequestBuilder(/* utterance= */ "");

        TScenarioInfraConfig scenarioConfig;

        auto& ctx = mockRequestBuilder.Context();
        const TString scenarioName = "test_scenario_name";
        google::protobuf::Value value;
        TState state;
        value.set_string_value("old value");
        state.MutableState()->PackFrom(value);
        auto scenarioSession = NewScenarioSession(state);
        const auto& scenarioTimestamp = TInstant::Now().MicroSeconds();
        scenarioSession.SetTimestamp(scenarioTimestamp);

        const auto& session = MakeSessionBuilder()
                                  ->SetPreviousScenarioName(scenarioName)
                                  .SetScenarioSession(scenarioName, scenarioSession)
                                  .SetLastWhisperTimeMs(LAST_WHISPER_TIME)
                                  .Build();

        EXPECT_CALL(ctx, Session()).WillRepeatedly(Return(session.Get()));
        EXPECT_CALL(ctx, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(ctx, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
        TClientInfoProto clientInfoProto;
        TClientInfo clientInfo(clientInfoProto);
        EXPECT_CALL(ctx, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        auto& walkerCtx = mockRequestBuilder.LightWalkerCtx();
        const TFeatures features;
        const auto wrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*wrapper, ShouldBecomeActiveScenario()).WillRepeatedly(Return(false));
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;
        EXPECT_CALL(*wrapper, GetApplyEnv(_, _))
            .WillRepeatedly(Return(TLightScenarioEnv{ctx, request, /* semanticFrames= */ {}, state, analyticsInfoBuilder, userInfoBuilder}));
        TSemanticFrame frame;
        TVector<TSemanticFrame> semanticFrames{frame};
        EXPECT_CALL(*wrapper, GetSemanticFrames()).WillRepeatedly(ReturnRef(semanticFrames));
        NMegamind::TModifiersStorage modifiersStorage;
        EXPECT_CALL(*wrapper, GetModifiersStorage()).WillRepeatedly(ReturnRef(modifiersStorage));
        TResponseFinalizer finalizerVisitor(wrapper, walkerCtx, request, scenarioName, features,
                                            /* requestIsExpected= */ false);

        TResponseBuilderProto proto;
        TResponseBuilder response(speechKitRequest, request, scenarioName, proto);

        const auto status = finalizerVisitor.Finalize(&response);
        UNIT_ASSERT(!status.Defined());
        const auto& serializedSession = proto.GetResponse().GetSessions().at("");
        const auto& newSession = DeserializeSession(serializedSession);

        UNIT_ASSERT_EQUAL(newSession->GetLastWhisperTimeMs(), 0);
    }

    Y_UNIT_TEST_F(TestMultipleSessions, NMegamind::TPredefinedRequestFixture) {
        const TString scenarioName = "TestScenario";
        const TString prevScenarioName = "PrevTestScenario";
        const TString recentlyDisabledScenarioName = "RecentlyDisabledTestScenario";
        const TString disabledScenarioName = "DisabledTestScenario";
        auto speechKitRequest =
            TSpeechKitRequestBuilder{TSpeechKitApiRequestBuilder(SPEECHKIT_REQUEST).UpdateServerTime().BuildJson()}
                .Build();

        auto mockRequestBuilder = MockRequestBuilder(/* utterance= */ "");
        auto& walkerCtx = mockRequestBuilder.LightWalkerCtx();
        EXPECT_CALL(walkerCtx, RequestCtx()).WillRepeatedly(ReturnRef(mockRequestBuilder.RequestCtx()));
        EXPECT_CALL(mockRequestBuilder.Context(), Logger()).WillRepeatedly(ReturnRef(TRTLogger::StderrLogger()));
        EXPECT_CALL(mockRequestBuilder.Context(), SpeechKitRequest()).WillRepeatedly(Return(speechKitRequest));
        TScenarioConfigRegistry registry{};
        constexpr auto makeScenarioConfig = [](const auto name, const auto enabled) {
            TScenarioConfig config{};
            config.SetName(name);
            config.SetEnabled(enabled);
            return config;
        };
        registry.AddScenarioConfig(makeScenarioConfig(scenarioName, /* enabled= */ true));
        registry.AddScenarioConfig(makeScenarioConfig(prevScenarioName, /* enabled= */ true));
        registry.AddScenarioConfig(makeScenarioConfig(disabledScenarioName, /* enabled= */ false));
        registry.AddScenarioConfig(makeScenarioConfig(recentlyDisabledScenarioName, /* enabled= */ false));
        EXPECT_CALL(mockRequestBuilder.GlobalCtx(), ScenarioConfigRegistry()).WillRepeatedly(ReturnRef(registry));
        TConfig config{};
        config.SetDisabledScenarioSessionTimeoutSeconds(1000);
        EXPECT_CALL(mockRequestBuilder.GlobalCtx(), Config()).WillRepeatedly(ReturnRef(config));
        auto& context = mockRequestBuilder.Context();

        google::protobuf::Value value;
        TState oldState;
        value.set_string_value("old value");
        oldState.MutableState()->PackFrom(value);

        TState anotherState;
        value.set_number_value(42);
        oldState.MutableState()->PackFrom(value);

        TScenarioInfraConfig scenarioConfig;

        auto prevScenarioSession = NewScenarioSession(anotherState);
        prevScenarioSession.SetActivityTurn(2);
        prevScenarioSession.SetConsequentIrrelevantResponseCount(1);
        prevScenarioSession.SetConsequentUntypedSlotRequests(3);
        const auto& prevTimestamp = TInstant::Now().MicroSeconds();
        prevScenarioSession.SetTimestamp(prevTimestamp);

        auto oldScenarioSession = NewScenarioSession(oldState);
        const auto& scenarioTimestamp = TInstant::Now().MicroSeconds();
        oldScenarioSession.SetTimestamp(scenarioTimestamp);

        auto disabledScenarioSession = NewScenarioSession(oldState);
        auto recentlyDisabledScenarioSession = NewScenarioSession(oldState);
        recentlyDisabledScenarioSession.SetTimestamp(TInstant::Now().MicroSeconds());

        const auto& session = MakeSessionBuilder()
                                  ->SetPreviousScenarioName(prevScenarioName)
                                  .SetScenarioSession(prevScenarioName, prevScenarioSession)
                                  .SetScenarioSession(scenarioName, oldScenarioSession)
                                  .SetScenarioSession(disabledScenarioName, disabledScenarioSession)
                                  .SetScenarioSession(recentlyDisabledScenarioName, recentlyDisabledScenarioSession)
                                  .Build();

        EXPECT_CALL(context, Session()).WillRepeatedly(Return(session.Get()));
        EXPECT_CALL(context, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(context, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
        TClientInfoProto clientInfoProto;
        TClientInfo clientInfo(clientInfoProto);
        EXPECT_CALL(context, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);

        const auto wrapper = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*wrapper, ShouldBecomeActiveScenario()).WillRepeatedly(Return(false));

        TState newState;
        value.set_string_value("new value");
        newState.MutableState()->PackFrom(value);


        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;
        EXPECT_CALL(*wrapper, GetApplyEnv(_, _))
            .WillRepeatedly(Return(TLightScenarioEnv(context, request, /* semanticFrames= */ {}, newState, analyticsInfoBuilder, userInfoBuilder)));
        NMegamind::TModifiersStorage modifiersStorage;
        EXPECT_CALL(*wrapper, GetModifiersStorage()).WillRepeatedly(ReturnRef(modifiersStorage));

        const TFeatures features;

        {
            TResponseFinalizer finalizerVisitor(wrapper, walkerCtx, request, scenarioName,
                features, /* requestIsExpected= */ false);

            TResponseBuilderProto proto;
            TResponseBuilder response(speechKitRequest, request, scenarioName, proto);
            const TString psn = "previous product scenario name";
            response.SetProductScenarioName(psn);

            NScenarios::TLayout layout;
            layout.SetOutputSpeech("Hello!");
            google::protobuf::Map<TString, NScenarios::TFrameAction> actions;
            actions["action"].MutableFrame()->SetName("semantic_frame");

            response.SetLayout(std::make_unique<NScenarios::TLayout>(layout));
            response.PutActions(actions);

            finalizerVisitor.Finalize(&response);

            const auto& serializedSession = proto.GetResponse().GetSessions().at("");
            const auto& newSession = DeserializeSession(serializedSession);
            UNIT_ASSERT_MESSAGES_EQUAL(newSession->GetScenarioSession(scenarioName).GetState(), newState);
            UNIT_ASSERT_MESSAGES_EQUAL(newSession->GetScenarioSession(prevScenarioName).GetState(), anotherState);
            UNIT_ASSERT_EQUAL(newSession->GetScenarioSession(prevScenarioName).GetActivityTurn(), 0);
            UNIT_ASSERT_EQUAL(newSession->GetScenarioSession(prevScenarioName).GetConsequentIrrelevantResponseCount(), 0);
            UNIT_ASSERT_EQUAL(newSession->GetScenarioSession(prevScenarioName).GetConsequentUntypedSlotRequests(), 0);
            UNIT_ASSERT_EQUAL(newSession->GetScenarioSession(prevScenarioName).GetTimestamp(), prevTimestamp);
            UNIT_ASSERT(newSession->GetScenarioSession(scenarioName).GetTimestamp() > scenarioTimestamp);
            UNIT_ASSERT(!newSession->GetRequestIsExpected());
            UNIT_ASSERT(!newSession->GetScenarioSessions().count(disabledScenarioName));
            UNIT_ASSERT(newSession->GetScenarioSessions().count(recentlyDisabledScenarioName));
            UNIT_ASSERT(newSession->GetLayout().Defined());
            UNIT_ASSERT_MESSAGES_EQUAL(*newSession->GetLayout(), layout);
            UNIT_ASSERT_EQUAL(newSession->GetActions().size(), 1);
            UNIT_ASSERT_STRINGS_EQUAL((*newSession->GetActions().begin()).second.GetFrame().GetName(),
                              (*actions.begin()).second.GetFrame().GetName());
            UNIT_ASSERT_STRINGS_EQUAL(newSession->GetPreviousProductScenarioName(), psn);
        }

        {
            EXPECT_CALL(context, HasExpFlag(EXP_DISABLE_MULTIPLE_SESSIONS)).WillRepeatedly(Return(true));
            TResponseFinalizer finalizerVisitor(wrapper, walkerCtx, request, scenarioName,
                features, /* requestIsExpected= */ false);

            TResponseBuilderProto proto;
            TResponseBuilder response(speechKitRequest, request, scenarioName, proto);
            finalizerVisitor.Finalize(&response);

            const auto& serializedSession = proto.GetResponse().GetSessions().at("");
            const auto& newSession = DeserializeSession(serializedSession);
            UNIT_ASSERT_MESSAGES_EQUAL(newSession->GetScenarioSession(scenarioName).GetState(), newState);
            UNIT_ASSERT_MESSAGES_EQUAL(newSession->GetScenarioSession(prevScenarioName), TSessionProto::TScenarioSession{});
            UNIT_ASSERT(newSession->GetScenarioSession(scenarioName).GetTimestamp() > scenarioTimestamp);
            UNIT_ASSERT(!newSession->GetRequestIsExpected());
        }

        {
            EXPECT_CALL(context, HasExpFlag(EXP_DISABLE_MULTIPLE_SESSIONS)).WillRepeatedly(Return(true));
            TResponseFinalizer finalizerVisitor(wrapper, walkerCtx, request, scenarioName,
                features, /* requestIsExpected= */ true);

            TResponseBuilderProto proto;
            TResponseBuilder response(speechKitRequest, request, scenarioName, proto);
            finalizerVisitor.Finalize(&response);

            const auto& serializedSession = proto.GetResponse().GetSessions().at("");
            const auto& newSession = DeserializeSession(serializedSession);
            UNIT_ASSERT_MESSAGES_EQUAL(newSession->GetScenarioSession(scenarioName).GetState(), newState);
            UNIT_ASSERT_MESSAGES_EQUAL(newSession->GetScenarioSession(prevScenarioName), TSessionProto::TScenarioSession{});
            UNIT_ASSERT(newSession->GetScenarioSession(scenarioName).GetTimestamp() > scenarioTimestamp);
            UNIT_ASSERT(newSession->GetRequestIsExpected());
        }
    }

    Y_UNIT_TEST_F(PureSession, NMegamind::TPredefinedRequestFixture) {
        const TString scenarioName = "TestScenario";
        const TString prevScenarioName = "PrevTestScenario";

        auto mockRequestBuilder = MockRequestBuilder(/* utterance= */ "");
        auto& walkerCtx = mockRequestBuilder.LightWalkerCtx();
        auto& context = mockRequestBuilder.Context();

        google::protobuf::Value value;
        TState oldState;
        value.set_string_value("old value");
        oldState.MutableState()->PackFrom(value);

        TState anotherState;
        value.set_number_value(42);
        oldState.MutableState()->PackFrom(value);

        auto prevScenarioSession = NewScenarioSession(anotherState);
        prevScenarioSession.SetActivityTurn(2);
        prevScenarioSession.SetConsequentIrrelevantResponseCount(1);
        prevScenarioSession.SetConsequentUntypedSlotRequests(3);
        const auto& prevTimestamp = TInstant::Now().MicroSeconds();
        prevScenarioSession.SetTimestamp(prevTimestamp);

        auto oldScenarioSession = NewScenarioSession(oldState);
        const auto& scenarioTimestamp = TInstant::Now().MicroSeconds();
        oldScenarioSession.SetTimestamp(scenarioTimestamp);

        const auto& session = MakeSessionBuilder()
            ->SetPreviousScenarioName(prevScenarioName)
            .SetScenarioSession(prevScenarioName, prevScenarioSession)
            .SetScenarioSession(scenarioName, oldScenarioSession)
            .SetRequestIsExpected(true)
            .Build();

        TScenarioInfraConfig scenarioConfig;
        scenarioConfig.SetPureSession(true);

        EXPECT_CALL(context, Session()).WillRepeatedly(Return(session.Get()));
        EXPECT_CALL(context, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(context, ScenarioConfig(scenarioName)).WillRepeatedly(ReturnRef(scenarioConfig));

        auto speechKitRequest = TSpeechKitRequestBuilder{SPEECHKIT_REQUEST}.Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);

        const auto wrapper = MakeIntrusive<StrictMock<TMockScenarioWrapper>>();
        EXPECT_CALL(*wrapper, ShouldBecomeActiveScenario()).WillRepeatedly(Return(false));

        TState newState;
        value.set_string_value("new value");
        newState.MutableState()->PackFrom(value);

        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;
        EXPECT_CALL(*wrapper, GetApplyEnv(_, _))
            .WillRepeatedly(Return(TLightScenarioEnv{context, request, /* semanticFrames= */ {}, newState, analyticsInfoBuilder, userInfoBuilder}));
        NMegamind::TModifiersStorage modifiersStorage;
        EXPECT_CALL(*wrapper, GetModifiersStorage()).WillRepeatedly(ReturnRef(modifiersStorage));

        const TFeatures features;

        {
            TResponseFinalizer finalizerVisitor{wrapper, walkerCtx, request, scenarioName,
                                               features, /* requestIsExpected= */ false};

            TResponseBuilderProto proto;
            TResponseBuilder response(speechKitRequest, request, scenarioName, proto);
            finalizerVisitor.Finalize(&response);

            const auto& serializedSession = proto.GetResponse().GetSessions().at("");
            const auto& newSession = DeserializeSession(serializedSession);
            UNIT_ASSERT_MESSAGES_EQUAL(newSession->GetScenarioSession(scenarioName), oldScenarioSession);
            UNIT_ASSERT_MESSAGES_EQUAL(newSession->GetScenarioSession(prevScenarioName), prevScenarioSession);
            UNIT_ASSERT(newSession->GetRequestIsExpected());
        }
    }

    Y_UNIT_TEST_F(StackEngine, NMegamind::TPredefinedRequestFixture) {
        const auto core = [](){
            NMegamind::TStackEngineCore core{};
            core.AddItems()->SetScenarioName("A");
            core.AddItems()->SetScenarioName("B");
            return core;
        }();

        auto mockRequestBuilder = MockRequestBuilder(/* utterance= */ {});
        auto& walkerCtx = mockRequestBuilder.LightWalkerCtx();
        auto& context = mockRequestBuilder.Context();
        const auto speechKitRequest =
            TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        const auto request =
            NMegamind::CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                                     /* iotUserInfo= */ Nothing(),
                                     /* requestSource= */ {},
                                     /* semanticFrames= */ {}, /* recognizedActionEffectFrames= */ {}, core);
        TScenarioInfraConfig scenarioConfig;
        EXPECT_CALL(context, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(context, StackEngineCore()).WillRepeatedly(ReturnRef(core));
        EXPECT_CALL(context, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
        TClientInfoProto clientInfoProto;
        TClientInfo clientInfo(clientInfoProto);
        EXPECT_CALL(context, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        const auto scenarioWrapperPtr = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        TState state;
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;
        EXPECT_CALL(*scenarioWrapperPtr, GetApplyEnv(_, _))
            .WillRepeatedly(Return(
                TLightScenarioEnv(context, request, /* semanticFrames= */ {}, state, analyticsInfoBuilder, userInfoBuilder)));
        NMegamind::TModifiersStorage modifiersStorage;
        EXPECT_CALL(*scenarioWrapperPtr, GetModifiersStorage()).WillRepeatedly(ReturnRef(modifiersStorage));

        const TFeatures features{};

        const TString prevScenarioName = "PreviousScenario";
        TResponseFinalizer finalizerVisitor(scenarioWrapperPtr, walkerCtx, request, prevScenarioName, features,
                                           /* requestIsExpected= */ false);

        TResponseBuilderProto proto;
        TResponseBuilder response(speechKitRequest, request, prevScenarioName, proto);
        finalizerVisitor.Finalize(&response);

        const auto& newSession = DeserializeSession(proto.GetResponse().GetSessions().at(""));
        UNIT_ASSERT_MESSAGES_EQUAL(newSession->GetStackEngineCore(), core);
    }

    Y_UNIT_TEST_F(ImmutableStackEngineOnWarmUpWithSemanticFrame, NMegamind::TPredefinedRequestFixture) {
        const auto updatedCore = []() {
            NMegamind::TStackEngineCore core{};
            core.AddItems()->SetScenarioName("A");
            core.AddItems()->SetScenarioName("B");
            return core;
        }();
        const auto originalCore = []() {
            NMegamind::TStackEngineCore core{};
            core.AddItems()->SetScenarioName("origA");
            core.AddItems()->SetScenarioName("origB");
            return core;
        }();
        auto mockRequestBuilder = MockRequestBuilder(/* utterance= */ {});
        auto& walkerCtx = mockRequestBuilder.LightWalkerCtx();
        auto& context = mockRequestBuilder.Context();
        const auto speechKitRequest =
            TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        const auto request =
            NMegamind::CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest,
                                     /* iotUserInfo= */ Nothing(),
                                     /* requestSource= */ {},
                                     /* semanticFrames= */ {}, /* recognizedActionEffectFrames= */ {}, updatedCore);
        TScenarioInfraConfig scenarioConfig;
        EXPECT_CALL(context, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(context, StackEngineCore()).WillRepeatedly(ReturnRef(originalCore));
        EXPECT_CALL(context, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
        TClientInfoProto clientInfoProto;
        TClientInfo clientInfo(clientInfoProto);
        EXPECT_CALL(context, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        const auto scenarioWrapperPtr = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        TState state;
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;
        EXPECT_CALL(*scenarioWrapperPtr, GetApplyEnv(_, _))
            .WillRepeatedly(Return(
                TLightScenarioEnv(context, request, /* semanticFrames= */ {}, state, analyticsInfoBuilder, userInfoBuilder)));
        NMegamind::TModifiersStorage modifiersStorage;
        EXPECT_CALL(*scenarioWrapperPtr, GetModifiersStorage()).WillRepeatedly(ReturnRef(modifiersStorage));

        const TFeatures features{};

        const TString prevScenarioName = "PreviousScenario";
        TResponseFinalizer finalizerVisitor(scenarioWrapperPtr, walkerCtx, request, prevScenarioName, features,
                                           /* requestIsExpected= */ false);
        { // should update
            TResponseBuilderProto proto;
            TResponseBuilder response(speechKitRequest, request, prevScenarioName, proto);
            response.SetStackEngine(std::make_unique<NMegamind::TStackEngine>(updatedCore));
            finalizerVisitor.Finalize(&response);
            const auto& newSession = DeserializeSession(proto.GetResponse().GetSessions().at(""));
            UNIT_ASSERT_MESSAGES_EQUAL(newSession->GetStackEngineCore(), updatedCore);
        }
        { // should not update
            EXPECT_CALL(*scenarioWrapperPtr, IsApplyNeededOnWarmUpRequestWithSemanticFrame()).WillOnce(Return(true));
            TResponseBuilderProto proto;
            TResponseBuilder response(speechKitRequest, request, prevScenarioName, proto);
            response.SetStackEngine(std::make_unique<NMegamind::TStackEngine>(updatedCore));
            finalizerVisitor.Finalize(&response);
            const auto& newSession = DeserializeSession(proto.GetResponse().GetSessions().at(""));
            UNIT_ASSERT_MESSAGES_EQUAL(newSession->GetStackEngineCore(), originalCore);
        }
    }

    Y_UNIT_TEST_F(TestSemanticFrameInSession, NMegamind::TPredefinedRequestFixture) {
        auto mockRequestBuilder = MockRequestBuilder(/* utterance= */ {});
        auto& walkerCtx = mockRequestBuilder.LightWalkerCtx();
        auto& context = mockRequestBuilder.Context();
        const auto speechKitRequest =
            TSpeechKitRequestBuilder(TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent).Build();
        const auto request = CreateRequest(IEvent::CreateEvent(speechKitRequest.Event()), speechKitRequest);
        TScenarioInfraConfig scenarioConfig;
        EXPECT_CALL(context, HasExpFlag(_)).WillRepeatedly(Return(false));
        EXPECT_CALL(context, ScenarioConfig(_)).WillRepeatedly(ReturnRef(scenarioConfig));
        TClientInfoProto clientInfoProto;
        TClientInfo clientInfo(clientInfoProto);
        EXPECT_CALL(context, ClientInfo()).WillRepeatedly(ReturnRef(clientInfo));

        const auto scenarioWrapperPtr = MakeIntrusive<NiceMock<TMockScenarioWrapper>>();
        TState state;
        NMegamind::TAnalyticsInfoBuilder analyticsInfoBuilder;
        NMegamind::TUserInfoBuilder userInfoBuilder;
        EXPECT_CALL(*scenarioWrapperPtr, GetApplyEnv(_, _))
            .WillRepeatedly(Return(
                TLightScenarioEnv(context, request, /* semanticFrames= */ {}, state, analyticsInfoBuilder, userInfoBuilder)));
        NMegamind::TModifiersStorage modifiersStorage;
        EXPECT_CALL(*scenarioWrapperPtr, GetModifiersStorage()).WillRepeatedly(ReturnRef(modifiersStorage));

        const TFeatures features{};

        {
            const TString scenarioName = "ScenarioName";
            TResponseFinalizer finalizerVisitor(scenarioWrapperPtr, walkerCtx, request, scenarioName, features,
                /* requestIsExpected= */ false);

            TResponseBuilderProto proto;
            proto.MutableSemanticFrame()->SetName("SemanticFrame");
            TResponseBuilder response(speechKitRequest, request, scenarioName, proto);
            finalizerVisitor.Finalize(&response);

            const auto& newSession = DeserializeSession(proto.GetResponse().GetSessions().at(""));
            UNIT_ASSERT(newSession->GetResponseFrame().Defined());
        }

        {
            const TString scenarioName = ToString(MM_PROTO_VINS_SCENARIO);
            TResponseFinalizer finalizerVisitor(scenarioWrapperPtr, walkerCtx, request, scenarioName, features,
                /* requestIsExpected= */ false);

            TResponseBuilderProto proto;
            proto.MutableSemanticFrame()->SetName("SemanticFrame");
            TResponseBuilder response(speechKitRequest, request, scenarioName, proto);
            finalizerVisitor.Finalize(&response);

            const auto& newSession = DeserializeSession(proto.GetResponse().GetSessions().at(""));
            UNIT_ASSERT(!newSession->GetResponseFrame().Defined());
        }
    }
}

} // namespace NAlice

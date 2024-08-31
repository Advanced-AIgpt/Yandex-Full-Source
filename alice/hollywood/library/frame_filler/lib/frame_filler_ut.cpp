#include "frame_filler_handlers.h"
#include "frame_filler_utils.h"

#include <alice/megamind/protos/analytics/analytics_info.pb.h>
#include <alice/hollywood/library/frame_filler/lib/ut/analytics_info.pb.h>
#include <alice/hollywood/library/frame_filler/lib/ut/scenario_state.pb.h>

#include <alice/library/frame/builder.h>
#include <alice/library/response/defs.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>

#include <util/system/compiler.h>


#include <contrib/libs/protobuf/src/google/protobuf/util/message_differencer.h>


namespace NAlice {
namespace NFrameFiller {

namespace {

using namespace NAlice::NScenarios;
using namespace NAlice::NFrameFiller;
using namespace testing;

class TMockFrameFillerScenarioRunHandler : public IFrameFillerScenarioRunHandler {
public:
    MOCK_METHOD(TFrameFillerScenarioResponse, Do, (
        const NHollywood::TScenarioRunRequestWrapper& request,
        TRTLogger& logger
    ), (const, override));
};

class TMockFrameFillerScenarioCommitHandler : public IFrameFillerScenarioCommitHandler {
public:
    MOCK_METHOD(NScenarios::TScenarioCommitResponse, Do, (
        const NHollywood::TScenarioApplyRequestWrapper& request,
        TRTLogger& logger
    ), (const, override));
};

class TMockFrameFillerScenarioApplyHandler : public IFrameFillerScenarioApplyHandler {
public:
    MOCK_METHOD(NScenarios::TScenarioApplyResponse, Do, (
        const NHollywood::TScenarioApplyRequestWrapper& request,
        TRTLogger& logger
    ), (const, override));
};

TDirective BuildCallbackDirective(const TString& name, bool ignoreAnswer) {
    TDirective directive;
    directive.MutableCallbackDirective()->SetName(name);
    directive.MutableCallbackDirective()->SetIgnoreAnswer(ignoreAnswer);
    return directive;
}

TFrameAction BuildAction(TArrayRef<const TDirective> directives) {
    TFrameAction action;
    for (const auto& directive : directives) {
        *action.MutableDirectives()->AddList() = directive;
    }
    return action;
}

::google::protobuf::Any ToState(const ::google::protobuf::Message& msg) {
    ::google::protobuf::Any result;
    result.PackFrom(msg);
    return result;
}

::google::protobuf::Any ToState(const TFrameFillerRequest& ffRequest) {
    ::google::protobuf::Any result;
    TFrameFillerState ffState;
    *ffState.MutableRequest() = ffRequest;
    result.PackFrom(ffState);
    return result;
}

TScenarioRunRequest BuildScenarioRunRequest(
    const TMaybe<::google::protobuf::Any>& state = Nothing(),
    const TMaybe<TSemanticFrame>& inputFrame = Nothing(),
    const TMaybe<TDataSource>& dataSource = Nothing(),
    const EDataSourceType dataSourceType = EDataSourceType::WEB_SEARCH_DOCS
) {
    TScenarioRunRequest request;
    *request.MutableBaseRequest() = TScenarioBaseRequest{};
    if (state.Defined()) {
        *request.MutableBaseRequest()->MutableState() = *state;
    }
    if (inputFrame.Defined()) {
        *request.MutableInput()->AddSemanticFrames() = *inputFrame;
    }
    if (dataSource.Defined()) {
        (*request.MutableDataSources())[dataSourceType] = *dataSource;
    }
    return request;
}

TTestScenarioState BuildScenarioState(const TString& value = "scenario_state_value") {
    TTestScenarioState state;
    state.SetValue(value);
    return state;
}

NScenarios::TAnalyticsInfo BuildAnalyticsInfo() {
    NScenarios::TAnalyticsInfo info;
    info.SetIntent("test_intent");
    return info;
}

class TLayoutBuilder {
public:
    TLayoutBuilder() = default;

    TLayoutBuilder& AddText(const TString& text) {
        Layout.AddCards()->SetText(text);
        return *this;
    }

    TLayoutBuilder& SetOutputSpeech(const TString& text) {
        Layout.SetOutputSpeech(text);
        return *this;
    }

    TLayoutBuilder& SetShouldListen(bool shouldListen) {
        Layout.SetShouldListen(shouldListen);
        return *this;
    }

    TLayoutBuilder& AddDirective(const TDirective& directive) {
        *Layout.AddDirectives() = directive;
        return *this;
    }

    TLayout Build() const {
        return Layout;
    }

private:
    TLayout Layout;
};

TSlotRequirement BuildSlotRequirement(const TString& slotName) {
    TSlotRequirement requirement;
    requirement.SetSlotName(slotName);
    *requirement.AddLayoutAlternatives() = TLayoutBuilder()
        .AddText(slotName)
        .SetOutputSpeech(slotName)
        .Build();
    return requirement;
}

TVector<TString> GetAllRequestedSlots(const TSemanticFrame& frame) {
    TVector<TString> requestedSlots;
    for (auto& slot : frame.GetSlots()) {
        if (slot.GetIsRequested()) {
            requestedSlots.push_back(slot.GetName());
        }
    }
    return requestedSlots;
}

class TFrameFillerRequestBuilder {
public:

    TFrameFillerRequestBuilder() {
        Request.MutableScenarioResponse()->MutableState()->PackFrom(BuildScenarioState());
        *Request.MutableScenarioResponse()->MutableAnalyticsInfo() = BuildAnalyticsInfo();
    }

    explicit TFrameFillerRequestBuilder(const TFrameFillerRequest& request)
            : Request(request)
    {
    }

    TFrameFillerRequestBuilder& AddSlotRequirement(const TString& slotName) {
        *Request.AddSlotRequirements() = BuildSlotRequirement(slotName);
        return *this;
    }

    TFrameFillerRequestBuilder& SetOnSubmit(const TFrameAction& action) {
        *Request.MutableOnSubmit() = action;
        return *this;
    }

    TFrameFillerRequestBuilder& ClearOnSubmit() {
        Request.ClearOnSubmit();
        return *this;
    }

    TFrameFillerRequestBuilder& SetRequestedSlot(const TString& slotName) {
        auto& frame = *Request.MutableScenarioResponse()->MutableSemanticFrame();
        frame = MakeSlotRequested(frame, slotName);
        return *this;
    }

    TFrameFillerRequestBuilder& SetFrame(const TSemanticFrame& frame) {
        *Request.MutableScenarioResponse()->MutableSemanticFrame() = frame;
        return *this;
    }

    TFrameFillerRequestBuilder& SetLayout(const TLayout& layout) {
        *Request.MutableScenarioResponse()->MutableLayout() = layout;
        return *this;
    }

    TFrameFillerRequestBuilder Copy() const {
        return *this;
    }

    TFrameFillerRequest Build() const {
        return Request;
    }

private:
    TFrameFillerRequest Request;
};

struct TFixture : public NUnitTest::TBaseFixture {
    StrictMock<TMockFrameFillerScenarioRunHandler> RunHandler;
    StrictMock<TMockFrameFillerScenarioCommitHandler> CommitHandler;
    StrictMock<TMockFrameFillerScenarioApplyHandler> ApplyHandler;

    TMaybe<TString> NoType = Nothing();
    TMaybe<TString> NoValue = Nothing();

    TString FilledOptionalSlotName = "filled_optional_slot";
    TString FilledRequiredSlotName = "filled_required_slot";
    TString EmptyOptionalSlotName = "empty_optional_slot";
    TString FirstEmptyRequiredSlotName = "first_empty_required_slot";
    TString SecondEmptyRequiredSlotName = "second_empty_required_slot";

    TDirective CallbackDirective = BuildCallbackDirective("submit!", /* ignoreAnswer= */ false);

    const TSemanticFrame SimpleFrame = TSemanticFrameBuilder{"frame"}
        .AddSlot(FilledOptionalSlotName, {"type1"}, "type1", "optional_value_1")
        .AddSlot(FilledRequiredSlotName, {"type2"}, "type2", "required_value_2")
        .AddSlot(EmptyOptionalSlotName, {"type3"}, NoType, NoValue)
        .AddSlot(FirstEmptyRequiredSlotName, {"type4", "type5"}, NoType, NoValue)
        .AddSlot(SecondEmptyRequiredSlotName, {"type6"}, NoType, NoValue)
        .Build();
    const TSemanticFrame AlmostFilledFrame = TSemanticFrameBuilder(SimpleFrame)
        .SetSlotValue(FirstEmptyRequiredSlotName, "type5", "some value")
        .Build();
    const TSemanticFrame FilledFrame = TSemanticFrameBuilder(AlmostFilledFrame)
        .SetSlotValue(SecondEmptyRequiredSlotName, "type6", "some value")
        .Build();

    TFrameFillerRequest BaseFfRequest = TFrameFillerRequestBuilder{}
        .AddSlotRequirement(FilledRequiredSlotName)
        .AddSlotRequirement(FirstEmptyRequiredSlotName)
        .AddSlotRequirement(SecondEmptyRequiredSlotName)
        .SetOnSubmit(BuildAction({CallbackDirective}))
        .Build();
    TFrameFillerRequest SimpleFrameFfRequest = TFrameFillerRequestBuilder(BaseFfRequest)
        .Copy()
        .SetFrame(SimpleFrame)
        .Build();
    TFrameFillerRequest SimpleFrameRequestsSlotFfRequest = TFrameFillerRequestBuilder(BaseFfRequest)
        .Copy()
        .SetFrame(SimpleFrame)
        .SetRequestedSlot(FirstEmptyRequiredSlotName)
        .Build();
    TFrameFillerRequest AlmostFilledFrameFfRequest = TFrameFillerRequestBuilder(BaseFfRequest)
        .Copy()
        .SetFrame(AlmostFilledFrame)
        .Build();
    TFrameFillerRequest FilledFrameFfRequest = TFrameFillerRequestBuilder(BaseFfRequest)
        .Copy()
        .SetFrame(FilledFrame)
        .Build();

    TRTLogger& Logger = TRTLogger::NullLogger();
    NAppHost::NService::TTestContext ServiceContext{};
};

MATCHER_P(MessageEq, msg, "") {
    TMessageDiff diff((arg), (msg));
    UNIT_ASSERT_C(diff.AreEqual, diff.Diff);
    return diff.AreEqual;
}

MATCHER_P(MessageWrapperEq, msg, "") {
    TMessageDiff diff((arg.Proto()), (msg->Proto()));
    UNIT_ASSERT_C(diff.AreEqual, diff.Diff);
    return diff.AreEqual;
}

#define UNIT_ASSERT_RESPONSE_NOT_ERROR(response)                                                            \
    do {                                                                                                    \
        const bool isError = ((response).GetResponseCase() == TScenarioRunResponse::ResponseCase::kError);  \
        TString errorMsg;                                                                                   \
        if (isError) {                                                                                      \
            errorMsg = (response).GetError().GetMessage();                                                  \
        }                                                                                                   \
        UNIT_ASSERT_C(!isError, errorMsg);                                                                  \
    } while (false)

#define UNIT_ASSERT_RESPONSE_ERROR(response, expectedErrorMsg)                                              \
    do {                                                                                                    \
        using TResponseType = decltype(response);                                                           \
        const bool isError = ((response).GetResponseCase() == TResponseType::ResponseCase::kError);         \
        UNIT_ASSERT_C(isError, "error expected");                                                           \
        UNIT_ASSERT_VALUES_EQUAL((response).GetError().GetMessage(), expectedErrorMsg);                     \
    } while (false)

#define CHECK_RESPONSE(request, clientScenarioResponse, expectedResponse, expectedRequest)                  \
    do {                                                                                                    \
        EXPECT_CALL(RunHandler, Do(MessageWrapperEq(&expectedRequest), _))                                   \
            .WillOnce(Return(clientScenarioResponse));                                                      \
                                                                                                            \
        const auto response = NFrameFiller::Run(request, RunHandler, Logger);                               \
        UNIT_ASSERT_RESPONSE_NOT_ERROR(response);                                                           \
        UNIT_ASSERT_MESSAGES_EQUAL((expectedResponse), response);                                           \
    } while (false)

Y_UNIT_TEST_SUITE_F(FrameFiller, TFixture) {
    Y_UNIT_TEST(SmokeTest) {
        const TString expectedErrorMsg = "got error!";
        EXPECT_CALL(RunHandler, Do(_, _))
            .WillOnce(Return(ToScenarioResponse(TError<TScenarioRunResponse>{} << expectedErrorMsg)));
        EXPECT_CALL(CommitHandler, Do(_, _))
            .WillOnce(Return(TScenarioCommitResponse{TError<TScenarioCommitResponse>{} << expectedErrorMsg}));
        EXPECT_CALL(ApplyHandler, Do(_, _))
            .WillOnce(Return(TScenarioApplyResponse{TError<TScenarioApplyResponse>{} << expectedErrorMsg}));

        {
            const auto requestProto = BuildScenarioRunRequest();
            const NHollywood::TScenarioRunRequestWrapper request{requestProto, ServiceContext};
            const auto response = NFrameFiller::Run(request, RunHandler, Logger);
            UNIT_ASSERT_RESPONSE_ERROR(response, expectedErrorMsg);
        }

        {
            const TScenarioApplyRequest requestProto{};
            const NHollywood::TScenarioApplyRequestWrapper request{requestProto, ServiceContext};
            const auto response = NFrameFiller::Commit(request, CommitHandler, Logger);
            UNIT_ASSERT_RESPONSE_ERROR(response, expectedErrorMsg);
        }

        {
            const TScenarioApplyRequest requestProto{};
            const NHollywood::TScenarioApplyRequestWrapper request{requestProto, ServiceContext};
            const auto response = NFrameFiller::Apply(request, ApplyHandler, Logger);
            UNIT_ASSERT_RESPONSE_ERROR(response, expectedErrorMsg);
        }
    }

    Y_UNIT_TEST(Proxy) {
        TScenarioResponseBody scenarioResponseBody;
        *scenarioResponseBody.MutableLayout() = TLayoutBuilder().SetOutputSpeech("There is no spoon!").Build();

        TScenarioRunResponse scenarioRunResponse;
        *scenarioRunResponse.MutableResponseBody() = scenarioResponseBody;
        scenarioRunResponse.SetVersion(DEFAULT_VERSION);

        TScenarioCommitResponse scenarioCommitResponse;
        *scenarioCommitResponse.MutableSuccess() = TScenarioCommitResponse::TSuccess();

        TScenarioApplyResponse scenarioApplyResponse;
        *scenarioApplyResponse.MutableResponseBody() = scenarioResponseBody;

        const auto runRequestProto = BuildScenarioRunRequest(ToState(BuildScenarioState()), Nothing(), TDataSource());
        const NHollywood::TScenarioRunRequestWrapper runRequest{runRequestProto, ServiceContext};
        const TScenarioApplyRequest applyRequestProto{};
        const NHollywood::TScenarioApplyRequestWrapper applyRequest{applyRequestProto, ServiceContext};
        const TScenarioApplyRequest commtRequestProto{};
        const NHollywood::TScenarioApplyRequestWrapper commitRequest{commtRequestProto, ServiceContext};

        EXPECT_CALL(RunHandler, Do(MessageWrapperEq(&runRequest), _)).WillOnce(Return(ToScenarioResponse(scenarioRunResponse)));
        EXPECT_CALL(CommitHandler, Do(MessageWrapperEq(&applyRequest), _)).WillOnce(Return(scenarioCommitResponse));
        EXPECT_CALL(ApplyHandler, Do(MessageWrapperEq(&commitRequest), _)).WillOnce(Return(scenarioApplyResponse));

        {
            const auto response = NFrameFiller::Run(runRequest, RunHandler, Logger);
            UNIT_ASSERT_MESSAGES_EQUAL(response, scenarioRunResponse);
        }
        {
            const auto response = NFrameFiller::Commit(applyRequest, CommitHandler, Logger);
            UNIT_ASSERT_MESSAGES_EQUAL(response, scenarioCommitResponse);
        }
        {
            const auto response = NFrameFiller::Apply(commitRequest, ApplyHandler, Logger);
            UNIT_ASSERT_MESSAGES_EQUAL(response, scenarioApplyResponse);
        }
    }

    Y_UNIT_TEST(FrameFilling_givenEmptyRequiredSlot_AsksForFirstEmptyRequiredSlot) {
        const TScenarioRunResponse expectedRunResponse = BuildScenarioRunResponse(
            SimpleFrameFfRequest,
            TLayoutBuilder{}
                .AddText(FirstEmptyRequiredSlotName)
                .SetOutputSpeech(FirstEmptyRequiredSlotName)
                .SetShouldListen(true)
                .Build(),
            /* requestedSlot= */ FirstEmptyRequiredSlotName
        );

        const auto runRequestProto = BuildScenarioRunRequest();
        const NHollywood::TScenarioRunRequestWrapper runRequest{runRequestProto, ServiceContext};

        const auto unwrappedRunRequestProto = UnwrapState(runRequestProto);
        const NHollywood::TScenarioRunRequestWrapper unwrappedRunRequest{unwrappedRunRequestProto, ServiceContext};

        CHECK_RESPONSE(runRequest, ToScenarioResponse(SimpleFrameFfRequest),
                       expectedRunResponse, unwrappedRunRequest);
    }

    Y_UNIT_TEST(FrameFilling_givenNoRelatedFrameInInput_RequestsSlot) {
        const TScenarioRunRequest runRequestProto = BuildScenarioRunRequest(
            ToState(SimpleFrameRequestsSlotFfRequest),
            TSemanticFrameBuilder(AlmostFilledFrame)
                .SetName(AlmostFilledFrame.GetName() + "-irrelevant")
                .Build()
        );
        const NHollywood::TScenarioRunRequestWrapper runRequest{runRequestProto, ServiceContext};

        const TScenarioRunRequest unwrappedRunRequestProto = UnwrapState(runRequestProto);
        const NHollywood::TScenarioRunRequestWrapper unwrappedRunRequest{unwrappedRunRequestProto, ServiceContext};

        const TScenarioRunResponse expectedRunResponse = BuildScenarioRunResponse(
            SimpleFrameFfRequest,
            TLayoutBuilder{}
                .AddText(FirstEmptyRequiredSlotName)
                .SetOutputSpeech(FirstEmptyRequiredSlotName)
                .SetShouldListen(true)
                .Build(),
            /* requestedSlot= */ FirstEmptyRequiredSlotName
        );

        CHECK_RESPONSE(runRequest, ToScenarioResponse(SimpleFrameFfRequest),
                       expectedRunResponse, unwrappedRunRequest);
    }

    Y_UNIT_TEST(givenFrameWithFilledRequestedSlot_RequestsNextEmptyRequiredSlot) {
        const TScenarioRunRequest runRequestProto = BuildScenarioRunRequest(
            ToState(SimpleFrameRequestsSlotFfRequest),
            AlmostFilledFrame
        );
        const NHollywood::TScenarioRunRequestWrapper runRequest{runRequestProto, ServiceContext};

        const TScenarioRunRequest unwrapperRunRequestProto = UnwrapState(runRequestProto);
        const NHollywood::TScenarioRunRequestWrapper unwrapperRunRequest{unwrapperRunRequestProto, ServiceContext};

        const TScenarioRunResponse expectedRunResponse = BuildScenarioRunResponse(
            AlmostFilledFrameFfRequest,
            TLayoutBuilder{}
                .AddText(SecondEmptyRequiredSlotName)
                .SetOutputSpeech(SecondEmptyRequiredSlotName)
                .SetShouldListen(true)
                .Build(),
                /* requestedSlot= */ SecondEmptyRequiredSlotName
        );

        CHECK_RESPONSE(runRequest, ToScenarioResponse(AlmostFilledFrameFfRequest),
                       expectedRunResponse, unwrapperRunRequest);
    }

    Y_UNIT_TEST(FrameFilling_givenFilledFrame_InvokesOnSubmitEventAction) {
        const TScenarioRunRequest runRequestProto = BuildScenarioRunRequest(ToState(AlmostFilledFrameFfRequest), FilledFrame);
        const NHollywood::TScenarioRunRequestWrapper runRequest{runRequestProto, ServiceContext};

        const TScenarioRunRequest unwrappedRunRequestProto = UnwrapState(runRequestProto);
        const NHollywood::TScenarioRunRequestWrapper unwrappedRunRequest{unwrappedRunRequestProto, ServiceContext};

        const TScenarioRunResponse expectedRunResponse = BuildScenarioRunResponse(
            FilledFrameFfRequest,
            TLayoutBuilder{}.AddText(NResponse::THREE_DOTS).AddDirective(CallbackDirective).Build(),
            /* requestedSlot= */ Nothing()
        );

        CHECK_RESPONSE(runRequest, ToScenarioResponse(FilledFrameFfRequest),
                       expectedRunResponse, unwrappedRunRequest);
    }

    Y_UNIT_TEST(FrameFilling_givenFilledFrameInScenarioResponseAndNoSubmit_ReportsError) {
        const TString expectedErrorMsg = TStringBuilder{}
            << "Requested for frame filling with no unmet slot requirements "
            << "and with no submit action provided";

        const TFrameFillerRequest prevFfRequest = TFrameFillerRequestBuilder(AlmostFilledFrameFfRequest)
            .ClearOnSubmit()
            .Build();
        const TFrameFillerRequest curFfRequest = TFrameFillerRequestBuilder(FilledFrameFfRequest)
            .ClearOnSubmit()
            .Build();

        const TScenarioRunRequest runRequest = BuildScenarioRunRequest(ToState(prevFfRequest), FilledFrame);
        const NHollywood::TScenarioRunRequestWrapper request{runRequest, ServiceContext};

        EXPECT_CALL(RunHandler, Do(_, _))
            .WillOnce(Return(ToScenarioResponse(curFfRequest)));

        const auto response = NFrameFiller::Run(request, RunHandler, Logger);
        UNIT_ASSERT_RESPONSE_ERROR(response, expectedErrorMsg);
    }

    Y_UNIT_TEST(FrameFilling_givenFilledFrameToScenarioRequestAndNoSubmit_AddsCallbackMmOnSubmit) {
        const TFrameFillerRequest ffRequest = TFrameFillerRequestBuilder(FilledFrameFfRequest)
            .ClearOnSubmit()
            .Build();

        const TScenarioRunRequest runRequest = BuildScenarioRunRequest(ToState(ffRequest), FilledFrame);
        const NHollywood::TScenarioRunRequestWrapper request{runRequest, ServiceContext};

        TScenarioRunRequest expectedRequestProto = UnwrapState(runRequest);
        expectedRequestProto.MutableInput()->MutableCallback()->SetName(ON_SUBMIT_CALLBACK_NAME);

        const NHollywood::TScenarioRunRequestWrapper expectedRequest{expectedRequestProto, ServiceContext};

        EXPECT_CALL(RunHandler, Do(MessageWrapperEq(&expectedRequest), _));

        const auto response = NFrameFiller::Run(request, RunHandler, Logger);
    }

    Y_UNIT_TEST(FrameFilling_givenLayoutInScenarioResponse_returnsIt) {
        const TString scenarioResponseText = "some text";
        const TLayout scenarioLayout = TLayoutBuilder{}
            .AddText(scenarioResponseText)
            .SetOutputSpeech(scenarioResponseText)
            .Build();
        const TFrameFillerRequest newFfRequest = TFrameFillerRequestBuilder{SimpleFrameFfRequest}
            .SetLayout(scenarioLayout)
            .Build();
        const TFrameFillerScenarioResponse clientScenarioResponse = ToScenarioResponse(newFfRequest);

        const TScenarioRunResponse expectedRunResponse = UnwrapState(BuildScenarioRunResponse(newFfRequest));

        const auto runRequestProto = BuildScenarioRunRequest(ToState(SimpleFrameFfRequest), AlmostFilledFrame);
        const NHollywood::TScenarioRunRequestWrapper runRequest{runRequestProto, ServiceContext};

        const auto unwrappedRunRequestProto = UnwrapState(runRequestProto);
        const NHollywood::TScenarioRunRequestWrapper unwrappedRunRequest{unwrappedRunRequestProto, ServiceContext};

        CHECK_RESPONSE(runRequest, clientScenarioResponse,
                       expectedRunResponse, unwrappedRunRequest);
    }

    Y_UNIT_TEST(FrameFilling_givenLayoutInCommitCandidate_returnsIt) {
        const TString scenarioResponseText = "some text";
        const TLayout scenarioLayout = TLayoutBuilder{}
            .AddText(scenarioResponseText)
            .SetOutputSpeech(scenarioResponseText)
            .Build();
        TFrameFillerRequest newFfRequest = TFrameFillerRequestBuilder{SimpleFrameFfRequest}.Build();
        *newFfRequest.MutableCommitCandidate()->MutableResponseBody()->MutableLayout() = scenarioLayout;
        const TFrameFillerScenarioResponse clientScenarioResponse = ToScenarioResponse(newFfRequest);

        const TScenarioRunResponse expectedRunResponse = UnwrapState(BuildScenarioRunResponse(newFfRequest));

        const auto requestProto = BuildScenarioRunRequest(ToState(SimpleFrameFfRequest), AlmostFilledFrame);
        const NHollywood::TScenarioRunRequestWrapper request{requestProto, ServiceContext};

        const auto unwrappedRequestProto{UnwrapState(requestProto)};
        const NHollywood::TScenarioRunRequestWrapper unwrappedRequest{unwrappedRequestProto, ServiceContext};

        EXPECT_CALL(RunHandler, Do(MessageWrapperEq(&unwrappedRequest), _))                                                     \
            .WillOnce(Return(clientScenarioResponse));

        const auto response = NFrameFiller::Run(request, RunHandler, Logger);
        UNIT_ASSERT_RESPONSE_NOT_ERROR(response);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedRunResponse, response);
        UNIT_ASSERT(expectedRunResponse.HasCommitCandidate());
    }

    Y_UNIT_TEST(MakeSlotRequested_changesRequestedSlot) {
        TSemanticFrame frame = SimpleFrame;
        UNIT_ASSERT_VALUES_EQUAL(GetAllRequestedSlots(frame), TVector<TString>{});
        MakeSlotRequested(frame, FirstEmptyRequiredSlotName);
        UNIT_ASSERT_VALUES_EQUAL(GetAllRequestedSlots(frame), TVector<TString>{FirstEmptyRequiredSlotName});
        MakeSlotRequested(frame, SecondEmptyRequiredSlotName);
        UNIT_ASSERT_VALUES_EQUAL(GetAllRequestedSlots(frame), TVector<TString>{SecondEmptyRequiredSlotName});
        MakeSlotRequested(frame, Nothing());
        UNIT_ASSERT_VALUES_EQUAL(GetAllRequestedSlots(frame), TVector<TString>{});
    }
}

} // namespace

} // namespace NFrameFiller
} // namespace NAlice

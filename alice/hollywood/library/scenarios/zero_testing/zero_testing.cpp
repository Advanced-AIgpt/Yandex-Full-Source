#include "zero_testing.h"

#include <alice/hollywood/library/scenarios/zero_testing/proto/zero_testing_state.pb.h>

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/protos/blackbox/blackbox.pb.h>

#include <alice/library/proto/proto.h>

#include <util/string/subst.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::ZeroTesting {

namespace {

constexpr TStringBuf FRAME_ACTIVATE = "alice.zero_testing_activate";
constexpr TStringBuf FRAME_DEACTIVATE = "alice.zero_testing_deactivate";
constexpr TStringBuf FRAME_TELL_ME_CODE = "alice.zero_testing_tell_me_code";
constexpr TStringBuf SLOT_EXP_ID = "exp_id";

const TString ZERO_TESTING_CODE_EXPERIMENT{TStringBuf("zero_testing_code=")};

constexpr TStringBuf NLG_ZERO_TESTING = "zero_testing";
constexpr TStringBuf NLG_PHRASE_RENDER_ACTIVATE = "render_activate";
constexpr TStringBuf NLG_PHRASE_RENDER_DEACTIVATE = "render_deactivate";
constexpr TStringBuf NLG_PHRASE_RENDER_TELL_ME_CODE = "render_tell_me_code";


class TZeroTestingRunHandleImpl {
public:
    TZeroTestingRunHandleImpl(TRTLogger& logger, const TScenarioRunRequestWrapper& request, TNlgWrapper& nlg)
        : Logger_(logger)
        , Request_(request)
        , Nlg_(nlg) {
    }

    std::unique_ptr<TScenarioRunResponse> Do() {
        if (const auto userInfo = GetUserInfoProto(Request_); userInfo) {
            if (userInfo->GetIsStaff() || userInfo->GetIsBetaTester()) {
                LOG_INFO(Logger_) << "User is either staff or betatester, processing request...";
                return DoAuthorized();
            } else {
                LOG_INFO(Logger_) << "User is neither staff nor betatester, ZeroTesting scenario is forbidden.";
                return TRunResponseBuilder::MakeIrrelevantResponse(Nlg_,
                    TStringBuf("У Вас нет доступа к сценарию ZeroTesting."));
            }
        } else {
            const TStringBuf msg = "BlackBox userInfo not found, cannot proceed.";
            LOG_INFO(Logger_) << msg;
            return MakeErrorResponse(Nlg_, msg);
        }
    }

    TZeroTestingState UnpackZeroTestingState() {
        TZeroTestingState ztState;
        const bool haveZtState = Request_.BaseRequestProto().GetState().UnpackTo(&ztState);
        if (!haveZtState) {
            LOG_INFO(Logger_) << "Have no ZeroTestingState in the request. Proceed with empty state.";
        }
        return ztState;
    }

private:
    std::unique_ptr<TScenarioRunResponse> DoAuthorized() {
        const auto& input = Request_.Input();
        if (const auto framePtr = input.FindSemanticFrame(FRAME_ACTIVATE); framePtr) {
            auto frame = TFrame::FromProto(*framePtr);
            return DoActivate(frame);
        } else if (const auto framePtr = input.FindSemanticFrame(FRAME_DEACTIVATE); framePtr) {
            return DoDeactivate();
        } else if (const auto framePtr = input.FindSemanticFrame(FRAME_TELL_ME_CODE); framePtr) {
            return DoTellMeCode();
        }

        const TStringBuf msg = "No relevant frame found for ZeroTesting.";
        LOG_INFO(Logger_) << msg;
        return MakeErrorResponse(Nlg_, msg);
    }

    std::unique_ptr<TScenarioRunResponse> DoActivate(const TFrame& frame) {
        const auto expIdSlot = frame.FindSlot(SLOT_EXP_ID);
        if (!expIdSlot) {
            return MakeErrorResponse(Nlg_, TStringBuilder() << "Slot not found: " << SLOT_EXP_ID);
        }

        const auto testIdStr = expIdSlot->Value.AsString();
        ui64 testId;
        if (!TryFromString(testIdStr, testId)) {
            return MakeErrorResponse(Nlg_, TStringBuilder() << "Failed to parse test-id: " << testIdStr);
        }

        auto ztState = UnpackZeroTestingState();
        if (auto it = Find(ztState.GetTestIds(), testId); it == ztState.GetTestIds().end()) {
            ztState.AddTestIds(testId);
        } else {
            LOG_INFO(Logger_) << "testId = " << testId << " already activated.";
        }

        TRunResponseBuilder builder(&Nlg_);

        auto& bodyBuilder = builder.CreateResponseBodyBuilder();

        TNlgData nlgData{Logger_, Request_};
        nlgData.Context["test_ids"] = MakeTestIdsJsonArray(ztState);
        LOG_INFO(Logger_) << "nlgData.Context is " << JsonToString(nlgData.Context);
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_ZERO_TESTING, NLG_PHRASE_RENDER_ACTIVATE,
                                                       /* buttons = */ {}, nlgData);

        NSc::TValue data;
        for (const auto testId : ztState.GetTestIds()) {
            data["uaas_tests"].Push(testId);
        }

        TDirective directive;
        auto& setCookiesDirective = *directive.MutableSetCookiesDirective();
        setCookiesDirective.SetName("set_cookies");
        setCookiesDirective.SetValue(data.ToJson());
        bodyBuilder.AddDirective(std::move(directive));

        bodyBuilder.SetState(ztState);

        return std::move(builder).BuildResponse();
    }

    std::unique_ptr<TScenarioRunResponse> DoDeactivate() {
        const auto ztState = UnpackZeroTestingState();

        TRunResponseBuilder builder(&Nlg_);
        auto& bodyBuilder = builder.CreateResponseBodyBuilder();

        TNlgData nlgData{Logger_, Request_};
        nlgData.Context["test_ids_count"] = ztState.GetTestIds().size();
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_ZERO_TESTING, NLG_PHRASE_RENDER_DEACTIVATE,
                                                       /* buttons = */ {}, nlgData);

        TDirective directive;
        auto& setCookiesDirective = *directive.MutableSetCookiesDirective();
        setCookiesDirective.SetName("set_cookies");
        setCookiesDirective.ClearValue();
        bodyBuilder.AddDirective(std::move(directive));

        TZeroTestingState newZtState;
        bodyBuilder.SetState(newZtState);

        return std::move(builder).BuildResponse();
    }

    std::unique_ptr<TScenarioRunResponse> DoTellMeCode() {
        TRunResponseBuilder builder(&Nlg_);
        auto& bodyBuilder = builder.CreateResponseBodyBuilder();

        TNlgData nlgData{Logger_, Request_};

        const auto ztState = UnpackZeroTestingState();
        nlgData.Context["test_ids"] = MakeTestIdsJsonArray(ztState);

        const auto codeStr = Request_.GetValueFromExpPrefix(ZERO_TESTING_CODE_EXPERIMENT);
        nlgData.Context["exp_code"] = codeStr.Defined() ? codeStr.GetRef() : TString();
        LOG_INFO(Logger_) << "nlgData.Context is " << JsonToString(nlgData.Context);
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_ZERO_TESTING, NLG_PHRASE_RENDER_TELL_ME_CODE,
                                                       /* buttons = */ {}, nlgData);

        return std::move(builder).BuildResponse();
    }

    static NJson::TJsonArray MakeTestIdsJsonArray(const TZeroTestingState& ztState) {
        NJson::TJsonArray result;
        for (const auto testId : ztState.GetTestIds()) {
            result.AppendValue(ToString(testId));
        }
        return result;
    }

    // TODO: Move it to TRunResponseBuilder maybe?..
    static std::unique_ptr<TScenarioRunResponse> MakeErrorResponse(TNlgWrapper& nlg, const TStringBuf msg) {
        TRunResponseBuilder builder(&nlg);
        builder.SetError("error", TString(msg));
        return std::move(builder).BuildResponse();
    }

private:
    TRTLogger& Logger_;
    const TScenarioRunRequestWrapper& Request_;
    TNlgWrapper& Nlg_;
};

} // namespace

void TZeroTestingRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    auto& logger = ctx.Ctx.Logger();
    auto nlg = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TZeroTestingRunHandleImpl handler(logger, request, nlg);
    auto response = handler.Do();
    ctx.ServiceCtx.AddProtobufItem(std::move(*response), RESPONSE_ITEM);
}

REGISTER_SCENARIO("zero_testing",
                  AddHandle<TZeroTestingRunHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NZeroTesting::NNlg::RegisterAll));

} // namespace NAlice::NHollywood::ZeroTesting

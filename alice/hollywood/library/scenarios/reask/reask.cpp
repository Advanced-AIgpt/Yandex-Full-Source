#include "reask.h"

#include <alice/hollywood/library/scenarios/reask/nlg/register.h>
#include <alice/hollywood/library/scenarios/reask/proto/state.pb.h>

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/experiments.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/common/events.pb.h>

#include <library/cpp/iterator/mapped.h>

#include <util/string/join.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

constexpr ui32 MAX_REASK_COUNT = 1;
constexpr ui32 SECONDS_SINCE_PAUSE = 2 * 60 * 60;

constexpr TStringBuf NLG_REASK = "reask";
constexpr TStringBuf REASK_PLAY_FRAME = "alice.reask_play";

TString JoinAsrDataWords(const NAlice::TAsrResult asrData) {
    const auto words = MakeMappedRange(asrData.GetWords(), [](const NAlice::TAsrResult::TWord& word) {
        return word.GetValue();
    });
    return JoinRange(/* delim= */ TStringBuf(" "), words.begin(), words.end());
}

bool ShouldReaskByAsrHypotheses(const NAlice::NScenarios::TInput& input, NAlice::TRTLogger& logger) {
    if (!input.HasVoice() || input.GetVoice().AsrDataSize() < 2) {
        return false;
    }

    const auto utterance = JoinAsrDataWords(input.GetVoice().GetAsrData(0));
    const auto hypothesis = JoinAsrDataWords(input.GetVoice().GetAsrData(1));

    LOG_INFO(logger) << "Utterance: " << utterance;
    LOG_INFO(logger) << "Hypothesis: " << hypothesis;

    return hypothesis.StartsWith(utterance + " ");
}

} // namespace

void TReaskRunHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

    auto& logger = ctx.Ctx.Logger();
    auto nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
    TRunResponseBuilder builder{&nlgWrapper};
    auto& bodyBuilder = builder.CreateResponseBodyBuilder();

    const auto frameProto = request.Input().FindSemanticFrame(REASK_PLAY_FRAME);

    TReaskState reaskState;
    ui32 reaskCount = 0;
    if (!request.IsNewSession() && request.BaseRequestProto().HasState() &&
        request.BaseRequestProto().GetState().Is<TReaskState>())
    {
        request.BaseRequestProto().GetState().UnpackTo(&reaskState);
        reaskCount = reaskState.GetReaskCount();
    }

    if (!frameProto) {
        LOG_WARNING(logger) << "Failed to get " << REASK_PLAY_FRAME << " semantic frame";
        builder.SetIrrelevant();
    } else if (!request.ClientInfo().IsSmartSpeaker()) {
        LOG_WARNING(logger) << "Scenario works on smart speakers only";
        builder.SetIrrelevant();
    } else if (reaskCount >= MAX_REASK_COUNT) {
        LOG_WARNING(logger) << "Max number of reasks exceeded";
        builder.SetIrrelevant();
    } else if (!ShouldReaskByAsrHypotheses(request.Input().Proto(), logger) &&
               !request.HasExpFlag(EXP_REASK_SKIP_ASR_HYPO))
    {
        LOG_WARNING(logger) << "Restricted by ASR hypotheses";
        builder.SetIrrelevant();
    } else {
        reaskState.SetReaskCount(reaskCount + 1);
        bodyBuilder.SetState(reaskState);
        bodyBuilder.SetShouldListen(true);
        builder.FillPlayerFeatures(true, SECONDS_SINCE_PAUSE);

        TNlgData nlgData{logger, request};
        bodyBuilder.AddRenderedTextWithButtonsAndVoice(NLG_REASK, "reask_play", /* buttons = */ {}, nlgData);
    }

    auto response = std::move(builder).BuildResponse();
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

REGISTER_SCENARIO("reask",
                  AddHandle<TReaskRunHandle>()
                  .SetNlgRegistration(NAlice::NHollywood::NLibrary::NScenarios::NReask::NNlg::RegisterAll)
);

} // namespace NAlice::NHollywood

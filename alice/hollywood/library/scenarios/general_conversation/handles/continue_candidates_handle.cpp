#include "continue_candidates_handle.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/aggregated_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/entity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/filter_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/filter_by_embedding_model.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/generative_tale_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/proactivity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/search_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/seq2seq_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/aggregated_reply_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/entity.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>
#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_fast_data.h>
#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_resources.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/gif_card/gif_card.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/paskills/social_sharing/proto/api/api.pb.h>


using namespace NAlice::NScenarios;
using namespace NAlice::NSocialSharing;

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

void PreprocessTaleText(TString& taleText) {
    taleText = Strip(taleText);
    SubstGlobal(taleText, "\n", " \\n ");
    if (!taleText.empty() && !IsIn(".!?", taleText.back())) {
        taleText.push_back('.');
    }
}

void PatchReplyWithGenerativeTale(const TClassificationResult& classificationResult, TGeneralConversationApplyContextWrapper& contextWrapper, TReplyInfo* replyInfo) {
    TString taleText = replyInfo->GetGenerativeTaleReply().GetText();
    TString taleQuestion = replyInfo->GetGenerativeTaleReply().GetTaleState().GetActiveQuestion();
    const auto& taleState = classificationResult.GetReplyInfo().GetGenerativeTaleReply().GetTaleState();
    auto stage = taleState.GetStage();
    TNlgData nlgData{contextWrapper.Logger(), contextWrapper.RequestWrapper()};

    if (IsIn({TGenerativeTaleState::SendMeMyTale, TGenerativeTaleState::SharingDone}, stage)) {
        nlgData.Context["stage"] = GetStageName(stage);
        nlgData.Context["tale_name"] = taleState.GetTaleName();
        nlgData.Context["social_sharing_link"] = taleState.GetSharedLink();
        nlgData.Context["has_obscene"] = taleState.GetHasObscene();
        nlgData.Context["has_silence"] = taleState.GetHasSilence();
        nlgData.Context["got_tale_name_from_user"] = taleState.GetGotTaleNameFromUser();
        nlgData.Context["client_can_render_div2_cards"] = contextWrapper.RequestWrapper().Interfaces().GetCanRenderDiv2Cards();
        nlgData.Context["has_tale_name_and_text"] = !taleState.GetTaleName().empty() && !taleState.GetText().empty();
        const auto renderedPhrase = contextWrapper.NlgWrapper().RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_generative_tale", nlgData);
        replyInfo->SetRenderedText(renderedPhrase.Text);
        replyInfo->SetRenderedVoice(renderedPhrase.Voice);
        return;
    }

    if (stage == TGenerativeTaleState::Sharing) {
        taleText = TaleDropEnd(taleText);
    }
    if (taleState.GetOpenQuestions()) {
        const size_t rPos = taleQuestion.rfind(":");
        if (rPos != TString::npos) {
            taleQuestion = taleQuestion.resize(rPos) + "?";
        }
    }

    PreprocessTaleText(taleText);

    int nonResponseCounter = taleState.GetBadSeq2SeqCounter();
    if (taleText.empty()) {
        stage = MoveToPreviousQuestion(stage);

        nonResponseCounter += 1;
        if (nonResponseCounter >= MAX_BAD_GEN_REQUESTS) {
            stage = TGenerativeTaleState::Error;
        }
    } else {
        nonResponseCounter = 0;
    }

    const auto taleQuestionVoice = MakeQuestionVoice(taleQuestion);

    nlgData.Context["character"] = replyInfo->GetGenerativeTaleReply().GetCharacter();
    nlgData.Context["tale"] = taleText;
    if (stage != TGenerativeTaleState::Sharing) {
        nlgData.Context["question"] = taleQuestion;
        nlgData.Context["question_voice"] = taleQuestionVoice;
        nlgData.Context["user_turn"] = taleQuestion == TALE_QUESTION_POSTFIX;
    }
    nlgData.Context["stage"] = GetStageName(stage);
    nlgData.Context["obscene_prefix"] = taleState.GetHasObscene();
    nlgData.Context["client_can_render_div2_cards"] = contextWrapper.RequestWrapper().Interfaces().GetCanRenderDiv2Cards();
    nlgData.Context["is_logged_in"] = taleState.GetIsLoggedIn();

    bool needOnboarding = NeedTalesOnboarding(contextWrapper, taleState);
    nlgData.Context["onboarding"] = needOnboarding;

    const auto renderedPhrase = contextWrapper.NlgWrapper().RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_generative_tale", nlgData);
    replyInfo->SetRenderedText(renderedPhrase.Text);
    replyInfo->SetRenderedVoice(renderedPhrase.Voice);
    replyInfo->MutableGenerativeTaleReply()->MutableTaleState()->SetStage(stage);
    replyInfo->MutableGenerativeTaleReply()->MutableTaleState()->SetBadSeq2SeqCounter(nonResponseCounter);
    replyInfo->MutableGenerativeTaleReply()->MutableTaleState()->SetHadOnboarding(taleState.GetHadOnboarding() || needOnboarding);
}

} // namespace

void TGeneralConversationContinueCandidatesHandle::Do(TScenarioHandleContext& ctx) const {
    TGeneralConversationApplyContextWrapper contextWrapper(&ctx);
    auto sessionState = GetOnlyProtoOrThrow<TSessionState>(ctx.ServiceCtx, STATE_SESSION);
    auto classificationResult = GetOnlyProtoOrThrow<TClassificationResult>(ctx.ServiceCtx, STATE_CLASSIFICATION_RESULT);
    auto& replyInfo = *classificationResult.MutableReplyInfo();

    if (classificationResult.GetHasGenerativeTaleRequest()) {
        if (auto seq2seqResponse = RetireReplySeq2SeqCandidatesResponse(ctx); seq2seqResponse.Defined() && !seq2seqResponse->empty()) {
            ParseGenerativeTaleQuestion(seq2seqResponse.GetRef(), replyInfo);
        } else if (ctx.ServiceCtx.HasProtobufItem(SOCIAL_SHARING_LINK_CREATE_RESPONSE_ITEM) &&
                   ctx.ServiceCtx.HasProtobufItem(SOCIAL_SHARING_LINK_COMMIT_RESPONSE_ITEM))
        {
            const auto& createResponseItem =
                    GetOnlyProtoOrThrow<TCreateCandidateResponse>(ctx.ServiceCtx, SOCIAL_SHARING_LINK_CREATE_RESPONSE_ITEM);
            const auto& commitResponseItem =
                    GetOnlyProtoOrThrow<TCommitCandidateResponse>(ctx.ServiceCtx, SOCIAL_SHARING_LINK_COMMIT_RESPONSE_ITEM);

            if (commitResponseItem.HasOk()) {
                replyInfo.MutableGenerativeTaleReply()->MutableTaleState()->SetSharedLink(createResponseItem.GetLink().GetUrl());
            }
        }
    }

    switch (replyInfo.GetReplySourceCase()) {
        case TReplyInfo::ReplySourceCase::kGenerativeTaleReply:
            PatchReplyWithGenerativeTale(classificationResult, contextWrapper, &replyInfo);
            break;
        case TReplyInfo::ReplySourceCase::kAggregatedReply:
        case TReplyInfo::ReplySourceCase::kProactivityReply:
        case TReplyInfo::ReplySourceCase::kGenericStaticReply:
        case TReplyInfo::ReplySourceCase::kMovieAkinatorReply:
        case TReplyInfo::ReplySourceCase::kEasterEggReply:
        case TReplyInfo::ReplySourceCase::kGenerativeToastReply:
        case TReplyInfo::ReplySourceCase::kSeq2SeqReply:
        case TReplyInfo::ReplySourceCase::kNlgSearchReply:
        case TReplyInfo::ReplySourceCase::REPLYSOURCE_NOT_SET: {
            Y_ENSURE(false);
            break;
        }
    }

    TReplyState replyState;
    *replyState.MutableReplyInfo() = std::move(replyInfo);
    LOG_INFO(contextWrapper.Logger()) << "Reply state: " << SerializeProtoText(replyState);
    ctx.ServiceCtx.AddProtobufItem(replyState, STATE_REPLY);
}

} // namespace NAlice::NHollywood::NGeneralConversation

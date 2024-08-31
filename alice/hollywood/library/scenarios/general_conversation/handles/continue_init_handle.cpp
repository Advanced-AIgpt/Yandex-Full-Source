#include "continue_init_handle.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/generative_tale_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/entity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/search_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/seq2seq_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>
#include <alice/hollywood/library/scenarios/general_conversation/classification/frame_classifier.h>
#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_resources.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/response/response_builder.h>

#include <alice/paskills/social_sharing/proto/api/api.pb.h>
#include <alice/paskills/social_sharing/proto/api/directives.pb.h>
#include <alice/paskills/social_sharing/proto/api/web_page.pb.h>

#include <alice/library/logger/logger.h>
#include <alice/library/proto/proto.h>

#include <util/string/subst.h>


using namespace NAlice::NScenarios;
using namespace NAlice::NSocialSharing;

namespace NAlice::NHollywood::NGeneralConversation {


namespace {

class TTaleTextPreprocessor {
public:
    TTaleTextPreprocessor(const TUtf16String& taleText)
        : Symbols_(taleText.begin(), taleText.end())
        , IndicesToAddNewLinesForDashes_()
    {
    }

    TUtf16String Run() && {
        FixNewLinesBeforeDashes();
        AddDotInTheEndIfNoPunctuationIsPresent();
        return TUtf16String(Symbols_.begin(), Symbols_.end());;
    }

private:
    void AddDotInTheEndIfNoPunctuationIsPresent() {
        while (!Symbols_.empty() && IsIn(u" \n", Symbols_.back())) {
            Symbols_.pop_back();
        }
        if (!Symbols_.empty() && !IsIn(PUNCTUATION, Symbols_.back())) {
            Symbols_.push_back(u'.');
        }
    }

    void FixNewLinesBeforeDashes() {
        ComputeIndicesToAddNewLinesForDashes();
        DoAddNewLinesBeforeDashes();
    }

    void ComputeIndicesToAddNewLinesForDashes() {
        int currentState = NONE_STATE;
        for (int i = 0; i < Symbols_.ysize(); ++i) {
            if (
                (currentState == NONE_STATE && IsIn(PUNCTUATION, Symbols_[i])) ||
                (currentState == PUNCT_STATE && Symbols_[i] == u' ') ||
                (currentState == PUNCT_SPACE_STATE && Symbols_[i] == u'—') ||
                (currentState == PUNCT_SPACE_DASH_STATE && Symbols_[i] == u' ')
            ) {
                ++currentState;
            } else if (currentState == PUNCT_SPACE_DASH_SPACE_STATE && IsUpperCaseRussianLetter(Symbols_[i])) {
                IndicesToAddNewLinesForDashes_.push_back(i - 3);
                currentState = NONE_STATE;
            } else {
                currentState = NONE_STATE;
            }
        }
    }

    void DoAddNewLinesBeforeDashes() {
        for (int i : IndicesToAddNewLinesForDashes_) {
            Symbols_[i] = u'\n';
        }
    }

    static bool IsUpperCaseRussianLetter(const wchar16 ch) {
        return u'А' <= ch && ch <= u'Я';
    }

private:
    TVector<wchar16> Symbols_;
    TVector<int> IndicesToAddNewLinesForDashes_;

    static constexpr int NONE_STATE = 0;
    static constexpr int PUNCT_STATE = 1;
    static constexpr int PUNCT_SPACE_STATE = 2;
    static constexpr int PUNCT_SPACE_DASH_STATE = 3;
    static constexpr int PUNCT_SPACE_DASH_SPACE_STATE = 4;
    static constexpr TWtringBuf PUNCTUATION = u".!?";
};

class TGenerativeTaleContinueRequestContextSetter {
public:
    TGenerativeTaleContinueRequestContextSetter(TClassificationResult& classificationResult,
                                                const TScenarioApplyRequestWrapper& requestWrapper,
                                                TGeneralConversationApplyContextWrapper& contextWrapper)
        : ClassificationResult(classificationResult)
        , RequestWrapper(requestWrapper)
        , ContextWrapper(contextWrapper)
    {
    }

    void SetContext() && {
        if (IsInGenerativeStage()) {
            ComputeTalePrefix();
            AddReplySeq2SeqCandidatesRequest(ToString(GENERATIVE_TALE_URL), ContextWrapper, TalePrefix, TALE_NUM_HYPOS);
        } else if (IsInCompletingSharingStage()) {
            AddCreateAndCommitSharedLinkRequest();
        }
    }

private:
    const auto& TaleState() const {
        return ClassificationResult.GetReplyInfo().GetGenerativeTaleReply().GetTaleState();
    }

    auto& MutableTaleState() {
        return *ClassificationResult.MutableReplyInfo()->MutableGenerativeTaleReply()->MutableTaleState();
    }

    bool IsInGenerativeStage() const {
        return IsIn(
            {
                TGenerativeTaleState::FirstQuestion,
                TGenerativeTaleState::ClosedQuestion,
                TGenerativeTaleState::OpenQuestion,
                TGenerativeTaleState::Sharing,
                TGenerativeTaleState::UndefinedQuestion,
            },
            TaleState().GetStage()
        );
    }

    bool IsInCompletingSharingStage() const {
        return IsIn(
            {
                TGenerativeTaleState::SendMeMyTale,
                TGenerativeTaleState::SharingDone,
            },
            TaleState().GetStage()
        );
    }

    void ComputeTalePrefix() {
        if (TaleState().GetText().empty()) {
            ComputeInitialTalePrefix();
        } else {
            ComputeContinuedTalePrefix();
        }
    }

    void ComputeInitialTalePrefix() {
        const auto mainCharacter = ClassificationResult.GetReplyInfo().GetGenerativeTaleReply().GetCharacter();
        if (mainCharacter) {
            TalePrefix = TALE_INIT_PREFIX + mainCharacter + "\n";
        } else{
            TalePrefix = TALE_BAN_PREFIX;
        }
    }

    void ComputeContinuedTalePrefix() {
        auto inputText = GetUtterance(RequestWrapper, true);
        if (TaleState().GetSkipUtterance() || TaleState().GetBadSeq2SeqCounter() == MAX_BAD_GEN_REQUESTS - 1) {
            const auto& activeAnswers = TaleState().GetActiveAnswers();
            inputText = activeAnswers.empty() ? "" : activeAnswers[0];
        }

        if (TaleState().GetStage() == TGenerativeTaleState::UndefinedQuestion) {
            TalePrefix = TaleState().GetText();
        } else {
            TalePrefix = TaleAddQuestion(TaleState().GetActiveQuestion(), inputText, TaleState().GetText());
        }
    }

    void AddCreateAndCommitSharedLinkRequest() {
        auto request = AssembleSocialSharingLinkCandidateCreateRequest();
        ContextWrapper.Ctx()->ServiceCtx.AddProtobufItem(request, SOCIAL_SHARING_LINK_CREATE_REQUEST_ITEM);
    }

    TString PrepareTaleTextToPage(TString taleText) const {
        size_t idx = taleText.find("\n");
        if (idx != TString::npos) {
            taleText = taleText.substr(idx + 1);
        }

        return taleText;
    }

    TCreateCandidateRequest AssembleSocialSharingLinkCandidateCreateRequest() {
        TCreateCandidateRequest result;
        auto& scenarioSharePage = *result.MutableCreateSocialLinkDirective()->MutableScenarioSharePage();
        auto& fairyTalePage = *scenarioSharePage.MutableFairyTaleTemplate();
        *scenarioSharePage.MutableFrame()->MutableActivateGenerativeTaleSemanticFrame() = TActivateGenerativeTaleSemanticFrame();

        auto& pageContent = *fairyTalePage.MutablePageContent();
        const auto taleText = WideToUTF8(TTaleTextPreprocessor(UTF8ToWide(TaleState().GetText())).Run());
        pageContent.SetText(PrepareTaleTextToPage(taleText));
        pageContent.SetTitle(TaleState().GetTaleName());
        pageContent.MutableImage()->SetAvatarsId(TaleState().GetAvatarsIdForSharedLink());
        return result;
    }

private:
    TClassificationResult& ClassificationResult;
    const TScenarioApplyRequestWrapper& RequestWrapper;
    TGeneralConversationApplyContextWrapper& ContextWrapper;

    TString TalePrefix;
};

}


void TGeneralConversationContinueInitHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper requestWrapper(requestProto, ctx.ServiceCtx);

    TSessionState sessionState;
    requestWrapper.Proto().GetBaseRequest().GetState().UnpackTo(&sessionState);
    TGeneralConversationContinueArguments continueArguments;
    TClassificationResult classificationResult;
    if (!requestWrapper.Proto().GetArguments().UnpackTo(&continueArguments)) {
        requestWrapper.Proto().GetArguments().UnpackTo(&classificationResult);
    } else {
        classificationResult = continueArguments.GetClassificationResult();
    }

    TGeneralConversationApplyContextWrapper contextWrapper(&ctx);
    if (classificationResult.GetHasGenerativeTaleRequest()) {
        TGenerativeTaleContinueRequestContextSetter(classificationResult, requestWrapper, contextWrapper).SetContext();
    }

    ctx.ServiceCtx.AddProtobufItem(classificationResult, STATE_CLASSIFICATION_RESULT);
    ctx.ServiceCtx.AddProtobufItem(sessionState, STATE_SESSION);
}

}  // namespace NAlice::NHollywood::NGeneralConversation

#include "generative_tale_strategy.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/generative_tale_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/render_utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>

#include <alice/hollywood/library/base_scenario/scenario.h>

#include <alice/library/logger/logger.h>

#include <alice/megamind/protos/common/data_source_type.pb.h>

#include <library/cpp/iterator/zip.h>
#include <util/random/shuffle.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

constexpr ui32 CHARACTER_SUGGESTS = 5;

bool AddUserConfigs(TMementoChangeUserObjectsDirective& mementoDirective, NMemento::EConfigKey key, const NProtoBuf::Message& value) {
    NMemento::TConfigKeyAnyPair mementoConfig;
    mementoConfig.SetKey(key);
    if (mementoConfig.MutableValue()->PackFrom(value)) {
        *mementoDirective.MutableUserObjects()->AddUserConfigs() = std::move(mementoConfig);
        return true;
    }
    
    return false;
}

TString GlueTalePrefixWithNextPart(TString talePrefix, const TString& nextPart) {
    if (nextPart.empty()) {
        return talePrefix;
    }
    if (talePrefix.empty()) {
        return nextPart;
    }

    if (!IsIn(" \n", talePrefix.back())) {
        const wchar_t nextPartFirstSymbol = UTF8ToWide(nextPart)[0];
        const bool nextPartFirstSymbolIsACapitalLetter = u'А' <= nextPartFirstSymbol && nextPartFirstSymbol <= u'Я';
        if (nextPartFirstSymbolIsACapitalLetter && !IsIn(".,!?:", talePrefix.back())) {
            talePrefix.push_back('.');
        }
        talePrefix.push_back(' ');
    }
    return talePrefix + nextPart;
}

}   // namespace

template <typename TContextWrapper>
TGenerativeTaleRenderStrategy<TContextWrapper>::TGenerativeTaleRenderStrategy(TContextWrapper& contextWrapper, const TClassificationResult& classificationResult, const TReplyInfo& replyInfo)
    : ContextWrapper_(contextWrapper)
    , ClassificationResult_(classificationResult)
    , ReplyInfo_(replyInfo)
    , MementoGenerativeTale_(contextWrapper.RequestWrapper().BaseRequestProto().GetMemento().GetUserConfigs().GetGenerativeTale())
    , Interfaces_(ContextWrapper_.RequestWrapper().Proto().GetBaseRequest().GetInterfaces())
{
    Y_ENSURE(replyInfo.GetReplySourceCase() == TReplyInfo::ReplySourceCase::kGenerativeTaleReply);
    LOG_INFO(ContextWrapper_.Logger()) << "ReplySourceRenderStrategy: GenerativeTaleRenderStrategy";
}


template <typename TContextWrapper>
void TGenerativeTaleRenderStrategy<TContextWrapper>::AddResponse(TGeneralConversationResponseWrapper* responseWrapper) {
    const auto& taleReply = ReplyInfo_.GetGenerativeTaleReply();
    TString taleText = taleReply.GetText();
    TString character = taleReply.GetCharacter();

    TGenerativeTaleState taleState = taleReply.GetTaleState();
    TString taleQuestion = taleState.GetActiveQuestion();
    const auto& stage = taleState.GetStage();

    if (stage == TGenerativeTaleState::Sharing) {
        taleText = TaleDropEnd(taleText);
    }

    auto talePrefix = taleState.GetText();
    if (talePrefix.empty() && !IsIn({TGenerativeTaleState::Undefined, TGenerativeTaleState::UndefinedCharacter}, taleState.GetStage())) {
        talePrefix = character.empty() ? TALE_BAN_PREFIX : TALE_INIT_PREFIX + character + ".\n";
    }

    taleState.SetText(GlueTalePrefixWithNextPart(talePrefix, taleText));

    taleState.SetActiveQuestion(taleQuestion);
    *responseWrapper->SessionState.MutableGenerativeTaleState() = taleState;

    TNlgData nlgData{ContextWrapper_.Logger(), ContextWrapper_.RequestWrapper()};
    nlgData.Context["frame"] = ClassificationResult_.GetRecognizedFrame().GetName();
    nlgData.Context["intent"] = ReplyInfo_.GetIntent();
    TString renderedText = ReplyInfo_.GetRenderedText();
    SubstGlobal(renderedText, "\n", " \\n ");
    nlgData.Context["rendered_text"] = renderedText;
    nlgData.Context["rendered_voice"] = ReplyInfo_.GetRenderedVoice();
    nlgData.Context["fairytale_url"] = taleState.GetSharedLink();
    nlgData.Context["tale_name"] = taleState.GetTaleName();
    nlgData.Context["image_url"] = MakeSharedLinkImageUrl(taleState.GetAvatarsIdForSharedLink());

    TResponseBodyBuilder* bodyBuilder;
    if constexpr (std::is_same_v<TContextWrapper, TGeneralConversationRunContextWrapper>) {
        bodyBuilder = responseWrapper->Builder.GetResponseBodyBuilder();
    } else {
        bodyBuilder = responseWrapper->ContinueBuilder.GetResponseBodyBuilder();
    }

    if (IsIn(GENERATIVE_TALE_TERMINAL_STAGES, stage)) {
        responseWrapper->SessionState.ClearGenerativeTaleState();
        bodyBuilder->SetExpectsRequest(false);
    } else {
        bodyBuilder->SetExpectsRequest(true);
    }

    bool newActivation = ClassificationResult_.GetRecognizedFrame().GetName() == FRAME_GENERATIVE_TALE;
    if (newActivation && Interfaces_.GetSupportsCloudUi() && Interfaces_.GetCanRenderDiv2Cards()) {
        nlgData.Context["init_image_url"] = MakeSharedLinkImageUrl(taleState.GetAvatarsIdForSharedLink(), true);
        bodyBuilder->AddRenderedDiv2Card(GENERAL_CONVERSATION_SCENARIO_NAME, "tale_image_card", nlgData);
    }

    bodyBuilder->AddRenderedTextWithButtonsAndVoice(GENERAL_CONVERSATION_SCENARIO_NAME, NLG_RENDER_GENERIC_STATIC_REPLY, /* buttons = */ {}, nlgData);

    if (IsIn({TGenerativeTaleState::SharingDone, TGenerativeTaleState::SendMeMyTale}, stage) &&
        Interfaces_.GetCanRenderDiv2Cards())
    {
        bodyBuilder->AddRenderedDiv2Card(GENERAL_CONVERSATION_SCENARIO_NAME, "share_generative_tale_card", nlgData);
    }

    if (stage == TGenerativeTaleState::Undefined && !taleState.GetHasObscene()) {
        MementoGenerativeTale_.SetUsageCounter(MementoGenerativeTale_.GetUsageCounter() + 1);
        UpdateMementoGenerativeTale(bodyBuilder);
    } else if (stage == TGenerativeTaleState::SharingDone) {
        MementoGenerativeTale_.SetTaleName(taleState.GetTaleName());
        MementoGenerativeTale_.SetTaleText(taleState.GetText());
        UpdateMementoGenerativeTale(bodyBuilder);
    }

    const auto silenceMayBeHandled = (
        ContextWrapper_.RequestWrapper().HasExpFlag(EXP_HW_GENERATIVE_TALE_HANDLE_SILENCE) &&
        Interfaces_.GetHasDirectiveSequencer()
    );
    if (stage == TGenerativeTaleState::SharingAskTaleName && silenceMayBeHandled) {
        AddListenDirectiveWithCallback(20'000, bodyBuilder);
    }
}

template <typename TContextWrapper>
void TGenerativeTaleRenderStrategy<TContextWrapper>::MakeSharingSuggests(TResponseBodyBuilder& responseBodyBuilder) const {
    const auto canRenderDiv2Cards = Interfaces_.GetCanRenderDiv2Cards();
    const auto isLoggedIn = ReplyInfo_.GetGenerativeTaleReply().GetTaleState().GetIsLoggedIn();
    const auto taleCanBeShared = canRenderDiv2Cards || isLoggedIn;

    TVector<std::pair<TString, TString>> framesNamesWithSuggests;

    if (taleCanBeShared) {
        const auto confirmSharingSuggest = canRenderDiv2Cards ? "Отправить сказку" : "Отправить сказку в телефон";
        framesNamesWithSuggests.emplace_back(TString(FRAME_GENERATIVE_TALE_CONFIRM_SHARING), confirmSharingSuggest);
    }

    framesNamesWithSuggests.emplace_back(TString(FRAME_GENERATIVE_TALE_STOP), taleCanBeShared ? "" : "Хватит");
    framesNamesWithSuggests.emplace_back(TString(FRAME_GENERATIVE_TALE_CONTINUE_GENERATING_TALE), "Продолжить сочинять");
    framesNamesWithSuggests.emplace_back(TString(FRAME_PROACTIVITY_DECLINE), "");

    for (const auto& [frameName, suggestTitle] : framesNamesWithSuggests) {
        AddAction(frameName, responseBodyBuilder);
        if (!suggestTitle.empty()) {
            AddSuggest("suggest_" + frameName, suggestTitle, ToString(SUGGEST_TYPE), false, responseBodyBuilder);
        }
    }
}

template <typename TContextWrapper>
void TGenerativeTaleRenderStrategy<TContextWrapper>::AddSuggests(TGeneralConversationResponseWrapper* responseWrapper) {
    auto& nlgWrapper = ContextWrapper_.NlgWrapper();
    TNlgData nlgData{ContextWrapper_.Logger(), ContextWrapper_.RequestWrapper()};

    const auto& taleState = ReplyInfo_.GetGenerativeTaleReply().GetTaleState();
    const auto stage = taleState.GetStage();
    const auto& intent = ReplyInfo_.GetIntent();

    if constexpr (std::is_same_v<TContextWrapper, TGeneralConversationRunContextWrapper>) {
        auto& responseBodyBuilder = *responseWrapper->Builder.GetResponseBodyBuilder();
        if (intent == INTENT_GENERATIVE_TALE_CHARACTER || intent == INTENT_GENERATIVE_TALE_BANLIST_ACTIVATION) {
            AddAction(ToString(FRAME_GENERATIVE_TALE_CHARACTER), responseBodyBuilder);
            AddAction(ToString(FRAME_GENERATIVE_TALE_STOP), responseBodyBuilder);
            AddAction(ToString(FRAME_GC_FORCE_EXIT), responseBodyBuilder);
            AddAction(ToString(FRAME_GENERATIVE_TALE), responseBodyBuilder);
            for (const auto& frameName : WIZ_DETECTION_FRAMES) {
                AddAction(ToString(frameName), responseBodyBuilder);
            }
            auto& rng = ContextWrapper_.Ctx()->Rng;
            auto shuffledCharacters = CHARACTERS;
            ShuffleRange(shuffledCharacters, rng);
            for (size_t i = 0; i < CHARACTER_SUGGESTS; ++i) {
                AddSuggest("suggest_generative_tale_character_" + ToString(i), shuffledCharacters[i], ToString(SUGGEST_TYPE),
                           /* forceGcResponse= */ false, responseBodyBuilder);
            }
        } else if (stage == TGenerativeTaleState::SharingAskTaleName) {
            AddAction(ToString(FRAME_GENERATIVE_TALE_TALE_NAME), responseBodyBuilder);
            if (const auto& character = taleState.GetCharacter()) {
                const auto suggestText = "Сказка про " + character;
                AddSuggest("suggest_" + ToString(FRAME_GENERATIVE_TALE_TALE_NAME), suggestText, ToString(SUGGEST_TYPE),
                           /* forceGcResponse= */ false, responseBodyBuilder);
            }
        } else if (stage == TGenerativeTaleState::SharingReask) {
            MakeSharingSuggests(responseBodyBuilder);
        }
    } else {
        auto& responseBodyBuilder = *responseWrapper->ContinueBuilder.GetResponseBodyBuilder();
        const auto& renderedPhrase = nlgWrapper.RenderPhrase(GENERAL_CONVERSATION_SCENARIO_NAME, "render_tales_deactivate_suggest", nlgData).Text;
        if (IsIn({TGenerativeTaleState::FirstQuestion, TGenerativeTaleState::ClosedQuestion, TGenerativeTaleState::OpenQuestion, TGenerativeTaleState::UndefinedQuestion}, stage)) {
            AddSuggest("suggest_tale_deactivate", renderedPhrase, ToString(SUGGEST_TYPE),
                   /* forceGcResponse= */ false, responseBodyBuilder);
        }

        if (!taleState.GetOpenQuestions() && IsIn({TGenerativeTaleState::FirstQuestion, TGenerativeTaleState::ClosedQuestion}, stage)) {
            const auto& taleAnswers = ReplyInfo_.GetGenerativeTaleReply().GetTaleState().GetActiveAnswers();
            for (int i = 0; i < taleAnswers.size(); ++i) {
                AddSuggest("suggest_generative_tale_answer_" + ToString(i), taleAnswers[i], ToString(SUGGEST_TYPE),
                           /* forceGcResponse= */ false, responseBodyBuilder);
            }
        } else if (stage == TGenerativeTaleState::Sharing) {
            MakeSharingSuggests(responseBodyBuilder);
        } else if (IsIn({TGenerativeTaleState::SharingDone, TGenerativeTaleState::SendMeMyTale}, stage) &&
                   !Interfaces_.GetCanRenderDiv2Cards() &&
                   taleState.GetIsLoggedIn())
        {
            AddPush(responseBodyBuilder, "Сказка с Алисой", "Поделитесь сказкой, которую мы сочинили",
                    taleState.GetSharedLink(), MakeSharedLinkImageUrl(taleState.GetAvatarsIdForSharedLink()),
                    "alice_generative_tale");
        }
    }
}


template <typename TContextWrapper>
bool TGenerativeTaleRenderStrategy<TContextWrapper>::ShouldListen() const {
    const auto& taleState = ReplyInfo_.GetGenerativeTaleReply().GetTaleState();
    const auto& stage = taleState.GetStage();
    return !IsIn(GENERATIVE_TALE_TERMINAL_STAGES, stage);
}

template <typename TContextWrapper>
void TGenerativeTaleRenderStrategy<TContextWrapper>::UpdateMementoGenerativeTale(TResponseBodyBuilder* bodyBuilder) {
    TMementoChangeUserObjectsDirective mementoDirective;
    if (AddUserConfigs(mementoDirective, NMemento::EConfigKey::CK_GENERATIVE_TALE, MementoGenerativeTale_)) {
        *bodyBuilder->GetResponseBody().AddServerDirectives()->MutableMementoChangeUserObjectsDirective() = std::move(mementoDirective);
    } else {
        LOG_ERROR(ContextWrapper_.Logger()) << "Generative tales: UpdateMementoGenerativeTale failed";
    }
}

template class TGenerativeTaleRenderStrategy<TGeneralConversationRunContextWrapper>;
template class TGenerativeTaleRenderStrategy<TGeneralConversationApplyContextWrapper>;

} // namespace NAlice::NHollywood::NGeneralConversation

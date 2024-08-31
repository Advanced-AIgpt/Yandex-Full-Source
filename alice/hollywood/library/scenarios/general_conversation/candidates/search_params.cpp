#include "search_params.h"

#include <alice/hollywood/library/scenarios/general_conversation/candidates/context_wrapper.h>

#include <alice/hollywood/library/scenarios/general_conversation/candidates/entity_candidates.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/utils.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/consts.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/entity.h>
#include <alice/hollywood/library/scenarios/general_conversation/common/flags.h>
#include <alice/hollywood/library/scenarios/general_conversation/candidates/aggregated_candidates.h>

#include <library/cpp/string_utils/quote/quote.h>

#include <util/string/builder.h>

namespace NAlice::NHollywood::NGeneralConversation {

namespace {

const TStringBuf SEARCH_PATH = "/yandsearch?g=0..100&ms=proto&hr=json&fsgta=_Seq2SeqResult&fsgta=_NlgSearchFactorsResult";

void PatchParamsAfterProactivity(TGeneralConversationRunContextWrapper& contextWrapper, const TClassificationResult& classificationResult,
        const TSessionState& sessionState, TSearchParams* params) {
    const auto& frameName = classificationResult.GetRecognizedFrame().GetName();
    const auto& requestWrapper = contextWrapper.RequestWrapper();
    TStringBuf entityBoostFlag;
    if (frameName == FRAME_MOVIE_DISCUSS || frameName == FRAME_LETS_DISCUSS_SPECIFIC_MOVIE) {
        entityBoostFlag = EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_ENTITY_BOOST;
    } else if (frameName == FRAME_YES_I_WATCH_IT) {
        entityBoostFlag = EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SPECIFIC_WATCHED_ENTITY_BOOST;
    } else if (frameName == FRAME_NO_I_DID_NOT_WATCH_IT) {
        entityBoostFlag = EXP_HW_GC_PROACTIVITY_MOVIE_DISCUSS_SPECIFIC_NOT_WATCHED_ENTITY_BOOST;
    } else {
        return;
    }

    auto entityBoost = GetExperimentTypedValue<double>(requestWrapper.ExpFlags(), entityBoostFlag);
    if (!entityBoost && IsMovieDisscussionAllowedByDefault(requestWrapper, sessionState.GetModalModeEnabled())) {
        entityBoost = 15;
    }

    if (entityBoost) {
        params->SetParam("EntityBoost", ToString(entityBoost.GetRef()));
    }
}

void PatchParamsForModalMode(TGeneralConversationRunContextWrapper& contextWrapper,
        const TSessionState& sessionState, TSearchParams* params) {
    if (!sessionState.GetModalModeEnabled()) {
        return;
    }
    const auto& requestWrapper = contextWrapper.RequestWrapper();
    params->SetParam("MaxResults", "54");
    params->SetParam("ProactivityBoost", "0.0");
    params->SetParam("RankByLinearCombination", "false");
    params->SetParam("Seq2SeqCandidatesEnabled", "false");
    params->SetParam("TfRankerEnabled", "false");
    params->Patch(requestWrapper.ExpFlags(), EXP_HW_GC_MODAL_REPLY_PREFIX);
}

} // namespace

TSearchParams::TSearchParams(const TString& context, const THashMap<TString, TString>& params)
    : Context(context)
    , Params(params)
{
}

void TSearchParams::SetContext(const TString& context) {
    Context = context;
}

void TSearchParams::SetParam(const TString& key, const TString& value) {
    Params[key] = value;
}

void TSearchParams::Patch(const TExpFlags& expFlags, const TStringBuf& prefix) {
    for (const auto& [key, _] : expFlags) {
        TStringBuf afterPrefix;
        if (TStringBuf{key}.AfterPrefix(prefix, afterPrefix)) {
            TStringBuf paramKey;
            TStringBuf paramValue;
            if (afterPrefix.NextTok('=', paramKey) && afterPrefix.NextTok('=', paramValue)) {
                Params[paramKey] = paramValue;
            }
        }
    }
}

TString TSearchParams::ToString() const {
    TStringBuilder relevParams;
    for (const auto& [key, value] : Params) {
        relevParams << key << '=' << value << ';';
    }

    TStringBuilder url;
    url << SEARCH_PATH;
    url << "&relev=" << CGIEscapeRet(relevParams);
    url << "&text=" << CGIEscapeRet(Context);

    return url;
}

TSearchParams ConstructReplyCandidatesParams(TGeneralConversationRunContextWrapper& contextWrapper, const TString& context,
                                             const TSessionState& sessionState, const TClassificationResult& classificationResult) {
    const auto& requestWrapper = contextWrapper.RequestWrapper();
    TSearchParams params {
        /* Context = */ context,
        /* Params = */ {
            {"MaxResults", "10"},
            {"MinRatioWithBestResponse", "1.0"},
            {"SearchFor", "context_and_reply"},
            {"SearchBy", "context"},
            {"ContextWeight", "1.0"},
            {"RankerModelName", "catboost"},
            {"ProtocolRequest", ToString(true)},
            {"RandomSeed", ToString(requestWrapper.BaseRequestProto().GetRandomSeed())},
            {"ProactivityIndexEnabled", ToString(false)},
        }
    };
    const auto isMovieDissussionAllowedByDefault = IsMovieDisscussionAllowedByDefault(requestWrapper, sessionState.GetModalModeEnabled());
    if (isMovieDissussionAllowedByDefault) {
        params.SetParam("ProactivityBoost", ToString(0.5));
        params.SetParam("EntityRankerModelName", "catboost_movie_interest");
    }

    params.Patch(requestWrapper.ExpFlags(), EXP_HW_GC_REPLY_PREFIX);

    if (requestWrapper.HasExpFlag(EXP_HW_GC_SAMPLE_POLICY)) {
        params.SetParam("TfRankerLogitRelev", ToString(true));
    }

    const TEntity* entity = GetEntity(classificationResult.GetReplyInfo(), sessionState);
    const bool isEntityIndexAllowed = isMovieDissussionAllowedByDefault || requestWrapper.HasExpFlag(EXP_HW_ENABLE_GC_ENTITY_INDEX);
    if (entity && isEntityIndexAllowed) {
        auto entityKey = TStringBuilder{} << GetEntityKey(*entity);
        if (RequiresSentimentForDiscussion(*entity)) {
            entityKey << "_";
            const auto discussionSentiment = GetDiscussionSentiment(classificationResult.GetReplyInfo(), sessionState);
            if (discussionSentiment == TEntityDiscussion::NEGATIVE) {
                entityKey << "0";
            } else {
                entityKey << "1";
            }
        }

        params.SetParam("Entities", entityKey);
    } else if (isMovieDissussionAllowedByDefault || requestWrapper.HasExpFlag(EXP_HW_ENABLE_GC_PROACTIVITY)) {
        params.SetParam("ProactivityIndexEnabled", ToString(true));
    }
    if (classificationResult.GetIsAggregatedRequest()) {
        params.SetParam("TfRankerEnabled", ToString(false));
        params.SetParam("MaxResults", "54");
        params.SetParam("Seq2SeqCandidatesEnabled", ToString(false));
    }

    PatchParamsAfterProactivity(contextWrapper, classificationResult, sessionState, &params);
    PatchParamsForModalMode(contextWrapper, sessionState, &params);

    return params;
}

TSearchParams ConstructSuggestCandidatesParams(const TString& context, const TScenarioRunRequestWrapper& requestWrapper) {
    TSearchParams params {
        /* Context = */ context,
        /* Params = */ {
            {"MaxResults", "10"},
            {"MinRatioWithBestResponse", "1.0"},
            {"SearchFor", "context_and_reply"},
            {"SearchBy", "context"},
            {"ContextWeight", "1.0"},
            {"RankerModelName", ""},
            {"ProtocolRequest", ToString(true)},
            {"RandomSeed", ToString(requestWrapper.BaseRequestProto().GetRandomSeed())},
            {"Seq2SeqCandidatesEnabled", ToString(false)},
            {"ProactivityIndexEnabled", ToString(false)},
            {"UseBaseModelsOnly", ToString(true)}
        }
    };

    params.Patch(requestWrapper.ExpFlags(), EXP_HW_GC_SUGGEST_PREFIX);

    return params;
}
} // namespace NAlice::NHollywood::NGeneralConversation

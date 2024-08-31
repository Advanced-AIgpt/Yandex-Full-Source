#pragma once

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/registry/registry.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <memory>

namespace NAlice::NHollywood {

inline const TString SLOT_PERSUADE = "persuade";

class TBaseSuggestResponseBuilder {
public:
    struct TConfig {
        TVector<TString> AcceptedFrameNames;
        TString NlgTemplate;

        TString ConfirmGranetName;
        TMaybe<TString> ConfirmButtonTitle;

        TString DeclineGranetName;
        TVector<TString> DeclinePhrases;
        TMaybe<TString> DeclineButtonTitle;

        TString DeclineEffectFrameName;
        THashSet<TString> DeclineEffectFrameCopiedSlots;
    };

    TBaseSuggestResponseBuilder(TRTLogger& logger, const TScenarioRunRequestWrapper& request, const TConfig& config,
                                TRunResponseBuilder& builder);

    const TMaybe<TFrame>& GetFrame() const {
        return Frame;
    }

    TResponseBodyBuilder& GetBodyBuilder() {
        return BodyBuilder;
    }

    TMaybe<TString> TryGetSlotValue(const TStringBuf slotName) const;

    void AddNlg(const TMaybe<TString>& suggestText);

    void AddConfirmAction(TSemanticFrame&& frameEffect);
    void AddDeclineAction(const TMaybe<TString>& persuadeAboutItemId);

    std::unique_ptr<NScenarios::TScenarioRunResponse> BuildIrrelevantResponse() &&;

protected:
    TRunResponseBuilder& GetBuilder() {
        return Builder;
    }

private:
    TRTLogger& Logger;
    const TScenarioRunRequestWrapper& Request;
    const TConfig& Config;
    TRunResponseBuilder& Builder;

    const TMaybe<TFrame> Frame;
    TResponseBodyBuilder& BodyBuilder;

private:
    void AddDeclineAction(const TMaybe<TString>& persuadeAboutItemId, bool matchByHints);
};

template <typename TSuggesterState>
class TSuggestResponseBuilder : public TBaseSuggestResponseBuilder {
public:
    TSuggestResponseBuilder(TRTLogger& logger, const TScenarioRunRequestWrapper& request, const TConfig& config,
                            TRunResponseBuilder& builder)
        : TBaseSuggestResponseBuilder(logger, request, config, builder)
        , State(request.LoadState<TSuggesterState>())
    {
    }

    const TSuggesterState& GetState() const {
        return State;
    }

    TSuggesterState& GetState() {
        return State;
    }

    std::unique_ptr<NScenarios::TScenarioRunResponse> BuildSuccessResponse() &&;
    std::unique_ptr<NScenarios::TScenarioRunResponse> BuildNoMoreRecommendationsResponse() &&;

private:
    TSuggesterState State;
};

template <typename TSuggesterState>
std::unique_ptr<NScenarios::TScenarioRunResponse>
TSuggestResponseBuilder<TSuggesterState>::BuildSuccessResponse() && {
    auto response = std::move(GetBuilder()).BuildResponse();
    auto& responseBody = *response->MutableResponseBody();

    responseBody.MutableLayout()->SetShouldListen(true);
    responseBody.MutableState()->PackFrom(State);

    return response;
}

template <typename TSuggesterState>
std::unique_ptr<NScenarios::TScenarioRunResponse>
TSuggestResponseBuilder<TSuggesterState>::BuildNoMoreRecommendationsResponse() && {
    if (State.SuggestionsHistorySize() == 0) {
        return std::move(*this).BuildIrrelevantResponse();
    }

    AddNlg(/* suggestText= */ Nothing());

    auto response = std::move(GetBuilder()).BuildResponse();
    auto& responseBody = *response->MutableResponseBody();
    responseBody.MutableState()->PackFrom(State);

    return response;
}

} // namespace NAlice::NHollywood

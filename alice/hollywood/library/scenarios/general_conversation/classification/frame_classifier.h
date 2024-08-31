#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_fast_data.h>
#include <alice/hollywood/library/scenarios/general_conversation/containers/general_conversation_resources.h>
#include <alice/hollywood/library/scenarios/general_conversation/proto/general_conversation.pb.h>

#include <alice/hollywood/library/base_scenario/fwd.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/util/rng.h>

namespace NAlice::NHollywood::NGeneralConversation {

class TFrameClassifier {
public:
    explicit TFrameClassifier(const TGeneralConversationResources& resources, const TGeneralConversationFastData* fastData, const TScenarioRunRequestWrapper& requestWrapper, const TScenario::THandleBase& handle, const TSessionState& sessionState);

    TClassificationResult ClassifyRequest(TScenarioHandleContext& ctx) const;

private:
    void AddGcSensors(TScenarioHandleContext& ctx, float gcScore, bool isModal, bool isSmartSpeaker) const;
    bool DecideIsAggregatedRequestEnabled(TScenarioHandleContext& ctx, float gcScore) const;
    void SetDefaultClassificationResult(TScenarioHandleContext& ctx, TClassificationResult& classificationResult, float gcScore) const;
    bool TryDetectBanlist(TClassificationResult& classificationResult) const;
    bool TryDetectFeedback(TClassificationResult& classificationResult) const;
    bool TryDetectMicrointent(TClassificationResult& classificationResult) const;
    bool TryDetectProactivity(TScenarioHandleContext& ctx, TClassificationResult& classificationResult) const;
    bool TryDetectPureGcActivate(TClassificationResult& classificationResult) const;
    bool TryDetectPureGcDeactivate(TClassificationResult& classificationResult) const;
    bool TryDetectEntityDiscussRecognizedEntity(IRng& rng, TClassificationResult& classificationResult) const;
    bool TryDetectRequestForSpecificMovie(const TStringBuf frameName, IRng& rng, TClassificationResult& classificationResult, bool canAskQuestion = true) const;
    bool TryDetectRequestForSpecificMusic(const TStringBuf frameName, TClassificationResult& classificationResult) const;
    bool TryDetectRequestForSpecificGame(const TStringBuf frameName, TClassificationResult& classificationResult) const;
    bool TryDetectMovieDiscussWatched(const TStringBuf frameName, bool canAskQuestion, IRng& rng, TClassificationResult& classificationResult) const;
    bool TryDetectMovieDiscussIDontKnow(TClassificationResult& classificationResult) const;
    bool TryDetectLetsDiscussMovie(IRng& rng, TClassificationResult& classificationResult) const;
    bool TryDetectMovieAkinator(TClassificationResult& classificationResult) const;
    bool TryDetectEasterEgg(TClassificationResult& classificationResult) const;
    bool TryDetectGenerativeTales(TClassificationResult& classificationResult, IRng& rng, TRTLogger& logger) const;
    bool TryDetectGenerativeToast(IRng& rng, TClassificationResult& classificationResult) const;
    bool TryDetectViolation(TClassificationResult& classificationResult) const;
    bool TryDetectLocalBanlist(TClassificationResult& classificationResult) const;
    bool TryDetectNotRussian(const ELang lang, TClassificationResult& classificationResult) const;

private:
    const TGeneralConversationResources& Resources_;
    const TGeneralConversationFastData* FastData_;
    const TScenarioRunRequestWrapper& RequestWrapper_;
    const TScenario::THandleBase& Handle_;
    const TSessionState& SessionState_;
    const TMaybe<TFrame> CallbackFrame_;
};

} // namespace NAlice::NHollywood::NGeneralConversation


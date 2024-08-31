#pragma once

#include <alice/megamind/library/analytics/megamind_analytics_info.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/context/responses.h>
#include <alice/megamind/library/kv_saas/response.h>
#include <alice/megamind/library/session/session.h>
#include <alice/megamind/library/speechkit/request.h>

#include <alice/megamind/protos/modifiers/modifiers.pb.h>
#include <alice/megamind/protos/proactivity/proactivity.pb.h>

#include <alice/memento/proto/api.pb.h>

#include <alice/library/geo/user_location.h>
#include <alice/library/proto_eval/proto_eval.h>

#include <dj/services/alisa_skills/server/proto/client/proactivity_response.pb.h>
#include <dj/services/alisa_skills/server/proto/client/proactivity_request.pb.h>

#include <library/cpp/timezone_conversion/convert.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/random/fast.h>

namespace NAlice::NMegamind {

TMaybe<NSc::TValue> ParsePersonalData(const TStringBuf json);

class TResponseModifierContext final {
public:
    TResponseModifierContext(const IContext& ctx,
                             TModifiersStorage& modifiersStorage,
                             TModifiersInfo& modifiersInfo,
                             TProactivityLogStorage& proactivityLogStorage,
                             const TVector<TSemanticFrame>& semanticFrames,
                             const TVector<TSemanticFrame>& recognizedActionEffectFrames,
                             TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                             const TProactivityAnswer& proactivity);
    // constructor with custom random seed
    TResponseModifierContext(const IContext& ctx,
                             TModifiersStorage& modifiersStorage,
                             TModifiersInfo& modifiersInfo,
                             TProactivityLogStorage& proactivityLogStorage,
                             const TVector<TSemanticFrame>& semanticFrames,
                             const TVector<TSemanticFrame>& recognizedActionEffectFrames,
                             TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                             ui64 randomSeed,
                             const TProactivityAnswer& proactivity);

    const TSpeechKitRequest& SpeechKitRequest() const {
        return SpeechKitRequest_;
    }

    TMaybe<TString> ExpFlag(TStringBuf name) const {
        return SpeechKitRequest_.ExpFlag(name);
    }

    bool HasExpFlag(TStringBuf name) const {
        return SpeechKitRequest_.HasExpFlag(name);
    }

    const IContext::TExpFlags& ExpFlags() const {
        return SpeechKitRequest_.ExpFlags();
    }

    const TClientFeatures& ClientFeatures() const {
        return SpeechKitRequest_.ClientFeatures();
    }

    const TClientInfo& ClientInfo() const {
        return SpeechKitRequest_.ClientInfo();
    }

    const ISession* Session() const {
        return Session_;
    }

    const TMaybe<NSc::TValue>& PersonalData() const {
        return PersonalData_;
    }

    const TVector<TSemanticFrame>& RecognizedActionEffectFrames() const {
        return RecognizedActionEffectFrames_;
    }

    const TBlackBoxFullUserInfoProto& BlackBoxResponse() const {
        return Responses_.BlackBoxResponse();
    }

    const TWizardResponse& WizardResponse() const {
        return Responses_.WizardResponse();
    }

    const TModifiersStorage& ModifiersStorage() const {
        return ModifiersStorage_;
    }

    TModifiersStorage& ModifiersStorage() {
        return ModifiersStorage_;
    }

    const TModifiersInfo& ModifiersInfo() const {
        return ModifiersInfo_;
    }

    TModifiersInfo& ModifiersInfo() {
        return ModifiersInfo_;
    }

    const TProactivityLogStorage& ProactivityLogStorage() const {
        return ProactivityLogStorage_;
    }

    TProactivityLogStorage& ProactivityLogStorage() {
        return ProactivityLogStorage_;
    }

    const TVector<TSemanticFrame>& SemanticFrames() const {
        return SemanticFrames_;
    }

    TReallyFastRng32& Rng() {
        return Rng_;
    }

    TMegamindAnalyticsInfoBuilder& MegamindAnalyticsInfoBuilder() {
        return MegamindAnalyticsInfoBuilder_;
    }

    // Skill Recommender result for winner scenario/intent
    const TProactivityAnswer& Proactivity() const {
        return Proactivity_;
    }

    const ru::yandex::alice::memento::proto::TUserConfigs& UserConfigs() const {
        return UserConfigs_;
    }

    NMetrics::ISensors& Sensors() {
        return Sensors_;
    }

    TRTLogger& Logger() const {
        return Ctx.Logger();
    }

private:
    const IContext& Ctx;
    const TSpeechKitRequest SpeechKitRequest_;
    const ISession* const Session_;
    const TMaybe<NSc::TValue> PersonalData_;
    const TVector<TSemanticFrame> RecognizedActionEffectFrames_;
    const IResponses& Responses_;
    TModifiersStorage& ModifiersStorage_;
    TModifiersInfo& ModifiersInfo_;
    TReallyFastRng32 Rng_;
    const TProactivityAnswer& Proactivity_;
    TProactivityLogStorage& ProactivityLogStorage_;
    const TVector<TSemanticFrame>& SemanticFrames_;
    const ru::yandex::alice::memento::proto::TUserConfigs& UserConfigs_;
    TMegamindAnalyticsInfoBuilder& MegamindAnalyticsInfoBuilder_;
    NMetrics::ISensors& Sensors_;
};

} // namespace NAlice::NMegamind

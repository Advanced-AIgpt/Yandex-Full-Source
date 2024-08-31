#pragma once

#include "video_database.h"

#include <alice/hollywood/library/scenarios/video_recommendation/proto/video_recommendation_state.pb.h>

#include <alice/library/client/protos/client_info.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <google/protobuf/any.pb.h>

#include <library/cpp/json/json_value.h>

#include <util/generic/hash.h>

namespace NAlice::NHollywood {

class TStateUpdater {
public:
    struct TSlotDescription {
        TString SlotName;
        TVector<TString> AcceptedTypes;
    };

    using TFormDescription = TVector<TSlotDescription>;
    using TProtoState = ::google::protobuf::Any;

    TStateUpdater(const TString& formName, const TFormDescription& formDescription,
                  const TVideoDatabase& database,
                  const NScenarios::TScenarioRunRequest& requestProto,
                  size_t gallerySize = 5);

    const TSemanticFrame& GetSemanticFrame() const {
        return GetCurrentState().GetSemanticFrame();
    }

    const ::google::protobuf::RepeatedPtrField<TClientEntity>& GetEntities() const {
        return GetCurrentState().GetEntities();
    }

    const TVideoRecommendationState& GetState() const {
        return State;
    }

    const NJson::TJsonValue& GetNlgContext() const {
        return NlgContext;
    }

    const TMaybe<NScenarios::TShowGalleryDirective>& GetShowGalleryDirective() const {
        return Directive;
    }

    THashMap<TString, NScenarios::TFrameAction> GetFrameActions() const;

    void Update();

private:
    const TFormDescription& FormDescription;
    const TVideoDatabase& VideoDatabase;
    const TClientInfoProto& ClientInfo;
    const bool IsTvPluggedIn;
    const size_t GallerySize;

    TVideoRecommendationState State;
    NJson::TJsonValue NlgContext;
    TMaybe<NScenarios::TShowGalleryDirective> Directive;

private:
    const TVideoRecommendationStateElement& GetCurrentState() const;
    TVideoRecommendationStateElement& GetCurrentState();

    bool TryLoadFrameFromRequest(const NScenarios::TScenarioRunRequest& requestProto, const TString& formName);
    void UpdateStateByRequest(const NScenarios::TScenarioRunRequest& requestProto, const TString& formName);
    void UpdatePositionInGallery(int offset);

    bool CanGoBackward() const;

    void RequestSlot();
    void BuildShowGalleryDirective(const TVector<TVideoItem>& items);
    void BuildPlainTextRecommendations(const TVector<TVideoItem>& recommendedItems);
    void AddAttention(const TStringBuf attention);
};

} // namespace NAlice::NHollywood

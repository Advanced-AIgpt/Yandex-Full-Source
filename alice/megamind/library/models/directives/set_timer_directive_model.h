#pragma once

#include "client_directive_model.h"

#include <google/protobuf/struct.pb.h>

#include <util/generic/vector.h>

#include <utility>

namespace NAlice::NMegamind {

class TSetTimerDirectiveModel final : public TClientDirectiveModel {
public:
    TSetTimerDirectiveModel(const TString& analyticsType, const ui64 duration, const bool listeningIsPossible,
                            const ui64 timestamp, google::protobuf::Struct onSuccessCallbackPayload,
                            google::protobuf::Struct onFailureCallbackPayload,
                            TVector<TIntrusivePtr<IDirectiveModel>>&& directives)
        : TClientDirectiveModel("set_timer", analyticsType)
        , Duration(duration)
        , ListeningIsPossible(listeningIsPossible)
        , Timestamp(timestamp)
        , OnSuccessCallbackPayload(std::move(onSuccessCallbackPayload))
        , OnFailureCallbackPayload(std::move(onFailureCallbackPayload))
        , Directives(directives) {
    }

    void Accept(IModelSerializer& serializer) const final {
        serializer.Visit(*this);
    }

    [[nodiscard]] ui64 GetDuration() const {
        return Duration;
    }

    [[nodiscard]] bool GetListeningIsPossible() const {
        return ListeningIsPossible;
    }

    [[nodiscard]] ui64 GetTimestamp() const {
        return Timestamp;
    }

    [[nodiscard]] const google::protobuf::Struct& GetOnSuccessCallbackPayload() const {
        return OnSuccessCallbackPayload;
    }

    [[nodiscard]] const google::protobuf::Struct& GetOnFailureCallbackPayload() const {
        return OnFailureCallbackPayload;
    }

    [[nodiscard]] bool IsTimestampCase() const {
        return Timestamp != 0;
    }

    [[nodiscard]] const TVector<TIntrusivePtr<IDirectiveModel>>& GetDirectives() const {
        return Directives;
    }

private:
    ui64 Duration;
    bool ListeningIsPossible;
    ui64 Timestamp;
    google::protobuf::Struct OnSuccessCallbackPayload;
    google::protobuf::Struct OnFailureCallbackPayload;
    TVector<TIntrusivePtr<IDirectiveModel>> Directives;
};

} // namespace NAlice::NMegamind

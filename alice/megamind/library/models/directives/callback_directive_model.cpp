#include "callback_directive_model.h"

#include <google/protobuf/util/json_util.h>

#include <utility>

namespace NAlice::NMegamind {

TCallbackDirectiveModel::TCallbackDirectiveModel(const TString& name, bool ignoreAnswer,
                                                 google::protobuf::Struct payload, bool isLedSilent,
                                                 TMaybe<TString> multiroomSessionId,
                                                 TMaybe<TString> roomId,
                                                 TMaybe<NScenarios::TLocationInfo> locationInfo)
    : TServerDirectiveModel(name, ignoreAnswer, std::move(multiroomSessionId), std::move(roomId), std::move(locationInfo))
    , Payload(std::move(payload))
    , IsLedSilent(isLedSilent)
{
}

TCallbackDirectiveModel::TCallbackDirectiveModel(TCallbackDirectiveModel&& callbackDirectiveModel)
    : TServerDirectiveModel(callbackDirectiveModel.GetName(),
                            callbackDirectiveModel.GetIgnoreAnswer(),
                            callbackDirectiveModel.GetMultiroomSessionId(),
                            callbackDirectiveModel.GetRoomId(),
                            callbackDirectiveModel.GetLocationInfo())
    , Payload(std::move(callbackDirectiveModel.Payload))
    , IsLedSilent(callbackDirectiveModel.GetIsLedSilent()) {
}

void TCallbackDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const google::protobuf::Struct& TCallbackDirectiveModel::GetPayload() const {
    return Payload;
}

bool TCallbackDirectiveModel::GetIsLedSilent() const {
    return IsLedSilent;
}

} // namespace NAlice::NMegamind

#include "universal_client_directive_model.h"

#include <utility>

namespace NAlice::NMegamind {

TUniversalClientDirectiveModel::TUniversalClientDirectiveModel(const TString& name,
                                                               const TString& analyticsType,
                                                               google::protobuf::Struct payload,
                                                               TMaybe<TString> multiroomSessionId,
                                                               TMaybe<TString> roomId,
                                                               TMaybe<NScenarios::TLocationInfo> locationInfo)
    : TClientDirectiveModel(name, analyticsType)
    , Payload(std::move(payload))
    , MultiroomSessionId(std::move(multiroomSessionId))
    , RoomId(std::move(roomId))
    , LocationInfo(std::move(locationInfo))
{
}

void TUniversalClientDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const google::protobuf::Struct& TUniversalClientDirectiveModel::GetPayload() const {
    return Payload;
}

const TMaybe<TString>& TUniversalClientDirectiveModel::GetMultiroomSessionId() const {
    return MultiroomSessionId;
}

const TMaybe<TString>& TUniversalClientDirectiveModel::GetRoomId() const {
    return RoomId;
}

const TMaybe<NScenarios::TLocationInfo>& TUniversalClientDirectiveModel::GetLocationInfo() const {
    return LocationInfo;
}

} // namespace NAlice::NMegamind

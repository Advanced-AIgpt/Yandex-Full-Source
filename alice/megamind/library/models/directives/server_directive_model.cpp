#include "server_directive_model.h"

namespace NAlice::NMegamind {

TServerDirectiveModel::TServerDirectiveModel(const TString& name, bool ignoreAnswer,
                                             TMaybe<TString> multiroomSessionId,
                                             TMaybe<TString> roomId,
                                             TMaybe<NScenarios::TLocationInfo> locationInfo)
    : Name(name)
    , IgnoreAnswer(ignoreAnswer)
    , MultiroomSessionId(std::move(multiroomSessionId))
    , RoomId(std::move(roomId))
    , LocationInfo(std::move(locationInfo))
{
}

const TString& TServerDirectiveModel::GetName() const {
    return Name;
}

bool TServerDirectiveModel::GetIgnoreAnswer() const {
    return IgnoreAnswer;
}

EDirectiveType TServerDirectiveModel::GetType() const {
    return EDirectiveType::ServerAction;
}

const TMaybe<TString>& TServerDirectiveModel::GetMultiroomSessionId() const {
    return MultiroomSessionId;
}

const TMaybe<TString>& TServerDirectiveModel::GetRoomId() const {
    return RoomId;
}

const TMaybe<NScenarios::TLocationInfo>& TServerDirectiveModel::GetLocationInfo() const {
    return LocationInfo;
}

} // namespace NAlice::NMegamind

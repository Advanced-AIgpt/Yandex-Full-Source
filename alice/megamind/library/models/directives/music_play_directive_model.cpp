#include "music_play_directive_model.h"

#include <utility>

namespace NAlice::NMegamind {

TMusicPlayDirectiveModel::TMusicPlayDirectiveModel(const TString& analyticsType, const TString& uid,
                                                   const TString& sessionId, double offset, TMaybe<TString> alarmId,
                                                   TMaybe<TString> firstTrackId, TMaybe<TString> roomId,
                                                   TMaybe<NScenarios::TLocationInfo> locationInfo)
    : TClientDirectiveModel("music_play", analyticsType)
    , Uid(uid)
    , SessionId(sessionId)
    , Offset(offset)
    , AlarmId(std::move(alarmId))
    , FirstTrackId(std::move(firstTrackId))
    , RoomId(std::move(roomId))
    , LocationInfo(std::move(locationInfo))
{
}

void TMusicPlayDirectiveModel::Accept(IModelSerializer& serializer) const {
    serializer.Visit(*this);
}

const TString& TMusicPlayDirectiveModel::GetUid() const {
    return Uid;
}

const TString& TMusicPlayDirectiveModel::GetSessionId() const {
    return SessionId;
}

double TMusicPlayDirectiveModel::GetOffset() const {
    return Offset;
}

const TMaybe<TString>& TMusicPlayDirectiveModel::GetAlarmId() const {
    return AlarmId;
}

const TMaybe<TString>& TMusicPlayDirectiveModel::GetFirstTrackId() const {
    return FirstTrackId;
}

const TMaybe<TString>& TMusicPlayDirectiveModel::GetRoomId() const {
    return RoomId;
}

const TMaybe<NScenarios::TLocationInfo>& TMusicPlayDirectiveModel::GetLocationInfo() const {
    return LocationInfo;
}

} // namespace NAlice::NMegamind

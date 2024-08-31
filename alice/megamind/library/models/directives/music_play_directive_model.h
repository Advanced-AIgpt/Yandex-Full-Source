#pragma once

#include <alice/megamind/library/models/directives/client_directive_model.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

namespace NAlice::NMegamind {

class TMusicPlayDirectiveModel final : public virtual TClientDirectiveModel {
public:
    explicit TMusicPlayDirectiveModel(const TString& analyticsType, const TString& uid, const TString& sessionId,
                                      double offset, TMaybe<TString> alarmId, TMaybe<TString> firstTrackId,
                                      TMaybe<TString> roomId, TMaybe<NScenarios::TLocationInfo> locationInfo);

    void Accept(IModelSerializer& serializer) const final;

    [[nodiscard]] const TString& GetUid() const;
    [[nodiscard]] const TString& GetSessionId() const;
    [[nodiscard]] double GetOffset() const;
    [[nodiscard]] const TMaybe<TString>& GetAlarmId() const;
    [[nodiscard]] const TMaybe<TString>& GetFirstTrackId() const;
    [[nodiscard]] const TMaybe<TString>& GetRoomId() const;
    [[nodiscard]] const TMaybe<NScenarios::TLocationInfo>& GetLocationInfo() const;

private:
    const TString Uid;
    const TString SessionId;
    const double Offset;
    const TMaybe<TString> AlarmId;
    const TMaybe<TString> FirstTrackId;
    const TMaybe<TString> RoomId;
    const TMaybe<NScenarios::TLocationInfo> LocationInfo;
};

} // namespace NAlice::NMegamind

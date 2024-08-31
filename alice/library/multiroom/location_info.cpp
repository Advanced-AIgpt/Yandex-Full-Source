#include "location_info.h"

#include <util/generic/maybe.h>

namespace NAlice {

ILocationInfoVisitor::ILocationInfoVisitor(const NScenarios::TLocationInfo& locationInfo)
    : LocationInfo_{locationInfo}
{
}

void ILocationInfoVisitor::Visit() {
    for (auto roomId : LocationInfo_.GetRoomsIds()) {
        VisitRoomId(roomId);
    }
    for (auto groupId : LocationInfo_.GetGroupsIds()) {
        VisitGroupId(groupId);
    }
    for (auto deviceId : LocationInfo_.GetDevicesIds()) {
        VisitDeviceId(deviceId);
    }
    for (auto smartSpeakerModel : LocationInfo_.GetSmartSpeakerModels()) {
        VisitSmartSpeakerModel(static_cast<EUserDeviceType>(smartSpeakerModel));
    }
    if (LocationInfo_.GetEverywhere()) {
        VisitEverywhere();
    }
}

TStringBuf CollectLocationId(const NScenarios::TLocationInfo& locationInfo) {
    class TLocationInfoCollectorVisitor : public ILocationInfoVisitor {
    public:
        using ILocationInfoVisitor::ILocationInfoVisitor;

        TStringBuf GetId() {
            return Id_;
        }

    private:
        void VisitRoomId(TStringBuf roomId) override {
            Id_ = (roomId == EVERYWHERE_LOCATION_SLOT) ? ALL_ROOMS_ROOM_ID : roomId;
        }
        void VisitGroupId(TStringBuf groupId) override {
            Id_ = groupId;
        }
        void VisitDeviceId(TStringBuf deviceId) override {
            Id_ = deviceId;
        }
        void VisitSmartSpeakerModel(EUserDeviceType /* smartSpeakerModel */) override {
            // no location id name for this case...
        }
        void VisitEverywhere() override {
            Id_ = ALL_ROOMS_ROOM_ID;
        }

    private:
        // DON'T return empty id, because the firmware recognizes empty "room_id" as "__all__"
        TStringBuf Id_ = UNKNOWN_COLLECTED_LOCATION_ID;
    };

    TLocationInfoCollectorVisitor visitor{locationInfo};
    visitor.Visit();
    return visitor.GetId();
}

} // namespace NAlice

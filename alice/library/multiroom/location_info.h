#pragma once

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/data/device/info.pb.h>

namespace NAlice {

inline constexpr TStringBuf EVERYWHERE_LOCATION_SLOT = "everywhere";
inline constexpr TStringBuf ALL_ROOMS_ROOM_ID = "__all__";
inline constexpr TStringBuf LOCATION_SLOT_NAME = "location";
inline constexpr TStringBuf LOCATION_ROOM_SLOT_NAME = "location_room";
inline constexpr TStringBuf LOCATION_DEVICE_SLOT_NAME = "location_device";
inline constexpr TStringBuf LOCATION_GROUP_SLOT_NAME = "location_group";
inline constexpr TStringBuf LOCATION_EVERYWHERE_SLOT_NAME = "location_everywhere";
inline constexpr TStringBuf LOCATION_SMART_SPEAKER_MODEL_SLOT_NAME = "location_smart_speaker_model";
inline constexpr TStringBuf UNKNOWN_COLLECTED_LOCATION_ID = "unknown-id";

class ILocationInfoVisitor {
public:
    virtual ~ILocationInfoVisitor() = default;
    ILocationInfoVisitor(const NScenarios::TLocationInfo& locationInfo);
    void Visit();

private:
    virtual void VisitRoomId(TStringBuf roomId) = 0;
    virtual void VisitGroupId(TStringBuf groupId) = 0;
    virtual void VisitDeviceId(TStringBuf deviceId) = 0;
    virtual void VisitSmartSpeakerModel(EUserDeviceType smartSpeakerModel) = 0;
    virtual void VisitEverywhere() = 0;

private:
    const NScenarios::TLocationInfo& LocationInfo_;
};

TStringBuf CollectLocationId(const NScenarios::TLocationInfo& locationInfo);

} // namespace NAlice

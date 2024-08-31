#include "quasar_devices_info.h"

#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/common/quasar_devices.pb.h>

#include <alice/protos/data/device/info.pb.h>
#include <alice/protos/data/location/room.pb.h>
#include <alice/protos/data/location/group.pb.h>

namespace NAlice::NMegamind {

namespace {

template <typename TSrc, typename TDst>
TStatus CopyFromTo(const TSrc& src, TDst& dst, TStringBuf type) {
    TString buffer;
    if (!src.SerializeToString(&buffer)) {
        return TError{TError::EType::Parse} << "Unable to serialize '" << type << " for copy: " << src.ShortUtf8DebugString();
    }
    if (!dst.ParseFromString(buffer)) {
        return TError{TError::EType::Parse} << "Unable to deserialize '" << type << "' for copy: " << src.ShortUtf8DebugString();
    }
    return Success();
}

template <typename T>
void CopyRepeated(const google::protobuf::RepeatedPtrField<T>& src, google::protobuf::RepeatedPtrField<T>& dst) {
    dst = { src.begin(), src.end() };
}

} // ns

TStatus CreateQuasarDevicesInfo(const TIoTUserInfo& iotUserInfo, TQuasarDevicesInfo& qdi) {
    auto& rooms = *qdi.MutableRooms();
    for (const auto& r : iotUserInfo.GetRooms()) {
        if (auto err = CopyFromTo(r, *rooms.Add(), "room")) {
            return std::move(*err);
        }
    }

    auto& groups = *qdi.MutableGroups();
    for (const auto& g : iotUserInfo.GetGroups()) {
        if (auto err = CopyFromTo(g, *groups.Add(), "group")) {
            return std::move(*err);
        }
    }

    auto& devices = *qdi.MutableDevices();
    for (const auto& from : iotUserInfo.GetDevices()) {
        if (from.GetSkillId() != "Q") {
            continue;
        }

        auto& to = *devices.Add();
        to.SetId(from.GetId());
        to.SetName(from.GetName());
        to.SetType(from.GetType());
        to.SetRoomId(from.GetRoomId());
        to.SetRoomId(from.GetRoomId());
        to.SetHouseholdId(from.GetHouseholdId());
        to.SetOriginalType(from.GetOriginalType());
        to.SetFavorite(from.GetFavorite());
        CopyRepeated(from.GetGroupIds(), *to.MutableGroupIds());
        if (auto err = CopyFromTo(from.GetDeviceInfo(), *to.MutableDeviceInfo(), "device_info")) {
            return std::move(*err);
        }
        if (auto err = CopyFromTo(from.GetQuasarInfo(), *to.MutableQuasarInfo(), "quasar_info")) {
            return std::move(*err);
        }
    }

    return Success();
}

} // namespace NAlice::NMegamind


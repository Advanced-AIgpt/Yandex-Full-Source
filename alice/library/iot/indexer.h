#pragma once

#include "structs.h"

#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>


namespace NAlice::NIot {

struct TSmartHomeIndex {
    THashMap<TString, THashSet<TString>> DeviceTypeToDevicesAndGroupsIds;
    THashMap<TString, THashSet<TString>> DeviceTypeToDevicesIds;
    THashMap<TString, TString> DeviceLikeIdToType;
    THashMap<TString, TString> DeviceIdToRoom;
    THashMap<TString, TString> DeviceIdToHousehold;
    THashMap<TString, THashSet<TString>> GroupIdToRooms;
    THashMap<TString, THashSet<TString>> RoomToDeviceIds;
    THashMap<TString, THashSet<TString>> HouseholdToDeviceIds;
    THashMap<TString, int> EntityToSmartHomeNumber;
    THashMap<std::pair<TString, TString>, THashSet<TString>> CapabilityTypeValueToInstances;
    THashMap<std::pair<TString, TString>, THashSet<TString>> CapabilityTypeInstanceToValues;
    THashMap<std::tuple<TString, TString, TString>, THashSet<TString>> DeviceLikeCapabilityTypeInstanceToValues;
    THashSet<TString> DeviceLikeWithCapabilityInfo;
    THashSet<std::tuple<TString, TString, TString>> DeviceLikePropertyTypeInstances;
    THashSet<TString> ModeInstances;
    TVector<TString> SmartSpeakersIds;
    TVector<TString> DeviceIds;
    TVector<TString> GroupsIds;
    TString ClientDeviceId;
    TString ClientRoom;
    TString ClientHousehold;
    int SmartHomesCount;
};

TSmartHomeIndex BuildIndex(const TIoTEnvironment& env, TStringBuilder* logBuilder = nullptr);

} // namespace NAlice::NIot

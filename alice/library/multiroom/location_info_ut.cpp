#include "location_info.h"

#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/protos/data/device/info.pb.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

Y_UNIT_TEST_SUITE(LocationInfoSuite) {
    Y_UNIT_TEST(CollectLocationIdSingleRoom) {
        NScenarios::TLocationInfo locationInfo;
        locationInfo.AddRoomsIds("room-1");
        UNIT_ASSERT_VALUES_EQUAL(CollectLocationId(locationInfo), "room-1");
    }

    Y_UNIT_TEST(CollectLocationIdManyGroups) {
        NScenarios::TLocationInfo locationInfo;
        locationInfo.AddGroupsIds("group-1");
        locationInfo.AddGroupsIds("group-2");
        locationInfo.AddGroupsIds("group-3");
        UNIT_ASSERT_VALUES_EQUAL(CollectLocationId(locationInfo), "group-3");
    }

    Y_UNIT_TEST(CollectLocationIdSingleDeviceId) {
        NScenarios::TLocationInfo locationInfo;
        locationInfo.AddDevicesIds("super-puper-device-3000");
        UNIT_ASSERT_VALUES_EQUAL(CollectLocationId(locationInfo), "super-puper-device-3000");
    }

    Y_UNIT_TEST(CollectLocationIdSingleSmartSpeakerModel) {
        // can't return a meaningfyl location id now
        NScenarios::TLocationInfo locationInfo;
        locationInfo.AddSmartSpeakerModels(EUserDeviceType::KettleDeviceType);
        UNIT_ASSERT_VALUES_EQUAL(CollectLocationId(locationInfo), "unknown-id");
    }
}

#include "multiroom.h"

#include <alice/library/json/json.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood {

Y_UNIT_TEST_SUITE(MultiroomTestSuite) {

Y_UNIT_TEST(MakeLocationInfo) {
    {
        // ROOM ID
        TStartMultiroomSemanticFrame frame;
        frame.AddLocationRoom()->SetUserIotRoomValue("test_room");

        NScenarios::TLocationInfo locationInfo;
        locationInfo.AddRoomsIds("test_room");

        UNIT_ASSERT_VALUES_EQUAL(JsonStringFromProto(locationInfo), JsonStringFromProto(MakeLocationInfo(frame)));
    }

    {
        // ROOM ID with special value "everywhere"
        TStartMultiroomSemanticFrame frame;
        frame.AddLocationRoom()->SetUserIotRoomValue("everywhere");

        NScenarios::TLocationInfo locationInfo;
        locationInfo.SetEverywhere(true);

        UNIT_ASSERT_VALUES_EQUAL(JsonStringFromProto(locationInfo), JsonStringFromProto(MakeLocationInfo(frame)));
    }

    {
        // GROUP ID
        TStartMultiroomSemanticFrame frame;
        frame.AddLocationGroup()->SetUserIotGroupValue("test_group");

        NScenarios::TLocationInfo locationInfo;
        locationInfo.AddGroupsIds("test_group");

        UNIT_ASSERT_VALUES_EQUAL(JsonStringFromProto(locationInfo), JsonStringFromProto(MakeLocationInfo(frame)));
    }

    {
        // DEVICE ID
        TStartMultiroomSemanticFrame frame;
        frame.AddLocationDevice()->SetUserIotDeviceValue("test_device");

        NScenarios::TLocationInfo locationInfo;
        locationInfo.AddDevicesIds("test_device");

        UNIT_ASSERT_VALUES_EQUAL(JsonStringFromProto(locationInfo), JsonStringFromProto(MakeLocationInfo(frame)));
    }

    {
        // EVERYWHERE
        TStartMultiroomSemanticFrame frame;
        frame.MutableLocationEverywhere()->SetUserIotMultiroomAllDevicesValue("__all__");

        NScenarios::TLocationInfo locationInfo;
        locationInfo.SetEverywhere(true);

        UNIT_ASSERT_VALUES_EQUAL(JsonStringFromProto(locationInfo), JsonStringFromProto(MakeLocationInfo(frame)));
    }

    {
        // big test
        TStartMultiroomSemanticFrame frame;
        frame.AddLocationRoom()->SetUserIotRoomValue("test_room_1");
        frame.AddLocationRoom()->SetUserIotRoomValue("test_room_2");
        frame.AddLocationRoom()->SetUserIotRoomValue("test_room_3");
        frame.AddLocationGroup()->SetUserIotGroupValue("test_group_1");
        frame.AddLocationGroup()->SetUserIotGroupValue("test_group_2");
        frame.AddLocationDevice()->SetUserIotDeviceValue("test_device_1");
        frame.AddLocationDevice()->SetUserIotDeviceValue("test_device_2");
        frame.MutableLocationEverywhere()->SetUserIotMultiroomAllDevicesValue("__all__");

        NScenarios::TLocationInfo locationInfo;
        locationInfo.AddRoomsIds("test_room_1");
        locationInfo.AddRoomsIds("test_room_2");
        locationInfo.AddRoomsIds("test_room_3");
        locationInfo.AddGroupsIds("test_group_1");
        locationInfo.AddGroupsIds("test_group_2");
        locationInfo.AddDevicesIds("test_device_1");
        locationInfo.AddDevicesIds("test_device_2");
        locationInfo.SetEverywhere(true);

        UNIT_ASSERT_VALUES_EQUAL(JsonStringFromProto(locationInfo), JsonStringFromProto(MakeLocationInfo(frame)));
    }

    {
        // old frame test
        TFrame frame{"personal_assistant.scenarios.player.pause"};
        {
            const auto slot = TSlot{
                .Name = "location",
                .Type = "user.iot.room",
                .Value = TSlot::TValue{"orangerie"},
                .AcceptedTypes = {"user.iot.room", "user.iot.group", "user.iot.device", "user.iot.multiroom_all_devices"},
            };
            frame.AddSlot(slot);
        }
        {
            const auto slot = TSlot{
                .Name = "location_room",
                .Type = "user.iot.room",
                .Value = TSlot::TValue{"orangerie"},
                .AcceptedTypes = {"user.iot.room"},
            };
            frame.AddSlot(slot);
        }

        NScenarios::TLocationInfo locationInfo;
        locationInfo.AddRoomsIds("orangerie");

        UNIT_ASSERT_VALUES_EQUAL(JsonStringFromProto(locationInfo), JsonStringFromProto(MakeLocationInfo(frame)));
    }
}

}

} // namespace NAlice::NHollywood

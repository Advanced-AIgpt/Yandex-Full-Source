#include "quasar_devices_info.h"

#include <library/cpp/testing/unittest/registar.h>

#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/common/quasar_devices.pb.h>
#include <alice/protos/data/device/info.pb.h>
#include <alice/protos/data/location/room.pb.h>
#include <alice/protos/data/location/group.pb.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;

Y_UNIT_TEST_SUITE(QuasarDevicesInfoDataSource) {

Y_UNIT_TEST(Smoke) {
    TIoTUserInfo ui;
    auto& d1 = *ui.MutableDevices()->Add();
    d1.SetName("d1");
    d1.SetType(EUserDeviceType::SmartSpeakerDeviceType);
    d1.SetSkillId("Q");
    d1.MutableGroupIds()->Add("g1");
    d1.MutableDeviceInfo()->SetModel("Super-Duper");
    d1.MutableQuasarInfo()->SetDeviceId("123456");

    auto& d2 = *ui.MutableDevices()->Add();
    d2.SetName("d2");
    d2.SetType(EUserDeviceType::WashingMachineDeviceType);
    d2.SetSkillId("I");

    auto& r1 = *ui.MutableRooms()->Add();
    r1.SetId("r1");

    auto& g1 = *ui.MutableGroups()->Add();
    g1.SetId("g1");

    TQuasarDevicesInfo qdi;
    auto err = CreateQuasarDevicesInfo(ui, qdi);
    UNIT_ASSERT(!err.Defined());
    UNIT_ASSERT_VALUES_EQUAL(qdi.DevicesSize(), 1);
    UNIT_ASSERT_VALUES_EQUAL(qdi.GetDevices()[0].GetName(), "d1");
    UNIT_ASSERT_VALUES_EQUAL(qdi.GetDevices()[0].GroupIdsSize(), 1);
    UNIT_ASSERT_VALUES_EQUAL(qdi.GetDevices()[0].GetDeviceInfo().GetModel(), "Super-Duper");
    UNIT_ASSERT_VALUES_EQUAL(qdi.GetDevices()[0].GetQuasarInfo().GetDeviceId(), "123456");
    UNIT_ASSERT_VALUES_EQUAL(qdi.RoomsSize(), 1);
    UNIT_ASSERT_VALUES_EQUAL(qdi.GetRooms()[0].GetId(), "r1");
    UNIT_ASSERT_VALUES_EQUAL(qdi.GroupsSize(), 1);
    UNIT_ASSERT_VALUES_EQUAL(qdi.GetGroups()[0].GetId(), "g1");
}

} // Y_UNIT_TEST_SUITE(QuasarDevicesInfoDataSource)

} // ns

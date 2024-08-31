#include "common.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/context_load/client/iot.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NAlice::NCuttlefish::NExpFlags;

Y_UNIT_TEST_SUITE(StreamConverterSetupIoT) {

    Y_UNIT_TEST(TestDefault) {
        TTestFixture fixture;

        SetupIotUserInfoForOwner(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_IOT_USER_INFO));
    }

    template <typename T>
    void TestSmartHomeUidImpl(const T uidJsonValue) {
        TTestFixture fixture;
        fixture.Message.Json["event"]["payload"]["request"]["additional_options"]["quasar_auxiliary_config"]["alice4business"]["smart_home_uid"] = uidJsonValue;

        SetupIotUserInfoForOwner(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_IOT_USER_INFO));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_SMARTHOME_UID));

        const auto& obj = fixture.AppHostContext->GetOnlyProtobufItem<NAliceProtocol::TContextLoadSmarthomeUid>(
            ITEM_TYPE_SMARTHOME_UID
        );
        UNIT_ASSERT(obj.HasValue());
        UNIT_ASSERT_VALUES_EQUAL(obj.GetValue(), ToString(uidJsonValue));
    }

    Y_UNIT_TEST(TestSmartHomeUidInt) {
        TestSmartHomeUidImpl<int>(12321);
    }

    Y_UNIT_TEST(TestSmartHomeUidString) {
        TestSmartHomeUidImpl<TString>("tgbnhy");
    }

    Y_UNIT_TEST(TestPredefinedForRobot) {
        TTestFixture fixture;
        fixture.SessionContext.MutableUserInfo()->SetUuidKind(NAliceProtocol::TUserInfo::ROBOT);
        fixture.Message.Json["event"]["payload"]["request"]["iot_config"] = "1qw23er45t";

        SetupIotUserInfoForOwner(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_IOT_USER_INFO));

        const auto& obj = fixture.AppHostContext->GetOnlyItem(ITEM_TYPE_PREDEFINED_IOT_CONFIG);
        UNIT_ASSERT_VALUES_EQUAL(obj["has_predefined_iot_config"], true);
        UNIT_ASSERT_VALUES_EQUAL(obj["serialized_iot_config"], "1qw23er45t");
    }

}

#include "common.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/context_load/client/laas.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NAliceProtocol;

Y_UNIT_TEST_SUITE(StreamConverterSetupLaas) {

    Y_UNIT_TEST(TestFirst) {
        TTestFixture fixture;

        SetupLaasForOwner(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_LAAS_REQUEST_OPTIONS));
        const auto& options = fixture.AppHostContext->GetOnlyProtobufItem<TContextLoadLaasRequestOptions>(
            ITEM_TYPE_LAAS_REQUEST_OPTIONS
        );
        UNIT_ASSERT(options.HasUseCoordinatesFromIoT());
        UNIT_ASSERT_VALUES_EQUAL(options.GetUseCoordinatesFromIoT(), false);
    }

    Y_UNIT_TEST(TestSecond) {
        TTestFixture fixture;
        fixture.RequestContext.MutableExpFlags()->insert({"use_coordinates_from_iot_in_laas_request", "1"});

        SetupLaasForOwner(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_LAAS_REQUEST_OPTIONS));
        const auto& options = fixture.AppHostContext->GetOnlyProtobufItem<TContextLoadLaasRequestOptions>(
            ITEM_TYPE_LAAS_REQUEST_OPTIONS
        );
        UNIT_ASSERT(options.HasUseCoordinatesFromIoT());
        UNIT_ASSERT_VALUES_EQUAL(options.GetUseCoordinatesFromIoT(), true);
    }

}

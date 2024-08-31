#include "common.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/context_load/client/datasync.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;

Y_UNIT_TEST_SUITE(StreamConverterSetupDatasync) {

    Y_UNIT_TEST(TestGuest) {
        TTestFixture fixture;

        SetupDatasyncForGuest(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_GUEST_CONTEXT_SOURCE_DATASYNC));
    }

    Y_UNIT_TEST(TestOwner) {
        TTestFixture fixture;

        SetupDatasyncForOwner(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_DATASYNC));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_SAVE_CONTEXT_SOURCE_DATASYNC));
    }

}

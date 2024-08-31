#include "common.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/context_load/client/memento.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;

Y_UNIT_TEST_SUITE(StreamConverterSetupMemento) {

    Y_UNIT_TEST(TestAll) {
        TTestFixture fixture;

        SetupMementoForOwner(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_MEMENTO));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_SAVE_CONTEXT_SOURCE_MEMENTO));
    }

}

#include "common.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/context_load/client/flags_json.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NAlice::NCuttlefish::NExpFlags;

Y_UNIT_TEST_SUITE(StreamConverterSetupFlagsJson) {

    Y_UNIT_TEST(TestDisregard) {
        TTestFixture fixture;
        fixture.RequestContext.MutableExpFlags()->insert({DISREGARD_UAAS, "1"});

        SetupFlagsJsonForOwner(fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext);

        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_FLAGS_JSON));
        const auto& options = fixture.AppHostContext->GetOnlyProtobufItem<NAliceProtocol::TAbFlagsProviderOptions>(
            ITEM_TYPE_AB_EXPERIMENTS_OPTIONS
        );
        UNIT_ASSERT(options.HasDisregardUaas());
        UNIT_ASSERT(options.GetDisregardUaas());
        UNIT_ASSERT(options.HasOnly100PercentFlags());
        UNIT_ASSERT(!options.GetOnly100PercentFlags());
        UNIT_ASSERT_VALUES_EQUAL(options.GetTestIds().size(), 0);
    }

    Y_UNIT_TEST(TestOnly100) {
        TTestFixture fixture;
        fixture.RequestContext.MutableExpFlags()->insert({ONLY_100_PERCENT_FLAGS, "1"});

        SetupFlagsJsonForOwner(fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext);

        const auto& options = fixture.AppHostContext->GetOnlyProtobufItem<NAliceProtocol::TAbFlagsProviderOptions>(
            ITEM_TYPE_AB_EXPERIMENTS_OPTIONS
        );
        UNIT_ASSERT(options.HasDisregardUaas());
        UNIT_ASSERT(!options.GetDisregardUaas());
        UNIT_ASSERT(options.HasOnly100PercentFlags());
        UNIT_ASSERT(options.GetOnly100PercentFlags());
        UNIT_ASSERT_VALUES_EQUAL(options.GetTestIds().size(), 0);
    }

    Y_UNIT_TEST(TestIds) {
        TTestFixture fixture;
        fixture.Message.Json["event"]["payload"]["request"]["uaas_tests"].AppendValue("2808");
        fixture.Message.Json["event"]["payload"]["request"]["uaas_tests"].AppendValue(4188);
        fixture.Message.Json["event"]["payload"]["request"]["uaas_tests"].AppendValue("1938");

        SetupFlagsJsonForOwner(fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext);

        const auto& options = fixture.AppHostContext->GetOnlyProtobufItem<NAliceProtocol::TAbFlagsProviderOptions>(
            ITEM_TYPE_AB_EXPERIMENTS_OPTIONS
        );
        UNIT_ASSERT(options.HasDisregardUaas());
        UNIT_ASSERT(!options.GetDisregardUaas());
        UNIT_ASSERT(options.HasOnly100PercentFlags());
        UNIT_ASSERT(!options.GetOnly100PercentFlags());
        UNIT_ASSERT_VALUES_EQUAL(options.GetTestIds().size(), 3);
        UNIT_ASSERT_VALUES_EQUAL(options.GetTestIds()[0], "2808");
        UNIT_ASSERT_VALUES_EQUAL(options.GetTestIds()[1], "4188");
        UNIT_ASSERT_VALUES_EQUAL(options.GetTestIds()[2], "1938");
    }

    Y_UNIT_TEST(TestAll) {
        // JFYI: There is invalid configuration: only-100% + test-ids.

        TTestFixture fixture;
        fixture.Message.Json["event"]["payload"]["request"]["uaas_tests"].AppendValue("432");
        fixture.Message.Json["event"]["payload"]["request"]["uaas_tests"].AppendValue(86);
        fixture.RequestContext.MutableExpFlags()->insert({DISREGARD_UAAS, "false"});
        fixture.RequestContext.MutableExpFlags()->insert({ONLY_100_PERCENT_FLAGS, "1"});

        SetupFlagsJsonForOwner(fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext);

        const auto& options = fixture.AppHostContext->GetOnlyProtobufItem<NAliceProtocol::TAbFlagsProviderOptions>(
            ITEM_TYPE_AB_EXPERIMENTS_OPTIONS
        );
        UNIT_ASSERT(options.HasDisregardUaas());
        UNIT_ASSERT(!options.GetDisregardUaas());
        UNIT_ASSERT(options.HasOnly100PercentFlags());
        UNIT_ASSERT(options.GetOnly100PercentFlags());
        UNIT_ASSERT_VALUES_EQUAL(options.GetTestIds().size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(options.GetTestIds()[0], "432");
        UNIT_ASSERT_VALUES_EQUAL(options.GetTestIds()[1], "86");
    }

    Y_UNIT_TEST(TestIsPrimaryContext) {
        TTestFixture fixture;
        fixture.RequestContext.MutableExpFlags()->insert({"ignore_secondary_context", "1"});
        SetupFlagsJsonForOwner(fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext);
        UNIT_ASSERT(fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_AB_EXPERIMENTS_OPTIONS));
    }

}

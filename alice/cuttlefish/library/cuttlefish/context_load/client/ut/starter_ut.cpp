#include "common.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/context_load/client/starter.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NAlice::NCuttlefish::NExpFlags;

Y_UNIT_TEST_SUITE(StreamConverterContextLoadStarter) {

    Y_UNIT_TEST(TestDefault) {
        TTestFixture fixture;

        StartContextLoad(fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext);

        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CACHALOT_MM_SESSION_LOCAL_DC));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_DATASYNC));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_FLAGS_JSON));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_IOT_USER_INFO));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_MEMENTO));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_GUEST_CONTEXT_SOURCE_DATASYNC));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_SAVE_CONTEXT_SOURCE_CACHALOT_MM_SESSION_LOCAL_DC));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_SAVE_CONTEXT_SOURCE_DATASYNC));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_SAVE_CONTEXT_SOURCE_MEMENTO));
    }

    Y_UNIT_TEST(TestContactsJsonEnabled) {
        TTestFixture fixture;
        fixture.RequestContext.MutableExpFlags()->insert({"use_contacts", "1"});

        StartContextLoad(fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext);

        UNIT_ASSERT(!fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_PREDEFINED_CONTACTS));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_PREDEFINED_CONTACTS));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_CONTACTS_JSON));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_JSON));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_CONTACTS_PROTO));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_PROTO));
    }

    Y_UNIT_TEST(TestContactsProtoEnabled) {
        TTestFixture fixture;
        fixture.RequestContext.MutableExpFlags()->insert({"contacts_as_proto", "1"});

        StartContextLoad(fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext);

        UNIT_ASSERT(!fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_PREDEFINED_CONTACTS));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_PREDEFINED_CONTACTS));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_CONTACTS_JSON));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_JSON));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_CONTACTS_PROTO));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_PROTO));
    }

    Y_UNIT_TEST(TestDisregardUaas) {
        TTestFixture fixture;
        fixture.RequestContext.MutableExpFlags()->insert({DISREGARD_UAAS, "1"});

        StartContextLoad(fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext);

        #ifdef UNIPROXY_142_ALREADY_RELEASED

        UNIT_ASSERT(!fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_AB_EXPERIMENTS_OPTIONS));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_FLAGS_JSON));

        #else

        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_FLAGS_JSON));
        const auto& options = fixture.AppHostContext->GetOnlyProtobufItem<NAliceProtocol::TAbFlagsProviderOptions>(
            ITEM_TYPE_AB_EXPERIMENTS_OPTIONS
        );
        UNIT_ASSERT(options.HasDisregardUaas());
        UNIT_ASSERT(options.GetDisregardUaas());
        UNIT_ASSERT(options.HasOnly100PercentFlags());
        UNIT_ASSERT(!options.GetOnly100PercentFlags());
        UNIT_ASSERT_VALUES_EQUAL(options.GetTestIds().size(), 0);

        #endif
    }

    Y_UNIT_TEST(TestPredefinedContactsDisableSources) {
        TTestFixture fixture;
        fixture.Message.Json["event"]["payload"]["request"]["predefined_contacts"] = "+79999999999";
        fixture.RequestContext.MutableExpFlags()->insert({"contacts_as_proto", "1"});
        fixture.RequestContext.MutableExpFlags()->insert({"use_contacts", "1"});

        StartContextLoad(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_PREDEFINED_CONTACTS));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_PREDEFINED_CONTACTS));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_CONTACTS_JSON));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_JSON));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_CONTACTS_PROTO));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_PROTO));
    }

}

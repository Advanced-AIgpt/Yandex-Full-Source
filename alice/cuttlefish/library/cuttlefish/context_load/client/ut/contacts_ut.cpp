#include "common.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/context_load/client/contacts.h>
#include <alice/cuttlefish/library/protos/context_load.pb.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NAlice::NCuttlefish::NExpFlags;

Y_UNIT_TEST_SUITE(StreamConverterSetupContacts) {

    Y_UNIT_TEST(TestJsonDefault) {
        TTestFixture fixture;

        SetupContactsJsonForOwner(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(!fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_PREDEFINED_CONTACTS));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_PREDEFINED_CONTACTS));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_CONTACTS_JSON));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_JSON));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_CONTACTS_PROTO));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_PROTO));
    }

    Y_UNIT_TEST(TestProtoDefault) {
        TTestFixture fixture;

        SetupContactsProtoForOwner(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(!fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_PREDEFINED_CONTACTS));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_PREDEFINED_CONTACTS));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_CONTACTS_JSON));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_JSON));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_CONTACTS_PROTO));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_PROTO));
    }

    Y_UNIT_TEST(TestJsonWithPredefined) {
        TTestFixture fixture;
        fixture.Message.Json["event"]["payload"]["request"]["predefined_contacts"] = "+78888888888";

        SetupContactsJsonForOwner(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_PREDEFINED_CONTACTS));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_PREDEFINED_CONTACTS));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_CONTACTS_JSON));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_JSON));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_CONTACTS_PROTO));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_PROTO));

        const auto& obj = fixture.AppHostContext->GetOnlyProtobufItem<NAliceProtocol::TContextLoadPredefinedContacts>(
            ITEM_TYPE_PREDEFINED_CONTACTS
        );
        UNIT_ASSERT(obj.HasValue());
        UNIT_ASSERT_VALUES_EQUAL(obj.GetValue(), "+78888888888");
    }

    Y_UNIT_TEST(TestProtoWithPredefined) {
        TTestFixture fixture;
        fixture.Message.Json["event"]["payload"]["request"]["predefined_contacts"] = "+79999999999";

        SetupContactsProtoForOwner(
            fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
        );

        UNIT_ASSERT(fixture.AppHostContext->HasProtobufItem(ITEM_TYPE_PREDEFINED_CONTACTS));
        UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_PREDEFINED_CONTACTS));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_CONTACTS_JSON));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_JSON));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_CONTACTS_PROTO));
        UNIT_ASSERT(!fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CONTACTS_PROTO));

        const auto& obj = fixture.AppHostContext->GetOnlyProtobufItem<NAliceProtocol::TContextLoadPredefinedContacts>(
            ITEM_TYPE_PREDEFINED_CONTACTS
        );
        UNIT_ASSERT(obj.HasValue());
        UNIT_ASSERT_VALUES_EQUAL(obj.GetValue(), "+79999999999");
    }

}

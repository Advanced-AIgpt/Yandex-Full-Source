#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/context_load/client/selector.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NAlice::NCuttlefish::NExpFlags;

Y_UNIT_TEST_SUITE(ContextSelector) {

    Y_UNIT_TEST(TestGuestDisabled) {
        NAliceProtocol::TRequestContext requestContext;
        requestContext.MutableAdditionalOptions()->SetIgnoreGuestContext(true);
        TGuestContextLoadSourceSelector guestSelector(requestContext);
        UNIT_ASSERT(!guestSelector.UseBlackbox);
        UNIT_ASSERT(!guestSelector.UseDatasync);
    }

    Y_UNIT_TEST(TestGuestManuallyEnabled) {
        NAliceProtocol::TRequestContext requestContext;
        requestContext.MutableAdditionalOptions()->SetIgnoreGuestContext(false);
        TGuestContextLoadSourceSelector guestSelector(requestContext);
        UNIT_ASSERT(guestSelector.UseBlackbox);
        UNIT_ASSERT(guestSelector.UseDatasync);
    }

    Y_UNIT_TEST(TestGuestEnabledByDefault) {
        NAliceProtocol::TRequestContext requestContext;
        TGuestContextLoadSourceSelector guestSelector(requestContext);
        UNIT_ASSERT(guestSelector.UseBlackbox);
        UNIT_ASSERT(guestSelector.UseDatasync);
    }

    Y_UNIT_TEST(TestOwnerDefault) {
        NAliceProtocol::TRequestContext requestContext;
        TOwnerContextLoadSourceSelector ownerSelector(requestContext);
        UNIT_ASSERT(ownerSelector.UseAntirobot);
        UNIT_ASSERT(ownerSelector.UseBlackbox);
        UNIT_ASSERT(ownerSelector.UseCachalotMMSession);
        UNIT_ASSERT(!ownerSelector.UseContactsJson);
        UNIT_ASSERT(!ownerSelector.UseContactsProto);
        UNIT_ASSERT(ownerSelector.UseDatasync);
        UNIT_ASSERT(ownerSelector.UseFlagsJson);
        UNIT_ASSERT(ownerSelector.UseIotUserInfo);
        UNIT_ASSERT(ownerSelector.UseLaas);
        UNIT_ASSERT(ownerSelector.UseMemento);
    }

    Y_UNIT_TEST(TestOwnerSecondaryIgnored) {
        NAliceProtocol::TRequestContext requestContext;
        requestContext.MutableAdditionalOptions()->SetIgnoreSecondaryContext(true);
        TOwnerContextLoadSourceSelector ownerSelector(requestContext);
        UNIT_ASSERT(ownerSelector.UseAntirobot);
        UNIT_ASSERT(ownerSelector.UseBlackbox);
        UNIT_ASSERT(!ownerSelector.UseCachalotMMSession);
        UNIT_ASSERT(!ownerSelector.UseContactsJson);
        UNIT_ASSERT(!ownerSelector.UseContactsProto);
        UNIT_ASSERT(!ownerSelector.UseDatasync);
        UNIT_ASSERT(ownerSelector.UseFlagsJson);
        UNIT_ASSERT(!ownerSelector.UseIotUserInfo);
        UNIT_ASSERT(ownerSelector.UseLaas);
        UNIT_ASSERT(!ownerSelector.UseMemento);
    }

    Y_UNIT_TEST(TestOwnerFlagsAndPredefined) {
        NAliceProtocol::TRequestContext requestContext;
        requestContext.MutableExpFlags()->insert({"use_contacts", "1"});
        requestContext.MutableExpFlags()->insert({"contacts_as_proto", "1"});
        requestContext.MutableExpFlags()->insert({DISREGARD_UAAS, "1"});
        requestContext.MutablePredefinedResults()->SetMegamindSession("sleep, plz");
        TOwnerContextLoadSourceSelector ownerSelector(requestContext);
        UNIT_ASSERT(ownerSelector.UseAntirobot);
        UNIT_ASSERT(ownerSelector.UseBlackbox);
        UNIT_ASSERT(!ownerSelector.UseCachalotMMSession);
        UNIT_ASSERT(ownerSelector.UseContactsJson);
        UNIT_ASSERT(ownerSelector.UseContactsProto);
        UNIT_ASSERT(ownerSelector.UseDatasync);
        UNIT_ASSERT(ownerSelector.UseFlagsJson);
        UNIT_ASSERT(ownerSelector.UseIotUserInfo);
        UNIT_ASSERT(ownerSelector.UseLaas);
        UNIT_ASSERT(ownerSelector.UseMemento);
    }

    Y_UNIT_TEST(TestOwnerCombo) {
        NAliceProtocol::TRequestContext requestContext;
        requestContext.MutableAdditionalOptions()->SetIgnoreSecondaryContext(true);
        requestContext.MutableExpFlags()->insert({"use_contacts", "1"});
        requestContext.MutableExpFlags()->insert({"contacts_as_proto", "1"});
        requestContext.MutableExpFlags()->insert({DISREGARD_UAAS, "1"});
        requestContext.MutablePredefinedResults()->SetMegamindSession("sleep, plz");
        TOwnerContextLoadSourceSelector ownerSelector(requestContext);
        UNIT_ASSERT(ownerSelector.UseAntirobot);
        UNIT_ASSERT(ownerSelector.UseBlackbox);
        UNIT_ASSERT(!ownerSelector.UseCachalotMMSession);
        UNIT_ASSERT(!ownerSelector.UseContactsJson);
        UNIT_ASSERT(!ownerSelector.UseContactsProto);
        UNIT_ASSERT(!ownerSelector.UseDatasync);
        UNIT_ASSERT(ownerSelector.UseFlagsJson);
        UNIT_ASSERT(!ownerSelector.UseIotUserInfo);
        UNIT_ASSERT(ownerSelector.UseLaas);
        UNIT_ASSERT(!ownerSelector.UseMemento);
    }

}

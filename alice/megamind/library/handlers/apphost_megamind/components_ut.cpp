#include "components.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/testing/apphost_helpers.h>

#include <alice/library/client/client_info.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;

class TLocalFixture : public TAppHostFixture {
public:
    TLocalFixture()
        : AhCtx{CreateAppHostContext()}
    {
        GlobalCtx.GenericInit();
    }

    TFromAppHostInitContext CreateInitCtx() {
        return TFromAppHostInitContext{AhCtx, {}, {}, {}};
    }

    TTestAppHostCtx AhCtx;
};

Y_UNIT_TEST_SUITE(AppHostMegamindComponents) {
    Y_UNIT_TEST_F(EventSmoke, TLocalFixture) {

        {
            TEventComponent::TEventProto eventProto;
            eventProto.SetEndOfUtterance(true);
            eventProto.SetText("hello");
            eventProto.SetType(NAlice::EEventType::text_input);

            auto initCtx = CreateInitCtx();
            initCtx.EventProtoPtr->CopyFrom(eventProto);

            auto result = TFromAppHostEventComponent::Create(initCtx);
            UNIT_ASSERT(result.IsSuccess());
            TMaybe<TFromAppHostEventComponent> component;
            result.MoveTo(component);
            UNIT_ASSERT(component.Defined());
            UNIT_ASSERT_VALUES_EQUAL(component->Event().ShortUtf8DebugString(), eventProto.ShortUtf8DebugString());
        }
    }

    Y_UNIT_TEST_F(ClientSmoke, TLocalFixture) {

        {
            auto initCtx = CreateInitCtx();
            auto result = TFromAppHostClientComponent::Create(initCtx);
            UNIT_ASSERT_C(!result.IsSuccess(), "no client proto in context");
        }

        {
            NMegamindAppHost::TClientItem clientProto;
            clientProto.SetClientIp("::1");
            clientProto.SetAuthToken("auth-token-duper");
            clientProto.MutableClientInfo()->SetEpoch("12345");
            clientProto.MutableClientInfo()->SetAppId("super-duper");
            AhCtx.TestCtx().AddProtobufItem(clientProto, AH_ITEM_SKR_CLIENT_INFO, NAppHost::EContextItemKind::Input);

            auto initCtx = CreateInitCtx();
            auto result = TFromAppHostClientComponent::Create(initCtx);
            UNIT_ASSERT(result.IsSuccess());
            TMaybe<TFromAppHostClientComponent> component;
            result.MoveTo(component);
            UNIT_ASSERT(component.Defined());

            UNIT_ASSERT(component->AuthToken());
            UNIT_ASSERT_VALUES_EQUAL(*component->AuthToken(), clientProto.GetAuthToken());

            UNIT_ASSERT(component->ClientIp());
            UNIT_ASSERT_VALUES_EQUAL(*component->ClientIp(), clientProto.GetClientIp());

            TClientInfo info{component->ClientFeatures()};
            UNIT_ASSERT_VALUES_EQUAL(info.Name, clientProto.GetClientInfo().GetAppId());
            UNIT_ASSERT_VALUES_EQUAL(info.Epoch, 12345);
        }
    }
}

} // namespace

#include "common.h"

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/context_load/client/cachalot_mm_session.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>

#include <voicetech/library/settings_manager/proto/settings.pb.h>

#include <library/cpp/testing/unittest/registar.h>


using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices;
using namespace NAlice::NCuttlefish::NExpFlags;

Y_UNIT_TEST_SUITE(StreamConverterSetupCachalotMMSession) {

    Y_UNIT_TEST(TestAll) {
        const TString uuid = "asdfg";
        uint64_t balancingHint = 0;

        {
            TTestFixture fixture;
            fixture.Message.Json["event"]["payload"]["header"]["dialog_id"] = "qwerty";
            fixture.Message.Json["event"]["payload"]["header"]["prev_req_id"] = "zxcvb";
            fixture.SessionContext.MutableUserInfo()->SetVinsApplicationUuid(uuid);
            fixture.RequestContext.MutableSettingsFromManager()->SetCacheMegamindSessionCrossDc(true);

            SetupCachalotMMSessionForOwner(
                fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
            );

            UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CACHALOT_MM_SESSION_LOCAL_DC));
            UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_SAVE_CONTEXT_SOURCE_CACHALOT_MM_SESSION_CROSS_DC));

            const auto& req = fixture.AppHostContext->GetOnlyProtobufItem<NCachalotProtocol::TMegamindSessionRequest>(
                ITEM_TYPE_MEGAMIND_SESSION_REQUEST
            );
            UNIT_ASSERT(req.HasLoadRequest());
            UNIT_ASSERT_VALUES_EQUAL(req.GetLoadRequest().GetUuid(), uuid);
            UNIT_ASSERT_VALUES_EQUAL(req.GetLoadRequest().GetDialogId(), "qwerty");
            UNIT_ASSERT_VALUES_EQUAL(req.GetLoadRequest().GetRequestId(), "zxcvb");
            UNIT_ASSERT(fixture.AppHostContext->GetBalancingHints().contains("CACHALOT_MM_SESSION"));

            balancingHint = fixture.AppHostContext->GetBalancingHints().at("CACHALOT_MM_SESSION");
        }

        {
            TTestFixture fixture;
            fixture.SessionContext.MutableUserInfo()->SetUuid(uuid);
            fixture.RequestContext.MutableSettingsFromManager()->SetLoadMegamindSessionCrossDc(true);

            SetupCachalotMMSessionForOwner(
                fixture.Message, fixture.SessionContext, fixture.RequestContext, fixture.AppHostContext
            );

            UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_LOAD_CONTEXT_SOURCE_CACHALOT_MM_SESSION_CROSS_DC));
            UNIT_ASSERT(fixture.HasFlag(EDGE_FLAG_SAVE_CONTEXT_SOURCE_CACHALOT_MM_SESSION_LOCAL_DC));

            const auto& req = fixture.AppHostContext->GetOnlyProtobufItem<NCachalotProtocol::TMegamindSessionRequest>(
                ITEM_TYPE_MEGAMIND_SESSION_REQUEST
            );
            UNIT_ASSERT(req.HasLoadRequest());
            UNIT_ASSERT_VALUES_EQUAL(req.GetLoadRequest().GetUuid(), uuid);
            UNIT_ASSERT(!req.GetLoadRequest().HasDialogId());
            UNIT_ASSERT(!req.GetLoadRequest().HasRequestId());
            UNIT_ASSERT(fixture.AppHostContext->GetBalancingHints().contains("CACHALOT_MM_SESSION"));

            UNIT_ASSERT_VALUES_EQUAL(
                balancingHint,
                fixture.AppHostContext->GetBalancingHints().at("CACHALOT_MM_SESSION")
            );
        }
    }

}

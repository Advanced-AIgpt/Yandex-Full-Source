#include "proactivity.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/scenarios/helpers/interface/scenario_ref.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_responses.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/speechkit_api_builder.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/builder.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;
using namespace testing;

Y_UNIT_TEST_SUITE(AppHostMegamindProactivityRequest) {
    Y_UNIT_TEST_F(SetupSmoke, TAppHostFixture) {
        GlobalCtx.GenericInit();

        auto ahCtx = CreateAppHostContext();
        const auto skr = TSpeechKitRequestBuilder{TSpeechKitRequestBuilder::EPredefined::MinimalWithTextEvent}.Build();

        TMockContext ctx;
        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, SpeechKitRequest()).WillRepeatedly(Return(skr));
        EXPECT_CALL(ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));
        EXPECT_CALL(ctx, ExpFlags()).WillRepeatedly(ReturnRefOfCopy(THashMap<TString, TMaybe<TString>>{}));
        EXPECT_CALL(ctx, HasExpFlag(_)).WillRepeatedly(Return(true));
        NiceMock<TMockResponses> responses;
        EXPECT_CALL(ctx, Responses()).WillRepeatedly(ReturnRef(responses));

        const auto requestModel = CreateRequest(IEvent::CreateEvent(skr.Event()), skr);
        TProactivityStorage storage;
        TScenarioToRequestFrames frames;
        const auto status = AppHostProactivitySetup(ahCtx, ctx, requestModel, storage, frames);
        UNIT_ASSERT_C(!status.Defined(), TStringBuilder{} << "proactivity request setup error: " << status);
        UNIT_ASSERT(ahCtx.TestCtx().HasProtobufItem(AH_ITEM_PROACTIVITY_HTTP_REQUEST_NAME));
    }
}

} // namespace

#include "postclassify_state.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/protos/scenario_errors.pb.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/testing/apphost_helpers.h>

#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NMegamind {

namespace {

const TString TEST_SCENARIO_NAME = "test_scenario_name";

} // namespace

using namespace ::testing;

Y_UNIT_TEST_SUITE(PostClassifyState) {
    Y_UNIT_TEST(TestFieldsSuccess) {
        NiceMock<TMockGlobalContext> globalCtx;
        globalCtx.GenericInit();
        auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{globalCtx};

        NScenarios::TScenarioContinueResponse continueResponseExpected;
        continueResponseExpected.MutableResponseBody()->MutableLayout()->SetOutputSpeech("kek");
        ahCtx.TestCtx().AddProtobufItem(continueResponseExpected, NMegamind::AH_ITEM_CONTINUE_RESPONSE_POSTCLASSIFY,
                                        NAppHost::EContextItemKind::Input);

        TQualityStorage qualityExpected;
        qualityExpected.SetPostclassificationWinReason(EWinReason::WR_BOOSTED);
        ahCtx.TestCtx().AddProtobufItem(qualityExpected, NMegamind::AH_ITEM_QUALITYSTORAGE_POSTCLASSIFY,
                                        NAppHost::EContextItemKind::Input);

        TMegamindAnalyticsInfo analyticsExpected;
        *analyticsExpected.MutableOriginalUtterance() = "kek";
        ahCtx.TestCtx().AddProtobufItem(analyticsExpected, NMegamind::AH_ITEM_ANALYTICS_POSTCLASSIFY,
                                        NAppHost::EContextItemKind::Input);

        NMegamindAppHost::TScenarioProto winnerProtoExpected;
        winnerProtoExpected.SetName(TEST_SCENARIO_NAME);
        ahCtx.TestCtx().AddProtobufItem(winnerProtoExpected, NMegamind::AH_ITEM_WINNER_SCENARIO,
                                        NAppHost::EContextItemKind::Input);

        NMegamindAppHost::TScenarioErrorsProto scenarioErrorsExpected;
        {
            NMegamindAppHost::TScenarioErrorsProto_TScenarioError error;
            error.SetScenario("scenario2");
            error.SetStage("postclassify_stage");
            *error.MutableError() = NMegamind::ErrorToProto(TError{TError::EType::Http});
            *scenarioErrorsExpected.AddScenarioErrors() = std::move(error);
        }
        ahCtx.TestCtx().AddProtobufItem(scenarioErrorsExpected, NMegamind::AH_ITEM_SCENARIO_ERRORS,
                                        NAppHost::EContextItemKind::Input);

        NMegamind::TItemProxyAdapter& itemAdapter = ahCtx.ItemProxyAdapter();
        TPostClassifyState postClassifyState{itemAdapter};

        const auto qualityStorage = postClassifyState.GetQualityStorage();
        UNIT_ASSERT_C(!qualityStorage.Error(), *qualityStorage.Error());
        UNIT_ASSERT_MESSAGES_EQUAL(qualityStorage.Value(), qualityExpected);

        const auto analytics = postClassifyState.GetAnalytics();
        UNIT_ASSERT_C(!analytics.Error(), *analytics.Error());
        UNIT_ASSERT_MESSAGES_EQUAL(analytics.Value(), analyticsExpected);

        UNIT_ASSERT_VALUES_EQUAL(postClassifyState.GetWinnerScenario().Value(), TEST_SCENARIO_NAME);

        const auto scenarioErrors = postClassifyState.GetScenarioErrors();

        UNIT_ASSERT(scenarioErrors.Defined());

        UNIT_ASSERT_MESSAGES_EQUAL(*scenarioErrors, scenarioErrorsExpected);

        auto status = postClassifyState.GetPostClassifyStatus();
        UNIT_ASSERT_C(!status.Defined(), *status);

        auto continueResponse = postClassifyState.GetContinueResponse();
        UNIT_ASSERT(continueResponse.Defined());
        UNIT_ASSERT_MESSAGES_EQUAL(*continueResponse, continueResponseExpected);
    }

    Y_UNIT_TEST(TestFieldsError) {
        NiceMock<TMockGlobalContext> globalCtx;
        globalCtx.GenericInit();
        auto ahCtx = NAlice::NMegamind::NTesting::TTestAppHostCtx{globalCtx};

        NMegamindAppHost::TErrorProto postClassifyError =
            NMegamind::ErrorToProto(TError{TError::EType::Logic} << "this is error");
        ahCtx.TestCtx().AddProtobufItem(postClassifyError, NMegamind::AH_ITEM_ERROR_POSTCLASSIFY,
                                        NAppHost::EContextItemKind::Input);

        NMegamind::TItemProxyAdapter& itemAdapter = ahCtx.ItemProxyAdapter();
        TPostClassifyState postClassifyState{itemAdapter};

        auto storage = postClassifyState.GetQualityStorage();
        UNIT_ASSERT_MESSAGES_EQUAL(
            NMegamind::ErrorToProto(*storage.Error()),
            NMegamind::ErrorToProto(TError{TError::EType::NotFound}
                                    << "Item \'mm_qualitystorage_postclassify\' is not in context"));

        auto analytics = postClassifyState.GetAnalytics();
        UNIT_ASSERT_MESSAGES_EQUAL(NMegamind::ErrorToProto(*analytics.Error()),
                                   NMegamind::ErrorToProto(TError{TError::EType::NotFound}
                                                           << "Item \'mm_analytics_postclassify\' is not in context"));

        auto error = postClassifyState.GetWinnerScenario();
        UNIT_ASSERT_MESSAGES_EQUAL(NMegamind::ErrorToProto(*error.Error()),
                                   NMegamind::ErrorToProto(TError{TError::EType::NotFound}
                                                           << "Item \'mm_winner_scenario\' is not in context"));

        auto errors = postClassifyState.GetScenarioErrors();
        UNIT_ASSERT(!errors.Defined());

        auto status = postClassifyState.GetPostClassifyStatus();
        UNIT_ASSERT_MESSAGES_EQUAL(NMegamind::ErrorToProto(*status), postClassifyError);

        UNIT_ASSERT(!postClassifyState.GetWinnerCombinator().Defined());
        UNIT_ASSERT(!postClassifyState.GetContinueResponse().Defined());
    }
}

} // namespace NAlice::NMegamind

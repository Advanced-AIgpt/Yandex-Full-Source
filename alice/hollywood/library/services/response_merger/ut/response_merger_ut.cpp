#include "gmock/gmock-matchers.h"
#include "gtest/gtest-param-test.h"

#include <alice/hollywood/library/services/response_merger/response_merger.h>

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/hw_service_context/context.h>
#include <alice/hollywood/library/scenarios/fast_command/common.h>
#include <alice/hollywood/library/testing/mock_global_context.h>
#include <alice/hollywood/library/util/service_context.h>

#include <alice/library/logger/fwd.h>
#include <alice/library/logger/logger.h>
#include <alice/library/proto/proto.h>
#include <alice/library/proto/protobuf.h>
#include <alice/megamind/library/scenarios/interface/response_builder.h>
#include <alice/protos/api/renderer/api.pb.h>

#include <apphost/api/service/cpp/context_data_storage.h>
#include <apphost/api/service/cpp/service_context.h>
#include <apphost/example/howto/unit_tested_servant/handlers/handlers.h>
#include <apphost/lib/service_testing/service_testing.h>

#include <contrib/restricted/googletest/googletest/include/gtest/gtest.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/gtest/gtest.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/path.h>
#include <util/generic/set.h>
#include <util/stream/file.h>
#include <util/system/env.h>

using namespace NAlice::NHollywood::NResponseMerger;

namespace {

TFsPath GetDataPath() {
    return TFsPath(ArcadiaSourceRoot()) / "alice/hollywood/library/services/response_merger/ut/data";
}

const TString SCENARIO_FILE = "__mm_scenario_response.pb.txt";
const TString RENDER_RESULT_FILE = "__render_result.pb.txt";
const TString DIV_PATCH_RESULT_FILE = "__div_patch_result.pb.txt";
const TString SCENARIO_CANONDATA_FILE = "__mm_scenario_response_canonized.pb.txt";

struct TParam {
    TString TestName;
};

class ResponseMergerRunTestFixture : public ::testing::TestWithParam<TParam> {
};

TEST_P(ResponseMergerRunTestFixture, MergingTest) {
    const auto& testName = GetParam().TestName;

    NAlice::NHollywood::TMockGlobalContext globalCtx;
    NAlice::TRTLogger& logger = NAlice::TRTLogger::NullLogger();
    NAppHost::NService::TTestContext apphostCtx = NAppHost::NService::TTestContext();

    const auto mmScenarioFilename = testName + SCENARIO_FILE;
    const auto mmScenarioResponsStr = TFileInput(GetDataPath() / mmScenarioFilename).ReadAll();
    const auto mmScenarioResponse = NAlice::ParseProtoText<NAlice::NScenarios::TScenarioRunResponse>(mmScenarioResponsStr);
    apphostCtx.AddProtobufItem(mmScenarioResponse, NAlice::NHollywood::RESPONSE_ITEM);

    const auto renderScenarioFilename = testName + RENDER_RESULT_FILE;
    const auto renderResultStr = TFileInput(GetDataPath() / renderScenarioFilename).ReadAll();
    const auto renderResult = NAlice::ParseProtoText<NAlice::NRenderer::TRenderResponse>(renderResultStr);
    apphostCtx.AddProtobufItem(renderResult, NAlice::NHollywood::RENDER_RESULT);

    NAlice::NHollywood::THwServiceContext ctx(globalCtx, apphostCtx, logger);

    TResponseMergerHandle().Do(ctx);

    const auto& scenarioResponse = ctx.GetProtoOrThrow<NAlice::NScenarios::TScenarioRunResponse>(RENDERED_RESPONSE_ITEM);

    const auto mmScenarioCanonizedFilename = testName + SCENARIO_CANONDATA_FILE;
    const auto mmScenarioCanonizedResponsStr = TFileInput(GetDataPath() / mmScenarioCanonizedFilename).ReadAll();
    ASSERT_EQ(mmScenarioCanonizedResponsStr, NAlice::SerializeProtoText(scenarioResponse, false));
}

INSTANTIATE_TEST_SUITE_P(ResponseMergerTests, ResponseMergerRunTestFixture, ::testing::Values(
     TParam{ "test_show_view_directive_merging" },
     TParam{ "test_add_card_directive_merging" },
     TParam{ "test_set_upper_shutter_directive_merging"},
     TParam{ "test_show_view_string_body_directive_merging"},
     TParam{ "test_div_patch_directive_merging"},
     TParam{ "test_div_patch_string_body_directive_merging"}
));

class ResponseMergerContinueTestFixture : public ::testing::TestWithParam<TParam> {
};

TEST_P(ResponseMergerContinueTestFixture, MergingContinueTest) {
    const auto& testName = GetParam().TestName;

    NAlice::NHollywood::TMockGlobalContext globalCtx;
    NAlice::TRTLogger& logger = NAlice::TRTLogger::NullLogger();
    NAppHost::NService::TTestContext apphostCtx = NAppHost::NService::TTestContext();

    const auto mmScenarioFilename = testName + SCENARIO_FILE;
    const auto mmScenarioResponsStr = TFileInput(GetDataPath() / mmScenarioFilename).ReadAll();
    const auto mmScenarioResponse = NAlice::ParseProtoText<NAlice::NScenarios::TScenarioContinueResponse>(mmScenarioResponsStr);
    apphostCtx.AddProtobufItem(mmScenarioResponse, CONTINUE_RESPONSE_ITEM);

    const auto renderScenarioFilename = testName + RENDER_RESULT_FILE;
    const auto renderResultStr = TFileInput(GetDataPath() / renderScenarioFilename).ReadAll();
    const auto renderResult = NAlice::ParseProtoText<NAlice::NRenderer::TRenderResponse>(renderResultStr);
    apphostCtx.AddProtobufItem(renderResult, NAlice::NHollywood::RENDER_RESULT);

    NAlice::NHollywood::THwServiceContext ctx(globalCtx, apphostCtx, logger);

    TResponseMergerHandle().Do(ctx);

    const auto& scenarioResponse = ctx.GetProtoOrThrow<NAlice::NScenarios::TScenarioContinueResponse>(RENDERED_RESPONSE_ITEM);

    const auto mmScenarioCanonizedFilename = testName + SCENARIO_CANONDATA_FILE;
    const auto mmScenarioCanonizedResponsStr = TFileInput(GetDataPath() / mmScenarioCanonizedFilename).ReadAll();
    ASSERT_EQ(mmScenarioCanonizedResponsStr, NAlice::SerializeProtoText(scenarioResponse, false));
}

INSTANTIATE_TEST_SUITE_P(ResponseMergerContinueTests, ResponseMergerContinueTestFixture, ::testing::Values(
     TParam{ "test_show_view_directive_merging" },
     TParam{ "test_add_card_directive_merging" },
     TParam{ "test_set_upper_shutter_directive_merging"},
     TParam{ "test_show_view_string_body_directive_merging"},
     TParam{ "test_div_patch_directive_merging"},
     TParam{ "test_div_patch_string_body_directive_merging"}
));

class ResponseMergerApplyTestFixture : public ::testing::TestWithParam<TParam> {
};

TEST_P(ResponseMergerApplyTestFixture, MergingApplyTest) {
    const auto& testName = GetParam().TestName;

    NAlice::NHollywood::TMockGlobalContext globalCtx;
    NAlice::TRTLogger& logger = NAlice::TRTLogger::NullLogger();
    NAppHost::NService::TTestContext apphostCtx = NAppHost::NService::TTestContext();

    const auto mmScenarioFilename = testName + SCENARIO_FILE;
    const auto mmScenarioResponsStr = TFileInput(GetDataPath() / mmScenarioFilename).ReadAll();
    const auto mmScenarioResponse = NAlice::ParseProtoText<NAlice::NScenarios::TScenarioApplyResponse>(mmScenarioResponsStr);
    apphostCtx.AddProtobufItem(mmScenarioResponse, APPLY_RESPONSE_ITEM);

    const auto renderScenarioFilename = testName + RENDER_RESULT_FILE;
    const auto renderResultStr = TFileInput(GetDataPath() / renderScenarioFilename).ReadAll();
    const auto renderResult = NAlice::ParseProtoText<NAlice::NRenderer::TRenderResponse>(renderResultStr);
    apphostCtx.AddProtobufItem(renderResult, NAlice::NHollywood::RENDER_RESULT);

    NAlice::NHollywood::THwServiceContext ctx(globalCtx, apphostCtx, logger);

    TResponseMergerHandle().Do(ctx);

    const auto& scenarioResponse = ctx.GetProtoOrThrow<NAlice::NScenarios::TScenarioApplyResponse>(RENDERED_RESPONSE_ITEM);

    const auto mmScenarioCanonizedFilename = testName + SCENARIO_CANONDATA_FILE;
    const auto mmScenarioCanonizedResponsStr = TFileInput(GetDataPath() / mmScenarioCanonizedFilename).ReadAll();
    ASSERT_EQ(mmScenarioCanonizedResponsStr, NAlice::SerializeProtoText(scenarioResponse, false));
}

INSTANTIATE_TEST_SUITE_P(ResponseMergerApplyTests, ResponseMergerApplyTestFixture, ::testing::Values(
     TParam{ "test_show_view_directive_merging" },
     TParam{ "test_add_card_directive_merging" },
     TParam{ "test_set_upper_shutter_directive_merging"},
     TParam{ "test_show_view_string_body_directive_merging"},
     TParam{ "test_div_patch_directive_merging"},
     TParam{ "test_div_patch_string_body_directive_merging"}
));


GTEST_TEST(TestMainScreen, TestMerge) {
    NAlice::NHollywood::TMockGlobalContext globalCtx;
    NAlice::TRTLogger& logger = NAlice::TRTLogger::NullLogger();
    NAppHost::NService::TTestContext apphostCtx = NAppHost::NService::TTestContext();

    NAlice::NScenarios::TScenarioRunResponse scenarioResponse;
    auto& mainScreenDirective = *scenarioResponse.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableSetMainScreenDirective();
    auto& tab = *mainScreenDirective.AddTabs();
    tab.SetId("tab1");

    auto& musicBlock = *tab.AddBlocks();
    musicBlock.SetId("music_block1");
    auto& musicCard = *musicBlock.MutableHorizontalMediaGalleryBlock()->AddCards();
    musicCard.SetId("music_id");

    auto& videoBlock = *tab.AddBlocks();
    videoBlock.SetId("kinopoisk_block1");
    auto& videoCard = *videoBlock.MutableHorizontalMediaGalleryBlock()->AddCards();
    videoCard.SetId("video_id");

    apphostCtx.AddProtobufItem(scenarioResponse, NAlice::NHollywood::RESPONSE_ITEM);

    NAlice::NRenderer::TRenderResponse musicRender;
    musicRender.SetCardId("music_id");
    musicRender.SetCardName("music_card");
    auto* globalTemplates = musicRender.MutableGlobalDiv2Templates();
    auto div2CardTemplate = NAlice::NRenderer::TRenderResponse_Div2Template();
    (*div2CardTemplate.MutableBody()->mutable_fields())["data"].set_string_value("some_template_data");
    (*globalTemplates)["template1"] = div2CardTemplate;
    (*musicRender.MutableDiv2Body()->mutable_fields())["data"].set_string_value("some_music_data");
    apphostCtx.AddProtobufItem(musicRender, NAlice::NHollywood::RENDER_RESULT);

    NAlice::NRenderer::TRenderResponse videoRender;
    videoRender.SetCardId("video_id");
    (*videoRender.MutableDiv2Body()->mutable_fields())["data"].set_string_value("some_video_data");
    apphostCtx.AddProtobufItem(videoRender, NAlice::NHollywood::RENDER_RESULT);

    NAlice::NHollywood::THwServiceContext ctx(globalCtx, apphostCtx, logger);

    TResponseMergerHandle().Do(ctx);

    const auto resultResponse = ctx.GetProtoOrThrow<NAlice::NScenarios::TScenarioRunResponse>(RENDERED_RESPONSE_ITEM);

    const auto mmScenarioCanonizedFilename = "test_set_main_screen_mm_scenario_response_canonized.pb.txt";
    const auto mmScenarioCanonizedResponsStr = TFileInput(GetDataPath() / mmScenarioCanonizedFilename).ReadAll();
    ASSERT_EQ(mmScenarioCanonizedResponsStr, NAlice::SerializeProtoText(resultResponse, false));
}

}

#include "begemot.h"
#include "ut_helper.h"

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/components.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_request_context.h>
#include <alice/megamind/library/testing/speechkit.h>
#include <alice/megamind/library/testing/speechkit_api_builder.h>
#include <alice/begemot/lib/api/experiments/flags.h>

#include <search/begemot/rules/alice/response/proto/alice_response.pb.h>

#include <library/cpp/resource/resource.h>

#include <util/generic/string.h>


namespace {

using namespace testing;
using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NImpl;
using namespace NAlice::NMegamind::NTesting;

namespace {

bool HasAlicePolyglotMergeResponseRule(const NJson::TJsonValue& begemotRequest) {
    return FindPtr(begemotRequest["params"]["rwr"].GetArray(), "AlicePolyglotMergeResponse") != nullptr;
}

} // namespace

Y_UNIT_TEST_SUITE_F(AppHostMegamindBegemot, TAppHostFixture) {

    Y_UNIT_TEST(SetupNativeSmoke) {
        auto ahCtx = CreateAppHostContext();
        const TString textInput = "hello";
        NJson::TJsonValue request = TSpeechKitApiRequestBuilder().SetTextInput(textInput).BuildJson();
        FakeSkrInit(ahCtx, TSpeechKitRequestBuilder{request});

        TAppHostRequestCtx requestCtx(ahCtx);
        TFromAppHostSpeechKitRequest::TPtr skr;
        UNIT_ASSERT(!TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skr).Defined());
        UNIT_ASSERT(skr);
        TContext ctx{*skr, /* responses= */{}, requestCtx};

        {
            auto err = AppHostBegemotSetup(ahCtx, "", ctx);
            UNIT_ASSERT(err.Defined());
        }

        {
            auto err = AppHostBegemotSetup(ahCtx, textInput, ctx);
            UNIT_ASSERT(!err.Defined());
        }

        {
            auto megabegemotRequest = ahCtx.TestCtx().GetOnlyItem(AH_ITEM_BEGEMOT_NATIVE_REQUEST_NAME);
            UNIT_ASSERT_VALUES_EQUAL(megabegemotRequest["params"]["text"][0].GetString(), textInput);
            UNIT_ASSERT_VALUES_EQUAL(megabegemotRequest["params"]["uil"][0].GetString(), "ru");
            UNIT_ASSERT(megabegemotRequest["params"]["rwr"].GetArray().size());
            UNIT_ASSERT(megabegemotRequest["params"]["wizextra"].GetArray().size());
            auto mergerRequest = ahCtx.TestCtx().GetOnlyItem(AH_ITEM_BEGEMOT_MERGER_REQUEST);
            UNIT_ASSERT(HasAlicePolyglotMergeResponseRule(mergerRequest));
            UNIT_ASSERT(mergerRequest["params"]["wizextra"].GetArray().size());
            UNIT_ASSERT_VALUES_EQUAL(mergerRequest["params"]["wizextra"], megabegemotRequest["params"]["wizextra"]);
            UNIT_ASSERT_VALUES_EQUAL_C(mergerRequest["params"].GetMap().size(), 2,
                                       "Only wizextra and rwr params should be in merger request");
        }
    }

    Y_UNIT_TEST(PolyglotBegemotSetupByLang) {
        auto ahCtx = CreateAppHostContext();
        const TString originalUtternace = "hello";
        const TString translatedUtternace = "привет";
        NJson::TJsonValue request = TSpeechKitApiRequestBuilder()
                                        .SetTextInput(originalUtternace)
                                        .UpdateLang("ar-SA")
                                        .EnableExpFlag("mm_allow_lang_ar")
                                        .BuildJson();
        FakeSkrInit(ahCtx, TSpeechKitRequestBuilder{request});

        TAppHostRequestCtx requestCtx(ahCtx);
        TFromAppHostSpeechKitRequest::TPtr skr;
        UNIT_ASSERT(!TFromAppHostSpeechKitRequest::Create(ahCtx).MoveTo(skr).Defined());
        UNIT_ASSERT(skr);
        TContext ctx{*skr, /* responses= */ {}, requestCtx};

        {
            auto err = AppHostBegemotSetup(ahCtx, "", ctx);
            UNIT_ASSERT(err.Defined());
            auto err2 = AppHostPolyglotBegemotSetup(ahCtx, "", ctx);
            UNIT_ASSERT(err2.Defined());
        }

        {
            auto err = AppHostBegemotSetup(ahCtx, translatedUtternace, ctx);
            UNIT_ASSERT(!err.Defined());
            auto err2 = AppHostPolyglotBegemotSetup(ahCtx, originalUtternace, ctx);
            UNIT_ASSERT(!err2.Defined());
        }

        auto validateBegemotApphostItem = [&ahCtx](const TStringBuf ahItemName, const TStringBuf language, const TStringBuf text) {
            auto json = ahCtx.TestCtx().GetOnlyItem(ahItemName);
            UNIT_ASSERT_VALUES_EQUAL(json["params"]["text"][0].GetString(), text);
            UNIT_ASSERT_VALUES_EQUAL(json["params"]["uil"][0].GetString(), language);
            UNIT_ASSERT(json["params"]["wizextra"].GetArray().size());
            UNIT_ASSERT(!HasAlicePolyglotMergeResponseRule(json));
        };

        auto validateMergerApphostItem = [&ahCtx](const TStringBuf ahItemName) {
            auto json = ahCtx.TestCtx().GetOnlyItem(ahItemName);
            UNIT_ASSERT(json["params"]["wizextra"].GetArray().size());
            UNIT_ASSERT(!HasAlicePolyglotMergeResponseRule(json));
        };

        validateBegemotApphostItem(AH_ITEM_BEGEMOT_NATIVE_REQUEST_NAME, "ru", translatedUtternace);
        validateBegemotApphostItem(AH_ITEM_POLYGLOT_BEGEMOT_NATIVE_REQUEST_NAME, "ar", originalUtternace);
        validateBegemotApphostItem(AH_ITEM_BEGEMOT_NATIVE_BEGGINS_REQUEST_NAME, "ru", translatedUtternace);
        validateBegemotApphostItem(AH_ITEM_POLYGLOT_BEGEMOT_NATIVE_BEGGINS_REQUEST_NAME, "ar", originalUtternace);
        validateMergerApphostItem(AH_ITEM_BEGEMOT_MERGER_REQUEST);
        validateMergerApphostItem(AH_ITEM_POLYGLOT_BEGEMOT_MERGER_REQUEST);
        validateMergerApphostItem(AH_ITEM_POLYGLOT_BEGEMOT_MERGER_MERGER_REQUEST);
    }

    Y_UNIT_TEST(PostNativeSetupSmoke) {
        auto ahCtx = CreateAppHostContext();

        {
            NJson::TJsonValue response;
            NJson::ReadJsonFastTree(NResource::Find("/begemot_native_proto_response"), &response);
            NBg::NProto::TAlicePolyglotMergeResponseResult protoResponse;
            Y_PROTOBUF_SUPPRESS_NODISCARD protoResponse.ParseFromString(Base64Decode(response["response"][0]["binary"].GetString()));
            ahCtx.TestCtx().AddProtobufItem(protoResponse, AH_ITEM_BEGEMOT_ALICE_POLYGLOT_MERGER_RESPONSE_NAME, NAppHost::EContextItemKind::Input);
        }

        TMockContext ctx;
        EXPECT_CALL(ctx, Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));
        EXPECT_CALL(ctx, Logger()).WillRepeatedly(ReturnRef(TRTLogger::NullLogger()));
        EXPECT_CALL(ctx, HasExpFlag(_)).WillRepeatedly(Return(false));

        TWizardResponse wizardResponse;
        auto err = AppHostBegemotPostSetup(ahCtx, ctx, wizardResponse);
        UNIT_ASSERT(!err.Defined());
        NJson::TJsonValue jsonResponse;
        NJson::ReadJsonFastTree(NResource::Find("/begemot_native_json_response"), &jsonResponse);
        UNIT_ASSERT_VALUES_EQUAL(wizardResponse.RawResponse(), jsonResponse);
    }

    Y_UNIT_TEST(BegemotQualityLogPolicy) {
        UNIT_ASSERT_VALUES_EQUAL(GetLogPolicy(/* LogPolicyInfo= */ false), TLOG_DEBUG);
        UNIT_ASSERT_VALUES_EQUAL(GetLogPolicy(/* LogPolicyInfo= */ true), TLOG_INFO);
    }
}

} // namespace

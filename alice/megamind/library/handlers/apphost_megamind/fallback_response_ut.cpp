#include "fallback_response.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/response_meta/error.h>
#include <alice/megamind/library/testing/apphost_helpers.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;
using namespace NAlice::NMegamind::NTesting;

class TFallBackResponseFixture : public TAppHostFixture {
public:
    TFallBackResponseFixture()
        : FallbackResponseHandler{GlobalCtx}
    {
        GlobalCtx.GenericInit();
    }

    const TAppHostFallbackResponseNodeHandler FallbackResponseHandler;
};

Y_UNIT_TEST_SUITE_F(AppHostMegamindNodeFallbackResponse, TFallBackResponseFixture) {
    Y_UNIT_TEST(NoErrorItems) {
        auto ahCtx = CreateAppHostContext();
        FallbackResponseHandler.Execute(ahCtx);
        auto item = ahCtx.TestCtx().GetOnlyItem(AH_ITEM_HTTP_RESPONSE);
        UNIT_ASSERT_VALUES_EQUAL(item["status_code"].GetInteger(), 500);
        const auto js = NJson::ReadJsonFastTree(item["content"].GetString());
        const auto& errorItem = js["meta"][0];
        UNIT_ASSERT(!errorItem.IsNull());
        UNIT_ASSERT_VALUES_EQUAL(errorItem["type"].GetString(), "error");
        UNIT_ASSERT_VALUES_EQUAL(errorItem["http_code"].GetInteger(), 500);
        UNIT_ASSERT_VALUES_EQUAL(errorItem["origin"].GetString(), "Status");
        UNIT_ASSERT(!errorItem["message"].GetString().Empty());
    }

    Y_UNIT_TEST(BadRequestItem) {
        auto ahCtx = CreateAppHostContext();

        {
            PushErrorItem(ahCtx.TestCtx(), TErrorMetaBuilder{TError{TError::EType::BadRequest} << "test"},
                          NAppHost::EContextItemKind::Input);
        }

        FallbackResponseHandler.Execute(ahCtx);

        const auto& item = ahCtx.TestCtx().GetOnlyItem(AH_ITEM_HTTP_RESPONSE);
        UNIT_ASSERT_VALUES_EQUAL(item["status_code"].GetInteger(), 400);
        const auto js = NJson::ReadJsonFastTree(item["content"].GetString());
        const auto& errorItem = js["meta"][0];
        UNIT_ASSERT(!errorItem.IsNull());
        UNIT_ASSERT_VALUES_EQUAL(errorItem["type"].GetString(), "error");
        UNIT_ASSERT_VALUES_EQUAL(errorItem["error_type"].GetString(), "bad_request");
        UNIT_ASSERT_VALUES_EQUAL(errorItem["message"].GetString(), "test");
    }

    Y_UNIT_TEST(OtherErrorsCollected) {
        auto ahCtx = CreateAppHostContext();

        PushErrorItem(ahCtx.TestCtx(), TErrorMetaBuilder{TError{TError::EType::ScenarioError} << "first"},
                      NAppHost::EContextItemKind::Input);
        PushErrorItem(ahCtx.TestCtx(), TErrorMetaBuilder{TError{TError::EType::Parse} << "second"},
                      NAppHost::EContextItemKind::Input);
        PushErrorItem(ahCtx.TestCtx(), TErrorMetaBuilder{TError{TError::EType::BadRequest} << "last"},
                      NAppHost::EContextItemKind::Input);

        FallbackResponseHandler.Execute(ahCtx);

        const auto& item = ahCtx.TestCtx().GetOnlyItem(AH_ITEM_HTTP_RESPONSE, NAppHost::EContextItemSelection::Output);
        UNIT_ASSERT_VALUES_EQUAL(item["status_code"].GetInteger(), 400);
        const auto js = NJson::ReadJsonFastTree(item["content"].GetString());
        const auto& errorItem = js["meta"][0];
        UNIT_ASSERT(!errorItem.IsNull());
        UNIT_ASSERT_VALUES_EQUAL(errorItem["type"].GetString(), "error");
        UNIT_ASSERT_VALUES_EQUAL(errorItem["error_type"].GetString(), "bad_request");
        UNIT_ASSERT_VALUES_EQUAL(errorItem["http_code"].GetInteger(), 400);
        UNIT_ASSERT_VALUES_EQUAL(errorItem["message"].GetString(), "last");
        const auto& nested = errorItem["nested"];
        UNIT_ASSERT_VALUES_EQUAL(nested.GetArray().size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(nested[0]["message"].GetString(), "first");
        UNIT_ASSERT_VALUES_EQUAL(nested[0]["error_type"].GetString(), "scenario_error");
        UNIT_ASSERT_VALUES_EQUAL(nested[1]["message"].GetString(), "second");
        UNIT_ASSERT_VALUES_EQUAL(nested[1]["error_type"].GetString(), "parse");
    }
}

} // namespace

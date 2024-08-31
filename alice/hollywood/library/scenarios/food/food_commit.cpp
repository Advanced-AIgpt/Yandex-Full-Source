#include "food_commit.h"

#include <alice/hollywood/library/scenarios/food/backend/http_utils.h>
#include <alice/hollywood/library/scenarios/food/proto/apply_arguments.pb.h>
#include <alice/hollywood/library/scenarios/food/proto/cart.pb.h>

#include <alice/megamind/protos/common/location.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/version/version.h>

#include <util/string/builder.h>

namespace NAlice::NHollywood::NFood {

using namespace NAlice::NScenarios;

const TMaybe<TString> SUCCESS = Nothing();

namespace {

void MakeResponse(TScenarioHandleContext& ctx, const TMaybe<TString>& error = SUCCESS) {
    TScenarioCommitResponse response;
    if (!error.Defined()) {
        (void)response.MutableSuccess();
    } else {
        response.MutableError()->SetMessage(*error);
    }
    response.SetVersion(VERSION_STRING);
    ctx.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
}

} // namespace

void TCommitDispatchHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    TApplyArguments args;
    const auto& rawArgs = requestProto.GetArguments();
    if (!rawArgs.Is<TApplyArguments>() || !rawArgs.UnpackTo(&args)) {
        MakeResponse(ctx, "Unexpected apply arguments");
        return;
    }
    switch (args.GetCommitCase()) {
        case TApplyArguments::CommitCase::kPostOrderData: {
            ctx.ServiceCtx.AddFlag("sync_cart");
            ctx.ServiceCtx.AddProtobufItem(args.GetPostOrderData(), "post_order_data");
            break;
        }
        case TApplyArguments::CommitCase::COMMIT_NOT_SET: {
            MakeResponse(ctx, "Commit case undefined");
            break;
        }
    }
}

void TSyncCartProxyPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const TScenarioApplyRequest requestProto = GetOnlyProtoOrThrow<TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};

    const auto postOrderData = GetOnlyProtoOrThrow<TApplyArguments::TPostOrderData>(ctx.ServiceCtx, "post_order_data");
    NJson::TJsonValue cartJson;
    cartJson["items"] = JsonFromProto(postOrderData.GetCart())["items"];
    cartJson["items"].SetType(NJson::JSON_ARRAY);
    const TString path = TStringBuilder{} << "/cart/sync"
                                          << "?" << "longitude=" << request.Location().GetLon()
                                          << "&" << "latitude=" << request.Location().GetLat()
                                          << "&" << "shipping_type=delivery";
    THttpProxyRequest req;
    TString requestKey;
    req = PrepareHttpRequest(ctx, path, MakeHeadersWithTaxiUid(postOrderData.GetTaxiUid()), cartJson, /* useOAuth= */ true);
    requestKey = "http_pa_request";
    LOG_INFO(ctx.Ctx.Logger()) << "/cart/sync request: " << SerializeProtoText(req.Request, /* singleLineMode= */ false);

    AddHttpRequestItems(ctx, req, requestKey);
}

void TSyncCartResponseHandle::Do(TScenarioHandleContext& ctx) const {
    const TMaybe<NJson::TJsonValue> jsonResponse = RetireHttpResponseJsonMaybe(ctx, "http_response");
    if (!jsonResponse.Defined()) {
        MakeResponse(ctx, "Failed to sync cart.");
        return;
    }

    if (!(*jsonResponse)["cart"]["total"].IsDefined()) {
        MakeResponse(ctx, TString{"Bad sync cart response: "} + JsonToString(*jsonResponse));
        return;
    }
    MakeResponse(ctx, SUCCESS);
}

} // namespace NAlice::NHollywood::NFood

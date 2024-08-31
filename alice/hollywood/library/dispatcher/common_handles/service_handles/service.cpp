#include "service.h"

#include <alice/hollywood/library/dispatcher/common_handles/util/util.h>

#include <alice/library/json/json.h>
#include <alice/library/network/common.h>
#include <alice/library/network/headers.h>
#include <alice/library/version/version.h>

namespace NAlice::NHollywood {

namespace {

NJson::TJsonValue MakeHttpReponseItem(const NJson::TJsonValue& content) {
    NJson::TJsonValue response;

    NJson::TJsonValue contentTypeJson;
    contentTypeJson.AppendValue(NNetwork::HEADER_CONTENT_TYPE);
    contentTypeJson.AppendValue(NContentTypes::APPLICATION_JSON);

    response[HEADERS_KEY].AppendValue(contentTypeJson);
    response[CONTENT_KEY] = JsonToString(content) + "\n";

    return response;
}

} // anonymous namespace

void GetVersionHttpHandle(TGlobalContext& /* globalContext */, const NNeh::IRequestRef& req) {
    NNeh::TDataSaver ds;
    ds << JsonToString(CreateVersionData());
    req->SendReply(ds);
}

void GetVersionAppHostHandle(NAppHost::IServiceContext& ctx, TGlobalContext& /* globalContext */) {
    ctx.AddItem(MakeHttpReponseItem(JsonToString(CreateVersionData())), HTTP_RESPONSE_ITEM);
}

void UtilityHandler(NAppHost::IServiceContext& ctx, TGlobalContext& /* globalContext */) {
    TString path = GetPath(ctx.GetOnlyItem(HTTP_REQUEST_ITEM));
    TVector<TString> pathParts = StringSplitter(path).Split('/').SkipEmpty().Limit(3);

    NJson::TJsonValue response;
    if (pathParts.size() < 3) {
        response["error"] = "wrong path format for utility handler";
    } else {
        TString intent = pathParts[2];
        if (intent == "version" || intent == "version_json") {
            response = CreateVersionData();
        } else {
            response["error"] = "unknown intent for utility handler";
        }
    }

    ctx.AddItem(MakeHttpReponseItem(response), HTTP_RESPONSE_ITEM);
}

void DumpSolomonCountersHandle(TGlobalContext& globalContext, const NNeh::IRequestRef& req) {
    NNeh::TDataSaver ds;
    ds << globalContext.SensorsDumper().Dump("solomon");
    req->SendReply(ds);
}

void GetFastDataVersionHandle(TGlobalContext& globalContext, const NNeh::IRequestRef& req) {
    NNeh::TDataSaver ds;
    const int version = globalContext.FastData().GetVersion();
    LOG_INFO(globalContext.BaseLogger()) << "Returning FastData Version: " << version << Endl;
    ds << version;
    req->SendReply(ds);
}

void ReloadFastDataHandle(TGlobalContext& globalContext, const NNeh::IRequestRef& req) {
    LOG_INFO(globalContext.BaseLogger()) << "Reloading FastData" << Endl;
    NNeh::TDataSaver ds;

    try {
        globalContext.FastData().Reload();
        const TStringBuf msg = "Successfully reloaded FastData";
        LOG_INFO(globalContext.BaseLogger()) << msg;
        ds << msg;
    } catch (yexception& e) {
        const TString msg = TStringBuilder() << "Failed to reload FastData, error: " << e.what();
        LOG_ERROR(globalContext.BaseLogger()) << msg;
        ds << msg;
    }
    req->SendReply(ds);
}

void ReopenLogsHandle(TGlobalContext& globalContext, const NNeh::IRequestRef& req) {
    globalContext.ReopenLogs();
    LOG_INFO(globalContext.BaseLogger()) << "Successfully reopened logs";

    NNeh::TDataSaver ds;
    ds << "OK";
    req->SendReply(ds);
}

void PingHandle(TGlobalContext& /* globalContext */, const NNeh::IRequestRef& req) {
    NNeh::TDataSaver ds;
    ds << "pong";
    req->SendReply(ds);
}



} // namespace NAlice::NHollywood

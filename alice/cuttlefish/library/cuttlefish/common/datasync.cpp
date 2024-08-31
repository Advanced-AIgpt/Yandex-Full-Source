#include "datasync.h"
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>


namespace NAlice::NCuttlefish::NAppHostServices {


NAppHostHttp::THttpRequest TDatasyncClient::LoadVinsContextsRequest()
{
    static const TString content = R"({"items":[{"method":"GET","relative_url":"/v2/personality/profile/addresses"},{"method":"GET","relative_url":"/v1/personality/profile/alisa/kv"},{"method":"GET","relative_url":"/v1/personality/profile/alisa/settings"}]})";

    NAppHostHttp::THttpRequest req;
    req.SetPath("/v1/batch/request");
    req.SetMethod(NAppHostHttp::THttpRequest::Post);
    req.SetContent(content);
    AddHeader(req, "Content-Type", "application/json; charset=utf-8");
    return req;
}

NJson::TJsonValue TDatasyncClient::MakeVinsContextsResponseContent(
    TMaybe<TString> addressesResponse,
    TMaybe<TString> keyValueResponse,
    TMaybe<TString> settingsResponse
) {
    NJson::TJsonValue content;
    auto& items = content.InsertValue("items", NJson::TJsonValue(NJson::JSON_ARRAY));

    if (addressesResponse.Defined()) {
        auto& item = items.AppendValue(NJson::TJsonValue());
        item["body"] = std::move(*addressesResponse);
    }

    if (keyValueResponse.Defined()) {
        auto& item = items.AppendValue(NJson::TJsonValue());
        item["body"] = std::move(*keyValueResponse);
    }

    if (settingsResponse.Defined()) {
        auto& item = items.AppendValue(NJson::TJsonValue());
        item["body"] = std::move(*settingsResponse);
    }

    return content;
}

NAppHostHttp::THttpRequest TDatasyncClient::SaveVinsContextsRequest(const TVector<NJson::TJsonValue>& payloads)
{
    // construct request body
    NJson::TJsonValue items(NJson::JSON_ARRAY);

    for (const auto& payload : payloads) {
        TString url = payload["key"].GetStringSafe("");
        TString value = payload["value"].GetString();
        TString method = payload["method"].GetString();

        NJson::TJsonValue body;
        if (url.StartsWith("/v1/personality/profile/alisa/kv")) {
            body["value"] = std::move(value);
        } else {
            body = std::move(value);
        }

        NJson::TJsonValue item;
        item["method"] = std::move(method);
        item["relative_url"] = std::move(url);
        item["body"] = NJson::WriteJson(body, /* formatOutput = */ false);

        items.AppendValue(std::move(item));
    }

    NJson::TJsonValue content;
    content["items"] = std::move(items);

    // construct request
    NAppHostHttp::THttpRequest req;
    req.SetPath("/v1/batch/request");
    req.SetMethod(NAppHostHttp::THttpRequest::Post);
    req.SetContent(NJson::WriteJson(content, /* formatOutput = */ false));
    AddHeader(req, "Content-Type", "application/json; charset=utf-8");

    return req;
}

NAppHostHttp::THttpRequest TDatasyncClient::LoadPersonalSettingsRequest()
{
    NAppHostHttp::THttpRequest req;
    req.SetScheme(NAppHostHttp::THttpRequest::Https);
    req.SetPath("/v1/personality/profile/alisa/settings");
    req.SetMethod(NAppHostHttp::THttpRequest::Get);
    return req;
}

TDatasyncClient::TPersonalSettings TDatasyncClient::ParsePersonalSetingsResponse(TStringBuf response)
{
    // {"items":[{"id":"do_not_use_user_logs","do_not_use_user_logs":false}]}

    NJson::TJsonValue value;
    if (!NJson::ReadJsonTree(response, &value, /*throwOnError = */ false))
        return {};

    TPersonalSettings result;
    result.DoNotUseUserLogs = false;  // default value

    for (const auto& it : value["items"].GetArray()) {
        const auto& val = it["do_not_use_user_logs"];
        if (val.GetType() != NJson::JSON_UNDEFINED) {
            result.DoNotUseUserLogs = val.GetBoolean();
            break;
        }
    }
    return result;
}

}  // namespace NAlice::NCuttlefish::NAppHostServices

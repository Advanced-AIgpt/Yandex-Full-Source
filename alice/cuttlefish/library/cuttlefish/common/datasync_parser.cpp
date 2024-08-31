#include "datasync_parser.h"

namespace {

    class TItemsRef {
    public:
        TItemsRef(const NJson::TJsonValue& response) {
            if (const NJson::TJsonValue *bodyStr = response.GetMap().FindPtr("body"); bodyStr && bodyStr->IsString()) {
                Body = NAlice::JsonFromString(bodyStr->GetString());
                if (const NJson::TJsonValue *items = Body.GetMap().FindPtr("items"); items && items->IsArray()) {
                    items->GetArrayPointer(&Items);
                }
            }
        }

        bool Finished() const {
            return !((Items != nullptr) && (CurrentPos < Items->size()));
        }

        const NJson::TJsonValue* GetNextItem() {
            while (!Finished()) {
                const NJson::TJsonValue &item = Items->at(CurrentPos);
                CurrentPos++;
                if (item.IsMap()) {
                    return &item;
                }
            }
            return nullptr;
        }

    private:
        NJson::TJsonValue Body;
        const NJson::TJsonValue::TArray *Items = nullptr;
        size_t CurrentPos = 0;
    };
}

namespace NAlice::NCuttlefish::NAppHostServices {

    void TBaseDatasyncResponseParser::ParseDatasyncResponse(const NAppHostHttp::THttpResponse& response) {
        if (response.GetStatusCode() / 100 != 2) {
            // non 2xx http code
            return;
        }
        const NJson::TJsonValue json = NAlice::JsonFromString(response.GetContent());
        const NJson::TJsonValue::TArray &items = json["items"].GetArray();
        if (items.size() != 2 && items.size() != 3) {
            ythrow yexception() << "got unexpected number of response items";
        }

        ParseDatasyncAddressesResponse(items[0]);
        ParseDatasyncKeyValueResponse(items[1]);
        if (items.size() == 3) {
            ParseDatasyncSettingsResponse(items[2]);
        }
    }

    void TDatasyncResponseParser::ParseDatasyncResponseImpl(
        const NJson::TJsonValue& response,
        const TStringBuf prefix,
        const TStringBuf idKey,
        const std::function<NJson::TJsonValue(const NJson::TJsonValue&)> valueExtractor
    ) {
        TItemsRef items(response);
        const NJson::TJsonValue *item = nullptr;
        while (item = items.GetNextItem()) {
            if (const NJson::TJsonValue *idStr = item->GetMap().FindPtr(idKey); idStr && idStr->IsString()) {
                const TString key = TString::Join(prefix, "/", idStr->GetString());
                if (!PersonalData.Has(key)) {
                    PersonalData[key] = valueExtractor(*item);
                }
            }
        }
    }

    void TDatasyncResponseParser::ParseDatasyncAddressesResponse(const NJson::TJsonValue& addressesResponse) {
        ParseDatasyncResponseImpl(
            addressesResponse,
            "/v2/personality/profile/addresses",
            "address_id",
            [](const NJson::TJsonValue &value) { return value; }
        );
    }

    void TDatasyncResponseParser::ParseDatasyncKeyValueResponse(const NJson::TJsonValue& kvResponse) {
        ParseDatasyncResponseImpl(
            kvResponse,
            "/v1/personality/profile/alisa/kv",
            "id",
            [](const NJson::TJsonValue& value) { return value["value"]; }
        );
    }

    void TDatasyncResponseParser::ParseDatasyncSettingsResponse(const NJson::TJsonValue& response) {
        TItemsRef items(response);
        const NJson::TJsonValue *item = nullptr;
        while (item = items.GetNextItem()) {
            const NJson::TJsonValue *val = item->GetMap().FindPtr("do_not_use_user_logs");
            if (val && val->IsBoolean()) {
                DoNotUseUserLogs = val->GetBoolean();
            }
        }
    }

    void TShallowDatasyncResponseParser::ParseDatasyncAddressesResponse(const NJson::TJsonValue& addressesResponse) {
        ParseResponse.AddressesResponse = GetDatasyncResponseBody(addressesResponse);
    }

    void TShallowDatasyncResponseParser::ParseDatasyncKeyValueResponse(const NJson::TJsonValue& kvResponse) {
        ParseResponse.KeyValueResponse = GetDatasyncResponseBody(kvResponse);
    }

    void TShallowDatasyncResponseParser::ParseDatasyncSettingsResponse(const NJson::TJsonValue& response) {
        ParseResponse.SettingsResponse = GetDatasyncResponseBody(response);
    }

    TMaybe<TString> TShallowDatasyncResponseParser::GetDatasyncResponseBody(const NJson::TJsonValue& response) {
        if (const NJson::TJsonValue* bodyStr = response.GetMap().FindPtr("body"); bodyStr && bodyStr->IsString()) {
            return bodyStr->GetString();
        }
        return Nothing();
    }

    NJson::TJsonValue TDatasyncResponseParser::Parse(const NAppHostHttp::THttpResponse& response) {
        TDatasyncResponseParser parser;
        parser.ParseDatasyncResponse(response);

        return std::move(parser.PersonalData);
    }

    TShallowDatasyncResponseParser::TParseResponse TShallowDatasyncResponseParser::Parse(const NAppHostHttp::THttpResponse& response) {
        TShallowDatasyncResponseParser parser;
        parser.ParseDatasyncResponse(response);

        return std::move(parser.ParseResponse);
    }

}   // namespace NAlice::NCuttlefish::NAppHostServices

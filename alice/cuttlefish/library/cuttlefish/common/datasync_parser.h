#pragma once

#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <alice/library/json/json.h>
#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    class TBaseDatasyncResponseParser {
    public:
        void ParseDatasyncResponse(const NAppHostHttp::THttpResponse& response);

    protected:
        virtual void ParseDatasyncAddressesResponse(const NJson::TJsonValue& addressesResponse) = 0;
        virtual void ParseDatasyncKeyValueResponse(const NJson::TJsonValue& kvResponse) = 0;
        virtual void ParseDatasyncSettingsResponse(const NJson::TJsonValue& response) = 0;
    };


    class TDatasyncResponseParser : public TBaseDatasyncResponseParser {
    public:
        static NJson::TJsonValue Parse(const NAppHostHttp::THttpResponse& response);

    protected:
        void ParseDatasyncAddressesResponse(const NJson::TJsonValue& addressesResponse) override;
        void ParseDatasyncKeyValueResponse(const NJson::TJsonValue& kvResponse) override;
        void ParseDatasyncSettingsResponse(const NJson::TJsonValue& response) override;

    private:
        void ParseDatasyncResponseImpl(
            const NJson::TJsonValue& response,
            const TStringBuf prefix,
            const TStringBuf idKey,
            const std::function<NJson::TJsonValue(const NJson::TJsonValue &)> valueExtractor
        );

    public:
        NJson::TJsonValue PersonalData;
        TMaybe<bool> DoNotUseUserLogs = Nothing();
    };


    class TShallowDatasyncResponseParser : public TBaseDatasyncResponseParser {
    public:
        struct TParseResponse {
            TMaybe<TString> AddressesResponse;
            TMaybe<TString> KeyValueResponse;
            TMaybe<TString> SettingsResponse;
        };

        static TShallowDatasyncResponseParser::TParseResponse Parse(const NAppHostHttp::THttpResponse& response);

    public:
        TParseResponse ParseResponse;

    protected:
        void ParseDatasyncAddressesResponse(const NJson::TJsonValue& addressesResponse) override;
        void ParseDatasyncKeyValueResponse(const NJson::TJsonValue& kvResponse) override;
        void ParseDatasyncSettingsResponse(const NJson::TJsonValue& response) override;

    private:
        static TMaybe<TString> GetDatasyncResponseBody(const NJson::TJsonValue& response);
    };

}   // namespace NAlice::NCuttlefish::NAppHostServices

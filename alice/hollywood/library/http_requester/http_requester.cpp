#include "http_requester.h"

#include <library/cpp/http/simple/http_client.h>

#include <util/generic/hash.h>
#include <util/stream/str.h>


namespace NAlice::NHollywood {

namespace {

class TSimpleHttpRequester : public IHttpRequester {
public:
    IHttpRequester& Add(
        const TString& requestId,
        EMethod method,
        const TString& url,
        const TString& body,
        const THashMap<TString, TString>& headers = {}
    ) override {
        RequestIdToParams[requestId] = TRequestParams{method, url, body, headers};
        return *this;
    }

    IHttpRequester& Start() override {
        return *this;
    }

    TString Fetch(const TString& requestId) override {
        const TRequestParams& params = RequestIdToParams.at(requestId);

        TStringBuf host;
        TStringBuf path;
        SplitUrlToHostAndPath(params.Url, host, path);

        TStringStream out;
        try {
            if (params.Method == EMethod::Post) {
                TSimpleHttpClient(TSimpleHttpClient::TOptions(host)).DoPost(path, params.Body, &out, params.Headers);
            } else {
                TSimpleHttpClient(TSimpleHttpClient::TOptions(host)).DoGet(path, &out, params.Headers);
            }
        } catch (const THttpRequestException& e) {
            ythrow yexception() << "Error processing url " << params.Url << ": "
                                << "got status code " << e.GetStatusCode() << ", "
                                << e.what();
        }
        return out.Str();
    }

private:
    struct TRequestParams {
        EMethod Method = EMethod::Get;
        TString Url;
        TString Body;
        THashMap<TString, TString> Headers;
    };

    THashMap<TString, TRequestParams> RequestIdToParams;
};

} // namespace

THolder<IHttpRequester> MakeSimpleHttpRequester() {
    return MakeHolder<TSimpleHttpRequester>();
}

} // namespace NAlice::NHollywood

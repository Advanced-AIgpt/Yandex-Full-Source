#pragma once

#include <alice/megamind/library/util/http_response.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NMegamind::NTesting {

class TMockHttpResponse : public IHttpResponse {
public:
    TMockHttpResponse(bool defaultInit = false);

    MOCK_METHOD(IHttpResponse&, AddHeader, (const THttpInputHeader&), (override));
    MOCK_METHOD(IHttpResponse&, SetContent, (const TString&), (override));
    MOCK_METHOD(IHttpResponse&, SetContentType, (TStringBuf), (override));
    MOCK_METHOD(IHttpResponse&, SetHttpCode, (HttpCodes), (override));
    MOCK_METHOD(TString, Content, (), (const, override));
    MOCK_METHOD(HttpCodes, HttpCode, (), (const, override));
    MOCK_METHOD(THttpHeaders&, Headers, (), (const, override));

protected:
    MOCK_METHOD(void, DoOut, (), (const, override));

private:
    TString Content_;
    TString ContentType_;
    THttpHeaders Headers_;
    HttpCodes Code_ = HTTP_OK;
};

} // NAlice::NMegamind::NTesting

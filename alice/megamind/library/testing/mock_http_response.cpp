#include "mock_http_response.h"

namespace NAlice::NMegamind::NTesting {

TMockHttpResponse::TMockHttpResponse(bool defaultInit) {
    if (!defaultInit) {
        return;
    }

    using namespace testing;
    EXPECT_CALL(*this, AddHeader(_)).WillRepeatedly([this](const THttpInputHeader& h) mutable -> IHttpResponse& { Headers_.AddHeader(h); return *this;});
    EXPECT_CALL(*this, SetHttpCode(_)).WillRepeatedly([this](HttpCodes code) mutable -> IHttpResponse& { Code_ = code; return *this; });
    EXPECT_CALL(*this, SetContent(_)).WillRepeatedly([this](const TString& content) mutable -> IHttpResponse& { Content_ = content; return *this; });
    EXPECT_CALL(*this, SetContentType(_)).WillRepeatedly([this](TStringBuf contentType) mutable -> IHttpResponse& { ContentType_ = contentType; return *this; });
    EXPECT_CALL(*this, Headers()).WillRepeatedly([this]() -> THttpHeaders& { return Headers_; });
    EXPECT_CALL(*this, HttpCode()).WillRepeatedly([this]() { return Code_; });
    EXPECT_CALL(*this, Content()).WillRepeatedly([this]() { return Content_; });
}

} // NAlice::NMegamind::NTesting

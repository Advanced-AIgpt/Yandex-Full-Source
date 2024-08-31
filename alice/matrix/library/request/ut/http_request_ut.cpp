#include <alice/matrix/library/request/http_request.h>

#include <library/cpp/testing/gtest/gtest.h>

TEST(TMatrixHttpRequestTest, TestHttpRequestContentIsJson) {
    THttpHeaders headers;

    EXPECT_FALSE(NMatrix::NPrivate::HttpRequestContentIsJson(headers));

    headers.AddOrReplaceHeader("Content-Type", "none");
    EXPECT_FALSE(NMatrix::NPrivate::HttpRequestContentIsJson(headers));

    headers.AddOrReplaceHeader("Content-Type", "application/json");
    EXPECT_TRUE(NMatrix::NPrivate::HttpRequestContentIsJson(headers));
}

TEST(TMatrixHttpRequestTest, TestGetRequestIdFromHttpRequestHeaders) {
    {
        THttpHeaders headers;
        headers.AddHeader("unkown_header", "value");
        EXPECT_EQ(NMatrix::NPrivate::GetRequestIdFromHttpRequestHeaders(headers), "empty-request-id");
    }

    {
        THttpHeaders headers;
        headers.AddHeader("X-ReQuEst-id", "request_id");
        EXPECT_EQ(NMatrix::NPrivate::GetRequestIdFromHttpRequestHeaders(headers), "request_id");
    }

    {
        THttpHeaders headers;

        headers.AddHeader("x-apphost-request-reqid", "apphost_request_id");
        EXPECT_EQ(NMatrix::NPrivate::GetRequestIdFromHttpRequestHeaders(headers), "apphost_request_id");

        headers.AddHeader("x-apphost-request-ruid", "apphost_ruid");
        EXPECT_EQ(NMatrix::NPrivate::GetRequestIdFromHttpRequestHeaders(headers), "apphost_request_id-apphost_ruid");

        // The explicit x-request-id is more important than apphost headers
        headers.AddHeader("x-request-id", "request_id");
        EXPECT_EQ(NMatrix::NPrivate::GetRequestIdFromHttpRequestHeaders(headers), "request_id");
    }
}

TEST(TMatrixHttpRequestTest, TestGetCensoredHeaderValue) {
    // Do not censor well-known headers
    EXPECT_EQ(NMatrix::NPrivate::GetCensoredHeaderValue("HoSt", "valuevalue"), "valuevalue");

    // Censor unknown headers
    EXPECT_EQ(NMatrix::NPrivate::GetCensoredHeaderValue("unkown_header", "valuevalue"), "value*****");
    EXPECT_EQ(NMatrix::NPrivate::GetCensoredHeaderValue("unkown_header", "oddlengthvaluevalue"), "oddlength**********");
}

TEST(TMatrixHttpRequestTest, TestGetReplyHeadersString) {
    THttpHeaders headers;

    EXPECT_EQ(NMatrix::NPrivate::GetReplyHeadersString(headers), "");

    headers.AddHeader("foo", "value");
    EXPECT_EQ(NMatrix::NPrivate::GetReplyHeadersString(headers), "\r\nfoo: value");

    headers.AddHeader("bar", "value");
    EXPECT_EQ(NMatrix::NPrivate::GetReplyHeadersString(headers), "\r\nfoo: value\r\nbar: value");
}

#include "apphost_http_request_signer.h"
#include "constants.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NCuttlefish::NAws;

namespace {

NAppHostHttp::THttpRequest GetTestHttpRequest(bool withPath) {
    static const TVector<std::pair<TString, TString>> headers = {
        {"Host", "some.host.override.net"},
        {"header_name_1", "header_value_1"},
        {"header_name_2", "header_value_2"},
    };

    NAppHostHttp::THttpRequest httpRequest;

    httpRequest.SetMethod(NAppHostHttp::THttpRequest::Put);
    if (withPath) {
        httpRequest.SetPath("/path");
    }
    httpRequest.SetContent("content");

    for (const auto& [name, value] : headers) {
        auto* header = httpRequest.AddHeaders();
        header->SetName(name);
        header->SetValue(value);
    }

    return httpRequest;
}

} // namespace

class TCuttlefishAwsAppHostHttpRequestSignerTest: public TTestBase {
    UNIT_TEST_SUITE(TCuttlefishAwsAppHostHttpRequestSignerTest);
    UNIT_TEST(TestSimple);
    UNIT_TEST(TestEmptyCredentials);
    UNIT_TEST_SUITE_END();

public:
    void TestSimple() {
        TAppHostHttpRequestSigner signer(
            "aws_key_id",
            "aws_secret",
            S3_SERVICE_NAME,
            S3_YANDEX_INTERNAL_REGION
        );

        for (ui32 withPath = 0; withPath < 2; ++withPath) {
            NAppHostHttp::THttpRequest httpRequest = GetTestHttpRequest((bool)withPath);
            signer.SignRequest(S3_YANDEX_INTERNAL_HOST, httpRequest);

            UNIT_ASSERT_EQUAL(httpRequest.GetMethod(), NAppHostHttp::THttpRequest::Put);
            UNIT_ASSERT_VALUES_EQUAL(httpRequest.GetPath(), (withPath ? "/path" : ""));
            UNIT_ASSERT_VALUES_EQUAL(httpRequest.GetContent(), "content");

            TMap<TString, TString> headers;
            for (const auto& header : httpRequest.GetHeaders()) {
                headers[header.GetName()] = header.GetValue();
            }

            UNIT_ASSERT_VALUES_EQUAL(headers.size(), 6);
            // Current time is a part of signature
            // So we can't just canonize values of some headers
            UNIT_ASSERT(headers.contains("x-amz-date"));
            {
                const TString authorizationHeader = headers.at("authorization");
                UNIT_ASSERT_C(authorizationHeader.StartsWith("AWS4-HMAC-SHA256 "), authorizationHeader);
                UNIT_ASSERT_VALUES_UNEQUAL(authorizationHeader.find(" Credential=aws_key_id/"), TString::npos);
                UNIT_ASSERT_VALUES_UNEQUAL(authorizationHeader.find("/us-east-1/s3/aws4_request, "), TString::npos);
                UNIT_ASSERT_VALUES_UNEQUAL(authorizationHeader.find(" SignedHeaders=header_name_1;header_name_2;host;x-amz-content-sha256;x-amz-date,"), TString::npos);
                UNIT_ASSERT_VALUES_UNEQUAL(authorizationHeader.find(" Signature="), TString::npos);
            }

            UNIT_ASSERT_VALUES_EQUAL(headers.at("header_name_1"), "header_value_1");
            UNIT_ASSERT_VALUES_EQUAL(headers.at("header_name_2"), "header_value_2");
            UNIT_ASSERT_VALUES_EQUAL(headers.at("host"), "some.host.override.net");
            UNIT_ASSERT_VALUES_EQUAL(headers.at("x-amz-content-sha256"), "UNSIGNED-PAYLOAD");
        }
    }

    void TestEmptyCredentials() {
        TAppHostHttpRequestSigner signer(
            "",
            "",
            S3_SERVICE_NAME,
            S3_YANDEX_INTERNAL_REGION
        );

        for (ui32 withPath = 0; withPath < 2; ++withPath) {
            NAppHostHttp::THttpRequest httpRequest = GetTestHttpRequest((bool)withPath);
            signer.SignRequest(S3_YANDEX_INTERNAL_HOST, httpRequest);

            UNIT_ASSERT_EQUAL(httpRequest.GetMethod(), NAppHostHttp::THttpRequest::Put);
            UNIT_ASSERT_VALUES_EQUAL(httpRequest.GetPath(), (withPath ? "/path" : ""));
            UNIT_ASSERT_VALUES_EQUAL(httpRequest.GetContent(), "content");

            TMap<TString, TString> headers;
            for (const auto& header : httpRequest.GetHeaders()) {
                headers[header.GetName()] = header.GetValue();
            }

            UNIT_ASSERT_VALUES_EQUAL(headers.size(), 3);
            UNIT_ASSERT_VALUES_EQUAL(headers.at("header_name_1"), "header_value_1");
            UNIT_ASSERT_VALUES_EQUAL(headers.at("header_name_2"), "header_value_2");
            UNIT_ASSERT_VALUES_EQUAL(headers.at("host"), "some.host.override.net");
        }
    }

};

UNIT_TEST_SUITE_REGISTRATION(TCuttlefishAwsAppHostHttpRequestSignerTest)

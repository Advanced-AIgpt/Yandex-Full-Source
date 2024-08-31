#include "request.h"

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NHttpFetcher;

Y_UNIT_TEST_SUITE(BassFetcher) {
    Y_UNIT_TEST(ProxyCreateFromHeaderSmoke) {
        UNIT_ASSERT(TProxySettings::CreateFromHeader("host"));
        UNIT_ASSERT(!TProxySettings::CreateFromHeader("host:12345:invalid_type"));
        UNIT_ASSERT(TProxySettings::CreateFromHeader("host:12345"));
    }

    Y_UNIT_TEST(ProxyCreateFromHeaderBehindBalancer) {
        NUri::TUri uri;
        uri.Parse("http://ya.ru/");

        auto proxy = TProxySettings::CreateFromHeader("host:12345:behind_balancer");
        UNIT_ASSERT(proxy);

        TString url, headers;
        TMaybe<TString> overrideHostPort;
        proxy->PrepareRequest(uri, TCgiParameters{"a=b"}, url, headers, overrideHostPort);
        UNIT_ASSERT_VALUES_EQUAL(url, "http://host:12345/?a=b");
        UNIT_ASSERT_VALUES_EQUAL(headers, "X-Host: http://ya.ru/?a=b");
        UNIT_ASSERT(!overrideHostPort.Defined());
    }

    Y_UNIT_TEST(ProxyCreateFromHeaderDefault) {
        NUri::TUri uri;
        uri.Parse("http://ya.ru/");

        auto proxy = TProxySettings::CreateFromHeader("host:12345");
        UNIT_ASSERT(proxy);

        TString url, headers;
        TMaybe<TString> overrideHostPort;
        proxy->PrepareRequest(uri, TCgiParameters{"a=b"}, url, headers, overrideHostPort);
        UNIT_ASSERT_VALUES_EQUAL(url, "http://host:12345/?a=b");
        UNIT_ASSERT_VALUES_EQUAL(headers, "X-Host: http://ya.ru/?a=b");
        UNIT_ASSERT(!overrideHostPort.Defined());
    }

    Y_UNIT_TEST(ProxyCreateFromHeaderNormal) {
        NUri::TUri uri;
        uri.Parse("http://ya.ru/");

        auto proxy = TProxySettings::CreateFromHeader("host:12345:normal");
        UNIT_ASSERT(proxy);

        TString url, headers;
        TMaybe<TString> overrideHostPort;
        proxy->PrepareRequest(uri, TCgiParameters{"a=b"}, url, headers, overrideHostPort);
        UNIT_ASSERT(url.Empty());
        UNIT_ASSERT_VALUES_EQUAL(headers, "X-Host: http://ya.ru/?a=b");
        UNIT_ASSERT(overrideHostPort.Defined());
        UNIT_ASSERT_VALUES_EQUAL(*overrideHostPort, "host:12345");
    }
}

} // namespace

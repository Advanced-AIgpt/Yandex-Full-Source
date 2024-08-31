#include "request_builder.h"

#include <alice/library/network/headers.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>

namespace {

using namespace NAlice;
using namespace NAlice::NAppHostRequest;


Y_UNIT_TEST_SUITE(AppHostRequestBuilder) {
    Y_UNIT_TEST(MethodAndSchemeSmoke1) {
        TAppHostHttpProxyRequestBuilder builder;

        {
            auto proto = builder.CreateRequest();
            UNIT_ASSERT(proto.GetScheme() == TAppHostHttpProxyRequestBuilder::EScheme::THttpRequest_EScheme_Http);
            UNIT_ASSERT(proto.GetMethod() == TAppHostHttpProxyRequestBuilder::EMethod::THttpRequest_EMethod_Get);
        }
    }

    Y_UNIT_TEST(MethodAndSchemeSmoke2) {
        TAppHostHttpProxyRequestBuilder builder;

        {
            builder.SetScheme(TAppHostHttpProxyRequestBuilder::EScheme::THttpRequest_EScheme_Https);
            builder.SetMethod(TAppHostHttpProxyRequestBuilder::EMethod::THttpRequest_EMethod_Post);
            auto proto = builder.CreateRequest();
            UNIT_ASSERT(proto.GetScheme() == TAppHostHttpProxyRequestBuilder::EScheme::THttpRequest_EScheme_Https);
            UNIT_ASSERT(proto.GetMethod() == TAppHostHttpProxyRequestBuilder::EMethod::THttpRequest_EMethod_Post);
        }
    }

    Y_UNIT_TEST(ContentTypeSmoke1) {
        TAppHostHttpProxyRequestBuilder builder;
        builder.AddHeader("test", "value");

        {
            auto request = builder.CreateRequest();
            UNIT_ASSERT_VALUES_EQUAL(request.HeadersSize(), 1);
            const auto& header = request.GetHeaders(0);
            UNIT_ASSERT_VALUES_EQUAL(header.GetName(), "test");
            UNIT_ASSERT_VALUES_EQUAL(header.GetValue(), "value");
        }
    }

    Y_UNIT_TEST(ContentTypeSmoke2) {
        TAppHostHttpProxyRequestBuilder builder;
        builder.AddHeader("test", "value");

        {
            builder.SetContentType("superduper");
            auto request = builder.CreateRequest();

            UNIT_ASSERT_VALUES_EQUAL(request.HeadersSize(), 2);
            {
                const auto& h = request.GetHeaders(0);
                UNIT_ASSERT_VALUES_EQUAL(h.GetName(), "test");
                UNIT_ASSERT_VALUES_EQUAL(h.GetValue(), "value");
            }
            {
                const auto& h = request.GetHeaders(1);
                UNIT_ASSERT_VALUES_EQUAL(h.GetName(), NNetwork::HEADER_CONTENT_TYPE);
                UNIT_ASSERT_VALUES_EQUAL(h.GetValue(), "superduper");
            }
        }
    }

    Y_UNIT_TEST(ContentTypeSmoke3) {
        TAppHostHttpProxyRequestBuilder builder;
        builder.AddHeader("test", "value");

        {
            builder.SetContentType("newsuperduper");
            auto request = builder.CreateRequest();

            UNIT_ASSERT_VALUES_EQUAL(request.HeadersSize(), 2);

            {
                const auto& h = request.GetHeaders(0);
                UNIT_ASSERT_VALUES_EQUAL(h.GetName(), "test");
                UNIT_ASSERT_VALUES_EQUAL(h.GetValue(), "value");
            }
            {
                const auto& h = request.GetHeaders(1);
                UNIT_ASSERT_VALUES_EQUAL(h.GetName(), NNetwork::HEADER_CONTENT_TYPE);
                UNIT_ASSERT_VALUES_EQUAL(h.GetValue(), "newsuperduper");
            }
        }
    }

    Y_UNIT_TEST(BodyMethod1) {
        TAppHostHttpProxyRequestBuilder builder;

        {
            builder.SetBody("test", "post");
            auto proto = builder.CreateRequest();
            UNIT_ASSERT(proto.GetContent() == "test");
            UNIT_ASSERT(proto.GetMethod() == TAppHostHttpProxyRequestBuilder::EMethod::THttpRequest_EMethod_Post);
        }
    }

    Y_UNIT_TEST(BodyMethod2) {
        TAppHostHttpProxyRequestBuilder builder;

        {
            builder.SetBody("test", "PosT");
            auto proto = builder.CreateRequest();
            UNIT_ASSERT(proto.GetContent() == "test");
            UNIT_ASSERT(proto.GetMethod() == TAppHostHttpProxyRequestBuilder::EMethod::THttpRequest_EMethod_Post);
        }
    }
}

} // namespace

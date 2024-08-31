#include "rtlogtoken.h"

#include <alice/library/network/headers.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;
using namespace NAlice::NMegamind;

Y_UNIT_TEST_SUITE(RTLogTokenConstructor) {
    Y_UNIT_TEST(EmptyRuid) {
        TRTLoggerTokenConstructor token{{}};
        UNIT_ASSERT_VALUES_EQUAL_C(token.GetToken(), "", "empty ruid and nothing more");

        bool res = token.CheckHeader("x", "y");
        UNIT_ASSERT_C(!res, "return value for check header (empty ruid, irrelevant header)");
        UNIT_ASSERT_VALUES_EQUAL_C(token.GetToken(), "", "empty ruid and check irrelevant header");

        res = token.CheckHeader(NNetwork::HEADER_X_RTLOG_TOKEN, "invalid_token");
        UNIT_ASSERT_C(!res, "return value for check header (empty ruid and invalid version1 header without v2 header)");
        UNIT_ASSERT_VALUES_EQUAL_C(token.GetToken(), "", "get token (empty ruid, invalid version1 header, no version2 header)");

        res = token.CheckHeader(NNetwork::HEADER_X_RTLOG_TOKEN, "12345$v1$v1");
        UNIT_ASSERT_C(!res, "return value for check header (empty ruid and valid version1 header without v2 header)");
        UNIT_ASSERT_VALUES_EQUAL_C(token.GetToken(), "12345$v1$v1", "get token (empty ruid, valid version1 header, no version2 header)");

        res = token.CheckHeader(NNetwork::HEADER_X_YANDEX_REQ_ID, "v2");
        UNIT_ASSERT_C(!res, "return value for check header (empty ruid and valid version1 header, invalid version2 header)");
        UNIT_ASSERT_VALUES_EQUAL_C(token.GetToken(), "12345$v1$v1", "get token (empty ruid, valid version1 header, invalid version2 header)");

        res = token.CheckHeader(NNetwork::HEADER_X_YANDEX_REQ_ID, "12345$super$duper");
        UNIT_ASSERT_C(res, "return value for check header (empty ruid and valid version1 header, valid version2 header)");
        UNIT_ASSERT_VALUES_EQUAL_C(token.GetToken(), "12345$v1$v1", "get token (empty ruid, valid version1 header, valid version2 header)");
    }

    Y_UNIT_TEST(ValidRuid) {
        TRTLoggerTokenConstructor token{"0123456789"};
        UNIT_ASSERT_VALUES_EQUAL_C(token.GetToken(), "", "get token (valid ruid, neither headers nor apphostreqid)");

        bool res = token.CheckHeader("x", "y");
        UNIT_ASSERT_C(!res, "return value for check header (valid ruid, irrelevant header)");
        UNIT_ASSERT_VALUES_EQUAL_C(token.GetToken(), "", "valid ruid and check irrelevant header");

        res = token.CheckHeader(NNetwork::HEADER_X_RTLOG_TOKEN, "12345$v1$v1");
        UNIT_ASSERT_C(!res, "return value for check header (valid ruid and valid version1 header without v2 header)");
        UNIT_ASSERT_VALUES_EQUAL_C(token.GetToken(), "12345$v1$v1", "get token (empty ruid, valid version1 header, no version2 header)");

        res = token.CheckHeader(NNetwork::HEADER_X_YANDEX_REQ_ID, "v2");
        UNIT_ASSERT_C(!res, "return value for check header (valid ruid and valid version1 header, invalid version2 header)");
        UNIT_ASSERT_VALUES_EQUAL_C(token.GetToken(), "12345$v1$v1", "get token (empty ruid, valid version1 header, invalid version2 header)");

        res = token.CheckHeader(NNetwork::HEADER_X_YANDEX_REQ_ID, "12345$super$duper");
        UNIT_ASSERT_C(res, "return value for check header (valid ruid and valid version1 header, valid version2 header)");
        UNIT_ASSERT_VALUES_EQUAL_C(token.GetToken(), "12345$super$duper-0123456789", "get token (empty ruid, valid version1 header, valid version2 header)");
    }
}

} // namespace

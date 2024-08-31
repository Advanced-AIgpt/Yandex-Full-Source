#include "url_builder.h"

#include <alice/library/client/protos/client_info.pb.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {

const TClientFeatures CLIENT_FEATURES = TClientFeatures(TClientInfoProto(), TRawExpFlags());
const TUserLocation USER_LOCATION = TUserLocation("", "ru");
const TCgiParameters CGI_PARAMETERS = TCgiParameters(TStringBuf("query_source=dialog"));

Y_UNIT_TEST_SUITE(UrlBuilder) {
    Y_UNIT_TEST(UtmReferrer) {
        using namespace NAlice;
        TClientInfoProto proto;

        proto.SetAppId("ru.yandex.searchplugin");
        UNIT_ASSERT_EQUAL(AddUtmReferrer(TClientInfo{proto}, "http://ya.ru"), "http://ya.ru/?utm_referrer=https%253A%252F%252Fyandex.ru%252Fsearchapp%253Ffrom%253Dalice%2526text%253D");

        proto.SetAppId("yabro");
        UNIT_ASSERT_EQUAL(AddUtmReferrer(TClientInfo{proto}, "http://ya.ru"), "http://ya.ru/?utm_referrer=https%253A%252F%252Fyandex.ru%252F%253Ffrom%253Dalice");
    }
}

Y_UNIT_TEST_SUITE(SearchUri) {
    Y_UNIT_TEST(GenerateSearchUriDefault) {
        TString uri = GenerateSearchUri(CLIENT_FEATURES, USER_LOCATION, EContentSettings::without, "test", true);
        UNIT_ASSERT_STRING_CONTAINS(uri, "query_source=alice");
    }
    Y_UNIT_TEST(GenerateSearchUriSourceSpecified) {
        TString uri = GenerateSearchUri(CLIENT_FEATURES, USER_LOCATION, EContentSettings::without, "test", true, CGI_PARAMETERS);
        UNIT_ASSERT_STRING_CONTAINS(uri, "query_source=dialog");
        UNIT_ASSERT_EQUAL(uri.Contains("query_source=alice"), false);
    }
    Y_UNIT_TEST(GenerateSearchAdsUriDefault) {
        TString uri = GenerateSearchAdsUri(CLIENT_FEATURES, USER_LOCATION, EContentSettings::without, "test");
        UNIT_ASSERT_STRING_CONTAINS(uri, "query_source=alice");
    }
    Y_UNIT_TEST(GenerateSearchAdsUriSourceSpecified) {
        TString uri = GenerateSearchAdsUri(CLIENT_FEATURES, USER_LOCATION, EContentSettings::without, "test", CGI_PARAMETERS);
        UNIT_ASSERT_STRING_CONTAINS(uri, "query_source=dialog");
        UNIT_ASSERT_EQUAL(uri.Contains("query_source=alice"), false);
    }
}

} // namespace

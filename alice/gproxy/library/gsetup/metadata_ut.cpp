#include <library/cpp/testing/gtest/gtest.h>

#include <alice/gproxy/library/gproxy/metadata.h>


TEST(Metadata, SessionId) {
    std::multimap<grpc::string_ref, grpc::string_ref> from;
    NGProxy::TMetadata to;

    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-ya-session-id", "Hello"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-ya-session-id", "World"));

    EXPECT_TRUE(NGProxy::FillMetadata(from, to));
    EXPECT_EQ("Hello", to.GetSessionId());
}


TEST(Metadata, SupportedFeatures) {
    std::multimap<grpc::string_ref, grpc::string_ref> from;
    NGProxy::TMetadata to;

    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-ya-supported-features", "hello"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-ya-supported-features", "goodbye"));

    EXPECT_TRUE(NGProxy::FillMetadata(from, to));
    EXPECT_EQ(2u, to.SupportedFeaturesSize());
    EXPECT_EQ("hello", to.GetSupportedFeatures(0));
    EXPECT_EQ("goodbye", to.GetSupportedFeatures(1));
}

TEST(Metadata, Authorization) {
    std::multimap<grpc::string_ref, grpc::string_ref> from;
    NGProxy::TMetadata to;

    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("Authorization", "OAuth hello"));

    EXPECT_TRUE(NGProxy::FillMetadata(from, to));
    EXPECT_EQ("hello", to.GetOAuthToken());
}

TEST(Metadata, AppHostParams) {
    std::multimap<grpc::string_ref, grpc::string_ref> from;

    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-srcrwr", "FOO:www.yandex.ru:80"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-srcrwr", "BAR:alice.com:42"));

    NJson::TJsonValue params = NGProxy::CreateAppHostParams(from, true);

    Cerr << params << Endl;

    EXPECT_TRUE(params.Has("srcrwr"));
    EXPECT_TRUE(params["srcrwr"].Has("FOO"));
    EXPECT_TRUE(params["srcrwr"].Has("BAR"));

    EXPECT_EQ("www.yandex.ru:80", params["srcrwr"]["FOO"].GetString());
    EXPECT_EQ("alice.com:42", params["srcrwr"]["BAR"].GetString());
}

TEST(Metadata, AppHostParamsNoSrcrwr) {
    std::multimap<grpc::string_ref, grpc::string_ref> from;

    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-srcrwr", "FOO:www.yandex.ru:80"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-srcrwr", "BAR:alice.com:42"));

    NJson::TJsonValue params = NGProxy::CreateAppHostParams(from, false);

    Cerr << params << Endl;

    EXPECT_TRUE(params.Has("srcrwr"));
    EXPECT_FALSE(params["srcrwr"].Has("FOO"));
    EXPECT_FALSE(params["srcrwr"].Has("BAR"));
}

TEST(Metadata, AppHostDumpRequestsResponses) {
    std::multimap<grpc::string_ref, grpc::string_ref> from;

    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-dump-source-requests", "1"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-dump-source-responses", "1"));

    NJson::TJsonValue paramsDumpAllowed = NGProxy::CreateAppHostParams(from, false, true);

    Cerr << paramsDumpAllowed << Endl;

    EXPECT_EQ("1", paramsDumpAllowed["dump_source_requests"]);
    EXPECT_EQ("1", paramsDumpAllowed["dump_source_responses"]);

    NJson::TJsonValue paramsDumpNotAllowed = NGProxy::CreateAppHostParams(from, false, false);

    Cerr << paramsDumpNotAllowed << Endl;

    EXPECT_FALSE(paramsDumpNotAllowed.Has("dump_source_requests"));
    EXPECT_FALSE(paramsDumpNotAllowed.Has("dump_source_responses"));
}

TEST(Metadata, RequestMetaProperties) {
    std::multimap<grpc::string_ref, grpc::string_ref> from;
    NGProxy::TMetadata to;

    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-yandex-internal-request", "1"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-ya-random-seed", "12345"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-ya-user-lang", "RU-ru"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-ya-user-ticket", "buymetheticket"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-ya-user-agent", "shiny/UserAgent"));

    EXPECT_TRUE(NGProxy::FillMetadata(from, to));
    EXPECT_EQ(12345u, to.GetRandomSeed());
    EXPECT_EQ("RU-ru", to.GetUserLang());
    EXPECT_EQ("buymetheticket", to.GetUserTicket());
    EXPECT_EQ("shiny/UserAgent", to.GetUserAgent());
}

TEST(Metadata, RequestMetaPropertiesWithUA) {
    std::multimap<grpc::string_ref, grpc::string_ref> from;
    NGProxy::TMetadata to;

    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-yandex-internal-request", "1"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-ya-random-seed", "12345"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-ya-user-lang", "RU-ru"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("x-ya-user-ticket", "buymetheticket"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("Accept-Language", "ru-RU"));
    from.insert(std::make_pair<grpc::string_ref, grpc::string_ref>("User-Agent", "shiny/UserAgent"));

    EXPECT_TRUE(NGProxy::FillMetadata(from, to));
    EXPECT_EQ(12345u, to.GetRandomSeed());
    EXPECT_EQ("RU-ru", to.GetUserLang());
    EXPECT_EQ("buymetheticket", to.GetUserTicket());
    EXPECT_EQ("shiny/UserAgent", to.GetUserAgent());
    EXPECT_EQ("ru-RU", to.GetLanguage());
}

TEST(Metadata, LoggingHeaders) {
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-srcrwr"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-graphrwr"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-real-ip"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-session-id"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-request-id"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-device-id"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-uuid"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-firmware"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-language"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-app-type"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-app-id"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-rtlog-token"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-application"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-tags"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-location"));
    EXPECT_TRUE(NGProxy::LoggingIsAllowedForHeader("x-ya-user-agent"));

    EXPECT_FALSE(NGProxy::LoggingIsAllowedForHeader("x-auth-token"));
    EXPECT_FALSE(NGProxy::LoggingIsAllowedForHeader("X-auth-token"));
    EXPECT_FALSE(NGProxy::LoggingIsAllowedForHeader("X-Auth-Token"));

    EXPECT_FALSE(NGProxy::LoggingIsAllowedForHeader("authorization"));
    EXPECT_FALSE(NGProxy::LoggingIsAllowedForHeader("Authorization"));

    EXPECT_FALSE(NGProxy::LoggingIsAllowedForHeader("cookie"));
    EXPECT_FALSE(NGProxy::LoggingIsAllowedForHeader("Cookie"));
}

#include <alice/library/frame/directive_builder.h>

#include <library/cpp/testing/gtest/gtest.h>
#include <util/string/cast.h>

namespace {

using namespace NAlice;

TEST(FrameDirectiveBuilder, Empty) {
    TTypedSemanticFrameDirectiveBuilder builder;

    ASSERT_EQ(ToString(builder.BuildJsonDirective()),
              R"({"name":"@@mm_semantic_frame","payload":{},"type":"server_action"})");

    ASSERT_EQ(builder.BuildDialogUrl(),
              R"(dialog://?directives=%5B%7B%22name%22%3A%22@@mm_semantic_frame%22%2C%22payload%22%3A%7B%7D%2C%22type%22%3A%22server_action%22%7D%5D)");
}

TEST(FrameDirectiveBuilder, Smoke) {
    // Just for testing purposes.
    TTypedSemanticFrame tsf;
    tsf.MutableSearchSemanticFrame()->MutableQuery()->SetStringValue("мама ела раму");

    TTypedSemanticFrameDirectiveBuilder builder;
    builder.SetTypedSemanticFrame(tsf);
    builder.SetScenarioAnalyticsInfo("lovely-scenario", "lovely-purpose", "lovely-info");

    ASSERT_EQ(ToString(builder.BuildJsonDirective()),
              R"({"name":"@@mm_semantic_frame","payload":{"typed_semantic_frame":{"search_semantic_frame":{"query":{"string_value":"мама ела раму"}}},"analytics":{"product_scenario":"lovely-scenario","purpose":"lovely-purpose","origin":"Scenario","origin_info":"lovely-info"}},"type":"server_action"})");
    ASSERT_EQ(builder.BuildDialogUrl(),
              R"(dialog://?directives=%5B%7B%22name%22%3A%22@@mm_semantic_frame%22%2C%22payload%22%3A%7B%22typed_semantic_frame%22%3A%7B%22search_semantic_frame%22%3A%7B%22query%22%3A%7B%22string_value%22%3A%22%D0%BC%D0%B0%D0%BC%D0%B0+%D0%B5%D0%BB%D0%B0+%D1%80%D0%B0%D0%BC%D1%83%22%7D%7D%7D%2C%22analytics%22%3A%7B%22product_scenario%22%3A%22lovely-scenario%22%2C%22purpose%22%3A%22lovely-purpose%22%2C%22origin%22%3A%22Scenario%22%2C%22origin_info%22%3A%22lovely-info%22%7D%7D%2C%22type%22%3A%22server_action%22%7D%5D)");
}


} // ns

#include <alice/begemot/lib/feature_aggregator/config.h>
#include <alice/begemot/lib/feature_aggregator/enum_generator.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NFeatureAggregator;

Y_UNIT_TEST_SUITE(EnumGenerator) {

    Y_UNIT_TEST(EmptyConfig) {
        TFeatureAggregatorConfig cfg = ReadConfigFromProtoTxtString(R"(
            Features: []
        )");
        TString expected = TString::Join(
R"(syntax = "proto3";
enum ETest {
    )", RESERVED_FEATURE_NAME, R"( = 0;
}
)");

        UNIT_ASSERT_STRINGS_EQUAL(expected, GenerateProtoEnum(cfg, "ETest"));
    }

    Y_UNIT_TEST(OnlyDisabled) {
        TFeatureAggregatorConfig cfg = ReadConfigFromProtoTxtString(R"(
            Features: [
                {
                    Index: 1
                    Name: "F1"
                    IsDisabled: true
                    Rules: [
                        {
                            Entity: {
                                EntityType: "a"
                            }
                        }
                    ]
                }
            ]
        )");
        TString expected = TString::Join(
R"(syntax = "proto3";
enum ETest {
    reserved 1;
    reserved "F1";
    )", RESERVED_FEATURE_NAME, R"( = 0;
}
)");

        UNIT_ASSERT_STRINGS_EQUAL(expected, GenerateProtoEnum(cfg, "ETest"));
    }

    Y_UNIT_TEST(SeveralFeatures) {
        TFeatureAggregatorConfig cfg = ReadConfigFromProtoTxtString(R"(
            Features: [
                {
                    Index: 1
                    Name: "F1"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "a"
                            }
                        }
                    ]
                },
                {
                    Index: 2
                    Name: "F2"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "a"
                            }
                        }
                    ]
                },
                {
                    Index: 5
                    Name: "F5"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "a"
                            }
                        }
                    ]
                },
                {
                    Index: 10
                    Name: "F10"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "a"
                            }
                        }
                    ]
                }
            ]
        )");
        TString expected = TString::Join(
R"(syntax = "proto3";
enum ETest {
    )", RESERVED_FEATURE_NAME, R"( = 0;
    F1 = 1;
    F2 = 2;
    F5 = 5;
    F10 = 10;
}
)");

        UNIT_ASSERT_STRINGS_EQUAL(expected, GenerateProtoEnum(cfg, "ETest"));
    }

    Y_UNIT_TEST(DisabledFeatures) {
        TFeatureAggregatorConfig cfg = ReadConfigFromProtoTxtString(R"(
            Features: [
                {
                    Index: 1
                    Name: "F1"
                    IsDisabled: true
                    Rules: [
                        {
                            Entity: {
                                EntityType: "a"
                            }
                        }
                    ]
                },
                {
                    Index: 2
                    Name: "F2"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "a"
                            }
                        }
                    ]
                },
                {
                    Index: 5
                    Name: "F5"
                    IsDisabled: true
                    Rules: [
                        {
                            Entity: {
                                EntityType: "a"
                            }
                        }
                    ]
                },
                {
                    Index: 10
                    Name: "F10"
                    Rules: [
                        {
                            Entity: {
                                EntityType: "a"
                            }
                        }
                    ]
                }
            ]
        )");
        TString expected = TString::Join(
R"(syntax = "proto3";
enum ETest {
    reserved 1, 5;
    reserved "F1", "F5";
    )", RESERVED_FEATURE_NAME, R"( = 0;
    F2 = 2;
    F10 = 10;
}
)");

        UNIT_ASSERT_STRINGS_EQUAL(expected, GenerateProtoEnum(cfg, "ETest"));
    }

    Y_UNIT_TEST(ProtoPackage) {
        TFeatureAggregatorConfig cfg = ReadConfigFromProtoTxtString(R"(
            Features: []
        )");
        TString expected = TString::Join(
R"(syntax = "proto3";
package NTest;
enum ETest {
    )", RESERVED_FEATURE_NAME, R"( = 0;
}
)");

        UNIT_ASSERT_STRINGS_EQUAL(expected, GenerateProtoEnum(cfg, "ETest", "NTest"));
    }

    Y_UNIT_TEST(GoPackage) {
        TFeatureAggregatorConfig cfg = ReadConfigFromProtoTxtString(R"(
            Features: []
        )");
        TString expected = TString::Join(
R"(syntax = "proto3";
option go_package = "a.yandex-team.ru/test";
enum ETest {
    )", RESERVED_FEATURE_NAME, R"( = 0;
}
)");
        UNIT_ASSERT_STRINGS_EQUAL(expected, GenerateProtoEnum(cfg, "ETest", Nothing(), "a.yandex-team.ru/test"));
    }

    Y_UNIT_TEST(JavaPackage) {
        TFeatureAggregatorConfig cfg = ReadConfigFromProtoTxtString(R"(
            Features: []
        )");
        TString expected = TString::Join(
R"(syntax = "proto3";
option java_package = "ru.yandex.test";
enum ETest {
    )", RESERVED_FEATURE_NAME, R"( = 0;
}
)");

        UNIT_ASSERT_STRINGS_EQUAL(expected, GenerateProtoEnum(cfg, "ETest", Nothing(), Nothing(), "ru.yandex.test"));
    }
}

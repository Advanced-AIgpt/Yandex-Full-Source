#include <alice/cuttlefish/library/evcheck/builder.h>
#include <alice/cuttlefish/library/evcheck/parser.h>
#include <library/cpp/testing/unittest/registar.h>


using namespace NVoice;


Y_UNIT_TEST_SUITE(FenceBuilder) {

    Y_UNIT_TEST(CreateParser) {
        TParserBuilder builder;

        builder.AddNewProfile()
            .AddField("Some Field", NODE_STRING, ENodeMode::Required)
            .AddField("Numder", NODE_NUMBER)
            .AddField("SubMap", NODE_MAP|NODE_ARRAY)
                .BeginSubMap()
                .AddField("SubField1")
                .AddField("SubField2")
                .EndSubMap()
            .AddField("The Last", NODE_BOOLEAN|NODE_NULL);
        builder.AddNewProfile({"a", "b", "c"})
            .AddField("The Last", NODE_STRING, ENodeMode::Required)
            .AddField("SubMap", NODE_MAP|NODE_ARRAY)
                .BeginSubMap()
                .AddField("SubField3")
                .AddField("SubField1")
                .EndSubMap()
            .AddField("Another Numder", NODE_NUMBER, ENodeMode::Required);

        TParser parser = builder.CreateParser();

        UNIT_ASSERT(parser.GetNodeByPath("Some Field"));
        UNIT_ASSERT(parser.GetNodeByPath("Numder"));
        UNIT_ASSERT(parser.GetNodeByPath("SubMap"));
        UNIT_ASSERT(parser.GetNodeByPath("SubMap/SubField1"));
        UNIT_ASSERT(parser.GetNodeByPath("SubMap/SubField2"));
        UNIT_ASSERT(parser.GetNodeByPath("SubMap/SubField3"));
        UNIT_ASSERT(parser.GetNodeByPath("The Last"));
        UNIT_ASSERT(parser.GetNodeByPath("Another Numder"));
        UNIT_ASSERT(parser.GetNodeByPath("No such field") == nullptr);
        UNIT_ASSERT(parser.GetProfilesMap().size() == 2);
    }

    Y_UNIT_TEST(CopyProfile) {
        TParserBuilder builder;

        builder.AddNewProfile({"a", "a", "a"})
            .AddField("String", NODE_STRING, ENodeMode::Required)
            .AddField("Numder", NODE_NUMBER)
            .AddField("SubMap", NODE_MAP)
                .BeginSubMap()
                .AddField("Anything", NODE_ANY, ENodeMode::Required)
                .EndSubMap();
        builder.AddCopiedPofile({"a", "a", "a"}, {"b", "b", "b"})
            .AddField("SubMap", NODE_MAP)
                .BeginSubMap()
                .RemoveField("Anything")
                .AddField("Something", NODE_ANY, ENodeMode::Required)
                .EndSubMap();

        TParser parser = builder.CreateParser();

        const TProfileFieldsMask& first = parser.GetProfilesMap().at(TProfileKey{"a", "a", "a"});
        UNIT_ASSERT(first.RequiredFields.Test(parser.GetNodeByPath("String")->Idx));
        UNIT_ASSERT(first.RequiredFields.Test(parser.GetNodeByPath("SubMap/Anything")->Idx));
        UNIT_ASSERT(!first.AllowedFields.Test(parser.GetNodeByPath("SubMap/Something")->Idx));

        const TProfileFieldsMask& second = parser.GetProfilesMap().at(TProfileKey{"b", "b", "b"});
        UNIT_ASSERT(second.RequiredFields.Test(parser.GetNodeByPath("String")->Idx));
        UNIT_ASSERT(second.RequiredFields.Test(parser.GetNodeByPath("SubMap/Something")->Idx));
        UNIT_ASSERT(!second.AllowedFields.Test(parser.GetNodeByPath("SubMap/Anything")->Idx));
    }

    Y_UNIT_TEST(SimpleValidation) {
        TParserBuilder builder;

        builder.AddNewProfile()
            .AddField("Some Field", NODE_STRING)
            .AddField("Numder", NODE_NUMBER);
        builder.AddNewProfile({"a", "b", "c"})
            .AddField("Numder", NODE_BOOLEAN)
            .AddField("TheMap", NODE_MAP)
                .BeginSubMap()
                .AddField("TheOnlyField", NODE_ARRAY)
                .EndSubMap();

        TParser parser = builder.CreateParser();

        UNIT_ASSERT(parser.ParseJson(R"__({})__"));
        UNIT_ASSERT(parser.ParseJson(R"__({
            "Numder": 14.5,
            "TheMap": {}
        })__"));
        UNIT_ASSERT(parser.ParseJson(R"__({
            "Some Field": "value",
            "Numder": true
        })__"));
        UNIT_ASSERT(parser.ParseJson(R"__({
            "Numder": 1,
            "TheMap": {
                "TheOnlyField": []
            },
            "Some Field": ""
        })__"));

        // "Number" has incorrect type
        UNIT_ASSERT(!parser.ParseJson(R"__({
            "Numder": "1",
            "TheMap": {
                "TheOnlyField": []
            },
            "Some Field": ""
        })__"));
        // "TheMap" has extra field
        UNIT_ASSERT(!parser.ParseJson(R"__({
            "Some Field": "",
            "Numder": 1,
            "TheMap": {
                "TheOnlyField": [],
                "OneMore": {}
            },
        })__"));
    }

    Y_UNIT_TEST(ProfileValidation) {
        TParserBuilder builder;

        builder.AddNewProfile({"", "", "XXX"})
            .AddField("event", NODE_MAP).BeginSubMap()
                .AddField("payload", NODE_MAP).BeginSubMap()
                    .AddField("speechkitVersion", NODE_STRING)
                    .AddField("auth_token", NODE_STRING, ENodeMode::Required)
                    .AddField("string", NODE_STRING, ENodeMode::Required)
                    .AddField("map", NODE_MAP)
                    .AddField("number", NODE_NUMBER);
        builder.AddNewProfile({"YYY", "", ""})
            .AddField("event", NODE_MAP).BeginSubMap()
                .AddField("payload", NODE_MAP).BeginSubMap()
                    .AddField("speechkitVersion", NODE_STRING, ENodeMode::Required)
                    .AddField("auth_token", NODE_STRING)
                    .AddField("string", NODE_STRING)
                    .AddField("map", NODE_MAP)
                        .BeginSubMap()
                        .AddField("sub", NODE_STRING, ENodeMode::Required)
                        .EndSubMap();

        TParser parser = builder.CreateParser();

        // no profile - fields of all profiles are allowed
        UNIT_ASSERT(parser.ParseJson(R"__({"event": {"payload": {
            "speechkitVersion": "_",
            "auth_token": "_"
        }}})__"));
        // no "AAA" field in any profile
        UNIT_ASSERT(!parser.ParseJson(R"__({"event": {"payload": {
            "speechkitVersion": "_",
            "auth_token": "_",
            "AAA": 1
        }}})__"));

        // fisrt profile
        UNIT_ASSERT(parser.ParseJson(R"__({"event": {"payload": {
            "speechkitVersion": "",
            "auth_token": "XXX",
            "string": "value",
            "map": {},
            "number": 3.14
        }}})__"));
        // required "string" field is absent
        UNIT_ASSERT(!parser.ParseJson(R"__({"event": {"payload": {
            "speechkitVersion": "",
            "auth_token": "XXX",
            "map": {}
        }}})__"));
        // invalid type of "map" field
        UNIT_ASSERT(!parser.ParseJson(R"__({"event": {"payload": {
            "speechkitVersion": "",
            "auth_token": "XXX",
            "string": "value"
            "map": 14
        }}})__"));

        // seconds profile, only required fields
        UNIT_ASSERT(parser.ParseJson(R"__({"event": {"payload": {
            "speechkitVersion": "YYY",
            "map": {
                "sub": "value"
            }
        }}})__"));
        // required "map/sub" field is absent
        UNIT_ASSERT(!parser.ParseJson(R"__({"event": {"payload": {
            "speechkitVersion": "YYY",
            "string": "value"
            "map": {},
            "auth_token": ""
        }}})__"));
        // "number" field isn't allowed for the profile
        UNIT_ASSERT(!parser.ParseJson(R"__({"event": {"payload": {
            "speechkitVersion": "YYY",
            "string": "value",
            "map": {
                "sub": "value"
            },
            "auth_token": "",
            "number": 1
        }}})__"));
    }

};


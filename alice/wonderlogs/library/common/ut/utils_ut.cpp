#include <alice/wonderlogs/library/common/ut/invalid_enum_message.pb.h>

#include <alice/wonderlogs/library/common/utils.h>

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/text_format.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NWonderlogs {

namespace {

const TString messageIn = R"(
{
    "OptionalEnum": 10,
    "RequiredEnum": "E_A",
    "RepeatedEnum": [
        0,
        "E_B",
        10
    ],
    "NestedMessage": {
        "RequiredEnum": "E_A",
        "RepeatedEnum": [
            "E_A",
            "E_B"
        ]
    },
    "NestedMessages": [
        {
            "OptionalEnum": 100,
            "RequiredEnum": "E_B",
            "RepeatedEnum": [
                "E_A",
                "E_B"
            ]
        }
    ],
    "NestedMessage3": {
        "Enum": 10,
        "RepeatedEnum": [
            0,
            "E_B",
            10
        ],
        "NestedMessage": {
            "Enum": "E_A",
            "RepeatedEnum": [
                "E_A",
                "E_B"
            ]
        },
        "NestedMessages": [
            {
                "Enum": 100,
                "RepeatedEnum": [
                    "E_A",
                    "E_B"
                ]
            }
        ]
    }
}
)";

const TString messageOutRequest = R"(
RequiredEnum: E_A
RepeatedEnum: [
    E_A,
    E_B
]
NestedMessage {
    RequiredEnum: E_A
    RepeatedEnum: [
        E_A,
        E_B
    ]
}
NestedMessages : [
    {
        RequiredEnum: E_B
        RepeatedEnum: [
            E_A,
            E_B
        ]
    }
]
NestedMessage3: {
    Enum: E_A
    RepeatedEnum: [
        E_A,
        E_B,
        E_A
    ]
    NestedMessage {
        Enum: E_A
        RepeatedEnum: [
            E_A,
            E_B
        ]
    }
    NestedMessages : [
        {
            Enum: E_A
            RepeatedEnum: [
                E_A,
                E_B
            ]
        }
    ]
}
)";

} // namespace

Y_UNIT_TEST_SUITE(Utils) {
    Y_UNIT_TEST(NormalizeUuid) {
        const TString expected = "29920e541aa043b7a41e74e6553705c2";
        UNIT_ASSERT_EQUAL(expected, NormalizeUuid("29920e541aa043b7a41e74e6553705c2"));
        UNIT_ASSERT_EQUAL(expected, NormalizeUuid("29920E541AA043B7A41E74E6553705C2"));
        UNIT_ASSERT_EQUAL(expected, NormalizeUuid("29920e54-1aa0-43b7-a41e-74e6553705c2"));
        UNIT_ASSERT_EQUAL(expected, NormalizeUuid("29920E54-1AA0-43B7-A41E-74E6553705C2"));
    }

    Y_UNIT_TEST(FixInvalidEnums) {
        TMessage actual;
        NAlice::JsonToProto(NJson::ReadJsonFastTree(messageIn), actual, /* validateUtf8= */ true,
                            /* ignoreUnknownFields= */ true);
        TMessage expected;
        {
            NProtoBuf::TextFormat::Parser parser;
            parser.ParseFromString(messageOutRequest, &expected);
        }
        bool containsUnknownFields = false;
        FixInvalidEnums(actual, containsUnknownFields);
        actual.mutable_unknown_fields()->Clear();
        actual.MutableNestedMessages(0)->mutable_unknown_fields()->Clear();
        UNIT_ASSERT(actual.IsInitialized());
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(CheckIp) {
        UNIT_ASSERT(IsIpValid("2a02:6b8:c14:359c:0:4237:c6c8:0"));
        UNIT_ASSERT(IsIpValid("77.88.55.77"));
        UNIT_ASSERT(!IsIpValid("2a02:6b8:c14:4686:0:4237:6b2d"));
        UNIT_ASSERT(!IsIpValid("0.0"));
    }

    Y_UNIT_TEST(MaybeBoolFromJsonTrue) {
        const auto json = NJson::ReadJsonFastTree({R"({"b": true})"});
        const auto b = MaybeBoolFromJson(json["b"]);
        UNIT_ASSERT(b);
        UNIT_ASSERT(*b);
    }

    Y_UNIT_TEST(MaybeBoolFromJsonFalse) {
        const auto json = NJson::ReadJsonFastTree({R"({"b": false})"});
        const auto b = MaybeBoolFromJson(json["b"]);
        UNIT_ASSERT(b);
        UNIT_ASSERT(!(*b));
    }

    Y_UNIT_TEST(MaybeBoolFromJsonNotBool) {
        const auto json = NJson::ReadJsonFastTree({R"({"b": "lolkek"})"});
        const auto b = MaybeBoolFromJson(json["b"]);
        UNIT_ASSERT(!b);
    }

    Y_UNIT_TEST(NotAliceTopic) {
        UNIT_ASSERT(!AliceTopic("chats"));
        UNIT_ASSERT(!AliceTopic("chats-gpu"));
        UNIT_ASSERT(!AliceTopic("messenger"));
    }

    Y_UNIT_TEST(AliceTopic) {
        UNIT_ASSERT(AliceTopic("quasargeneral-gpu"));
        UNIT_ASSERT(AliceTopic("dialogeneral-gpu"));
        UNIT_ASSERT(AliceTopic("dialog-maps-gpu"));
        UNIT_ASSERT(AliceTopic("dialogeneral"));
        UNIT_ASSERT(AliceTopic("quasar-general"));
        UNIT_ASSERT(AliceTopic("tv-general-gpu"));
    }
}

} // namespace NAlice::NWonderlogs

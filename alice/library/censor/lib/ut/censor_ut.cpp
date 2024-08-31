#include <alice/library/censor/lib/censor.h>

#include <alice/library/censor/lib/ut/private_message.pb.h>

#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/text_format.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {

const TString messageIn = R"(
PrivateMessage {
    String: "lolkek"
    Int: 1337
}
PublicMessageWithPrivateFields {
    PrivateString: "keklol"
    PrivateInt: 80085
    Enum: SecondValue
    PrivateResponseString: "qwe"
    PrivateDouble: 0.42
    PublicUint: 999
    PrivateStrings: ["qwe", "ewq"]
    PrivateInts: [123, 321]
    RequiredPrivate: 123
}
PublicMessage {
    PublicString: "ewq"
}
PrivateAsFieldMessage {
    PublicString: "ewq"
}
PrivateAsFieldMessages [
    {
        PublicString: "qwe"
    },
    {
        PublicString: "ewq"
    }
]
PrivateMessages [
    {
        String: "qwe"
        Int: 123
    },
    {
        String: "qweqwe"
        Int: 1234
    }
]
MessageWithRequiredFields {
    String: "keklol"
    PublicMessage: {
        PublicString: "qwe"
    }
}
Bytes: "123"
Fixed64: 42
Fixed32: 42
SFixed64: 42
SFixed32: 42
SInt64: 42
SInt32: 42
Struct: {}
RecursiveMessage: {
    RecursiveMessage {
        RecursiveMessages [
            {
                RecursiveMessage {}
            }
        ]
    }
}
MessageWithPrivateFieldOne: {
    MessageWithPrivateField {
        PrivateMessage {
            String: "lolkek"
            Int: 1337
        }
    }
}
MessageWithPrivateFieldTwo: {
    MessageWithPrivateField {
        PrivateMessage {
            String: "lolkek"
            Int: 1337
        }
    }
}
)";

const TString messageOutRequest = R"(
PrivateMessage {
}
PublicMessageWithPrivateFields {
    PrivateString: "***"
    PrivateInt: 0
    Enum: DefaultValue
    PrivateResponseString: "qwe"
    PrivateDouble: 0
    PublicUint: 999
    PrivateStrings: ["***"]
    PrivateInts: [0]
    RequiredPrivate: 0
}
PublicMessage {
    PublicString: "ewq"
}
PrivateAsFieldMessage {
}
PrivateAsFieldMessages {
}
PrivateMessages [
    {}, {}
]
MessageWithRequiredFields {
    String: ""
}
Bytes: "***"
Fixed64: 0
Fixed32: 0
SFixed64: 0
SFixed32: 0
SInt64: 0
SInt32: 0
Struct: { }
RecursiveMessage: { }
MessageWithPrivateFieldOne: {
    MessageWithPrivateField {
        PrivateMessage {}
    }
}
MessageWithPrivateFieldTwo: {
    MessageWithPrivateField {
        PrivateMessage {}
    }
}
)";

const TString messageOutRequestResponse = R"(
PrivateMessage {
}
PublicMessageWithPrivateFields {
    PrivateString: "***"
    PrivateInt: 0
    Enum: DefaultValue
    PrivateResponseString: "***"
    PrivateDouble: 0
    PublicUint: 999
    PrivateStrings: ["***"]
    PrivateInts: [0]
    RequiredPrivate: 0
}
PublicMessage {
    PublicString: "ewq"
}
PrivateAsFieldMessage {
}
PrivateAsFieldMessages {
}
PrivateMessages [
    {}, {}
]
MessageWithRequiredFields {
    String: ""
}
Bytes: "***"
Fixed64: 0
Fixed32: 0
SFixed64: 0
SFixed32: 0
SInt64: 0
SInt32: 0
Struct: { }
RecursiveMessage: { }
MessageWithPrivateFieldOne: {
    MessageWithPrivateField {
        PrivateMessage {}
    }
}
MessageWithPrivateFieldTwo: {
    MessageWithPrivateField {
        PrivateMessage {}
    }
}
)";

const TString messageInEmptyMessage = R"(
MessageWithRequiredFields {
    String: "lolkek"
}
Bytes: "keklol"
)";

const TString messageOutEmptyMessage = R"(
MessageWithRequiredFields {
    String: ""
}
Bytes: "***"
)";

} // namespace

Y_UNIT_TEST_SUITE(Wonderlogs) {
    Y_UNIT_TEST(CensorRequest) {
        TMessage actual;
        TMessage expected;
        {
            NProtoBuf::TextFormat::Parser parser;
            parser.ParseFromString(messageIn, &actual);
            parser.ParseFromString(messageOutRequest, &expected);
        }
        TCensor censor;
        censor.ProcessMessage(TCensor::TFlags{EAccess::A_PRIVATE_REQUEST}, actual);
        UNIT_ASSERT(actual.IsInitialized());
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(CensorNothing) {
        TMessage actual;
        TMessage expected;
        {
            NProtoBuf::TextFormat::Parser parser;
            parser.ParseFromString(messageIn, &actual);
            parser.ParseFromString(messageIn, &expected);
        }
        TCensor censor;
        censor.ProcessMessage({}, actual);
        UNIT_ASSERT(actual.IsInitialized());
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(CensorEverything) {
        TMessage actual;
        TMessage expected;
        {
            NProtoBuf::TextFormat::Parser parser;
            parser.ParseFromString(messageIn, &actual);
            parser.ParseFromString(messageOutRequestResponse, &expected);
        }
        TCensor censor;
        censor.ProcessMessage(TCensor::TFlags{EAccess::A_PRIVATE_REQUEST} | EAccess::A_PRIVATE_RESPONSE, actual);
        UNIT_ASSERT(actual.IsInitialized());
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(DoNotInitializeEmptyMessage) {
        TMessage expected;
        TMessage actual;
        {
            NProtoBuf::TextFormat::Parser parser;
            parser.ParseFromString(messageInEmptyMessage, &actual);
            parser.ParseFromString(messageOutEmptyMessage, &expected);
        }
        UNIT_ASSERT(expected.IsInitialized());
        TCensor censor;
        censor.ProcessMessage(TCensor::TFlags{EAccess::A_PRIVATE_REQUEST} | EAccess::A_PRIVATE_RESPONSE, actual);
        UNIT_ASSERT(actual.IsInitialized());
        UNIT_ASSERT(!actual.HasPrivateMessage());
        UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
    }
}

#include <alice/cuttlefish/library/cuttlefish/converter/ut/common.h>
#include <alice/cuttlefish/library/cuttlefish/converter/converters.h>
#include <google/protobuf/util/json_util.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>


using namespace NAlice::NCuttlefish;
using namespace NAlice::NCuttlefish::NAppHostServices::NConverter;
using namespace NAliceProtocol;



Y_UNIT_TEST_SUITE(ConvertDirectives) {

Y_UNIT_TEST(Basic)
{
    const TStringBuf raw = R"__({
        "error": {
            "message": "Very severe error",
            "type": "Error"
        }
    })__";
    const NJson::TJsonValue json = ReadJsonValue(raw);

    const TEventException event = JsonToProtobuf(GetEventExceptionConverter(), json);
    UNIT_ASSERT_EQUAL(event.GetText(), "Very severe error");

    UNIT_ASSERT_EQUAL(
        AsSortedJsonString(ProtobufToJson(GetEventExceptionConverter(), event)),
        AsSortedJsonString(json)
    );
}

Y_UNIT_TEST(SerializeEventExceptionDirective)
{
    TDirective directive;
    directive.MutableHeader()->SetNamespace(NAliceProtocol::TEventHeader::SYSTEM);
    directive.MutableHeader()->SetName(NAliceProtocol::TEventHeader::EVENT_EXCEPTION);
    directive.MutableHeader()->SetMessageId("12345678-90ab-cdef-1234-567890abcdef");
    directive.MutableException()->SetText("My lovely error");

    const TString json = ProtobufToJson(GetDirectiveConverter(), directive);
    Cerr << "SerializeEventExceptionDirective: " << json << Endl;

    UNIT_ASSERT_EQUAL(
        AsSortedJsonString(json),
        AsSortedJsonString(R"__({"directive": {
            "header": {
                "namespace": "System",
                "name": "EventException",
                "messageId": "12345678-90ab-cdef-1234-567890abcdef"
            },
            "payload": {
                "error": {
                    "message": "My lovely error",
                    "type": "Error"
                }
            }
        }})__")
    );
}


Y_UNIT_TEST(SerializeInvalidAuthDirective)
{
    TDirective directive;
    directive.MutableHeader()->SetNamespace(NAliceProtocol::TEventHeader::SYSTEM);
    directive.MutableHeader()->SetName(NAliceProtocol::TEventHeader::INVALID_AUTH);
    directive.MutableHeader()->SetMessageId("aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");
    directive.MutableInvalidAuth();

    const TString json = ProtobufToJson(GetDirectiveConverter(), directive);
    Cerr << "SerializeInvalidAuthDirective: " << json << Endl;

    UNIT_ASSERT_EQUAL(
        AsSortedJsonString(json),
        AsSortedJsonString(R"__({"directive": {
            "header": {
                "namespace": "System",
                "name": "InvalidAuth",
                "messageId": "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
            },
            "payload": {}
        }})__")
    );
}

Y_UNIT_TEST(SerializeSynchronizeStateResponse)
{
    TDirective directive;
    directive.MutableHeader()->SetNamespace(NAliceProtocol::TEventHeader::SYSTEM);
    directive.MutableHeader()->SetName(NAliceProtocol::TEventHeader::SYNCHRONIZE_STATE_RESPONSE);
    directive.MutableHeader()->SetMessageId("bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb");
    TSynchronizeStateResponse* const payload = directive.MutableSyncStateResponse();
    payload->SetSessionId("aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee");
    payload->SetGuid("1234567890");

    const TString json = ProtobufToJson(GetDirectiveConverter(), directive);
    Cerr << "SerializeSynchronizeStateResponse: " << json << Endl;

    UNIT_ASSERT_EQUAL(
        AsSortedJsonString(json),
        AsSortedJsonString(R"__({"directive": {
            "header": {
                "namespace": "System",
                "name": "SynchronizeStateResponse",
                "messageId": "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb"
            },
            "payload": {
                "SessionId": "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee",
                "guid": "1234567890"
            }
        }})__")
    );
}

}

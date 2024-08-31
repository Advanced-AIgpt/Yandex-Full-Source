#include "common.h"
#include <alice/cuttlefish/library/proto_converters/converters.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>


using namespace NAlice::NCuttlefish;
using namespace NAliceProtocol;


Y_UNIT_TEST_SUITE(ConvertMessageHeader) {


Y_UNIT_TEST(Basic)
{
    const TStringBuf raw = R"__({
        "namespace": "System",
        "name": "SynchronizeState",
        "messageId": "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
    })__";
    const NJson::TJsonValue json = ReadJsonValue(raw);

    const TEventHeader header = JsonToProtobuf(MessageHeaderConverter(), json);
    UNIT_ASSERT_EQUAL(header.GetNamespace(), TEventHeader::SYSTEM);
    UNIT_ASSERT_EQUAL(header.GetName(), TEventHeader::SYNCHRONIZE_STATE);
    UNIT_ASSERT_EQUAL(header.GetMessageId(), "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");

    UNIT_ASSERT_EQUAL(
        AsSortedJsonString(ProtobufToJson(MessageHeaderConverter(), header)),
        AsSortedJsonString(raw)
    );

}

Y_UNIT_TEST(WithUnknownNameAndNamespace)
{
    const TStringBuf raw = R"__({
        "namespace": "SystemX",
        "name": "SynchronizeStateX",
        "messageId": "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
    })__";
    const NJson::TJsonValue json = ReadJsonValue(raw);

    const TEventHeader header = JsonToProtobuf(MessageHeaderConverter(), json);
    UNIT_ASSERT_EQUAL(header.GetNamespace(), TEventHeader::UNKNOWN_NAMESPACE);
    UNIT_ASSERT_EQUAL(header.GetName(), TEventHeader::UNKNOWN_NAME);
    UNIT_ASSERT_EQUAL(header.GetMessageId(), "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa");

    Cerr << "WithUnknownNameAndNamespace: " << ProtobufToJson(MessageHeaderConverter(), header) << Endl;
    UNIT_ASSERT_EQUAL(
        AsSortedJsonString(ProtobufToJson(MessageHeaderConverter(), header)),
        AsSortedJsonString(R"__({
            "namespace": "",
            "name": "",
            "messageId": "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
        })__")
    );
}

}

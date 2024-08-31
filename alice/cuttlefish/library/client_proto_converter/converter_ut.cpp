#include "convert.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NSpeechkitProtocol;

Y_UNIT_TEST_SUITE(ClientProtoConverter) {
    Y_UNIT_TEST(RejectGarbage)
    {
        std::string input = R"(
            {"random stuff": "idk, something here"}
        )";
        Json::Reader reader;
        Json::Value jvalue;
        reader.parse(input, jvalue);
        UNIT_ASSERT_EQUAL(EncodeMessage(jvalue), Nothing());
    }
    Y_UNIT_TEST(ParseStreamControl)
    {
        std::string input = R"(
            {"streamcontrol": {"messageId": "1", "streamId": 2, "action": 3}}
        )";
        Json::Reader reader;
        Json::Value jvalue;
        reader.parse(input, jvalue);
        TMaybe<TSpeechkitMessage> msg = EncodeMessage(jvalue);
        UNIT_ASSERT(msg);
        UNIT_ASSERT_EQUAL(msg->Payload_case(), TSpeechkitMessage::PayloadCase::kStreamControl);
        UNIT_ASSERT_EQUAL(msg->mutable_streamcontrol()->messageid(), "1");
        UNIT_ASSERT_EQUAL(msg->mutable_streamcontrol()->streamid(), 2);
        UNIT_ASSERT_EQUAL(msg->mutable_streamcontrol()->action(), 3);
    }
    Y_UNIT_TEST(UnparseStreamControl)
    {
        TSpeechkitMessage msg;
        TStreamControl *pStreamControl = msg.mutable_streamcontrol();
        pStreamControl->set_messageid("1");
        pStreamControl->set_streamid(2);
        pStreamControl->set_action((TStreamControl_EAction)3);
        TMaybe<Json::Value> js = DecodeMessage(msg);
        UNIT_ASSERT(js);
        const Json::Value& jStreamControl = (*js)["streamcontrol"];
        UNIT_ASSERT_EQUAL(jStreamControl["messageId"].asString(), "1");
        UNIT_ASSERT_EQUAL(jStreamControl["streamId"].asInt64(), 2);
        UNIT_ASSERT_EQUAL(jStreamControl["action"].asInt64(), 3);
   }
}

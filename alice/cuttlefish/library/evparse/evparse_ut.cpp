#include <library/cpp/testing/unittest/registar.h>

#include "evparse.h"


const char *EVENT_DATA="\
{\
    \"Session\": {\
        \"Uuid\": \"b9f8c7a22536bfecd1045e2d706d6868\",\
        \"AppType\": \"quasar\",\
        \"Timestamp\": \"2019-12-04T13:30:44.365282\",\
        \"IpAddr\": \"46.32.67.195\",\
        \"SessionId\": \"db3ae792-fb8b-453e-a306-5aed799056f0\",\
        \"Action\": \"request\"\
    },\
    \"Event\": {\
        \"event\": {\
            \"header\": {\
                \"streamId\": 9,\
                \"namespace\": \"Vins\",\
                \"name\": \"VoiceInput\",\
                \"messageId\": \"015788e2-a840-43a3-bf33-549ea8ec907f\"\
}}}}\
";


Y_UNIT_TEST_SUITE(EventParsing) {
    Y_UNIT_TEST(JsonValue) {
        NAliceProtocol::TEventHeader h;
        ParseEvent(EVENT_DATA, h);

        UNIT_ASSERT_VALUES_EQUAL("Vins", h.GetNamespace());
        UNIT_ASSERT_VALUES_EQUAL("VoiceInput", h.GetName());
        UNIT_ASSERT_VALUES_EQUAL("015788e2-a840-43a3-bf33-549ea8ec907f", h.GetMessageId());
        UNIT_ASSERT_VALUES_EQUAL(9, h.GetStreamId());
    }

    Y_UNIT_TEST(JsonParser) {
        NAliceProtocol::TEventHeader h;
        ParseEventFast(EVENT_DATA, h);

        UNIT_ASSERT_VALUES_EQUAL("Vins", h.GetNamespace());
        UNIT_ASSERT_VALUES_EQUAL("VoiceInput", h.GetName());
        UNIT_ASSERT_VALUES_EQUAL("015788e2-a840-43a3-bf33-549ea8ec907f", h.GetMessageId());
        UNIT_ASSERT_VALUES_EQUAL(9, h.GetStreamId());
    }
}

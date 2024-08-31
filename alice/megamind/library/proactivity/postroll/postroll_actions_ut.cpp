#include "postroll_actions.h"

#include <alice/megamind/library/testing/speechkit.h>

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {

constexpr auto SKR_MAKE_ACTION = TStringBuf(R"(
{
    "application": {
        "timestamp": "1337",
        "uuid": "test-uuid"
    },
    "header": {
        "request_id": "test-reqid"
    }
}
)");
constexpr auto ACTION_PROTO_VIEW = TStringBuf(R"(
    Type: 2
    Id: "test-uuid"
    ToType: 1
    ToId: "item-id"
    ActionType: 0
    Value: 1
    Timestamp: 1337
    RequestId: "test-reqid"
    Payload {
        [NDJ.NAS.TActionPayloadData.AlicePayloadExtension] {
            Source: "postroll_source"
        }
    }
)");
constexpr auto ACTION_PROTO_CLICK = TStringBuf(R"(
    Type: 2,
    Id: "test-uuid",
    ToType: 1,
    ToId: "item-id",
    ActionType: 1,
    Value: 1,
    Timestamp: 1337,
    RequestId: "test-reqid"
    Payload {
        [NDJ.NAS.TActionPayloadData.AlicePayloadExtension] {
            Source: "postroll_source"
        }
    }
)");
constexpr auto ACTION_PROTO_VIEW_WITH_TAGS = TStringBuf(R"(
    Type: 2
    Id: "test-uuid"
    ToType: 1
    ToId: "item-id"
    ActionType: 0
    Value: 1
    Timestamp: 1337
    RequestId: "test-reqid"
    Payload {
        [NDJ.NAS.TActionPayloadData.AlicePayloadExtension] {
            Tags: "tag_1"
            Source: "postroll_source"
        }
    }
)");


Y_UNIT_TEST_SUITE(ProactivityActions) {
    Y_UNIT_TEST(TestMakeAction) {
        auto skr = TSpeechKitRequestBuilder(SKR_MAKE_ACTION).Build();
        NDJ::NAS::TProtoItem item;
        item.SetId("item-id");
        const TString source = "postroll_source";
        NDJ::TActionProto expectedAction;
        {
            const auto status = ::google::protobuf::TextFormat::ParseFromString(ToString(ACTION_PROTO_VIEW), &expectedAction);
            UNIT_ASSERT_C(status, JsonStringFromProto(expectedAction));

            const auto action = MakePostrollAction(skr, item, NDJ::NAS::EAlisaSkillsActionType::AT_View, source);
            UNIT_ASSERT_MESSAGES_EQUAL(action, expectedAction);

        }
        {
            const auto status = ::google::protobuf::TextFormat::ParseFromString(ToString(ACTION_PROTO_CLICK), &expectedAction);
            UNIT_ASSERT_C(status, JsonStringFromProto(expectedAction));

            const auto action = MakePostrollAction(skr, item, NDJ::NAS::EAlisaSkillsActionType::AT_Click, source);
            UNIT_ASSERT_MESSAGES_EQUAL(action, expectedAction);
        }
        {
            const auto status = ::google::protobuf::TextFormat::ParseFromString(ToString(ACTION_PROTO_VIEW_WITH_TAGS), &expectedAction);
            UNIT_ASSERT_C(status, JsonStringFromProto(expectedAction));

            *item.AddTags() = "tag_1";
            const auto action = MakePostrollAction(skr, item, NDJ::NAS::EAlisaSkillsActionType::AT_View, source);
            UNIT_ASSERT_MESSAGES_EQUAL(action, expectedAction);

        }
    }
}

} // namespace

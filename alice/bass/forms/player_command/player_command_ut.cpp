#include <alice/bass/forms/player_command.h>

#include <alice/bass/forms/video/defs.h>
#include <alice/bass/forms/vins.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/ut/helpers.h>

#include <alice/library/biometry/biometry.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/serialized_enum.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

using namespace NBASS;
using namespace NTestingHelpers;

namespace {

using EUserSpecificKey = TPersonalDataHelper::EUserSpecificKey;

const auto USER_BIO_SCORES = NSc::TValue::FromJson(R"([
    {"score": 0.2, "user_id": "1"},
    {"score": 0.6, "user_id": "2"},
    {"score": 0.02, "user_id": "3"}
])");

const auto GUEST_BIO_SCORES = NSc::TValue::FromJson(R"([
    {"score": 0.1, "user_id": "1"},
    {"score": 0.2, "user_id": "2"},
    {"score": 0.01, "user_id": "3"}
])");

class TRequestResponseBuilder {
public:
    TRequestResponseBuilder& SetForm(TStringBuf formName) {
        result["form"]["name"] = TString("personal_assistant.scenarios.") + formName;
        return *this;
    }

    TRequestResponseBuilder& SetBasicMeta() {
        result["meta"]["epoch"] = 1484311159;
        result["meta"]["tz"] = "Europe/Moscow";
        result["meta"]["client_id"] = "ru.yandex.quasar/7.90 (none; none none)";
        return *this;
    }

    TRequestResponseBuilder& AddBioExperiments() {
        result["meta"]["experiments"]["music_biometry"].SetBool(true);
        result["meta"]["experiments"]["biometry_like"].SetBool(true);
        return *this;
    }

    TRequestResponseBuilder& AddBiometricScores(bool guest) {
        result["meta"]["biometrics_scores"]["status"] = "ok";
        result["meta"]["scores_with_mode"].SetArray();
        for (const auto& mode : GetEnumAllValues<NAlice::NBiometry::TBiometry::EMode>()) {
            auto modeName = ToString(mode);
            if (modeName.StartsWith("_")) {
                continue;
            }
            NSc::TValue scoreWithMode;
            scoreWithMode["mode"] = modeName;
            scoreWithMode["scores"] = guest ? GUEST_BIO_SCORES : USER_BIO_SCORES;
            result["meta"]["biometrics_scores"]["scores_with_mode"].Push(scoreWithMode);
        }
        return *this;
    }

    TRequestResponseBuilder& AddPlayer(TStringBuf player, double timestamp, bool paused) {
        result["meta"]["device_state"][player]["player"]["timestamp"].SetNumber(timestamp);
        result["meta"]["device_state"][player]["player"]["paused"].SetBool(paused);
        result["meta"]["device_state"][player]["currently_playing"]["track_info"] = "some_info";
        return *this;
    }

    TRequestResponseBuilder& SetScreen(NVideo::EScreenId screenId) {
        result["meta"]["device_state"]["is_tv_plugged_in"].SetBool(true);
        result["meta"]["current_screen"] = ToString(screenId);
        return *this;
    }

    TRequestResponseBuilder& AddAttentionBlock(TStringBuf attention_type) {
        NSc::TValue block;
        block["type"] = "attention";
        block["attention_type"] = attention_type;
        block["data"] = NSc::TValue();
        return AddBlock(block);
    }

    TRequestResponseBuilder& AddSlots() {
        result["form"]["slots"].SetArray();
        return *this;
    }

    TRequestResponseBuilder& AddUserNameSlot(TStringBuf userName) {
        NSc::TValue slot;
        slot["name"] = "user_name";
        slot["optional"].SetBool(true);
        slot["type"] = "string";
        slot["value"] = userName;
        result["form"]["slots"].Push(slot);
        return *this;
    }

    TRequestResponseBuilder& AddCommand(TStringBuf command, NSc::TValue data) {
        NSc::TValue block;
        block["type"] = "command";
        block["command_type"] = command;
        block["command_sub_type"] = command;
        block["data"] = data;
        return AddBlock(block);
    }

    TRequestResponseBuilder& AddAnalyticsInfoBlock(const TStringBuf data) {
        NSc::TValue block;
        block["type"] = "scenario_analytics_info";
        block["data"] = data;
        return AddBlock(block);
    }

    TRequestResponseBuilder& AddBlock(const NSc::TValue& block) {
        result["blocks"].Push(block);
        return *this;
    }

    NSc::TValue Build() const {
        return result;
    }



private:
    NSc::TValue result;
};

TResultValue Do(IHandler& handler, TContext::TPtr& ctx) {
    TRequestHandler requestHandler(ctx);
    return handler.Do(requestHandler);
}

Y_UNIT_TEST_SUITE(TPlayerAuthorizedCommandHandlerTest) {
    Y_UNIT_TEST(MusicLikeNoBiometry) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta()
                .SetForm("player_like")
                .AddSlots()
                .AddPlayer("music", 2.0 /* timestamp */, false /* paused */)
                .Build();

        auto response =
            TRequestResponseBuilder()
                .SetForm("player_like")
                .AddSlots()
                .AddCommand("player_like", {})
                .AddAnalyticsInfoBlock(
                    "EihwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLnBsYXllcl9saWtlSg9wbGF5ZXJfY29tbWFuZHM=")
                .Build();

        TContext::TPtr ctx = MakeAuthorizedContext(request);
        TPlayerAuthorizedCommandHandler handler(MakeHolder<TBlackBoxAPIFake>(), MakeHolder<TDataSyncAPIStub>());
        UNIT_ASSERT(!Do(handler, ctx));
        CheckResponse(*ctx, response);
    }

    Y_UNIT_TEST(MusicLikeBiometryGuest) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta()
                .AddBioExperiments()
                .AddBiometricScores(true /* guest */)
                .SetForm("player_like")
                .AddSlots()
                .AddPlayer("music", 2.0 /* timestamp */, false /* paused */)
                .Build();

        auto response =
            TRequestResponseBuilder()
                .SetForm("player_like")
                .AddSlots()
                .AddAttentionBlock("biometry_guest")
                .AddAnalyticsInfoBlock(
                    "EihwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLnBsYXllcl9saWtlSg9wbGF5ZXJfY29tbWFuZHM=")
                .Build();

        TContext::TPtr ctx = MakeAuthorizedContext(request);
        auto dataSyncApi = MakeHolder<TDataSyncAPIStub>();
        dataSyncApi->Save("1", {{EUserSpecificKey::GuestUID, TStringBuf("9357")}});
        TPlayerAuthorizedCommandHandler handler(MakeHolder<TBlackBoxAPIFake>("1"), std::move(dataSyncApi));
        UNIT_ASSERT(!Do(handler, ctx));
        CheckResponse(*ctx, response);
    }

    Y_UNIT_TEST(MusicLikeBiometryUser) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta()
                .AddBioExperiments()
                .AddBiometricScores(false /* guest */)
                .SetForm("player_like")
                .AddSlots()
                .AddPlayer("music", 2.0 /* timestamp */, false /* paused */)
                .Build();

        NSc::TValue expectedData;
        expectedData["uid"] = "2";
        auto response =
            TRequestResponseBuilder()
                .SetForm("player_like")
                .AddSlots()
                .AddCommand("player_like", expectedData)
                .AddAnalyticsInfoBlock(
                    "EihwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLnBsYXllcl9saWtlSg9wbGF5ZXJfY29tbWFuZHM=")
                .Build();

        TContext::TPtr ctx = MakeAuthorizedContext(request);
        TPlayerAuthorizedCommandHandler handler(MakeHolder<TBlackBoxAPIFake>("1"), MakeHolder<TDataSyncAPIStub>());
        UNIT_ASSERT(!Do(handler, ctx));
        CheckResponse(*ctx, response);
    }

        Y_UNIT_TEST(MusicDislikeBiometryGuest) {
            auto request =
                    TRequestResponseBuilder()
                            .SetBasicMeta()
                            .AddBioExperiments()
                            .AddBiometricScores(true /* guest */)
                            .SetForm("player_dislike")
                            .AddSlots()
                            .AddPlayer("music", 2.0 /* timestamp */, false /* paused */)
                            .AddPlayer("radio", 200.0 /* timestamp */, true /* paused */)
                            .Build();

            NSc::TValue expectedData;
            expectedData["uid"] = "9357";
            auto response =
                    TRequestResponseBuilder()
                            .SetForm("player_dislike")
                            .AddSlots()
                            .AddAttentionBlock("biometry_guest")
                            .AddAttentionBlock("changed_dislike_to_next_track")
                            .AddCommand("player_next_track", expectedData)
                            .AddAnalyticsInfoBlock(
                                "EitwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLnBsYXllcl9kaXNsaWtlSg9wbGF5ZXJfY29tbWFuZHM=")
                            .Build();

            TContext::TPtr ctx = MakeAuthorizedContext(request);
            auto dataSyncApi = MakeHolder<TDataSyncAPIStub>();
            dataSyncApi->Save("1", {
                    {EUserSpecificKey::GuestUID, TStringBuf("9357")}
            });
            TPlayerAuthorizedCommandHandler handler(MakeHolder<TBlackBoxAPIFake>("1"), std::move(dataSyncApi));
            UNIT_ASSERT(!Do(handler, ctx));
            CheckResponse(*ctx, response);
        }

    Y_UNIT_TEST(MusicDislikeBiometryGuestWithName) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta()
                .AddBioExperiments()
                .AddBiometricScores(true /* guest */)
                .SetForm("player_dislike")
                .AddSlots()
                .AddPlayer("music", 2.0 /* timestamp */, false /* paused */)
                .AddPlayer("radio", 200.0 /* timestamp */, true /* paused */)
                .Build();

        NSc::TValue expectedData;
        expectedData["uid"] = "9357";
        auto response =
            TRequestResponseBuilder()
                .SetForm("player_dislike")
                .AddSlots()
                .AddUserNameSlot("принцесса")
                .AddAttentionBlock("biometry_guest")
                .AddAttentionBlock("changed_dislike_to_next_track")
                .AddCommand("player_next_track", expectedData)
                .AddAnalyticsInfoBlock(
                    "EitwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLnBsYXllcl9kaXNsaWtlSg9wbGF5ZXJfY29tbWFuZHM=")
                .Build();

        TContext::TPtr ctx = MakeAuthorizedContext(request);
        auto dataSyncApi = MakeHolder<TDataSyncAPIStub>();
        dataSyncApi->Save("1", {{EUserSpecificKey::GuestUID, TStringBuf("9357")}});
        dataSyncApi->Save("2", {{EUserSpecificKey::UserName, TStringBuf("принцесса")}});
        TPlayerAuthorizedCommandHandler handler(MakeHolder<TBlackBoxAPIFake>("1"), std::move(dataSyncApi));
        UNIT_ASSERT(!Do(handler, ctx));
        CheckResponse(*ctx, response);
    }

    Y_UNIT_TEST(RadioDislikeBiometryGuest) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta()
                .AddBioExperiments()
                .AddBiometricScores(true /* guest */)
                .SetForm("player_dislike")
                .AddSlots()
                .AddPlayer("music", 200.0 /* timestamp */, true /* paused */)
                .AddPlayer("radio", 2.0 /* timestamp */, false /* paused */)
                .Build();

        TContext::TPtr ctx = MakeAuthorizedContext(request);
        TPlayerAuthorizedCommandHandler handler(MakeHolder<TBlackBoxAPIFake>("1"), MakeHolder<TDataSyncAPIStub>());
        // Unsupported in radio
        UNIT_ASSERT(Do(handler, ctx));
    }

    Y_UNIT_TEST(TvDislikeBiometryGuest) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta()
                .AddBioExperiments()
                .AddBiometricScores(true /* guest */)
                .SetForm("player_dislike")
                .AddSlots()
                .AddPlayer("music", 200.0 /* timestamp */, false /* paused */)
                .SetScreen(NVideo::EScreenId::SeasonGallery)
                .Build();

        auto response =
            TRequestResponseBuilder()
                .SetForm("player_dislike")
                .AddSlots()
                .AddCommand("player_dislike", {})
                .AddAnalyticsInfoBlock(
                    "EitwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLnBsYXllcl9kaXNsaWtlSg9wbGF5ZXJfY29tbWFuZHM=")
                .Build();

        TContext::TPtr ctx = MakeAuthorizedContext(request);
        TPlayerAuthorizedCommandHandler handler(MakeHolder<TBlackBoxAPIFake>("1"), MakeHolder<TDataSyncAPIStub>());
        UNIT_ASSERT(!Do(handler, ctx));
        CheckResponse(*ctx, response);
    }

    Y_UNIT_TEST(NothingDislikeBiometryGuest) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta()
                .AddBioExperiments()
                .AddBiometricScores(true /* guest */)
                .SetForm("player_dislike")
                .AddSlots()
                .Build();

        auto response =
            TRequestResponseBuilder()
                .SetForm("player_dislike")
                .AddSlots()
                .AddAttentionBlock("biometry_guest")
                .AddAnalyticsInfoBlock(
                    "EitwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLnBsYXllcl9kaXNsaWtlSg9wbGF5ZXJfY29tbWFuZHM=")
                .Build();

        TContext::TPtr ctx = MakeAuthorizedContext(request);
        auto dataSyncApi = MakeHolder<TDataSyncAPIStub>();
        dataSyncApi->Save("1", {{EUserSpecificKey::GuestUID, TStringBuf("9357")}});
        TPlayerAuthorizedCommandHandler handler(MakeHolder<TBlackBoxAPIFake>("1"), std::move(dataSyncApi));
        UNIT_ASSERT(!Do(handler, ctx));
        CheckResponse(*ctx, response);
    }
}
} // namespace

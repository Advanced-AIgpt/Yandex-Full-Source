#include "typed_frames.h"

#include <alice/library/unittest/message_diff.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {

using namespace NAlice;

void TestTypedFrameFromUntypedOne(const TSemanticFrame& frame) {
    const auto target = TryMakeTypedSemanticFrameFromSemanticFrame(frame);
    UNIT_ASSERT(target.Defined());
    UNIT_ASSERT_MESSAGES_EQUAL(target.GetRef(), frame.GetTypedSemanticFrame());
}

Y_UNIT_TEST_SUITE(TestTypedFrames) {
    Y_UNIT_TEST(TestSearchSemanticFrame) {
        const TString searchQuery = "поисковый запрос";

        TTypedSemanticFrame typedSemanticFrame{};
        typedSemanticFrame.MutableSearchSemanticFrame()->MutableQuery()->SetStringValue(searchQuery);

        const auto semanticFrame = MakeSemanticFrameFromTypedSemanticFrame(typedSemanticFrame);

        const auto expectedSemanticFrame = [&] {
            TSemanticFrame frame{};
            frame.SetName("personal_assistant.scenarios.search");
            auto& slot = *frame.AddSlots();
            slot.SetName("query");
            slot.SetType("string");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue(searchQuery);
            frame.MutableTypedSemanticFrame()->CopyFrom(typedSemanticFrame);
            return frame;
        }();

        UNIT_ASSERT_MESSAGES_EQUAL(expectedSemanticFrame, semanticFrame);
        TestTypedFrameFromUntypedOne(expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestMordoviaHomeScreenSemanticFrame) {
        const TString deviceID = "abcdefghijklmnopqrst";

        TTypedSemanticFrame typedSemanticFrame{};
        typedSemanticFrame.MutableMordoviaHomeScreenSemanticFrame()->MutableDeviceID()->SetStringValue(deviceID);

        const auto semanticFrame = MakeSemanticFrameFromTypedSemanticFrame(typedSemanticFrame);

        const auto expectedSemanticFrame = [&] {
            TSemanticFrame frame{};
            frame.SetName("quasar.mordovia.home_screen");
            auto& slot = *frame.AddSlots();
            slot.SetName("device_id");
            slot.SetType("string");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue(deviceID);
            frame.MutableTypedSemanticFrame()->CopyFrom(typedSemanticFrame);
            return frame;
        }();

        UNIT_ASSERT_MESSAGES_EQUAL(expectedSemanticFrame, semanticFrame);
        TestTypedFrameFromUntypedOne(expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestVideoPlaySemanticFrame) {
        TString contentType{"tv_show"};
        TString action{"play"};
        TString searchText{"123"};
        uint32_t season = 1;
        uint32_t episode = 10;

        TTypedSemanticFrame typedSemanticFrame{};
        auto& videoPlayFrame = *typedSemanticFrame.MutableVideoPlaySemanticFrame();
        videoPlayFrame.MutableContentType()->SetVideoContentTypeValue(contentType);
        videoPlayFrame.MutableAction()->SetVideoActionValue(action);
        videoPlayFrame.MutableSearchText()->SetStringValue(searchText);
        videoPlayFrame.MutableSeason()->SetNumValue(season);
        videoPlayFrame.MutableEpisode()->SetNumValue(episode);
        const auto semanticFrame = MakeSemanticFrameFromTypedSemanticFrame(typedSemanticFrame);

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("personal_assistant.scenarios.video_play");
        {
            auto& slot = *expectedSemanticFrame.AddSlots();
            slot.SetName("content_type");
            slot.SetType("video_content_type");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue(contentType);
        }
        {
            auto& slot = *expectedSemanticFrame.AddSlots();
            slot.SetName("action");
            slot.SetType("video_action");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue(action);
        }
        {
            auto& slot = *expectedSemanticFrame.AddSlots();
            slot.SetName("search_text");
            slot.SetType("string");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue(searchText);
        }
        {
            auto& slot = *expectedSemanticFrame.AddSlots();
            slot.SetName("season");
            slot.SetType("num");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue(ToString(season));
        }
        {
            auto& slot = *expectedSemanticFrame.AddSlots();
            slot.SetName("episode");
            slot.SetType("num");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue(ToString(episode));
        }
        expectedSemanticFrame.MutableTypedSemanticFrame()->CopyFrom(typedSemanticFrame);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedSemanticFrame, semanticFrame);
        TestTypedFrameFromUntypedOne(expectedSemanticFrame);
    }

    Y_UNIT_TEST(TestMusicPlaySemanticFrame) {
        TString playlist{"origin"};
        TString action{"autoplay"};
        TString specialAnswer{"answer"};
        TString epoch{"sixties"};
        TString searchText{"searchText"};
        TString genre{"rock"};
        TString mood{"relaxed"};
        TString activity{"driving"};
        TString language{"en"};
        TString vocal{"male"};
        TString novelty{"new"};
        TString personality{"is_personal"};
        TString order{"shuffle"};
        TString repeat{"repeat"};

        TTypedSemanticFrame typedSemanticFrame{};
        auto& musicFrame = *typedSemanticFrame.MutableMusicPlaySemanticFrame();
        musicFrame.MutableSpecialPlaylist()->SetSpecialPlaylistValue(playlist);
        musicFrame.MutableSpecialAnswerInfo()->SetSpecialAnswerInfoValue(specialAnswer);
        musicFrame.MutableActionRequest()->SetActionRequestValue(action);
        musicFrame.MutableEpoch()->SetEpochValue(epoch);
        musicFrame.MutableSearchText()->SetStringValue(searchText);
        musicFrame.MutableGenre()->SetGenreValue(genre);
        musicFrame.MutableMood()->SetMoodValue(mood);
        musicFrame.MutableActivity()->SetActivityValue(activity);
        musicFrame.MutableLanguage()->SetLanguageValue(language);
        musicFrame.MutableVocal()->SetVocalValue(vocal);
        musicFrame.MutableNovelty()->SetNoveltyValue(novelty);
        musicFrame.MutablePersonality()->SetPersonalityValue(personality);
        musicFrame.MutableOrder()->SetOrderValue(order);
        musicFrame.MutableRepeat()->SetRepeatValue(repeat);
        const auto semanticFrame = MakeSemanticFrameFromTypedSemanticFrame(typedSemanticFrame);

        TSemanticFrame expectedSemanticFrame{};
        expectedSemanticFrame.SetName("personal_assistant.scenarios.music_play");

        const auto addSlot = [&](TStringBuf name, TStringBuf type, const TString& value) {
            auto& slot = *expectedSemanticFrame.AddSlots();
            slot.SetName(TString{name});
            slot.SetType(TString{type});
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue(value);
        };
        const auto addSlotString = [&](TStringBuf name, const TString& value) { addSlot(name, "string", value); };
        const auto addSlotTypeAsName = [&](TStringBuf name, const TString& value) { addSlot(name, name, value); };

        addSlotTypeAsName("special_playlist", playlist);
        addSlotTypeAsName("special_answer_info", specialAnswer);
        addSlotTypeAsName("action_request", action);
        addSlotTypeAsName("epoch", epoch);
        addSlotString("search_text", searchText);
        addSlotTypeAsName("genre", genre);
        addSlotTypeAsName("mood", mood);
        addSlotTypeAsName("activity", activity);
        addSlotTypeAsName("language", language);
        addSlotTypeAsName("vocal", vocal);
        addSlotTypeAsName("novelty", novelty);
        addSlotTypeAsName("personality", personality);
        addSlotTypeAsName("order", order);
        addSlotTypeAsName("repeat", repeat);

        expectedSemanticFrame.MutableTypedSemanticFrame()->CopyFrom(typedSemanticFrame);

        UNIT_ASSERT_MESSAGES_EQUAL(expectedSemanticFrame, semanticFrame);
        TestTypedFrameFromUntypedOne(expectedSemanticFrame);
    }

    Y_UNIT_TEST(GetTypedSemanticFramesMappingDoesNotContainNull) {
        const auto mapping = NImpl::GetTypedSemanticFramesMapping();
        for (const auto& [k, v] : mapping) {
            UNIT_ASSERT_C(v.FieldDescriptor, k);
            for (const auto& [kk, vv] : v.Slots) {
                UNIT_ASSERT_C(vv.FieldDescriptor, k << " " << kk);
                for (const auto& [kkk, vvv] : vv.Values) {
                    UNIT_ASSERT_C(vvv, k << " " << kk << " " << kkk);
                }
            }
        }
    }

    Y_UNIT_TEST(TryMakeTypedSemanticFrameFromSemanticFrameReturnNothingOnUnknownFrame) {
        TSemanticFrame frame{};
        frame.SetName("_an_invalid_semantic_frame_name");
        UNIT_ASSERT(TryMakeTypedSemanticFrameFromSemanticFrame(frame).Empty());
    }

    Y_UNIT_TEST(TryMakeTypedSemanticFrameFromSemanticFrameIgnoreFewSameSlots) {
        TSemanticFrame frame{};
        frame.SetName("personal_assistant.scenarios.search");
        for (int i = 0; i < 2; ++i) {
            auto& slot = *frame.AddSlots();
            slot.SetName("query");
            slot.SetType("string");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue("value");
        }
        frame.MutableTypedSemanticFrame()->MutableSearchSemanticFrame()->MutableQuery()->SetStringValue("value");

        TestTypedFrameFromUntypedOne(frame);
    }

    Y_UNIT_TEST(TryMakeTypedSemanticFrameFromSemanticFrameIgnoreSlotWithInvalidName) {
        TSemanticFrame frame{};
        frame.SetName("personal_assistant.scenarios.search");
        auto& slot = *frame.AddSlots();
        slot.SetName("_an_invalid_slot_name");
        slot.SetType("string");
        slot.AddAcceptedTypes(slot.GetType());
        slot.SetValue("value");
        frame.MutableTypedSemanticFrame()->MutableSearchSemanticFrame();

        TestTypedFrameFromUntypedOne(frame);
    }

    Y_UNIT_TEST(TryMakeTypedSemanticFrameFromSemanticFrameIgnoreSlotWithInvalidType) {
        TSemanticFrame frame{};
        frame.SetName("personal_assistant.scenarios.search");
        auto& slot = *frame.AddSlots();
        slot.SetName("query");
        slot.SetType("_an_invalid_slot_type");
        slot.AddAcceptedTypes(slot.GetType());
        slot.SetValue("value");
        frame.MutableTypedSemanticFrame()->MutableSearchSemanticFrame();

        TestTypedFrameFromUntypedOne(frame);
    }

    Y_UNIT_TEST(TryMakeTypedSemanticFrameFromSemanticFrameWithInvalidAndValidSlots) {
        TSemanticFrame frame{};
        frame.SetName("personal_assistant.scenarios.music_play");

        {
            // valid slot
            auto& slot = *frame.AddSlots();
            slot.SetName("genre");
            slot.SetType("genre");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue("rock");
        }
        {
            // invalid slot
            auto& slot = *frame.AddSlots();
            slot.SetName("year");
            slot.SetType("year");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue("1960");
        }
        {
            // valid slot
            auto& slot = *frame.AddSlots();
            slot.SetName("language");
            slot.SetType("language");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue("russian");
        }
        {
            // invalid slot
            auto& slot = *frame.AddSlots();
            slot.SetName("likes");
            slot.SetType("likes");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue("15000");
        }
        {
            // valid slot
            auto& slot = *frame.AddSlots();
            slot.SetName("repeat");
            slot.SetType("repeat");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue("track");
        }

        // TSF has only valid slots
        auto& tsf = *frame.MutableTypedSemanticFrame()->MutableMusicPlaySemanticFrame();
        tsf.MutableGenre()->SetGenreValue("rock");
        tsf.MutableLanguage()->SetLanguageValue("russian");
        tsf.MutableRepeat()->SetRepeatValue("track");

        TestTypedFrameFromUntypedOne(frame);
    }

    Y_UNIT_TEST(TryMakeTypedSemanticFrameFromSemanticFrameWithRepeatedSlots) {
        TSemanticFrame frame{};
        frame.SetName("alice.multiroom.start_multiroom");

        // non-repeated slot
        {
            auto& slot = *frame.AddSlots();
            slot.SetName("location_everywhere");
            slot.SetType("user.iot.multiroom_all_devices");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue("test_everywhere");
        }

        // repeated slot
        for (int i = 0; i < 3; ++i) {
            auto& slot = *frame.AddSlots();
            slot.SetName("location_room");
            slot.SetType("user.iot.room");
            slot.AddAcceptedTypes(slot.GetType());
            slot.SetValue(TStringBuilder{} << "test_room_" << i);
        }

        auto& tsf = *frame.MutableTypedSemanticFrame()->MutableStartMultiroomSemanticFrame();
        tsf.MutableLocationEverywhere()->SetUserIotMultiroomAllDevicesValue("test_everywhere");
        tsf.AddLocationRoom()->SetUserIotRoomValue("test_room_0");
        tsf.AddLocationRoom()->SetUserIotRoomValue("test_room_1");
        tsf.AddLocationRoom()->SetUserIotRoomValue("test_room_2");

        TestTypedFrameFromUntypedOne(frame);
    }

    Y_UNIT_TEST(TestValidateTypedSemanticFrames) {
        UNIT_ASSERT_NO_EXCEPTION(ValidateTypedSemanticFrames());
    }
}

} // namespace

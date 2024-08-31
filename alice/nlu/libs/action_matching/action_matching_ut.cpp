#include <alice/nlu/libs/action_matching/action_matching.h>
#include <alice/nlu/libs/item_selector/interface/item_selector.h>
#include <alice/nlu/libs/item_selector/testing/mock.h>

#include <alice/begemot/lib/utils/form_to_frame.h>
#include <alice/library/frame/builder.h>
#include <alice/library/unittest/message_diff.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/string/split.h>

using namespace testing;
using namespace NAlice::NItemSelector;

namespace NAlice {

namespace {

TMaybe<TSemanticFrame> MakeSemanticFrame(const TMaybe<TString>& frameName) {
    if (!frameName.Defined()) {
        return Nothing();
    }
    TSemanticFrame frame;
    frame.SetName(*frameName);
    return frame;
}

TMaybe<TSemanticFrame> MakeSemanticFrame(const TSemanticFrame& frame) {
    return frame;
}

TVector<TSelectionResult> GenetateSelectionResult(const TVector<bool>& selectionMask) {
    TVector<TSelectionResult> result;
    for (const bool isSelected : selectionMask) {
        result.push_back({
            .Score = static_cast<double>(isSelected),
            .IsSelected = isSelected
        });
    }
    return result;
}

} //

#define PASS(...) __VA_ARGS__

#define ACTION_MATCHING_TEST(hints, requestText, requestLanguage, expectedFrame, frames, selectionMask)         \
    do {                                                                                                        \
        const TMaybe<TSemanticFrame>& expectedFrame__ = MakeSemanticFrame(expectedFrame);                       \
        const TVector<TSemanticFrame> frames__ = frames;                                                        \
        const TVector<bool> selectionMask__ = selectionMask;                                                    \
        NItemSelector::TSelectionRequest request{                                                               \
            .Phrase={requestText, requestLanguage}, .NonsenseTagging={}                                         \
        };                                                                                                      \
        TMockItemSelector selector;                                                                             \
        EXPECT_CALL(selector, Select(_, _)).Times(1).WillOnce(                                                  \
            Return(GenetateSelectionResult(selectionMask__))                                                    \
        );                                                                                                      \
        const auto foundActionFrame = NAlice::RecognizeAction(request, frames__, hints, &selector);             \
        if (expectedFrame__.Defined()) {                                                                        \
            UNIT_ASSERT(foundActionFrame.Defined());                                                            \
            UNIT_ASSERT_MESSAGES_EQUAL(*expectedFrame__, *foundActionFrame);                                    \
        } else {                                                                                                \
            UNIT_ASSERT(!foundActionFrame.Defined());                                                           \
        }                                                                                                       \
    } while (false)

Y_UNIT_TEST_SUITE(ActionMatchingSuite) {

Y_UNIT_TEST(Empty) {
    ACTION_MATCHING_TEST({}, "", ELanguage::LANG_UNK, Nothing(), {}, {});
}

Y_UNIT_TEST(ChooseVideo) {
    const TVector<TString> videoIds = {"1", "2", "3", "4", "5", "6"};

    TVector<TActionHint> hints;
    for (const TString& id : videoIds) {
        hints.push_back({
            .Phrases = {
                {id, ELanguage::LANG_UNK},
                {id + " фильм", ELanguage::LANG_RUS},
                {id + " видео", ELanguage::LANG_RUS}
            },
            .RecognizedFrameName = "video_" + id
        });
    }

    for (size_t i = 0; i < videoIds.size(); ++i) {
        const TString id = videoIds[i];
        TVector<bool> selectionMask(videoIds.size());
        selectionMask[i] = true;
        ACTION_MATCHING_TEST(hints, id, ELanguage::LANG_RUS, "video_" + id, {}, selectionMask);
        ACTION_MATCHING_TEST(hints, id + " фильм", ELanguage::LANG_RUS, "video_" + id, {}, selectionMask);
        ACTION_MATCHING_TEST(hints, id + " видео", ELanguage::LANG_RUS, "video_" + id, {}, selectionMask);
    }

    ACTION_MATCHING_TEST(hints, "давай любое", ELanguage::LANG_RUS, Nothing(), {}, TVector<bool>(videoIds.size()));
}

Y_UNIT_TEST(NonStrictMatching) {
    TVector<TActionHint> hints = {
        {
            .Phrases = {
                {"охотники за привидениями", ELanguage::LANG_RUS},
                {"фильм 1", ELanguage::LANG_RUS},
                {"номер 1", ELanguage::LANG_RUS},
                {"1", ELanguage::LANG_RUS}
            },
            .RecognizedFrameName = "video.ghostbusters"
        },
        {
            .Phrases = {
                {"назад в будущее", ELanguage::LANG_RUS},
                {"фильм 2", ELanguage::LANG_RUS},
                {"номер 2", ELanguage::LANG_RUS},
                {"2", ELanguage::LANG_RUS}
            },
            .RecognizedFrameName = "video.back_to_the_future"
        },
        {
            .Phrases = {
                {"назад в будущее 2", ELanguage::LANG_RUS},
                {"фильм 3", ELanguage::LANG_RUS},
                {"номер 3", ELanguage::LANG_RUS},
                {"3", ELanguage::LANG_RUS}
            },
            .RecognizedFrameName = "video.back_to_the_future_2"
        },
        {
            .Phrases = {
                {"назад в будущее 3", ELanguage::LANG_RUS},
                {"фильм 4", ELanguage::LANG_RUS},
                {"номер 4", ELanguage::LANG_RUS},
                {"4", ELanguage::LANG_RUS}
            },
            .RecognizedFrameName = "video.back_to_the_future_3"
        },
        {
            .Phrases = {
                {"назад", ELanguage::LANG_RUS},
                {"вернуться", ELanguage::LANG_RUS}
            },
            .RecognizedFrameName = "command.back"
        }
    };


    ACTION_MATCHING_TEST(hints, "давай охотников за привидениями", ELanguage::LANG_RUS, "video.ghostbusters", {}, PASS({1, 0, 0, 0, 0}));
    ACTION_MATCHING_TEST(hints, "назад в будущее", ELanguage::LANG_RUS, "video.back_to_the_future", {}, PASS({0, 1, 0, 0, 0}));
    ACTION_MATCHING_TEST(hints, "назад в будущее 2", ELanguage::LANG_RUS, "video.back_to_the_future_2", {}, PASS({0, 0, 1, 0, 0}));
    ACTION_MATCHING_TEST(hints, "назад в будущее 3", ELanguage::LANG_RUS, "video.back_to_the_future_3", {}, PASS({0, 0, 0, 1, 0}));
    ACTION_MATCHING_TEST(hints, "3 фильм", ELanguage::LANG_RUS, "video.back_to_the_future_2", {}, PASS({0, 0, 1, 0, 0}));
    ACTION_MATCHING_TEST(hints, "назад", ELanguage::LANG_RUS, "command.back", {}, PASS({0, 0, 0, 0, 1}));
    ACTION_MATCHING_TEST(hints, "вернись назад", ELanguage::LANG_RUS, "command.back", {}, PASS({0, 0, 0, 0, 1}));
    ACTION_MATCHING_TEST(hints, "да без разницы", ELanguage::LANG_RUS, Nothing(), {}, PASS({0, 0, 0, 0, 0}));
}

Y_UNIT_TEST(Frames) {
    TVector<TActionHint> hints = {
        {
            .Phrases = {},
            .RecognizedFrameName = "action1"
        },
        {
            .Phrases = {},
            .RecognizedFrameName = "action2"
        }
    };

    TSemanticFrame action1Frame;
    action1Frame.SetName("action1");

    TSemanticFrame action2Frame;
    action2Frame.SetName("action2");

    TSemanticFrame action3Frame;
    action3Frame.SetName("action3");

    ACTION_MATCHING_TEST(hints, "some request", ELanguage::LANG_ENG, "action1", {action1Frame}, PASS({0, 0})); // no phrases match; only action1 available
    ACTION_MATCHING_TEST(hints, "some request", ELanguage::LANG_ENG, Nothing(), {action3Frame}, PASS({0, 0})); // no phrases match; action 3 is not available
    ACTION_MATCHING_TEST(hints, "some request", ELanguage::LANG_ENG, "action1", PASS({action1Frame, action2Frame, action3Frame}), PASS({0, 0}));
}

Y_UNIT_TEST(HintsAndFrames_SameAction) {
    TVector<TActionHint> hints = {
        {
            .Phrases = {{"hello", ELanguage::LANG_ENG}},
            .RecognizedFrameName = "known_frame"
        }
    };

    const TSemanticFrame frame = TSemanticFrameBuilder("known_frame")
        .AddSlot("slot", "greeting", "hello")
        .Build();

    ACTION_MATCHING_TEST(hints, "hello", ELanguage::LANG_ENG, "known_frame", {}, PASS({1}));
    ACTION_MATCHING_TEST(hints, "bye", ELanguage::LANG_ENG, Nothing(), {}, PASS({0}));
    ACTION_MATCHING_TEST(hints, "hello", ELanguage::LANG_ENG, frame, {frame}, PASS({1})); // returns frame converted from frame instead of creating new
    ACTION_MATCHING_TEST(hints, "bye", ELanguage::LANG_ENG, frame, {frame}, PASS({0})); // returns frame converted from frame instead of creating new
}

Y_UNIT_TEST(HintsAndFrames_Negatives) {
    TVector<TActionHint> hints = {
        {
            .Negatives = {{"hello", ELanguage::LANG_ENG}},
            .RecognizedFrameName = "known_frame"
        }
    };

    const TSemanticFrame frame = TSemanticFrameBuilder("known_frame")
        .AddSlot("slot", "greeting", "g_hello")
        .Build();

    ACTION_MATCHING_TEST(hints, "hello", ELanguage::LANG_ENG, Nothing(), {frame}, PASS({0}));
}

Y_UNIT_TEST(HintsAndFramest_DifferentActions) {
    TVector<TActionHint> hints = {
        {
            .Phrases = {{"hello", ELanguage::LANG_ENG}},
            .RecognizedFrameName = "known_frame"
        },
        {
            .Phrases = {},
            .RecognizedFrameName = "adhoc_frame"
        }
    };

    const TSemanticFrame frame = TSemanticFrameBuilder("adhoc_frame").Build();

    ACTION_MATCHING_TEST(hints, "hello", ELanguage::LANG_ENG, "known_frame", {frame}, PASS({1, 0}));
}

Y_UNIT_TEST(RecognizeActionMultipleActions) {
    const TString matchedHintFrame = "__HINT_MATCH__";
    const TString skippedHintFrame = "__HINT_SKIP__";
    const TString matchedFrame = "__SF_MATCH__";
    const TString skippedFrame = "__SF_SKIP__";
    const TString matchedHintText = "hello";
    const TVector<TActionHint> hints = {
        {.Phrases = {{matchedHintText, ELanguage::LANG_ENG}}, .RecognizedFrameName = matchedHintFrame},
        {.RecognizedFrameName = matchedFrame},
        {.Phrases = {{skippedHintFrame, ELanguage::LANG_ENG}}, .RecognizedFrameName = skippedHintFrame},
    };
    const TVector<TSemanticFrame> semanticFrames = {
        *MakeSemanticFrame(matchedFrame),
        *MakeSemanticFrame(skippedFrame),
    };
    NItemSelector::TSelectionRequest request{.Phrase = {{matchedHintText}, ELanguage::LANG_ENG},
                                             .NonsenseTagging = {}};
    TMockItemSelector selector;
    EXPECT_CALL(selector, Select(_, _)).Times(1).WillOnce(Return(GenetateSelectionResult(PASS({1, 0, 0}))));
    const auto foundActionFrames = NAlice::RecognizeActions(request, semanticFrames, hints, &selector);
    UNIT_ASSERT_VALUES_EQUAL(foundActionFrames.size(), 2);
    UNIT_ASSERT_VALUES_EQUAL(foundActionFrames.front().GetName(), matchedHintFrame);
    UNIT_ASSERT_VALUES_EQUAL(foundActionFrames.back().GetName(), matchedFrame);
}

}

} // namespace NAlice

#include "entity_recognition.h"

#include <alice/nlu/libs/item_selector/testing/mock.h>
#include <alice/library/frame/builder.h>
#include <alice/library/unittest/message_diff.h>

#include <library/cpp/langs/langs.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice {

using namespace testing;

namespace NItemSelector {

bool operator==(const TSelectionItem& lhs, const TSelectionItem& rhs) {
    // NOTE(the0): For code simplification reason we ignore TSelectionItem::Meta field here as is not used in tests.
    return lhs.Synonyms == rhs.Synonyms && lhs.Negatives == rhs.Negatives;
}

} // namespace NItemSelector

namespace {

TVector<NItemSelector::TSelectionResult> SelectByExactSynonymTextMatch(
    const NItemSelector::TSelectionRequest& request,
    const TVector<NItemSelector::TSelectionItem>& items
) {
    TVector<NItemSelector::TSelectionResult> result;
    for (const auto& item : items) {
        bool isSelected = false;
        double score = 0.0;
        for (const auto& synonym : item.Synonyms) {
            isSelected = (synonym.Text == request.Phrase.Text);
            if (isSelected) {
                score = 1.0;
                break;
            }
        }
        result.push_back({score, isSelected});
    }
    return result;
}


struct TFixture : public NUnitTest::TBaseFixture {
    TFrameDescription FrameDescription = {
        {"slot_1", TVector<TString>{"gallery_1", "string"}},
        {"slot_2", TVector<TString>{"gallery_2_1", "gallery_2_2", "string"}},
        {"slot_3", TVector<TString>{"gallery_2_1", "string", "gallery_2_2"}}
    };
    TVector<TString> Tokens = {"choose", "item", "2", "or", "item", "3", "for", "me"};
    THashMap<TString, TEntityContents> Galleries;

    NItemSelector::TMockItemSelector ItemSelector;
    NItemSelector::TSelectorName SelectorName;
    ELanguage Lang = ELanguage::LANG_RUS;
};

} // namespace

Y_UNIT_TEST_SUITE_F(EntityRecognition, TFixture) {

Y_UNIT_TEST(RecognizeEntities_givenFrameWithRecognizableSlotAndNoGalleries_doesntChangeFrame) {
    const TFrameCandidate candidate{
        "frame",
        {
            {"slot_1", {{"string", "value_1"}}, 0, 1},
            {"slot_2", {{"string", "value_2"}}, 1, 3}
        }
    };
    const TSemanticFrame expected = TSemanticFrameBuilder("frame")
        .AddSlot(
            candidate.SlotValues[0].Name,
            FrameDescription.Slots.at(candidate.SlotValues[0].Name).Types,
            candidate.SlotValues[0].Variants[0].Type,
            candidate.SlotValues[0].Variants[0].Value
        )
        .AddSlot(
            candidate.SlotValues[1].Name,
            FrameDescription.Slots.at(candidate.SlotValues[1].Name).Types,
            candidate.SlotValues[1].Variants[0].Type,
            candidate.SlotValues[1].Variants[0].Value
        )
        .Build();
    const TSemanticFrame got = RecognizeEntities(candidate, Tokens, FrameDescription, Galleries, ItemSelector, SelectorName, Lang);
    UNIT_ASSERT_MESSAGES_EQUAL(expected, got);
}

Y_UNIT_TEST(RecognizeEntities_givenFrameWithUnrecognizableSlotAndAppropriateGallery_doesntChangeFrame) {
    const TString frameName = "frame";
    const TString galleryName = "gallery_2_2";
    const TFrameCandidate candidate{
        frameName,
        {
            {"slot_1", {{"string", "value_1"}}, 0, 1},
            {"slot_2", {{"string", "value_2"}}, 1, 3}
        }
    };
    const TString synonym = GetSlotValue(Tokens, candidate.SlotValues[1].Begin, candidate.SlotValues[1].End);
    Galleries = {
        {galleryName + "_other", {
            .Items = {},
            .Aliases = {}
        }},
        {galleryName, {
            .Items = {
                {.Synonyms = {{.Text = synonym + "_unrecognizable", .Language = Lang}}}
            },
            .Aliases = {
                "item_2"
            }
        }}
    };
    EXPECT_CALL(ItemSelector, Select(_, Galleries[galleryName].Items))
        .Times(1)
        .WillRepeatedly(Invoke(SelectByExactSynonymTextMatch));
    {
        const TSemanticFrame expected = TSemanticFrameBuilder(frameName)
            .AddSlot(
                candidate.SlotValues[0].Name,
                FrameDescription.Slots.at(candidate.SlotValues[0].Name).Types,
                candidate.SlotValues[0].Variants[0].Type,
                candidate.SlotValues[0].Variants[0].Value
            )
            .AddSlot(
                candidate.SlotValues[1].Name,
                FrameDescription.Slots.at(candidate.SlotValues[1].Name).Types,
                candidate.SlotValues[1].Variants[0].Type,
                candidate.SlotValues[1].Variants[0].Value
            )
            .Build();
        const TSemanticFrame got = RecognizeEntities(candidate, Tokens, FrameDescription, Galleries, ItemSelector, SelectorName, Lang);
        UNIT_ASSERT_MESSAGES_EQUAL(expected, got);
    }
}

Y_UNIT_TEST(RecognizeEntities_givenFrameWithRecognizableSlotAndAppropriateGallery_fillsThatSlot) {
    const TString frameName = "frame";
    const TString galleryName = "gallery_2_2";
    const TFrameCandidate candidate{
        frameName,
        {
            {"slot_1", {{"string", "value_1"}}, 0, 1},
            {"slot_2", {{"string", "value_2"}}, 1, 3},
            {"slot_3", {{"string", "value_3"}}, 4, 6}
        }
    };
    const TString firstSynonym = GetSlotValue(Tokens, candidate.SlotValues[1].Begin, candidate.SlotValues[1].End);
    const TString secondSynonym = GetSlotValue(Tokens, candidate.SlotValues[2].Begin, candidate.SlotValues[2].End);
    const TString value = "recognized_value";
    Galleries = {
        {galleryName + "_other", {
            .Items = {},
            .Aliases = {}
        }},
        {galleryName, {
            .Items = {
                {.Synonyms = {{.Text = firstSynonym, .Language = Lang}, {.Text = secondSynonym, .Language = Lang}}}
            },
            .Aliases = {
                value
            }
        }}
    };
    EXPECT_CALL(ItemSelector, Select(_, Galleries[galleryName].Items))
        .Times(1)
        .WillRepeatedly(Invoke(SelectByExactSynonymTextMatch));
    const TSemanticFrame expected = TSemanticFrameBuilder("frame")
        .AddSlot(
            candidate.SlotValues[0].Name,
            FrameDescription.Slots.at(candidate.SlotValues[0].Name).Types,
            candidate.SlotValues[0].Variants[0].Type,
            candidate.SlotValues[0].Variants[0].Value
        )
        .AddSlot(
            candidate.SlotValues[1].Name,
            FrameDescription.Slots.at(candidate.SlotValues[1].Name).Types,
            galleryName,
            value
        )
        .AddSlot(
            candidate.SlotValues[2].Name,
            FrameDescription.Slots.at(candidate.SlotValues[2].Name).Types,
            candidate.SlotValues[2].Variants[0].Type,
            candidate.SlotValues[2].Variants[0].Value
        )
        .Build();
    const TSemanticFrame got = RecognizeEntities(candidate, Tokens, FrameDescription, Galleries, ItemSelector, SelectorName, Lang);
    UNIT_ASSERT_MESSAGES_EQUAL(expected, got);
}

}

} // namespace NAlice

#include "what_is_playing_answer.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

Y_UNIT_TEST_SUITE(WhatIsPlayingAnswerTest) {

Y_UNIT_TEST(MinimalisticWeirdCase) {
    TQueueItem item;
    UNIT_ASSERT_STRINGS_EQUAL(MakeWhatIsPlayingAnswer(item), "песня");
}

Y_UNIT_TEST(MinimalisticRockCase) {
    TQueueItem item;
    item.MutableTrackInfo()->SetGenre("rock");
    UNIT_ASSERT_STRINGS_EQUAL(MakeWhatIsPlayingAnswer(item), "песня");
}

Y_UNIT_TEST(MinimalisticClassicalCase) {
    TQueueItem item;
    item.MutableTrackInfo()->SetGenre("classical");
    UNIT_ASSERT_STRINGS_EQUAL(MakeWhatIsPlayingAnswer(item), "композиция");
}

Y_UNIT_TEST(MinimalisticRockCaseWithTitle) {
    TQueueItem item;
    item.SetTitle("Title");
    item.MutableTrackInfo()->SetGenre("rock");
    UNIT_ASSERT_STRINGS_EQUAL(MakeWhatIsPlayingAnswer(item), "песня \"Title\"");
}

Y_UNIT_TEST(MinimalisticClassicalCaseWithTitle) {
    TQueueItem item;
    item.SetTitle("Title");
    item.MutableTrackInfo()->SetGenre("classical");
    UNIT_ASSERT_STRINGS_EQUAL(MakeWhatIsPlayingAnswer(item), "композиция \"Title\"");
}

Y_UNIT_TEST(OneArtistRockCase) {
    TQueueItem item;
    item.SetTitle("Title");
    item.MutableTrackInfo()->SetGenre("rock");
    auto& artistA = *item.MutableTrackInfo()->MutableArtists()->Add();
    artistA.SetName("Artist A");
    artistA.SetComposer(false);

    UNIT_ASSERT_STRINGS_EQUAL(MakeWhatIsPlayingAnswer(item), "Artist A, песня \"Title\"");
}

Y_UNIT_TEST(TwoArtistsRockCase) {
    TQueueItem item;
    item.SetTitle("Title");
    item.MutableTrackInfo()->SetGenre("rock");
    auto& artistB = *item.MutableTrackInfo()->MutableArtists()->Add();
    artistB.SetName("Artist B");
    artistB.SetComposer(false);

    auto& artistA = *item.MutableTrackInfo()->MutableArtists()->Add();
    artistA.SetName("Artist A");
    artistA.SetComposer(false);

    UNIT_ASSERT_STRINGS_EQUAL(MakeWhatIsPlayingAnswer(item), "Artist B, Artist A, песня \"Title\"");
}

Y_UNIT_TEST(ComposerRockCase) {
    TQueueItem item;
    item.SetTitle("Title");
    item.MutableTrackInfo()->SetGenre("rock");

    auto& composerA = *item.MutableTrackInfo()->MutableArtists()->Add();
    composerA.SetName("Composer A");
    composerA.SetComposer(true);

    UNIT_ASSERT_STRINGS_EQUAL(MakeWhatIsPlayingAnswer(item), "Composer A, песня \"Title\"");
}

Y_UNIT_TEST(TwoComposersClassicalCase) {
    TQueueItem item;
    item.SetTitle("Title");
    item.MutableTrackInfo()->SetGenre("classical");

    auto& composerB = *item.MutableTrackInfo()->MutableArtists()->Add();
    composerB.SetName("Composer B");
    composerB.SetComposer(true);

    auto& composerA = *item.MutableTrackInfo()->MutableArtists()->Add();
    composerA.SetName("Composer A");
    composerA.SetComposer(true);

    UNIT_ASSERT_STRINGS_EQUAL(MakeWhatIsPlayingAnswer(item), "Composer B, Composer A, композиция \"Title\"");
}

Y_UNIT_TEST(TwoComposersTwoArtistsRockCase) {
    TQueueItem item;
    item.SetTitle("Title");
    item.MutableTrackInfo()->SetGenre("rock");

    auto& artistB = *item.MutableTrackInfo()->MutableArtists()->Add();
    artistB.SetName("Artist B");
    artistB.SetComposer(false);

    auto& composerB = *item.MutableTrackInfo()->MutableArtists()->Add();
    composerB.SetName("Composer B");
    composerB.SetComposer(true);

    auto& composerA = *item.MutableTrackInfo()->MutableArtists()->Add();
    composerA.SetName("Composer A");
    composerA.SetComposer(true);

    auto& artistA = *item.MutableTrackInfo()->MutableArtists()->Add();
    artistA.SetName("Artist A");
    artistA.SetComposer(false);

    UNIT_ASSERT_STRINGS_EQUAL(MakeWhatIsPlayingAnswer(item), "Composer B, Composer A, Artist B, Artist A, песня \"Title\"");
}

Y_UNIT_TEST(GenerativeMusic) {
    TQueueItem item;
    item.SetTitle("Мне повезёт!");
    item.SetType("generative");
    UNIT_ASSERT_STRINGS_EQUAL(MakeWhatIsPlayingAnswer(item), "нейромузыка на станции \"Мне повезёт!\"");
}

Y_UNIT_TEST(Shot) {
    TQueueItem item;
    item.SetTitle("Шот от Алисы");
    item.SetType("shot");
    UNIT_ASSERT_STRINGS_EQUAL(MakeWhatIsPlayingAnswer(item), "\"Шот от Алисы\"");
}

}

} // namespace NAlice::NHollywood::NMusic {

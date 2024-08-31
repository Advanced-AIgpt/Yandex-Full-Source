#include "music_queue.h"

#include <alice/library/proto/protobuf.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/protos/data/scenario/music/content_info.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

namespace {

void FillDummyMusicQueueItems(TMusicQueueWrapper& mq, TMusicConfig cfg,
                              i32 historyItems = 0, bool moveFromQueueToHistory = true) {
    // TODO(vitvlkv): Take historyItems from cfg maybe?
    for (i32 i = 0; i < cfg.PageSize + historyItems; ++i) {
        TQueueItem item;
        item.SetPlayId(ToString(i));
        mq.TryAddItem(std::move(item), /* hasMusicSubscription = */ true);
    }
    mq.ChangeState(moveFromQueueToHistory);
    for (i32 i = 0; i < historyItems; ++i) {
        mq.ChangeToNextTrack();
    }
}

void FillDummyMusicQueueItemsWithoutHistory(TMusicQueueWrapper& mq, int itemsCount, i32 firstItemIndex = 0,
                                            std::function<void(TQueueItem&)>&& itemCb = {}) {
    for (i32 i = 0; i < itemsCount; ++i) {
        TQueueItem item;
        item.SetPlayId(ToString(firstItemIndex++));
        if (itemCb) {
            itemCb(item);
        }
        mq.TryAddItem(std::move(item), /* hasMusicSubscription = */ true);
    }
    mq.ChangeState();
}

TMusicQueueWrapper GetMusicQueue(
    TScenarioState& scState,
    const TContentId_EContentType contentType,
    const TString contentId = Default<TString>()
) {
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    TMusicConfig cfg;
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(contentType);
    id.SetId(contentId);

    TRng rng;
    mq.InitPlayback(id, rng);
    return mq;
}

TMusicQueueWrapper GetRadioPumpkinMusicQueue(
    TScenarioState& scState,
    const TContentId_EContentType contentType,
    const TString contentId = Default<TString>()
) {
    auto mq = GetMusicQueue(scState, contentType, contentId);
    mq.SetIsRadioPumpkin(true);
    return mq;
}

Y_UNIT_TEST_SUITE(MusicQueueTest) {

Y_UNIT_TEST(Next) {
    TScenarioState scState;
    TRng rng;

    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    auto cfg = CreateMusicConfig({});
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Artist);
    id.SetId("123");
    mq.InitPlayback(id, rng);

    mq.SetNextTotalTracks(cfg.PageSize * 2);

    FillDummyMusicQueueItems(mq, cfg);

    for (i32 i = 0; i < cfg.PageSize - 1; ++i) {
        UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), ToString(i));
        UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    }
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), ToString(cfg.PageSize - 1));
    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::NeedStateUpdate);
    UNIT_ASSERT(mq.NeedToChangeState());
    FillDummyMusicQueueItems(mq, cfg);

    for (i32 i = 0; i < cfg.PageSize - 1; ++i) {
        UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), ToString(i));
        UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    }
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), ToString(cfg.PageSize - 1));
    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::EndOfContent);
}

Y_UNIT_TEST(Prev) {
    TScenarioState scState;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    auto cfg = CreateMusicConfig({});
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Artist);
    id.SetId("123");
    mq.InitPlayback(id, rng);

    FillDummyMusicQueueItems(mq, cfg, cfg.HistorySize);

    for (i32 i = 0; i < cfg.HistorySize; ++i) {
        UNIT_ASSERT_EQUAL(mq.ChangeToPrevTrack(), ETrackChangeResult::TrackChanged);
    }
    UNIT_ASSERT_EQUAL(mq.ChangeToPrevTrack(), ETrackChangeResult::EndOfContent);
}

Y_UNIT_TEST(Shuffle) {
    TScenarioState scState;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    auto cfg = CreateMusicConfig({});
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Artist);
    id.SetId("123");
    mq.InitPlayback(id, rng);

    FillDummyMusicQueueItems(mq, cfg, cfg.HistorySize, /* moveFromQueueToHistory = */ false);

    mq.ShufflePlayback(rng);
    UNIT_ASSERT_STRINGS_EQUAL(mq.ContentId().GetId(), "123");
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), ToString(cfg.HistorySize - 1));
    UNIT_ASSERT_EQUAL(mq.ContentId().GetType(), TContentId_EContentType_Artist);
    UNIT_ASSERT_EQUAL(scState.GetQueue().GetPlaybackContext().GetShuffle(), true);
    UNIT_ASSERT_EQUAL(scState.GetQueue().GetPlaybackContext().GetRepeatType(), RepeatNone);
}

Y_UNIT_TEST(ShuffleAfterRepeat) {
    TRng rng;
    TScenarioState scState;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    auto cfg = CreateMusicConfig({});
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Artist);
    id.SetId("123");
    TMusicArguments::TPlaybackOptions playbackOptions;
    playbackOptions.SetRepeatType(RepeatAll);
    mq.InitPlayback(id, rng, playbackOptions);

    FillDummyMusicQueueItems(mq, cfg, cfg.HistorySize, /* moveFromQueueToHistory = */ false);

    mq.ShufflePlayback(rng);
    UNIT_ASSERT_STRINGS_EQUAL(mq.ContentId().GetId(), "123");
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), ToString(cfg.HistorySize - 1));
    UNIT_ASSERT_EQUAL(mq.ContentId().GetType(), TContentId_EContentType_Artist);
    UNIT_ASSERT_EQUAL(scState.GetQueue().GetPlaybackContext().GetShuffle(), true);
    UNIT_ASSERT_EQUAL(scState.GetQueue().GetPlaybackContext().GetRepeatType(), RepeatAll);
}

Y_UNIT_TEST(NextPrevWithRepeatTrack) {
    TRng rng;
    TScenarioState scState;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    TMusicConfig cfg{/* pageSize = */ 100, /* historySize = */ 2};
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Artist);
    id.SetId("123");
    mq.InitPlayback(id, rng);
    FillDummyMusicQueueItemsWithoutHistory(mq, 10); // Fills the queue and moves 0 track to the history, history=[0]
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "0");

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged); // history=[0, 1]
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "1");

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged); // history=[0, 1, 2]
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "2");

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(),
                             ETrackChangeResult::TrackChanged); // history=[1, 2, 3], item=0 is lost now
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");

    mq.RepeatPlayback(RepeatTrack);
    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::SameTrack);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::SameTrack);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToPrevTrack(), ETrackChangeResult::SameTrack);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToPrevTrack(), ETrackChangeResult::SameTrack);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");

    // Let us see what we have in our history=[1, 2, 3], where 3 is the current item.
    mq.RepeatPlayback(RepeatNone);
    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToPrevTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "2");

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToPrevTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "1");

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToPrevTrack(), ETrackChangeResult::EndOfContent);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "1");
}

Y_UNIT_TEST(IsCurrentTrackLastInTheEndOfThePage) {
    TRng rng;
    TScenarioState scState;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    TMusicConfig cfg{/* pageSize = */ 3, /* historySize = */ 100};
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Artist);
    id.SetId("123");
    mq.InitPlayback(id, rng);
    mq.SetNextTotalTracks(cfg.PageSize * 2);

    // "Load" the first page of content
    FillDummyMusicQueueItemsWithoutHistory(mq, cfg.PageSize);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "0");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "1");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "2");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::NeedStateUpdate);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "2");

    // Now "load" the next page of content
    FillDummyMusicQueueItemsWithoutHistory(mq, cfg.PageSize, /* firstItemIndex = */ 3);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "4");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "5");
    UNIT_ASSERT(mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::EndOfContent);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "5");
    UNIT_ASSERT(mq.IsCurrentTrackLast());
}

Y_UNIT_TEST(IsCurrentTrackLastInTheMiddleOfThePage) {
    TRng rng;
    TScenarioState scState;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    TMusicConfig cfg{/* pageSize = */ 3, /* historySize = */ 100};
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Artist);
    id.SetId("123");
    mq.InitPlayback(id, rng);
    mq.SetNextTotalTracks(cfg.PageSize * 2);

    // "Load" the first page of content
    FillDummyMusicQueueItemsWithoutHistory(mq, cfg.PageSize);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "0");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "1");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "2");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::NeedStateUpdate);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "2");

    // Now "load" the next page of content
    FillDummyMusicQueueItemsWithoutHistory(mq, cfg.PageSize - 1, /* firstItemIndex = */ 3);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "4");
    UNIT_ASSERT(mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::EndOfContent);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "4");
    UNIT_ASSERT(mq.IsCurrentTrackLast());
}

Y_UNIT_TEST(IsCurrentTrackLastWithRepeatAll) {
    TRng rng;
    TScenarioState scState;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    TMusicConfig cfg{/* pageSize = */ 100, /* historySize = */ 100};
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Artist);
    id.SetId("123");
    TMusicArguments::TPlaybackOptions playbackOptions;
    playbackOptions.SetRepeatType(RepeatAll);
    mq.InitPlayback(id, rng, playbackOptions);
    mq.SetNextTotalTracks(3);

    // "Load" all tracks of the content now
    FillDummyMusicQueueItemsWithoutHistory(mq, 3);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "0");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "1");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "2");
    UNIT_ASSERT_C(mq.IsCurrentTrackLast(), "Repeat mode should not affect the status of the last item of the content");

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::NeedStateUpdate);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "2");
    UNIT_ASSERT_C(mq.IsCurrentTrackLast(), "Repeat mode should not affect the status of the last item of the content");

    // The queue is empty. So we "load" the same content (or, more precisely, the first page of the same content) again. This is what music scenario does.
    FillDummyMusicQueueItemsWithoutHistory(mq, 3, /* firstItemIndex= */ 0);

    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "0");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "1");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "2");
    UNIT_ASSERT_C(mq.IsCurrentTrackLast(), "Repeat mode should not affect the status of the last item of the content");
}

Y_UNIT_TEST(RadioLoadBatch) {
    TRng rng;
    TScenarioState scState;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    TMusicConfig cfg;
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Radio);
    id.SetId("mood:sad");
    mq.InitPlayback(id, rng);
    mq.SetRadioBatchId("foo-bar-baz");
    mq.SetRadioSessionId("123-456-789");

    // "Load" the first batch of radio tracks
    FillDummyMusicQueueItemsWithoutHistory(mq, 3);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "0");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "1");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "2");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::NeedStateUpdate);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "2");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    // Now "load" the next batch of radio tracks
    FillDummyMusicQueueItemsWithoutHistory(mq, 2, /* firstItemIndex = */ 3);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "4");
    UNIT_ASSERT(!mq.IsCurrentTrackLast());

    UNIT_ASSERT_VALUES_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::NeedStateUpdate);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "4");
    UNIT_ASSERT_C(!mq.IsCurrentTrackLast(), "There should be no 'last track' in radio");
}

Y_UNIT_TEST(ModerateFiltrationWarning) {
    TRng rng;
    TMusicContext mCtx;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    TMusicConfig cfg({});
    mq.SetConfig(cfg);
    mq.SetFiltrationMode(NScenarios::TUserPreferences::Moderate);

    TContentId id;
    id.SetType(TContentId_EContentType_Album);
    id.SetId("123");
    mq.InitPlayback(id, rng);

    FillDummyMusicQueueItemsWithoutHistory(mq, 3, 0, [](auto& item) {
        if (item.GetPlayId() == "1") { // add one track with explicit lyrics to album
            item.SetContentWarning(EContentWarning::Explicit);
        }
    });
    mq.CalcContentErrorsAndAttentions(mCtx);
    UNIT_ASSERT(mCtx.HasContentStatus());
    UNIT_ASSERT(mCtx.GetContentStatus().GetAttentionVer2() != EContentAttentionVer2::NoAttention);
    UNIT_ASSERT_EQUAL(mCtx.GetContentStatus().GetAttentionVer2(),
                      EContentAttentionVer2::AttentionMayContainExplicitContentVer2);
}

Y_UNIT_TEST(FamilyFiltrationNoWarning) {
    TRng rng;
    TMusicContext mCtx;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    TMusicConfig cfg({});
    mq.SetConfig(cfg);
    mq.SetFiltrationMode(NScenarios::TUserPreferences::FamilySearch);

    TContentId id;
    id.SetType(TContentId_EContentType_Album);
    id.SetId("123");
    mq.InitPlayback(id, rng);

    FillDummyMusicQueueItemsWithoutHistory(mq, 10, 0, [](auto& item) {
        if (FromString<int>(item.GetPlayId()) < 2) { // two tracks is less than 25%
            item.SetContentWarning(EContentWarning::Explicit);
        }
    });
    mq.CalcContentErrorsAndAttentions(mCtx);
    UNIT_ASSERT(mCtx.HasContentStatus());
    UNIT_ASSERT_EQUAL(mCtx.GetContentStatus().GetAttentionVer2(),
                      EContentAttentionVer2::AttentionContainsAdultContentVer2);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), 8); //2 out of 10 were filtered out
}

Y_UNIT_TEST(FamilyFiltrationWarning) {
    TRng rng;
    TMusicContext mCtx;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    TMusicConfig cfg({});
    mq.SetConfig(cfg);
    mq.SetFiltrationMode(NScenarios::TUserPreferences::FamilySearch);

    TContentId id;
    id.SetType(TContentId_EContentType_Album);
    id.SetId("123");
    mq.InitPlayback(id, rng);

    FillDummyMusicQueueItemsWithoutHistory(mq, 10, 0, [](auto& item) {
        if (FromString<int>(item.GetPlayId()) < 5) { // five tracks is definitely more than 25%
            item.SetContentWarning(EContentWarning::Explicit);
        }
    });
    mq.CalcContentErrorsAndAttentions(mCtx);
    UNIT_ASSERT(mCtx.HasContentStatus());
    UNIT_ASSERT_EQUAL(mCtx.GetContentStatus().GetAttentionVer2(),
                      EContentAttentionVer2::AttentionExplicitContentFilteredVer2);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), 5); //5 out of 10 were filtered out
}

Y_UNIT_TEST(FamilyFiltrationAllFiltered) {
    TRng rng;
    TMusicContext mCtx;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    TMusicConfig cfg({});
    mq.SetConfig(cfg);
    mq.SetFiltrationMode(NScenarios::TUserPreferences::FamilySearch);

    TContentId id;
    id.SetType(TContentId_EContentType_Album);
    id.SetId("123");
    mq.InitPlayback(id, rng);

    FillDummyMusicQueueItemsWithoutHistory(mq, 10, 0, [](auto& item) {
        item.SetContentWarning(EContentWarning::Explicit);
    });
    mq.CalcContentErrorsAndAttentions(mCtx);
    UNIT_ASSERT(mCtx.HasContentStatus());
    UNIT_ASSERT_EQUAL(mCtx.GetContentStatus().GetErrorVer2(), EContentErrorVer2::ErrorForbiddenVer2);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), 0);
}

Y_UNIT_TEST(SafeFiltrationAllSafe) {
    TRng rng;
    TMusicContext mCtx;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    TMusicConfig cfg({});
    mq.SetConfig(cfg);
    mq.SetFiltrationMode(NScenarios::TUserPreferences::Safe);

    TContentId id;
    id.SetType(TContentId_EContentType_Album);
    id.SetId("123");
    mq.InitPlayback(id, rng);

    FillDummyMusicQueueItemsWithoutHistory(mq, 10, 0, [](auto& item) {
        item.SetContentWarning(EContentWarning::ChildSafe);
    });
    mq.CalcContentErrorsAndAttentions(mCtx);
    UNIT_ASSERT(!mCtx.HasContentStatus()); // all is safe, no warnings
}

Y_UNIT_TEST(SafeFiltrationNotAllSafe) {
    TRng rng;
    TMusicContext mCtx;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    TMusicConfig cfg({});
    mq.SetConfig(cfg);
    mq.SetFiltrationMode(NScenarios::TUserPreferences::Safe);

    TContentId id;
    id.SetType(TContentId_EContentType_Album);
    id.SetId("123");
    mq.InitPlayback(id, rng);

    FillDummyMusicQueueItemsWithoutHistory(mq, 10, 0, [](auto& item) {
        if (item.GetPlayId() != "7") {
            item.SetContentWarning(EContentWarning::ChildSafe);
        }
    });
    mq.CalcContentErrorsAndAttentions(mCtx);
    UNIT_ASSERT(mCtx.HasContentStatus());
    UNIT_ASSERT_EQUAL(mCtx.GetContentStatus().GetErrorVer2(), EContentErrorVer2::ErrorRestrictedByChildVer2);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), 0);
}

Y_UNIT_TEST(ShotsTest) {
    TRng rng;
    TMusicContext mCtx;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    TMusicConfig cfg({});
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Album);
    id.SetId("123");
    mq.InitPlayback(id, rng, {}, true);

    UNIT_ASSERT(mq.HasShotsEnabled());

    FillDummyMusicQueueItemsWithoutHistory(mq, 10);

    {
        TExtraPlayable_TShot shot;
        shot.SetTitle("shot title");
        shot.SetCoverUri("cover uri");
        shot.SetMdsUrl("mds url");
        shot.SetId("54321");
        mq.AddShotBeforeTrack(mq.CurrentItem().GetTrackId(), std::move(shot));
    }

    UNIT_ASSERT(mq.HasExtraBeforeCurrentItem());

    auto shot = mq.GetShotBeforeCurrentItem();
    UNIT_ASSERT(shot);
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetTitle(), "shot title");
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetCoverUri(), "cover uri");
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetMdsUrl(), "mds url");
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetId(), "54321");
    mq.SetShotPlayed(mq.MutableCurrentItem(), /* played = */ true, /* onlyFirstAvailable = */ true);
    UNIT_ASSERT(!mq.GetShotBeforeCurrentItem());
}

Y_UNIT_TEST(ShotsMarkTest) {
    TRng rng;
    TMusicContext mCtx;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    TMusicConfig cfg({});
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Album);
    id.SetId("123");
    mq.InitPlayback(id, rng, {}, true);

    UNIT_ASSERT(mq.HasShotsEnabled());

    FillDummyMusicQueueItemsWithoutHistory(mq, 10);

    mq.MarkBeforeTrackSlot(mq.CurrentItem().GetTrackId());

    UNIT_ASSERT(mq.HasExtraBeforeCurrentItem());
}

// TODO(vitvlkv): Add test for ChangeToPrevTrack + RepeatAll/RepeatTrack modes

Y_UNIT_TEST(PrevNextWithPlaySingle) {
    TScenarioState scState;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    auto cfg = CreateMusicConfig({});
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Artist);
    id.SetId("123");
    TMusicArguments::TPlaybackOptions playbackOptions;
    playbackOptions.SetPlaySingleTrack(true);
    mq.InitPlayback(id, rng, playbackOptions);
    UNIT_ASSERT(mq.IsPlayingSingleTrack());

    mq.SetPlaySingleTrack(false); // to fill history
    FillDummyMusicQueueItems(mq, cfg, 3);
    mq.SetPlaySingleTrack(true);
    UNIT_ASSERT(mq.IsPlayingSingleTrack());

    UNIT_ASSERT(mq.IsCurrentTrackLast());
    UNIT_ASSERT_EQUAL(mq.ChangeToPrevTrack(), ETrackChangeResult::EndOfContent);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");
    UNIT_ASSERT_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::EndOfContent);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");

    mq.RepeatPlayback(RepeatAll);
    UNIT_ASSERT(mq.IsCurrentTrackLast());
    UNIT_ASSERT_EQUAL(mq.ChangeToPrevTrack(), ETrackChangeResult::SameTrack);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");
    UNIT_ASSERT_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::SameTrack);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");

    mq.RepeatPlayback(RepeatTrack);
    UNIT_ASSERT(mq.IsCurrentTrackLast());
    UNIT_ASSERT_EQUAL(mq.ChangeToPrevTrack(), ETrackChangeResult::SameTrack);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");
    UNIT_ASSERT_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::SameTrack);
    UNIT_ASSERT_STRINGS_EQUAL(mq.CurrentItem().GetPlayId(), "3");
}

Y_UNIT_TEST(MakeRecoveryActionCallbackPayloadPaged) {
    TScenarioState scState;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    TMusicConfig cfg;
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Artist);
    id.SetId("artist:123456");
    TRng rng;
    NData::NMusic::TContentInfo contentInfo;
    contentInfo.SetName("Brian Olsdal");

    mq.InitPlayback(id, rng, Default<TMusicArguments::TPlaybackOptions>(), /* enableShots = */ false, contentInfo);

    mq.ChangeState();

    auto payload = mq.MakeRecoveryActionCallbackPayload();

    auto expected = TProtoStructBuilder()
    .Set("playback_context", TProtoStructBuilder()
        .Set("content_id", TProtoStructBuilder()
            .Set("id", "artist:123456")
            .Set("type", "Artist")
            .Build())
        .Set("content_info", TProtoStructBuilder()
            .Set("name", "Brian Olsdal")
            .Build())
        .Build())
    .Set("paged", TProtoStructBuilder()
        .Build())
    .Build();

    UNIT_ASSERT_MESSAGES_EQUAL(payload, expected);
}

Y_UNIT_TEST(MakeRecoveryActionCallbackPayloadRadio) {
    TScenarioState scState;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    TMusicConfig cfg;
    mq.SetConfig(cfg);

    TContentId id;
    id.SetType(TContentId_EContentType_Radio);
    id.SetId("mood:sad");
    TRng rng;
    mq.InitPlayback(id, rng);
    mq.SetRadioBatchId("foo-bar-baz");
    mq.SetRadioSessionId("123-456-789");

    mq.ChangeState();

    auto payload = mq.MakeRecoveryActionCallbackPayload();
    auto expected = TProtoStructBuilder()
        .Set("playback_context", TProtoStructBuilder()
            .Set("content_id", TProtoStructBuilder()
                .Set("id", "mood:sad")
                .Set("type", "Radio")
                .Build())
            .Build())
        .Set("radio", TProtoStructBuilder()
            .Set("batch_id", "foo-bar-baz")
            .Set("session_id", "123-456-789")
            .Build())
        .Build();

        UNIT_ASSERT_MESSAGES_EQUAL(payload, expected);
}

Y_UNIT_TEST(MakeFrom) {
    {
        TScenarioState scState;
        const auto mq = GetMusicQueue(scState, TContentId_EContentType_Track);
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-on_demand-catalogue_-track");
    }

    {
        TScenarioState scState;
        const auto mq = GetMusicQueue(scState, TContentId_EContentType_Album);
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-on_demand-catalogue_-album");
    }

    {
        TScenarioState scState;
        const auto mq = GetMusicQueue(scState, TContentId_EContentType_Artist);
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-on_demand-catalogue_-artist");
    }

    {
        TScenarioState scState;
        const auto mq = GetMusicQueue(scState, TContentId_EContentType_Playlist);
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-on_demand-user_playlist-playlist");
    }

    {
        TScenarioState scState;
        const auto mq = GetMusicQueue(scState, TContentId_EContentType_Radio, "mood:happy");
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-discovery-radio-mood");
    }

    {
        TScenarioState scState;
        const auto mq = GetMusicQueue(scState, TContentId_EContentType_Radio, "track:123");
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-discovery-radio-track");
    }

    {
        TScenarioState scState;
        const auto mq = GetMusicQueue(scState, TContentId_EContentType_Generative);
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-discovery-generative-genre");
    }

    {
        TScenarioState scState;
        const auto mq = GetMusicQueue(scState, TContentId_EContentType_Radio, "dummy_content_id");
        UNIT_ASSERT_EXCEPTION_CONTAINS(mq.MakeFrom(), yexception,
                                       "There is no ':' in radio content id dummy_content_id");
    }
}

Y_UNIT_TEST(MakeFromWhenRadioPumpkin) {
    {
        TScenarioState scState;
        auto mq = GetRadioPumpkinMusicQueue(scState, TContentId_EContentType_Radio, "user:onyourwave");
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-discovery-radio-pumpkin");
        mq.SetIsRadioPumpkin(false);
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-discovery-radio-user");
    }

    {
        TScenarioState scState;
        const auto mq = GetRadioPumpkinMusicQueue(scState, TContentId_EContentType_Radio, "dummy_content_id");
        UNIT_ASSERT_EXCEPTION_CONTAINS(mq.MakeFrom(), yexception,    // pumpkin doesn't save from incorrect format of content id
                                       "There is no ':' in radio content id dummy_content_id");
    }

    {
        TScenarioState scState;
        const auto mq = GetRadioPumpkinMusicQueue(scState, TContentId_EContentType_Track);
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-on_demand-catalogue_-track");
    }

    {
        TScenarioState scState;
        const auto mq = GetRadioPumpkinMusicQueue(scState, TContentId_EContentType_Album);
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-on_demand-catalogue_-album");
    }

    {
        TScenarioState scState;
        const auto mq = GetRadioPumpkinMusicQueue(scState, TContentId_EContentType_Artist);
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-on_demand-catalogue_-artist");
    }

    {
        TScenarioState scState;
        const auto mq = GetRadioPumpkinMusicQueue(scState, TContentId_EContentType_Playlist);
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-on_demand-user_playlist-playlist");
    }

    {
        TScenarioState scState;
        const auto mq = GetRadioPumpkinMusicQueue(scState, TContentId_EContentType_Generative);
        UNIT_ASSERT_STRINGS_EQUAL(mq.MakeFrom(), "alice-discovery-generative-genre");
    }
}

}

} // namespace

} //namespace NAlice::NHollywood::NMusic

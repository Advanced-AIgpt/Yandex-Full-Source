#include "shots.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

namespace {

void PrepareQueue(TMusicQueueWrapper& mq) {
    TContentId id;
    id.SetType(TContentId_EContentType_Album);
    id.SetId("123");
    TRng rng;
    mq.InitPlayback(id, rng, {}, true);
    TQueueItem track;
    track.SetTrackId("123");
    track.MutableTrackInfo()->SetAlbumId("123");
    track.MutableTrackInfo()->SetAvailable(true);
    mq.TryAddItem(std::move(track), /* hasMusicSubscription = */ true);
    mq.ChangeState();
}

}

Y_UNIT_TEST_SUITE(ShotsTest) {

Y_UNIT_TEST(ParseEmpty) {
    TStringBuf resp = R"({
   "invocationInfo" : {
      "exec-duration-millis" : "3",
      "hostname" : "music-stable-back-man-28.man.yp-c.yandex.net",
      "req-id" : "1610520359248853-14567546177670633031"
   },
   "result" : {}
}
)";
    TMusicContext mCtx;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());

    PrepareQueue(mq);

    ProcessShotsResponse(TRTLogger::NullLogger(), resp, mq);

    UNIT_ASSERT(mq.HasExtraBeforeCurrentItem());

}

Y_UNIT_TEST(ParseNotReadyEmpty) {
    TStringBuf resp = R"({
   "invocationInfo" : {
      "exec-duration-millis" : "13",
      "hostname" : "music-stable-back-vla-16.vla.yp-c.yandex.net",
      "req-id" : "1609303957208966-1465486148176237561"
   },
   "result" : {
      "shotEvent" : {
         "eventId" : "5fec07954a57d84301ae0b87",
         "shots" : [
            {
               "shotId" : "352544",
               "status" : "not ready"
            }
         ]
      }
   }
}
)";
    TMusicContext mCtx;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());

    PrepareQueue(mq);

    ProcessShotsResponse(TRTLogger::NullLogger(), resp, mq);

    UNIT_ASSERT(mq.HasExtraBeforeCurrentItem());
}

Y_UNIT_TEST(ParseNonEmpty) {
    TStringBuf resp = R"({
   "invocationInfo" : {
      "exec-duration-millis" : "13",
      "hostname" : "music-stable-back-vla-16.vla.yp-c.yandex.net",
      "req-id" : "1609303957208966-1465486148176237561"
   },
   "result" : {
      "shotEvent" : {
         "eventId" : "5fec07954a57d84301ae0b87",
         "shots" : [
            {
               "order" : 0,
               "played" : false,
               "shotData" : {
                  "coverUri" : "avatars.mds.yandex.net/get-music-misc/49997/img.5da435f1da39b871a74270e2/%%",
                  "mdsUrl" : "https://storage.mds.yandex.net/get-music-shots/3421582/f43b443b-ae6c-4d13-a9ab-db444b65c33c_1592620822.mp3",
                  "shotText" : "shot text",
                  "shotType" : {
                     "id" : "alice",
                     "title" : "Шот от Алисы"
                  }
               },
               "shotId" : "352544",
               "status" : "ready"
            }
         ]
      }
   }
}
)";
    TMusicContext mCtx;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());

    PrepareQueue(mq);

    ProcessShotsResponse(TRTLogger::NullLogger(), resp, mq);

    UNIT_ASSERT(mq.HasExtraBeforeCurrentItem());

    auto shot = mq.GetShotBeforeCurrentItem();
    UNIT_ASSERT(shot);
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetId(), "352544");
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetMdsUrl(), "https://storage.mds.yandex.net/get-music-shots/3421582/f43b443b-ae6c-4d13-a9ab-db444b65c33c_1592620822.mp3");
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetText(), "shot text");
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetTitle(), "Шот от Алисы");
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetCoverUri(), "avatars.mds.yandex.net/get-music-misc/49997/img.5da435f1da39b871a74270e2/%%");
    mq.SetShotPlayed(mq.MutableCurrentItem(), /* played = */ true, /* onlyFirstAvailable = */ true);

    shot = mq.GetLastShotOfCurrentItem();
    UNIT_ASSERT(shot);
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetId(), "352544");
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetMdsUrl(), "https://storage.mds.yandex.net/get-music-shots/3421582/f43b443b-ae6c-4d13-a9ab-db444b65c33c_1592620822.mp3");
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetText(), "shot text");
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetTitle(), "Шот от Алисы");
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetCoverUri(), "avatars.mds.yandex.net/get-music-misc/49997/img.5da435f1da39b871a74270e2/%%");
}

Y_UNIT_TEST(CheckOrder) {
    TStringBuf resp = R"({
   "invocationInfo" : {
      "exec-duration-millis" : "13",
      "hostname" : "music-stable-back-vla-16.vla.yp-c.yandex.net",
      "req-id" : "1609303957208966-1465486148176237561"
   },
   "result" : {
      "shotEvent" : {
         "eventId" : "5fec07954a57d84301ae0b87",
         "shots" : [
            {
               "order" : 1,
               "played" : false,
               "shotData" : {
                  "coverUri" : "avatars.mds.yandex.net/get-music-misc/49997/img.5da435f1da39b871a74270e2/%%",
                  "mdsUrl" : "https://storage.mds.yandex.net/get-music-shots/3421582/f43b443b-ae6c-4d13-a9ab-db444b65c33c_1592620822.mp3",
                  "shotText" : "shot text",
                  "shotType" : {
                     "id" : "alice",
                     "title" : "Шот от Алисы"
                  }
               },
               "shotId" : "2",
               "status" : "ready"
            },
            {
               "order" : 0,
               "played" : false,
               "shotData" : {
                  "coverUri" : "avatars.mds.yandex.net/get-music-misc/49997/img.5da435f1da39b871a74270e2/%%",
                  "mdsUrl" : "https://storage.mds.yandex.net/get-music-shots/3421582/f43b443b-ae6c-4d13-a9ab-db444b65c33c_1592620822.mp3",
                  "shotText" : "shot text",
                  "shotType" : {
                     "id" : "alice",
                     "title" : "Шот от Алисы"
                  }
               },
               "shotId" : "1",
               "status" : "ready"
            }
         ]
      }
   }
}
)";
    TMusicContext mCtx;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());

    PrepareQueue(mq);

    ProcessShotsResponse(TRTLogger::NullLogger(), resp, mq);

    UNIT_ASSERT(mq.HasExtraBeforeCurrentItem());

    auto shot = mq.GetShotBeforeCurrentItem();
    mq.SetShotPlayed(mq.MutableCurrentItem(), /* played = */ true, /* onlyFirstAvailable = */ true);
    UNIT_ASSERT(shot);
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetId(), "1");
    shot = mq.GetShotBeforeCurrentItem();
    mq.SetShotPlayed(mq.MutableCurrentItem(), /* played = */ true, /* onlyFirstAvailable = */ true);
    UNIT_ASSERT(shot);
    UNIT_ASSERT_STRINGS_EQUAL(shot->GetId(), "2");
    shot = mq.GetShotBeforeCurrentItem();
    UNIT_ASSERT(!shot);
}

}

} //namespace NAlice::NHollywood::NMusic

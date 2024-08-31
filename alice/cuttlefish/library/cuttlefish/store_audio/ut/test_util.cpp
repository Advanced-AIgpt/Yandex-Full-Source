#include <library/cpp/testing/unittest/registar.h>
#include <alice/cuttlefish/library/cuttlefish/store_audio/util.h>
#include <alice/cuttlefish/library/protos/session.pb.h>


using namespace NAlice::NCuttlefish::NAppHostServices;


Y_UNIT_TEST_SUITE(StoreAudio) {

    Y_UNIT_TEST(Channels) {
        for (const auto& [mime, channels] : TVector<std::pair<TString, int>>{
            {"audio/mpeg3", 1},
            {"audio/x-midi", 1},
            {"music/crescendo", 1},
            {"application/x-mathcad", 1},
            {"video/avi;channels=2", 2},
            {"video/avs-video;channels=4;encoding=zlib", 4},
            {"megaformat;channels=1024;something=else;year=2020", 1024},
        }) {
            UNIT_ASSERT_VALUES_EQUAL(ChannelsFromMime(mime), channels);
        }
    }

    Y_UNIT_TEST(MdsFilenameNonSpotter) {
        NAliceProtocol::TRequestContext req;
        req.MutableAudioOptions()->SetFormat("video/x-mpeg");
        req.MutableHeader()->SetSessionId("sessionId");
        req.MutableHeader()->SetMessageId("messageId");
        req.MutableHeader()->SetStreamId(1337);

        UNIT_ASSERT_VALUES_EQUAL(ConstructMdsFilename(req, /* isSpotter = */ false), "sessionId_messageId_1337.ogg");
    }

    Y_UNIT_TEST(MdsFilenameSpotter) {
        NAliceProtocol::TRequestContext req;
        req.MutableAudioOptions()->SetFormat("audio/opus");
        req.MutableHeader()->SetSessionId("sessionId");
        req.MutableHeader()->SetMessageId("messageId");
        req.MutableHeader()->SetStreamId(13);

        UNIT_ASSERT_VALUES_EQUAL(ConstructMdsFilename(req, /* isSpotter = */ true), "sessionId_messageId_13_spotter.opus");
    }

}

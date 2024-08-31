#include "music_what_is_playing.h"
#include "vins_scene.h"

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/hollywood/library/scenarios/music_what_is_playing/proto/music_what_is_playing.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NMusicWhatIsPlaying {

Y_UNIT_TEST_SUITE(MusicWhatIsPlayingDispatch) {

    Y_UNIT_TEST(MusicWhatIsPlayingDispatch) {
        TTestEnvironment testData(NProductScenarios::MUSIC_WHAT_IS_PLAYING, "ru-ru");
        UNIT_ASSERT(testData >> TTestDispatch(&TMusicWhatIsPlayingScenario::Dispatch) >> testData);

        UNIT_ASSERT(testData.GetSelectedSceneName() == TMusicWhatIsPlayingVinsScene::SceneName);
        const google::protobuf::Any& args = testData.GetSelectedSceneArguments();
        UNIT_ASSERT(args.Is<TMusicWhatIsPlayingVinsSceneArgs>());
    }
}

} // namespace NAlice::NHollywoodFw::NMusicWhatIsPlaying

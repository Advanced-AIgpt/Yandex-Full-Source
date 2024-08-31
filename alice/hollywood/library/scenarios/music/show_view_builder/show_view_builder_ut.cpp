#include "show_view_builder.h"

#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/music/player.pb.h>

#include <apphost/lib/service_testing/service_testing.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

Y_UNIT_TEST_SUITE(ShowViewBuilderTest) {

Y_UNIT_TEST(RepeatTypeTest) {
    THashMap<ERepeatType, NData::ERepeatMode> typeToMode = {
        {ERepeatType::RepeatNone, NData::ERepeatMode::NONE},
        {ERepeatType::RepeatTrack, NData::ERepeatMode::TRACK},
        {ERepeatType::RepeatAll, NData::ERepeatMode::ALL},
    };

    for (const auto& [type, mode] : typeToMode) {

        TScenarioState scState;
        TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
        auto cfg = CreateMusicConfig({});
        mq.SetConfig(cfg);
        TPlaybackContext playbackContext;

        TContentId id;
        id.SetType(TContentId_EContentType_Artist);
        id.SetId("123");

        *playbackContext.MutableContentId() = id;
        playbackContext.SetRepeatType(type);
        TQueueItem item;
        item.MutableTrackInfo()->SetAvailable(true);

        UNIT_ASSERT(mq.TryAddItem(std::move(item), /* hasMusicSubscription = */ true));
        mq.MoveTrackFromQueueToHistory();
        mq.InitPlaybackFromContext(playbackContext);

        NScenarios::TScenarioApplyRequest request;
        NAppHost::NService::TTestContext serviceCtx;
        TScenarioApplyRequestWrapper wrapper{request, serviceCtx};

        const auto divRenderData = TShowViewBuilder(TRTLogger::NullLogger(), mq, {}, &wrapper).BuildRenderData();
        const auto& data = divRenderData.GetScenarioData().GetMusicPlayerData();
        UNIT_ASSERT_EQUAL(data.GetRepeatMode(), mode);
    }
}

}

} // NAlice::NHollywood::NMusic

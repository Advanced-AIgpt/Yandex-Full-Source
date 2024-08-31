#include "easter_eggs.h"

namespace NBASS {
namespace NVideo {

void AddEasterEggOnStartPlayingVideo(TContext& ctx, TPlayVideoCommandDataScheme data) {
    const auto item = data.Item();
    const auto tvShowItem = data.TvShowItem();

    if (item.Type() == ToString(NVideo::EItemType::TvShowEpisode) &&
        item.ProviderName() == NVideoCommon::PROVIDER_AMEDIATEKA &&
        tvShowItem.ProviderItemId() == TStringBuf("mir-dikogo-zapada"))
    {
        ctx.AddAttention(TStringBuf("playing_westworld_tv_show"));
    }
}

} // namespace NVideo
} // namespace NBASS

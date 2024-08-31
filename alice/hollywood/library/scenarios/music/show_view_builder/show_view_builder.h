#pragma once

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/requests_helper/requests_helper.h>
#include <alice/hollywood/library/scenarios/music/scene/fm_radio/content_parser.h>
#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/music/content_id.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywood::NMusic {

void FillContentId(NData::NMusic::TContentId& contentId, const TContentId& musicQueueContentId);

struct TShowViewBuilderSources {
    const TAvatarColorsRequestHelper<ERequestPhase::After>* AvatarColors = nullptr;
    const TNeighboringTracksRequestHelper<ERequestPhase::After>* NeighboringTracks = nullptr;
    const TLikesTracksRequestHelper<ERequestPhase::After>* LikesTracks = nullptr;
    const TDislikesTracksRequestHelper<ERequestPhase::After>* DislikesTracks = nullptr;
    const NHollywoodFw::NMusic::NFmRadio::TFmRadioList* FmRadioList = nullptr;
};

class TShowViewBuilder {
public:
    TShowViewBuilder(TRTLogger& logger,
                     const TMusicQueueWrapper& mq,
                     const TShowViewBuilderSources sources,
                     const TScenarioApplyRequestWrapper* request = nullptr);

    NRenderer::TDivRenderData BuildRenderData() const;

private:
    TRTLogger* Logger_;
    const TMusicQueueWrapper& MusicQueue_;
    const TShowViewBuilderSources Sources_;
    const TScenarioApplyRequestWrapper* Request_;
};



} // NAlice::NHollywood::NMusic

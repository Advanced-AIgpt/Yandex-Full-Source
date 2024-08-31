#include "content_parser.h"
#include "album_parser.h"
#include "artist_parser.h"
#include "generative_parser.h"
#include "playlist_parser.h"
#include "radio_parser.h"
#include "track_parser.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>

#include <alice/library/json/json.h>

namespace NAlice::NHollywood::NMusic {

void ParseContent(TRTLogger& logger, NMetrics::ISensors& sensors, const TStringBuf contentResp,
                  TMusicQueueWrapper& mq, TMusicContext& mCtx, bool moveFromQueueToHistory) {
    auto json = JsonFromString(contentResp);
    const auto& result = json["result"];

    Y_ENSURE(!result.IsNull());

    const bool hasMusicSubscription = mCtx.GetAccountStatus().GetHasMusicSubscription();

    switch(mq.ContentId().GetType()) {
        case TContentId_EContentType_Track:
            ParseSingleTrack(result, mq, hasMusicSubscription);
            break;
        case TContentId_EContentType_Album:
            ParseAlbum(result, mq, mCtx);
            break;
        case TContentId_EContentType_Artist:
            ParseArtist(result, mq, mCtx);
            break;
        case TContentId_EContentType_Playlist:
            ParsePlaylist(result, mq, mCtx);
            break;
        case TContentId_EContentType_Radio:
            ParseRadio(logger, sensors, result, mq, hasMusicSubscription);
            break;
        case TContentId_EContentType_Generative:
            ParseGenerative(result, mq, hasMusicSubscription);
            break;
        case TContentId_EContentType_FmRadio:
            Y_UNREACHABLE(); // we parse fm radio in a scene
        case TContentId_EContentType_TContentId_EContentType_INT_MIN_SENTINEL_DO_NOT_USE_:
        case TContentId_EContentType_TContentId_EContentType_INT_MAX_SENTINEL_DO_NOT_USE_:
            //Make static analyzer happy
            break;
    }

    mq.ChangeState(moveFromQueueToHistory);
}

} // namespace NAlice::NHollywood::NMusic

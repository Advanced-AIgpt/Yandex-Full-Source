#include "yavideo_proxy_provider.h"

#include "defs.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/video_common/video.sc.h>

using namespace NVideoCommon;

namespace NBASS {
namespace NVideo {

IVideoClipsProvider::TPlayResult
TYaVideoProxyClipsProvider::GetPlayCommandDataImpl(TVideoItemConstScheme item,
        TPlayVideoCommandDataScheme commandData) const {

    if (item.HasPlayUri()) {
        LOG(DEBUG) << "Got proxy video request: " << item.PlayUri() << Endl;
        commandData->Uri() = item.PlayUri();
        return {};
    }

    TStringBuf err = "Could not play proxy video item without play_uri";
    LOG(ERR) << err << Endl;
    return TPlayError{EPlayError::VIDEOERROR, err};
}

} // namespace NVideo
} // namespace NBass

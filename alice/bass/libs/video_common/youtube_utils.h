#pragma once

#include "utils.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/source_request/source_request.h>

#include <util/system/env.h>

namespace NVideoCommon {

struct TYouTubeCredentials {
    TString GoogleAPIsKey;

    TYouTubeCredentials()
        : GoogleAPIsKey(GetEnv("GOOGLEAPIS_KEY"))
    {
        if (GoogleAPIsKey.empty()) {
            LOG(WARNING) << "YouTube credentials were not provided, can't use YouTube API" << Endl;
        }
    }
    void AddGoogleAPIsKey(TCgiParameters& cgis) const {
        cgis.ReplaceUnescaped(TStringBuf("key"), GoogleAPIsKey);
    }
};

const TYouTubeCredentials& GetYouTubeCredentials();

// More information about YouTube duration format is here:
// https://developers.google.com/youtube/v3/docs/videos
ui64 GetYouTubeVideoDuration(const NSc::TValue& rawDuration);
// Can parse only youtube /videos/ api response.
TMaybe<TVideoItem> TryParseYouTubeNode(const NSc::TValue& elem);

class TYouTubeContentRequestHandle : public IVideoItemHandle {
public:
    TYouTubeContentRequestHandle(const TSourceRequestFactory& source,
                                 NHttpFetcher::IMultiRequest::TRef multiRequest,
                                 TStringBuf id, bool enableYouTubeUserToken,
                                 TStringBuf authToken);
    TResult WaitAndParseResponse(TVideoItem& response) override;

private:
    NHttpFetcher::THandle::TRef Handle;
};

} // namespace NVideoCommon

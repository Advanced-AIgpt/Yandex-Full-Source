#pragma once

#include "change_track.h"
#include "defs.h"
#include "video_provider.h"

namespace NBASS {
namespace NVideo {

class TKinopoiskClipsProvider : public TVideoClipsHttpProviderBase {
public:
    explicit TKinopoiskClipsProvider(TContext& context)
        : TVideoClipsHttpProviderBase(context) {
    }

    TStringBuf GetProviderName() const override {
        return NVideoCommon::PROVIDER_KINOPOISK;
    }

    static TPlayResult ParseMasterPlaylistResponse(TStringBuf data, TPlayVideoCommandDataScheme commandData);

    static TVector<TSkippableFragment> GetSortedSkippableFragments(
        const NSc::TArray& skippableFragmentsArray);

public:
    // TVideoClipsHttpProviderBase overrides:
    std::unique_ptr<IWebSearchByProviderHandle> MakeWebSearchRequest(const TVideoClipsRequest& request) const override;

    std::unique_ptr<IVideoClipsHandle> MakeRecommendationsRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeNewVideosRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeTopVideosRequest(const TVideoClipsRequest& request) const override;
    std::unique_ptr<IVideoClipsHandle> MakeVideosByGenreRequest(const TVideoClipsRequest& request) const override;

    bool IsUnauthorized() const override {
        return false;
    }

    void FillSkippableFragmentsInfo(TVideoItemScheme item, const NSc::TValue& payload) const override;

    void FillAudioStreamsAndSubtitlesInfo(TVideoItemScheme item, const NSc::TValue& payload) const override;

protected:
    std::unique_ptr<IVideoItemHandle> DoMakeContentInfoRequest(TVideoItemConstScheme item,
                                                               NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                               bool /* forceUnfiltered */) const override;
    TResultValue DoGetSerialDescriptor(TVideoItemConstScheme tvShowItem,
                                       TSerialDescriptor* serialDescr) const override;

    std::unique_ptr<ISeasonDescriptorHandle>
    DoMakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr, const TSeasonDescriptor& seasonDescr,
                                  NHttpFetcher::IMultiRequest::TRef multiRequest) const override;

    TPlayResult GetPlayCommandDataImpl(TVideoItemConstScheme item,
                                       TPlayVideoCommandDataScheme commandData) const override;

private:
    enum class ERecommendationType {
        Any,
        New,
        Top,
    };

    std::unique_ptr<IVideoClipsHandle> Recommend(const TVideoClipsRequest& request, ERecommendationType type) const;
};

} // namespace NVideo
} // namespace NBASS

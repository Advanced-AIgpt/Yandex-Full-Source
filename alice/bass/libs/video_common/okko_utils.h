#pragma once

#include "alice/bass/libs/video_common/defs.h"
#include "alice/bass/libs/video_common/utils.h"

namespace NVideoCommon {

bool ParseOkkoItemFromUrl(TStringBuf url, TVideoItem& item);

class TOkkoContentInfoProvider : public IContentInfoProvider {
public:
    explicit TOkkoContentInfoProvider(std::unique_ptr<ISourceRequestFactory> source)
        : Source(std::move(source))
    {
    }

    // IContentInfoProvider overrides:
    std::unique_ptr<ISerialDescriptorHandle>
    MakeSerialDescriptorRequest(TVideoItemConstScheme /* tvShowItem */,
                                NHttpFetcher::IMultiRequest::TRef /* multiRequest */) override {
        return nullptr;
    }

    EPreferredSeasonDownloadMode GetPreferredSeasonDownloadMode() const override {
        return EPreferredSeasonDownloadMode::Individual;
    }

    std::unique_ptr<IAllSeasonsDescriptorHandle>
    MakeAllSeasonsDescriptorRequest(const TSerialDescriptor& /* serialDescr */,
                                    NHttpFetcher::IMultiRequest::TRef /* multiRequest */) override {
        return nullptr;
    }

    std::unique_ptr<ISeasonDescriptorHandle>
    MakeSeasonDescriptorRequest(const TSerialDescriptor& /* serialDescr */, const TSeasonDescriptor& /* seasonDescr */,
                                NHttpFetcher::IMultiRequest::TRef /* multiRequest */) override {
        return nullptr;
    }

protected:
    // IContentInfoProvider overrides:
    std::unique_ptr<IVideoItemHandle>
        MakeContentInfoRequestImpl(TLightVideoItemConstScheme /* item */,
                                   NHttpFetcher::IMultiRequest::TRef /* multiRequest */) override {
        return nullptr;
    }

private:
    std::unique_ptr<NVideoCommon::ISourceRequestFactory> Source;
};

} // namespace NVideoCommon

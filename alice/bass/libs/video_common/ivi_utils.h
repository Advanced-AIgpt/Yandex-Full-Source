#pragma once

#include "defs.h"
#include "utils.h"

#include <alice/bass/libs/fetcher/neh.h>

#include <util/generic/noncopyable.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

class TCgiParameters;

namespace NVideoCommon {

class TIviGenres;

bool ParseIviItemFromUrl(TStringBuf url, TVideoItem& item);
void ParseIviContentItem(TIviGenres& genres, const NSc::TValue& elem, TVideoItemScheme& item);

// This is a temporary semi-hardcoded solution for development.
// Proper values (for local run) are stored at wiki-page (with restricted access):
// https://wiki.yandex-team.ru/assistant/backend/private/
//
// TODO: obtain credentials from passport.
class TIviCredentials final : public NNonCopyable::TNonCopyable {
public:
    Y_DECLARE_SINGLETON_FRIEND()

    static const TIviCredentials& Instance();

    const TString& GetSession() const;
    void AddSession(TCgiParameters& cgis) const;

    const TString& GetApplicationName() const;

private:
    TIviCredentials();

private:
    TString Session;
    TString ApplicationName;
};

class TIviContentInfoProvider : public IContentInfoProvider {
public:
    TIviContentInfoProvider(std::unique_ptr<ISourceRequestFactory> source, TIviGenres& genres);

    // IContentInfoProvider overrides:
    std::unique_ptr<ISerialDescriptorHandle>
    MakeSerialDescriptorRequest(TVideoItemConstScheme tvShowItem,
                                NHttpFetcher::IMultiRequest::TRef multiRequest) override;

    EPreferredSeasonDownloadMode GetPreferredSeasonDownloadMode() const override {
        return EPreferredSeasonDownloadMode::All;
    }

    std::unique_ptr<IAllSeasonsDescriptorHandle>
    MakeAllSeasonsDescriptorRequest(const TSerialDescriptor& serialDescr,
                                    NHttpFetcher::IMultiRequest::TRef multiRequest) override;

    std::unique_ptr<ISeasonDescriptorHandle>
    MakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr, const TSeasonDescriptor& seasonDescr,
                                NHttpFetcher::IMultiRequest::TRef multiRequest) override;

    bool IsAuxInfoRequestFeasible(TLightVideoItemConstScheme item) override;
    std::unique_ptr<IVideoItemHandle> MakeAuxInfoRequest(TLightVideoItemConstScheme item,
                                                         NHttpFetcher::IMultiRequest::TRef multiRequest) override;

protected:
    // IContentInfoProvider overrides:
    std::unique_ptr<IVideoItemHandle>
    MakeContentInfoRequestImpl(TLightVideoItemConstScheme item,
                               NHttpFetcher::IMultiRequest::TRef multiRequest) override;

private:
    std::unique_ptr<ISourceRequestFactory> Source;
    TIviGenres& Genres;
};

} // namespace NVideoCommon

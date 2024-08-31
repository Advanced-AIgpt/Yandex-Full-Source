#pragma once

#include "defs.h"
#include "utils.h"

#include <util/generic/noncopyable.h>
#include <util/generic/singleton.h>
#include <util/generic/strbuf.h>

class TCgiParameters;

namespace NVideoCommon {

bool ParseAmediatekaItemFromUrl(TStringBuf url, TVideoItem& item);
bool ParseAmediatekaAgeRestriction(TStringBuf ageString, ui16& age);
void ParseAmediatekaContentItem(const NSc::TValue& elem, TVideoItemScheme& item);

// This is a temporary semi-hardcoded solution for development.
// Proper values (for local run) are stored at wiki-page (with
// restricted access):
// https://wiki.yandex-team.ru/assistant/backend/private/
//
// TODO: obtain credentials from passport.
class TAmediatekaCredentials final : public NNonCopyable::TNonCopyable {
public:
    Y_DECLARE_SINGLETON_FRIEND();

    static const TAmediatekaCredentials& Instance();

    const TString& GetApplicationName() const;
    const TString& GetAccessToken() const;
    void AddClientParams(TCgiParameters& cgis) const;

private:
    TAmediatekaCredentials();

    TString ApplicationName;
    TString ClientId;
    TString ClientSecret;

    TString AccessToken;
};

class TAmediatekaContentInfoProvider : public IContentInfoProvider {
public:
    TAmediatekaContentInfoProvider(std::unique_ptr<ISourceRequestFactory> source, bool enableShowingItemsComingSoon);

    // IContentInfoProvider overrides:
    std::unique_ptr<ISerialDescriptorHandle>
    MakeSerialDescriptorRequest(TVideoItemConstScheme tvShowItem,
                                NHttpFetcher::IMultiRequest::TRef multiRequest) override;

    EPreferredSeasonDownloadMode GetPreferredSeasonDownloadMode() const override {
        return EPreferredSeasonDownloadMode::Individual;
    }

    std::unique_ptr<ISeasonDescriptorHandle>
    MakeSeasonDescriptorRequest(const TSerialDescriptor& serialDescr, const TSeasonDescriptor& seasonDescr,
                                NHttpFetcher::IMultiRequest::TRef multiRequest) override;

protected:
    // IContentInfoProvider overrides:
    std::unique_ptr<IVideoItemHandle>
    MakeContentInfoRequestImpl(TLightVideoItemConstScheme item,
                               NHttpFetcher::IMultiRequest::TRef multiRequest) override;

private:
    std::unique_ptr<NVideoCommon::ISourceRequestFactory> Source;
    bool EnableShowingItemsComingSoon;
};

} // namespace NVideoCommon

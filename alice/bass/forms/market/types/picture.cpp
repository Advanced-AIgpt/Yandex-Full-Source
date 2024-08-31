#include "picture.h"

#include <util/string/builder.h>

namespace NBASS {

namespace NMarket {

TPicture::TPicture()
    : Url("http://yastatic.net/market-export/_/i/desktop/big-box.png")
    , Height(475)
    , Width(494)
    , ContainerHeight(475)
    , ContainerWidth(494)
    , OriginalRatio(1.0 * Width / Height)
{
}

TPicture::TPicture(const NSc::TValue& data)
    : Url(TStringBuilder() << TStringBuf("http:") << data["url"].GetString())
    , Height(data["height"].GetIntNumber())
    , Width(data["width"].GetIntNumber())
    , ContainerHeight(data["containerHeight"].GetIntNumber())
    , ContainerWidth(data["containerWidth"].GetIntNumber())
    , OriginalRatio(Height ? 1.0 * Width / Height : 1.0)
{
}

TPicture TPicture::GetMostSuitablePicture(const NSc::TValue& data, const TCgiGlFilters& glFilters)
{
    TPicture result;

    const TMaybe<NSc::TValue> picture = GetPictureByGlFilters(glFilters, data["pictures"].GetArray());
    if (!picture) {
        LOG(ERR) << "Pictures were not found in model " << data["id"].GetIntNumber() << Endl;
        return result;
    }

    TPicture originalPicture;
    if (!picture->Has(TStringBuf("original"))) {
        LOG(ERR) << "Original picture was not found in model " << data["id"].GetIntNumber() << Endl;
        originalPicture.SetOriginalRatio(1.0);
    } else {
        originalPicture = TPicture(picture->Get("original"));
    }

    const TMaybe<NSc::TValue> thumb = GetMostSuitableThumb(picture->Get("thumbnails").GetArray());
    if (thumb) {
        result = TPicture(*thumb);
        result.SetOriginalRatio(originalPicture.GetOriginalRatio());
    } else {
        result = originalPicture;
    }
    return result;
}

size_t TPicture::GetFittingFiltersCount(const TCgiGlFilters& glFilters, const NSc::TValue& picture)
{
    size_t fittingFilters = 0;
    for (const auto& kv : picture["filtersMatching"].GetDict()) {
        if (glFilters.contains(kv.first)) {
            // Судя по коду фронта, учитываются только первые значения фильтра. Остальные игнорируются.
            // https://github.yandex-team.ru/market/MarketNode/blob/756612db6044fc699d574764965333c5cc866e48/app/node_modules/helpers/filters-matching.js#L72-L114
            const auto pictureFilterValue = kv.second[0].GetString();
            const auto& allowableVals = glFilters.at(kv.first);
            if (pictureFilterValue && allowableVals && allowableVals[0] == pictureFilterValue) {
                fittingFilters++;
            }
        }
    }
    return fittingFilters;
}

TMaybe<NSc::TValue> TPicture::GetPictureByGlFilters(const TCgiGlFilters& glFilters, const NSc::TArray& pictures)
{
    if (pictures.empty()) {
        return Nothing();
    }
    if (glFilters.empty()) {
        return pictures[0];
    }

    size_t maxFittingFilters = GetFittingFiltersCount(glFilters, pictures[0]);
    size_t bestMatchIdx = 0;
    for (size_t i = 1; i < pictures.size(); i++) {
        size_t fittingFilters = GetFittingFiltersCount(glFilters, pictures[i]);
        if (fittingFilters > maxFittingFilters) {
            bestMatchIdx = i;
            if (fittingFilters == glFilters.size()) {
                break;
            }
            maxFittingFilters = fittingFilters;
        }
    }
    return pictures[bestMatchIdx];
}

TMaybe<NSc::TValue> TPicture::GetMostSuitableThumb(const NSc::TArray& thumbs)
{
    for (auto it = thumbs.rbegin(); it != thumbs.rend(); it++) {
        const auto& thumb = *it;
        if (thumb["height"].GetIntNumber() == thumb["width"].GetIntNumber()
            || thumb["containerHeight"].GetIntNumber() == thumb["containerWidth"].GetIntNumber())
        {
            return thumb;
        }
    }
    return Nothing();
}

} // namespace NMarket

} // namespace NBASS

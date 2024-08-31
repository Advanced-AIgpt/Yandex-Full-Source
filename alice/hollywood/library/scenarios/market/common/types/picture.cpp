#include "picture.h"

#include <util/string/builder.h>

namespace NAlice::NHollywood::NMarket {

TPicture::TPicture()
    : Url("http://yastatic.net/market-export/_/i/desktop/big-box.png")
    , Height(475)
    , Width(494)
    , ContainerHeight(475)
    , ContainerWidth(494)
    , OriginalRatio(1.0 * Width / Height)
{
}

TPicture::TPicture(const NJson::TJsonValue& data)
    : Url(TStringBuilder() << TStringBuf("http:") << data["url"].GetString())
    , Height(data["height"].GetUInteger())
    , Width(data["width"].GetUInteger())
    , ContainerHeight(data["containerHeight"].GetUInteger())
    , ContainerWidth(data["containerWidth"].GetUInteger())
    , OriginalRatio(Height ? 1.0 * Width / Height : 1.0)
{
}

NJson::TJsonValue TPicture::ToJson() const
{
    NJson::TJsonValue jsoned;
    jsoned["url"] = Url;
    jsoned["height"] = Height;
    jsoned["width"] = Width;
    jsoned["container_height"] = ContainerHeight;
    jsoned["container_width"] = ContainerWidth;
    jsoned["original_ratio"] = OriginalRatio;
    return jsoned;
}

TPicture TPicture::GetMostSuitablePicture(
    const NJson::TJsonValue::TArray& pictures,
    const TCgiGlFilters& glFilters)
{
    TPicture result;

    const auto picture = GetPictureByGlFilters(glFilters, pictures);
    if (!picture) {
        return result;
    }

    TPicture originalPicture;
    if (!picture->Has(TStringBuf("original"))) {
        originalPicture.SetOriginalRatio(1.0);
    } else {
        originalPicture = TPicture((*picture)["original"]);
    }

    const auto thumb = GetMostSuitableThumb((*picture)["thumbnails"].GetArray());
    if (thumb) {
        result = TPicture(*thumb);
        result.SetOriginalRatio(originalPicture.GetOriginalRatio());
    } else {
        result = originalPicture;
    }
    return result;
}

size_t TPicture::GetFittingFiltersCount(
    const TCgiGlFilters& glFilters,
    const NJson::TJsonValue& picture)
{
    size_t fittingFilters = 0;
    for (const auto& kv : picture["filtersMatching"].GetMap()) {
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

TMaybe<NJson::TJsonValue> TPicture::GetPictureByGlFilters(
    const TCgiGlFilters& glFilters,
    const NJson::TJsonValue::TArray& pictures)
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

TMaybe<NJson::TJsonValue> TPicture::GetMostSuitableThumb(const NJson::TJsonValue::TArray& thumbs)
{
    for (auto it = thumbs.rbegin(); it != thumbs.rend(); it++) {
        const auto& thumb = *it;
        if (thumb["height"].GetUInteger() == thumb["width"].GetUInteger()
            || thumb["containerHeight"].GetUInteger() == thumb["containerWidth"].GetUInteger())
        {
            return thumb;
        }
    }
    return Nothing();
}

} // namespace NAlice::NHollywood::NMarket

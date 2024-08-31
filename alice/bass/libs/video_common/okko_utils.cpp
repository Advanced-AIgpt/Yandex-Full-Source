#include "okko_utils.h"

#include <util/string/split.h>
#include <library/cpp/string_utils/url/url.h>

namespace NVideoCommon {

bool ParseOkkoItemFromUrl(TStringBuf url, TVideoItem& item) {
    TStringBuf path = GetPathAndQuery(url);

    TStringBuf urlType, hrid;
    Split(path, "/", urlType, hrid);
    if (hrid.empty())
        return false;

    EItemType type = EItemType::Null;

    if (urlType == TStringBuf("movie"))
        type = EItemType::Movie;
    else if (urlType == TStringBuf("serial") || urlType == TStringBuf("mp_movie"))
        type = EItemType::TvShow;
    else
        return false;

    item->HumanReadableId() = TString{hrid};
    item->Type() = ToString(type);
    item->ProviderName() = PROVIDER_OKKO;
    return true;
}

} // namespace NVideoCommon


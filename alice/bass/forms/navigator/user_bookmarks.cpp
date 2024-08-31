#include "user_bookmarks.h"

#include "bookmarks_matcher.h"
#include "show_on_map_intent.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/geo_resolver.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <ysite/yandex/reqanalysis/normalize.h>

#include <util/charset/unidata.h>

namespace NBASS {

namespace {

TString ClearString(const TString& str) {
    const TUtf16String strUtf16 = UTF8ToWide(str);
    TUtf16String result;
    result.reserve(strUtf16.length());

    for (auto wch: strUtf16) {
        if (IsAlphabetic(wch) || IsDigit(wch) || IsSpace(wch))
            result.append(wch);
    }

    return WideToUTF8(result);
}

} // namespace

// TUserBookmark ---------------------------------------------------------------
TUserBookmark::TUserBookmark(TConstScheme bookmark)
    : Latitude(bookmark.Lat())
    , Longitude(bookmark.Lon())
    , NameOriginal(bookmark.Name())
{
    NameNormalized = GetNormalizedName();
}

TUserBookmark::TUserBookmark(TStringBuf name)
    : NameOriginal(name)
{
    NameNormalized = GetNormalizedName();
}


TString TUserBookmark::GetNormalizedName() {
    static NQueryNorm::TSimpleNormalizer normalizer;
    try {
        if (!NameOriginal.empty()) {
            return ClearString(normalizer(NameOriginal));
        }
    } catch (...) {
        LOG(ERR) << "Exception while user bookmark name \""<< NameOriginal << "\" normalization: " << CurrentExceptionMessage() << Endl;
    }
    return NameOriginal;
}

NSc::TValue TUserBookmark::ToResolvedGeo(TContext& ctx) const {
    NSc::TValue value;
    NSc::TValue pos;
    pos["lat"] = Latitude;
    pos["lon"] = Longitude;
    value["location"] = pos;
    value["name"] = NameOriginal;

    TShowOnMapIntent navigatorIntent(ctx, TGeoPosition(Latitude, Longitude), ToString(NameOriginal));
    value["geo_uri"] = navigatorIntent.MakeIntentString();

    return value;
}

// TUserBookmarksHelper --------------------------------------------------------
TUserBookmarksHelper::TUserBookmarksHelper(const NBASS::TContext& context) {
    for (const auto& address : context.Meta().DeviceState().NavigatorState().UserAddresses()) {
        Bookmarks.emplace_back(TUserBookmark(address));
    }
}

TUserBookmarksHelper::TUserBookmarksHelper(const TVector<TStringBuf>& bookmarks) {
    for (const auto& name : bookmarks) {
        Bookmarks.emplace_back(TUserBookmark(name));
    }
}

TMaybe<TUserBookmark> TUserBookmarksHelper::GetUserBookmark(TStringBuf searchText, float threshold) {
    TBookmarksMatcher::TScore bestScore;
    TMaybe<TUserBookmark> bestAddress;

    TBookmarksMatcher forwardMatcher(searchText);
    LOG(DEBUG) << "Looking for <" << searchText << "> in user bookmarks" << Endl;
    for (auto& address : Bookmarks) {
        TBookmarksMatcher::TScore currentScore;
        const TString& bookmark = address.NameNorm();

        currentScore.ForwardScore = forwardMatcher.Match(bookmark, threshold);
        if (currentScore.ForwardScore < bestScore.ForwardScore) {
            continue;
        }

        TBookmarksMatcher backwardMatcher(bookmark);
        currentScore.BackwardScore = backwardMatcher.Match(searchText, threshold);

        if (bestScore < currentScore && currentScore.ForwardScore != TBookmarksMatcher::INVALID_SCORE) {
            bestScore = currentScore;
            bestAddress = address;
        }
    }

    if (bestAddress) {
        LOG(DEBUG) << "Best match for <" << searchText << "> in user bookmarks is <" << bestAddress->Name() << ">; "
                   << "score: " << bestScore.ToString() << Endl;
    } else {
        LOG(DEBUG) << "Nothing matches <" << searchText << "> in user bookmarks" << Endl;
    }

    return bestAddress;
}

TMaybe<TUserBookmark> TUserBookmarksHelper::GetUserBookmark(TStringBuf searchText) {
    return GetUserBookmark(searchText, TBookmarksMatcher::DEFAULT_THRESHOLD);
}

namespace {

const TVector<TStringBuf> HOME = {TStringBuf("дом"), TStringBuf("домой"), TStringBuf("домик"), TStringBuf("дом родной")};
const TVector<TStringBuf> WORK = {TStringBuf("работа")};

} // anon namespace

TMaybe<TUserBookmark> TUserBookmarksHelper::GetSavedAddressBookmark(TSpecialLocation addressId, TStringBuf searchText) {
    LOG(DEBUG) << "Will try to find <" << addressId << "> user address in Navigator user bookmarks" << Endl;

    TMaybe<TUserBookmark> userFav = GetUserBookmark(searchText, 1.0 /* threshold */);
    if (userFav) {
        return userFav;
    }

    LOG(DEBUG) << "Will try to find <" << addressId << "> synonyms in Navigator user bookmarks" << Endl;
    TMaybe<TVector<TStringBuf>> synonyms;
    switch (addressId) {
        case TSpecialLocation::EType::HOME:
            synonyms = HOME;
            break;
        case TSpecialLocation::EType::WORK:
            synonyms = WORK;
            break;
        default:
            LOG(DEBUG) << "Address id <" << addressId << "> is not supported by Navigator user bookmarks search" << Endl;
            return userFav;
    }

    for (const auto syn : *synonyms) {
        if (userFav = GetUserBookmark(syn, 1.0 /* threshold */)) {
            return userFav;
        }
    }

    return userFav;
}

} // namespace NBASS

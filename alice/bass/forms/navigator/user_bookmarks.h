#pragma once

#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/forms/special_location.h>

#include <alice/bass/libs/request/request.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

namespace NBASS {

class TUserBookmark {
public:
    using TConstScheme = NBASSRequest::TMeta<TSchemeTraits>::TDeviceState::TNavigatorState::TUserAddress::TConst;

    explicit TUserBookmark(TConstScheme bookmark);
    explicit TUserBookmark(TStringBuf name);

    double Lat() const {
        return Latitude;
    }

    double Lon() const {
        return Longitude;
    }

    const TString& NameNorm() const {
        return NameNormalized;
    }

    TString Name() const {
        return NameOriginal;
    }

    NSc::TValue ToResolvedGeo(TContext& ctx) const;

private:
    TString GetNormalizedName();

private:
    double Latitude;
    double Longitude;
    TString NameNormalized;
    TString NameOriginal;
};

class TUserBookmarksHelper {
public:
    explicit TUserBookmarksHelper(const TContext& context);
    explicit TUserBookmarksHelper(const TVector<TStringBuf>& bookmarks);
    const TVector<TUserBookmark>& GetBookmarks() {
        return Bookmarks;
    }

    TMaybe<TUserBookmark> GetUserBookmark(TStringBuf searchText);
    TMaybe<TUserBookmark> GetSavedAddressBookmark(TSpecialLocation addressId, TStringBuf searchText);

private:
    TMaybe<TUserBookmark> GetUserBookmark(TStringBuf searchText, float threshold);
private:
    TVector<TUserBookmark> Bookmarks;
};

}

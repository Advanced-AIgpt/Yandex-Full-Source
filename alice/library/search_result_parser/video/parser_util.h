#pragma once

#include <alice/protos/data/video/video.pb.h>
#include <alice/library/proto/proto_adapter.h>

#include <library/cpp/json/writer/json_value.h>

namespace NAlice::NHollywood::NVideo {

    static const TVector<TString> posterImageResolutions{"120x90", "400x300", "360x540", "1920x1080", "orig"};
    static const TVector<TString> thumbnailImageResolutions{"160x90", "720x360", "960x540", "1920x1080", "orig"};
    
    bool IsUsefulKpItem(const TOttVideoItem& video);
    bool IsUsefulOttItem(const TOttVideoItem& video);

    TMaybe<TAvatarMdsImage> BuildImage(const TString& url, const TVector<TString>& resolutions);

    TMaybe<TString> TryFindPersonKpId(const TString& s);
    TMaybe<TString> TryFindYoutubeUri(const TString& s);

    template <typename SomeValue>
    extern bool ValidPerson(const SomeValue& person);

    template <typename SomeValue>
    extern TMaybe<double> TryGetRating(const SomeValue& item);

    template <typename SomeValue>
    extern NProtoBuf::RepeatedPtrField<TOttVideoItem_TCinema> MakeCinemaData(const SomeValue& cinemaDataObj, const TStringBuf cinemaField = "cinemas", bool strict = true);

    namespace CinemaDataTools {
        static constexpr TStringBuf KINOPOISK_PACKAGE_NAME = "ru.kinopoisk.yandex.tv";

        template <typename SomeValue>
        extern TString GetPackageName(const SomeValue& cinema);

        bool IsKnownPackageName(const TStringBuf packageName);
    }
}

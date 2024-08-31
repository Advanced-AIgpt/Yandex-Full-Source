#include "parser_util.h"
#include "matcher_util.h"
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/map.h>
#include <util/generic/utility.h>
#include <util/system/rwlock.h>

#include <regex>

namespace NAlice::NHollywood::NVideo {

    namespace {

        bool IsUsefulPirateItem(const TOttVideoItem& video) {
            if (!video.HasMiscIds() || video.GetMiscIds().GetOntoId().Empty()) {
                return false;
            }
            auto isValidCinema = [](const TOttVideoItem_TCinema& cinema) {
                return !cinema.GetTvDeepLink().Empty() && !cinema.GetTvPackageName().Empty();
            };
            for (size_t i = 0; i < video.CinemasSize(); ++i) {
                if (isValidCinema(video.GetCinemas(i))) {
                    return true;
                }
            }
            return false;
        }

        static TRWMutex smutex;
    }

    bool IsUsefulKpItem(const TOttVideoItem& video) {
        if (!video.HasVhLicences()) {
            return false;
        }
        const auto& licenseContentType = video.GetVhLicences().GetContentType();
        if (licenseContentType == "TRAILER" || licenseContentType == "KP_TRAILER") {
            return false;
        }
        if (video.GetProviderItemId().Empty()) {
            return false;
        }
        return true;
    }

    bool IsUsefulOttItem(const TOttVideoItem& video) {
        if (!video.HasPoster()) {
            return false;
        }
        return IsUsefulKpItem(video) || IsUsefulPirateItem(video);
    }

    // TODO(kolchanovs): this 2 constants seems to be unused, why?
    static const TVector<TString> logoImageResolutions{"800x200", "orig"};
    static const TVector<TString> legalLogoImageResolutions{"544x306", "orig"};

    TMaybe<TAvatarMdsImage> BuildImage(const TString& url, const TVector<TString>& resolutions) {
        TMaybe<TString> maybeBaseUrl = GetMdsUrlInfo(url);
        if (!maybeBaseUrl.Defined()) {
            return Nothing();
        }

        TAvatarMdsImage result;
        result.SetBaseUrl(*maybeBaseUrl);
        for (const auto& size : resolutions) {
            result.AddSizes(size);
        }
        return result;
    }

    TMaybe<TString> TryFindPersonKpId(const TString& s) {
        std::regex PERSON_KP_ID_REGEX("name\\/([0-9]+)");
        std::smatch match{};
        std::regex_search(s.Data(), match, PERSON_KP_ID_REGEX);
        if (!match[1].str().empty()) {
            return match[1].str();
        }
        return Nothing();
    }

    TMaybe<TString> TryFindYoutubeUri(const TString& s) {
        std::regex YOUTUBE_VIDEO_ID_REGEX(R"(https?:\/\/www\.youtube\.com\/watch\?v=([\w\-]+))");
        std::smatch match{};
        std::regex_search(s.Data(), match, YOUTUBE_VIDEO_ID_REGEX);
        if (!match[1].str().empty()) {
            return match[1].str();
        }

        return Nothing();
    }

    template <typename SomeValue>
    bool ValidPerson(const SomeValue& person) {
        return BuildImage(person["image"]["original"].GetString(), posterImageResolutions).Defined() && person["ids"].Has("kinopoisk");
    }

    template bool ValidPerson(const NJson::TJsonValue& person);
    template bool ValidPerson(const TProtoAdapter& person);

    template <typename SomeValue>
    TMaybe<double> TryGetRating(const SomeValue& item) {
        for (const auto& rating : item["rating"].GetArray()) {
            if (rating["type"].GetString() == "kinopoisk" && rating.Has("original_rating_value") && rating.Has("original_best_rating")) {
                return rating["original_rating_value"].GetDouble() * (10 / rating["original_best_rating"].GetDouble());
            }
        }
        return Nothing();
    }

    template TMaybe<double> TryGetRating(const NJson::TJsonValue& jsonItem);
    template TMaybe<double> TryGetRating(const TProtoAdapter& protoItem);

    template <typename SomeValue>
    NProtoBuf::RepeatedPtrField<TOttVideoItem_TCinema> MakeCinemaData(const SomeValue& cinemaDataObj, const TStringBuf cinemaField, bool strict) {
        if (!cinemaDataObj.Has(cinemaField)) {
            return {};
        }

        NProtoBuf::RepeatedPtrField<TOttVideoItem_TCinema> result;

        auto DeepLinkIsBad = [&strict](const auto& cinema) {
            return strict && cinema["tv_deeplink"].GetString().Empty();
        };

        auto PackageNameIsBad = [](const auto& cinema) {
            auto pn = CinemaDataTools::GetPackageName(cinema);
            return !CinemaDataTools::IsKnownPackageName(pn) || pn == CinemaDataTools::KINOPOISK_PACKAGE_NAME;
        };

        for (const auto& cinema : cinemaDataObj[cinemaField].GetArray()) {
            if (DeepLinkIsBad(cinema) || PackageNameIsBad(cinema)) {
                continue;
            }

            TOttVideoItem_TCinema cinemaItem;

            // fill variants in special order
            {
                TMap<TString, TOttVideoItem_TCinema_TVariant> variantMap;
                for (const auto& variant : cinema["variants"].GetArray()) {
                    TOttVideoItem_TCinema_TVariant variantItem;
                    variantItem.SetEmbedUrl(variant["embed_url"].GetString());
                    variantItem.SetPrice(variant["price"].GetUInteger());
                    // TODO (fill currency)
                    variantItem.SetType(variant["type"].GetString());
                    variantItem.SetQuality(variant["quality"].GetString());
                    variantMap.insert({variant["type"].GetString(), std::move(variantItem)});
                }
                for (auto key : {"tvod", "est", "avod", "svod", "fvod"}) {
                    if (variantMap.contains(key)) {
                        *cinemaItem.AddVariants() = variantMap[key];
                    }
                }
            }
            // TODO(kolchanovs): is it correct?
            auto setKeyArt = [&cinemaItem](const SomeValue obj, const auto field) {
                const auto maybeUrl = GetMdsUrlInfo(obj[field].GetString());
                if (maybeUrl.Defined()) {
                    cinemaItem.MutableKeyArt()->SetBaseUrl(*maybeUrl);
                }
            };
            if (cinema.Has("keyart")) {
                setKeyArt(cinema, "keyart");
            } else if (cinemaDataObj.Has("keyart")) {
                setKeyArt(cinemaDataObj, "keyart");
            }
            if (auto image = BuildImage(cinema["favicon"].GetString(), {})) {
                cinemaItem.SetFavicon(FixSchema(image->GetBaseUrl()));
            }
            cinemaItem.SetCinemaName(cinema["cinema_name"].GetString());
            cinemaItem.SetHidePrice(cinema["hide_price"].GetBoolean());
            cinemaItem.SetCode(cinema["code"].GetString());
            cinemaItem.SetDuration(cinema["duration"].GetUInteger());
            cinemaItem.SetEmbedUrl(cinema["embed_url"].GetString());
            cinemaItem.SetLink(cinema["link"].GetString());
            cinemaItem.SetTvDeepLink(cinema["tv_deeplink"].GetString());
            cinemaItem.SetTvFallbackLink(cinema["tv_fallback_link"].GetString());

            cinemaItem.SetTvPackageName(cinema["tv_package_name"].GetString());
            result.Add(std::move(cinemaItem));
        }
        return result;
    }

    template NProtoBuf::RepeatedPtrField<TOttVideoItem_TCinema> MakeCinemaData(const NJson::TJsonValue& cinemaDataJson, const TStringBuf cinemaField, bool strict);
    template NProtoBuf::RepeatedPtrField<TOttVideoItem_TCinema> MakeCinemaData(const TProtoAdapter& cinemaDataProto, const TStringBuf cinemaField, bool strict);

    namespace CinemaDataTools {
        static const THashMap<TString, TString> knownPackageMapping = {
            {"okko", "ru.more.play"},
            {"kion", "ru.mts.mtstv"},
            {"1tv", "ru.tv1.android.tv"},
            {"ivi", "ru.ivi.client"},
            {"wink", "ru.rt.video.app.tv"},
            {"start", "ru.start.androidmobile"},
            {"more", "com.ctcmediagroup.videomore"},
            {"premier", "gpm.tnt_premier"},
            {"kp", TString(KINOPOISK_PACKAGE_NAME)}};

        template <typename SomeValue>
        TString GetPackageName(const SomeValue& cinema) {
            TString straightPN = cinema["tv_package_name"].GetString();
            if (!straightPN.Empty()) {
                return straightPN;
            }

            TString cinemaCode = cinema["code"].GetString();
            if (!cinemaCode.Empty()) {
                auto iter = knownPackageMapping.find(cinemaCode);
                if (iter != knownPackageMapping.end()) {
                    return iter->second;
                }
            }

            return TString{};
        };

        template TString GetPackageName(const NJson::TJsonValue& cinema);
        template TString GetPackageName(const TProtoAdapter& cinema);

        bool IsKnownPackageName(const TStringBuf packageName) {
            static THashSet<TString> knownPackageNames;
            if (knownPackageNames.empty()) {
                TWriteGuard lock(smutex);
                if (knownPackageNames.empty()) {
                    for (const auto& [code, name] : knownPackageMapping) {
                        Y_UNUSED(code);
                        knownPackageNames.insert(name);
                    }
                    // old Okko package name
                    knownPackageNames.insert("tv.okko.androidtv");
                }
            }

            return knownPackageNames.contains(packageName);
        }
    }

}

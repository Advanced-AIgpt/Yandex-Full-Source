#include "matcher_util.h"

#include <regex>

namespace NAlice::NHollywood::NVideo {

    static const std::regex GENERAL_AVATARS_MDS_REGEX("(https?:)?(//avatars.mds.yandex.net)(:80|:443)?(.+/)");

    TMaybe<TString> GetMdsUrlInfo(const TString& s) {
        std::smatch match{};

        if (std::regex_search(s.Data(), match, GENERAL_AVATARS_MDS_REGEX)) {
            return "https://avatars.mds.yandex.net" + match[4].str();
        }
        return Nothing();
    }

    TMaybe<TString> FindEmbedUrl(const TString& link) {
        std::regex VIDEO_SRC_REGEX("src=\"([^\"]+)\"");
        std::smatch match{};
        std::regex_search(link.Data(), match, VIDEO_SRC_REGEX);
        if (!match[1].str().empty()) {
            return match[1].str();
        }
        return Nothing();
    }

    bool HasNetloc(const TString& url) {
        // dandex@ logic from python contrib furl: https://nda.ya.ru/t/SzoXVxMT4r593m
        size_t searchEnd = url.Size();
        for (const auto searchChar : {'#', '?', '/', ':'}) {
            if (auto newend = url.find(searchChar); newend != TString::npos) {
                searchEnd = newend;
            }
        }

        if (searchEnd > 0) {
            const auto scheme = url.substr(0, searchEnd);
            std::regex VALIDATION_REGEX("[a-zA-Z][a-zA-Z\\-\\.\\+]*");
            std::smatch match{};
            std::regex_match(scheme.Data(), match, VALIDATION_REGEX);
            if (!match.empty()) {
                return url.StartsWith(scheme + "://");
            }
        }
        return url.StartsWith("//");
    }

    TString FixSchema(const TString& url) {
        if (url && url.StartsWith("//")) {
            return "https:" + url;
        } else if (url && !HasNetloc(url)) {
            // something like 'avatars.ya.ru/xx/yy'
            return "https://" + url;
        }
        return url;
    }

} // namespace NAlice::NHollywood::NVideo

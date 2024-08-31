#include "search_hosts_features.h"

#include <alice/megamind/protos/scenarios/web_search_source.pb.h>

#include <alice/library/factors/dcg.h>
#include <alice/library/response_similarity/response_similarity.h>

#include <google/protobuf/struct.pb.h>

#include <kernel/alice/search_scenario_factors_info/factors_gen.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/string_utils/url/url.h>

#include <search/web/util/sources/sources.h>

#include <util/generic/hash_set.h>
#include <util/generic/string.h>

#include <cmath>

namespace NAlice {

namespace {

const THashSet<TStringBuf> MUSIC_HOSTS = {
    "5music.ru",
    "amdm.ru",
    "cool.dj",
    "drivemusic.me",
    "en.lyrsense.com",
    "genius.com",
    "ipleer.fm",
    "lsdmusic.ru",
    "luxmp3.net",
    "m.z1.fm",
    "megapesni.me",
    "mp3-muzyka.ru",
    "mp3-tut.mobi",
    "mp3party.net",
    "music.xn--41a.ws",
    "music.yandex.ru",
    "muzaza.ru",
    "muzlain.ru",
    "muzlo.me",
    "muzmo.ru",
    "muzofond.fm",
    "muzulya.ru",
    "mvclip.ru",
    "mychords.net",
    "myzcloud.me",
    "patefon.net",
    "qmusic.me",
    "sefon.me",
    "www.amalgama-lab.com",
    "www.gl5.ru",
    "x-minus.me",
    "zaycev.net",
    "zoop.su",
    "zvooq.online"
};

const THashSet<TStringBuf> VIDEO_HOSTS = {
    "cinema-24.tv",
    "ctc.ru",
    "detskie-multiki.ru",
    "FanSerials.mobi",
    "filmshd.club",
    "filmzor.net",
    "fryd.ru",
    "hd-club.su",
    "hdlava.me",
    "iceage-mult.ru",
    "kino-fs.ucoz.net",
    "kinobos.net",
    "kinohabr.net",
    "kinoserial.tv",
    "kinoshack.net",
    "kinotan.ru",
    "mega-mult.ru",
    "multsforkids.ru",
    "mvclip.ru",
    "onlinemultfilmy.ru",
    "rutube.ru",
    "seasonvar.ru",
    "smotret-multiki.net",
    "videomeg.ru",
    "vseseriipodriad.ru",
    "w6.zona.plus",
    "www.film.ru",
    "www.ivi.ru",
    "www.ivi.tv",
    "www.kino-teatr.ru",
    "www.kinopoisk.ru",
    "www.moscatalogue.net"
};

using namespace NAliceSearchScenario;

const THashMap<TStringBuf, std::array<EFactorId, 2>> HOST_TO_FACTOR = {
    {TStringBuf("www.kinopoisk.ru"),   {FI_KINOPOISK_DCG, FI_KINOPOISK_DCG5}},
    {TStringBuf("www.ivi.ru"),         {FI_IVI_DCG, FI_IVI_DCG5}},
    {TStringBuf("rutube.ru"),          {FI_RUTUBE_DCG, FI_RUTUBE_DCG5}},
    {TStringBuf("zvuch.com"),          {FI_ZVOOQ_DCG, FI_ZVOOQ_DCG5}},
    {TStringBuf("zaycev.net"),         {FI_ZAYCEV_DCG, FI_ZAYCEV_DCG5}},
    {TStringBuf("www.youtube.com"),    {FI_YOUTUBE_DCG, FI_YOUTUBE_DCG5}},
    {TStringBuf("ru.wikipedia.org"),   {FI_WIKIPEDIA_DCG, FI_WIKIPEDIA_DCG5}},
    {TStringBuf("tv.yandex.ru"),       {FI_TV_HOST_DCG, FI_TV_HOST_DCG5}},
};

const THashMap<TStringBuf, EFactorId> SNIPPET_TO_FACTOR = {
    {"companies", FI_COMPAMIES_SNIPPET_POS_DCG},
    {"entity_search", FI_ENTITY_SNIPPET_POS_DCG},
    {"musicplayer", FI_MUSIC_SNIPPET_POS_DCG},
};

void SetSnippetsFactors(
    const google::protobuf::Struct& snippet,
    const float dcg,
    const TFactorView view
) {
    const auto it = snippet.fields().find("type");
    if (it != snippet.fields().end()) {
        if (const auto* idx = SNIPPET_TO_FACTOR.FindPtr(it->second.string_value())) {
            view[*idx] = dcg;
        }
    }
}

} // namespace

void ProcessWebResult(
    const NSc::TValue& doc,
    const ui32 position,
    const TStringBuf utterance,
    THostsPositions& hostsPositions,
    TVector<NResponseSimilarity::TSimilarity>& titleSimilarities,
    TVector<NResponseSimilarity::TSimilarity>& headlineSimilarities
) {
    if (!IsWebSource(doc["ServerDescr"].GetString())) {
        return;
    }

    const NSc::TValue& archiveInfo = doc["ArchiveInfo"];

    const TStringBuf url = archiveInfo["Url"].GetString();
    const TStringBuf host = GetHost(CutSchemePrefix(url));
    if (MUSIC_HOSTS.contains(host)) {
        hostsPositions.Music.emplace_back(position);
    }
    if (VIDEO_HOSTS.contains(host)) {
        hostsPositions.Video.emplace_back(position);
    }
    if (HOST_TO_FACTOR.FindPtr(host)) {
        hostsPositions.PerHost[host].emplace_back(position);
    }

    ELanguage lang = ELanguage::LANG_RUS;
    for (const NSc::TValue& gta : archiveInfo["GtaRelatedAttribute"].GetArray()) {
        if (gta["Key"].GetString() == TStringBuf("lang")) {
            lang = LanguageByName(gta["Value"].GetString());
            break;
        }
    }
    if (archiveInfo.Has("Title")) {
        titleSimilarities.emplace_back(NResponseSimilarity::CalculateResponseItemSimilarity(
            utterance, archiveInfo["Title"].GetString(), lang
        ));
    }
    if (archiveInfo.Has("Headline")) {
        headlineSimilarities.emplace_back(NResponseSimilarity::CalculateResponseItemSimilarity(
            utterance, archiveInfo["Headline"].GetString(), lang
        ));
    }
}

void ProcessWizardResult(
    const NSc::TValue& doc,
    const float dcg,
    const TFactorView view
) {
    for (const NSc::TValue& gta : doc["ArchiveInfo"]["GtaRelatedAttribute"].GetArray()) {
        if (gta["Key"].GetString() == TStringBuf("_Markers")
            && gta["Value"].GetString().StartsWith(TStringBuf("Slices=")))
        {
            TStringBuf wizardName = gta["Value"].GetString();
            wizardName.SkipPrefix(TStringBuf("Slices="));

            const auto fillFactor = [&](const TStringBuf name, const EFactorId idx) noexcept {
                if (wizardName.StartsWith(name)) {
                    view[idx] = dcg;
                }
            };

            fillFactor(TStringBuf("WIZMUSIC"), FI_MUSIC_WIZARD_POS_DCG);
            fillFactor(TStringBuf("WIZLYRICS"), FI_LYRICS_WIZARD_POS_DCG);
            fillFactor(TStringBuf("VIDEOWIZ"), FI_VIDEO_WIZARD_POS_DCG);
            fillFactor(TStringBuf("WIZIMAGES"), FI_IMAGE_WIZARD_POS_DCG);

            break;
        }
    }
}

void ProcessDocWebResult(
    const NScenarios::TWebSearchDoc& doc,
    const ui32 position,
    const TStringBuf utterance,
    THostsPositions& hostsPositions,
    TVector<NResponseSimilarity::TSimilarity>& titleSimilarities,
    TVector<NResponseSimilarity::TSimilarity>& headlineSimilarities
) {
    if (!IsWebSource(doc.GetServerDescr())) {
        return;
    }

    const TStringBuf url = doc.GetUrl();
    const TStringBuf host = GetHost(CutSchemePrefix(url));
    if (MUSIC_HOSTS.contains(host)) {
        hostsPositions.Music.emplace_back(position);
    }
    if (VIDEO_HOSTS.contains(host)) {
        hostsPositions.Video.emplace_back(position);
    }
    if (HOST_TO_FACTOR.FindPtr(host)) {
        hostsPositions.PerHost[host].emplace_back(position);
    }

    ELanguage lang = ELanguage::LANG_RUS;
    if (const TStringBuf langStr = doc.GetLang(); !langStr.Empty()) {
        lang = LanguageByName(langStr);
    }

    if (const TStringBuf title = doc.GetDoctitle(); !title.Empty()) {
        titleSimilarities.emplace_back(NResponseSimilarity::CalculateResponseItemSimilarity(
            utterance, title, lang
        ));
    }

    if (const TStringBuf headline = doc.GetHeadline(); !headline.Empty()) {
        headlineSimilarities.emplace_back(NResponseSimilarity::CalculateResponseItemSimilarity(
            utterance, headline, lang
        ));
    }
}

void ProcessDocWizardResult(
    const NScenarios::TWebSearchDoc& doc,
    const float dcg,
    const TFactorView view
) {
    const auto& slices = doc.GetMarkers().GetSlices();
    const auto fillFactor = [&view, &slices, dcg](const TStringBuf name, const EFactorId idx) noexcept {
        if (slices.StartsWith(name)) {
            view[idx] = dcg;
        }
    };

    fillFactor(TStringBuf("WIZMUSIC"), FI_MUSIC_WIZARD_POS_DCG);
    fillFactor(TStringBuf("WIZLYRICS"), FI_LYRICS_WIZARD_POS_DCG);
    fillFactor(TStringBuf("VIDEOWIZ"), FI_VIDEO_WIZARD_POS_DCG);
    fillFactor(TStringBuf("WIZIMAGES"), FI_IMAGE_WIZARD_POS_DCG);
    fillFactor(TStringBuf("WIZTRANSLATE"), FI_TRANSLATE_WIZARD_POS_DCG);
    fillFactor(TStringBuf("WIZWEATHER"), FI_WEATHER_WIZARD_POS_DCG);
    fillFactor(TStringBuf("UNITS_CONVERTER"), FI_CONVERTER_WIZARD_POS_DCG);
    fillFactor(TStringBuf("WIZMAPS"), FI_MAPS_WIZARD_POS_DCG);
    fillFactor(TStringBuf("WIZROUTES"), FI_ROUTES_WIZARD_POS_DCG);
    fillFactor(TStringBuf("NEWS_WIZARD"), FI_NEWS_WIZARD_POS_DCG);
    fillFactor(TStringBuf("WIZMARKET"), FI_MARKET_WIZARD_POS_DCG);
    fillFactor(TStringBuf("GEOV"), FI_GEOV_WIZARD_POS_DCG);
}

void FillHostDCG(
    const THostsPositions& hostsPositions,
    const TFactorView view
) {
    for (auto&& [host, positions] : hostsPositions.PerHost) {
        if (const auto* factorIds = HOST_TO_FACTOR.FindPtr(host)) {
            view[(*factorIds)[0]] = CalcDCGAt(positions, positions.size());
            view[(*factorIds)[1]] = CalcDCGAt(positions, 5);
        }
    }
}

void ProcessDocSnippetResult(
    const NScenarios::TWebSearchDoc& doc,
    const float dcg,
    const TFactorView view
) {
    SetSnippetsFactors(doc.GetSnippets().GetFull(), dcg, view);
}

} // namespace NAlice

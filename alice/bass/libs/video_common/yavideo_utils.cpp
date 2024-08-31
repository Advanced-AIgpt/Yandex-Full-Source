#include "yavideo_utils.h"

#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/video_common/video.sc.h>
#include <alice/bass/libs/video_common/defs.h>

#include <alice/library/video_common/defs.h>

#include <library/cpp/string_utils/quote/quote.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <util/string/builder.h>

#include <regex>

using namespace NVideoCommon;

namespace {
constexpr TStringBuf HOST_YOUTUBE = "www.youtube.com";

TStringBuf GetHostingId(TStringBuf url) {
    size_t pos = url.rfind('/');
    if (pos == TString::npos) {
        return "";
    }
    ++pos;
    return url.substr(pos, url.find('?', pos));
}

} // namespace

namespace NBASS::NVideo {

TContentInfoHandle::TContentInfoHandle(TSimpleSharedPtr<TYaVideoContentGetterDelegate> contentGetterDelegate,
                                       TVideoItemConstScheme item,
                                       NHttpFetcher::IMultiRequest::TRef multiRequest)
    : ContentGetterDelegate(contentGetterDelegate) {
    NHttpFetcher::TRequestPtr req = ContentGetterDelegate->AttachProviderRequest(multiRequest);

    TCgiParameters cgi;
    TString requestText = "";
    if (!item->Entref().IsNull() && !item->Entref()->Empty()) {
        cgi.InsertUnescaped(TStringBuf("entref"), ToString(item->Entref()));
    } else {
        TStringBuilder text;
        text << "url:\"";
        if (item.ProviderName() == PROVIDER_YOUTUBE) {
            text << MakeWebYoutubeUrl(item.ProviderItemId());
        } else if (item.HasProviderItemId()) { //for items from other video providers: web url and provider_item_id are equal
            text << item.ProviderItemId();
        } else {
            ythrow yexception() << TError(TError::EType::VIDEOERROR, TStringBuf("Empty video item url"));
        }
        text << '"';
        cgi.InsertUnescaped(TStringBuf("assistant"), TStringBuf("1"));
        cgi.InsertUnescaped(TStringBuf("g"), TStringBuilder() << TStringBuf("1.dg.1.1.-1"));
        cgi.InsertUnescaped(TStringBuf("text"), text);
        cgi.InsertUnescaped(TStringBuf("noredirect"), TStringBuf("1"));
        requestText = ToString(text);
    }

    contentGetterDelegate->FillCgis(cgi);
    req->AddCgiParams(cgi);
    DebugRequestText = requestText;
    DebugUrl = ToString(req->Url());
    Handle = req->Fetch();
}

NVideoCommon::TResult TContentInfoHandle::WaitAndParseResponse(TVideoItem& item) {
    if (!Handle) {
        TString err = "Empty request handle";
        return NVideoCommon::TError(err);
        ContentGetterDelegate->FillRequestForAnalyticsInfo(DebugUrl, DebugRequestText, 0, false);
    }
    NHttpFetcher::TResponse::TRef resp = Handle->Wait();
    if (resp->IsError()) {
        TString err = TStringBuilder() << "yavideo response: " << resp->GetErrorText();
        bool success = resp->Result == NHttpFetcher::TResponse::EResult::HttpError;
        ContentGetterDelegate->FillRequestForAnalyticsInfo(DebugUrl, DebugRequestText, success ? resp->Code : 0, success);
        return NVideoCommon::TError(err);
    }

    ContentGetterDelegate->FillRequestForAnalyticsInfo(DebugUrl, DebugRequestText, resp->Code, true);
    TVideoGallery results;
    NSc::TValue response;
    if (!NSc::TValue::FromJson(response, resp->Data)) {
        TStringBuf err = "Can not convert JSON to NSc::TValue.";
        return NVideoCommon::TError(err);
    }
    if (ContentGetterDelegate->IsTvDevice()) {
        item->Legal() = response.TrySelect("entity/base_info/legal").ToJsonSafe();
    }
    if (const auto error = ParseJsonResponse(response, *ContentGetterDelegate, &results.Scheme(),
                                             NAlice::NVideoCommon::VIDEO_SOURCE_YAVIDEO))
    {
        return error;
    }

    if (results->Items().Empty()) {
        TStringBuf err = "No valid items in yavideo response";
        if (item->HasEntref() || item->HasProviderItemId()) {
            Y_STATS_INC_INTEGER_COUNTER("video_id_not_found_in_search");
        }
        return NVideoCommon::TError{err};
    }

    item.Scheme() = results->Items(0);
    return {};
}

//------------------------------------------------------------------------------

TString GetSandboxVersion(const TYaVideoContentGetterDelegate& contentGetterDelegate) {
    if (contentGetterDelegate.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_ENABLE_ALL_HOSTS)) {
        return "0xabb2018a66";
    }
    if (contentGetterDelegate.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_ENABLE_TELEMETRY)) {
        return "0x0f4f07d0b4";
    }
    return "0x0fd98704a3";
}

TString BuildPlayerUri(TStringBuf playerCode, TStringBuf playerId,
                       const TYaVideoContentGetterDelegate& contentGetterDelegate) {
    TString escapedPlayerCode = CGIEscapeRet(playerCode);
    SubstGlobal(escapedPlayerCode, "+", "%20");

    TString sandboxVersion = GetSandboxVersion(contentGetterDelegate);
    return TStringBuilder() << TStringBuf("https://yastatic.net/video-player/")
                            << sandboxVersion << TStringBuf("/pages-common/") << playerId
                            << '/' << playerId << ".html#html=" << escapedPlayerCode
                            << TStringBuf("&autoplay=yes&tv=1&clean=true");
}

TString BuildVideoUriForBrowser(TStringBuf url, TStringBuf title,
                                const TYaVideoContentGetterDelegate& contentGetterDelegate)
{
    if (contentGetterDelegate.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_YABRO_URL_BY_SEARCH))
    {
        return TStringBuilder() << TStringBuf("https://yandex.") << contentGetterDelegate.UserTld()
                                << TStringBuf("/video/search?")
                                << TStringBuf("text=") << CGIEscapeRet(url);
    }

    return TStringBuilder() << TStringBuf("https://yandex.") << contentGetterDelegate.UserTld()
                            << TStringBuf("/video/preview?")
                            << TStringBuf("text=") << CGIEscapeRet(title);
}

TString MakeWebYoutubeUrl(TStringBuf id) {
    return TStringBuilder() << "https://www.youtube.com/watch?v=" << id;
}

TString MakeYoutubeUrl(TStringBuf id, TYaVideoContentGetterDelegate& contentGetterDelegate) {
    // TODO: yulika@, vi002@: Do we need to do smth with devices, that do not support video at all?
    if (contentGetterDelegate.IsSmartSpeaker()) {
        return TStringBuilder() << "youtube://" << id;
    }
    return MakeWebYoutubeUrl(id);
}

TMaybe<TString> FindEmbedUrl(const TBasicStringBuf<char> &link) {
    std::regex VIDEO_SRC_REGEX("src=\"([^\"]+)\"");
    std::smatch match{};
    std::regex_search(link.Data(), match, VIDEO_SRC_REGEX);
    if (!match[1].str().empty()) {
        return match[1].str();
    }
    return Nothing();
}

using TYaVideoClipConst = NBassApi::TYaVideoClip<TSchemeTraits>::TConst;

bool ParseVideoItemJsonResponse(const NSc::TValue& clipJson, TYaVideoContentGetterDelegate& contentGetterDelegate,
                                TVideoItemScheme& item, TStringBuf sourceTag) {
    TYaVideoClipConst clip(&clipJson);
    if (!clip->HasThmbHref() || !clip->HasTitle()) {
        LOG(ERR) << "Clip has a lack of important fields: " << clip->ToJson() << Endl;
        return false;
    }
    if (!clip->HasUrl()) {
        LOG(ERR) << "Url is null for clip: " << clip->ToJson() << Endl;
        return false;
    }

    if (!clip->HasPlayerId()) {
        LOG(ERR) << "PlayerId is null for clip: " << clip->ToJson() << Endl;
        return false;
    }

    if (!clip->HasHtmlAutoplayVideoPlayer()) {
        LOG(ERR) << "HtmlAutoplayVideoPlayer is null for clip: " << clip->ToJson() << Endl;
        return false;
    }

    TStringBuf url(clip->Url());
    if (TMaybe<TString> EmbedUri = FindEmbedUrl(clip->HtmlAutoplayVideoPlayer())) {
        item.EmbedUri() = EmbedUri.GetRef();
    }
    TStringBuf host = GetOnlyHost(url);
    item.SourceHost() = host;
    if (host == HOST_YOUTUBE) {
        TCgiParameters cgis(url.After('?'));
        const TString& id = cgis.Get(TStringBuf("v"));
        if (id) {
            item.PlayUri() = MakeYoutubeUrl(id, contentGetterDelegate);
            if (contentGetterDelegate.SupportsBrowserVideoGallery())
                item.PlayUri() = BuildVideoUriForBrowser(item.PlayUri(), clip->Title(), contentGetterDelegate);
            item.ProviderItemId() = id;
        }
        item.ProviderName() = PROVIDER_YOUTUBE;
        item.DebugInfo().WebPageUrl() = MakeWebYoutubeUrl(id);
    } else {
        item.ProviderItemId() = url;
        item.ProviderName() = PROVIDER_YAVIDEO;
        if (clip->PlayerId() == "vh" || clip->PlayerId() == "ott") {
            item.ProviderName() = PROVIDER_STRM;
            item.ProviderItemId() = GetHostingId(url);
            item.PlayUri() = url;
        } else if (contentGetterDelegate.SupportsBrowserVideoGallery()) {
            item.PlayUri() = BuildVideoUriForBrowser(url, clip->Title(), contentGetterDelegate);
        } else {
            TString qproxyUrl(clip->Qproxy());
            if (qproxyUrl) {
                item.PlayUri() = qproxyUrl;
                item.ProviderName() = PROVIDER_YAVIDEO_PROXY;
            } else {
                item.PlayUri() = BuildPlayerUri(clip->HtmlAutoplayVideoPlayer(),
                                                clip->PlayerId(), contentGetterDelegate);
            }
        }
        item.DebugInfo().WebPageUrl() = url;
    }

    item->Type() = ToString(EItemType::Video);

    /**
     * TODO: Clarify where to get thumbs. Currently serpData["thmb_href", "thmb_w", "thmb_h"] returns thumbnails in 120x90 (4:3)
     * item.CoverUrl2x3();
     * item.CoverUrl16x9();
     * item.ThumbnailUrl16x9();
     */

    // Try to make required thumbnails
    if (clip->HasThmbHref()) {
        TString thmbHref(clip->ThmbHref());
        if (thmbHref.StartsWith("//")) {
            thmbHref.prepend("https:");
        }

        if (thmbHref.StartsWith("https://avatars.mds.yandex.net/") ||
            thmbHref.StartsWith("http://avatars.mds.yandex.net/") ||
            thmbHref.StartsWith("https://avatars.yandex.net/") ||
            thmbHref.StartsWith("http://avatars.yandex.net/")) {
            if (!thmbHref.EndsWith(TStringBuf("/")) && !thmbHref.StartsWith(TStringBuf("/800x360"))) {
                thmbHref.append("/800x360");
            }
            item.ThumbnailUrl16X9() = thmbHref;
            item.ThumbnailUrl16X9Small() = thmbHref;
        } else {
            item.ThumbnailUrl16X9() = thmbHref + "&w=504&h=284";
            item.ThumbnailUrl16X9Small() = thmbHref + "&w=88&h=48";
        }
    }

    item.Name() = clip->Title();
    item.Description() = clip->Pass();

    item.Duration() = clip->Duration();

    auto objectCard = clip->VideoObjects().ObjectCards()[0];
    if (!objectCard.IsNull()) {
        /**
         * TODO: Clarify that we can use this field and in this way
         * TODO: Add normalization
         * item.Genre() = NBass::NVideo::JoinStringArray(objectCard["Genres"].GetArray());
         */
        item.Rating() = objectCard.KinopoiskRating();

        /**
         * TODO:
         * item.ReviewAvailable(); - kinopoisk API? videosearch?
         * item.SeasonsCount(); - not avaliable at yandex.ru/video-xml
         */
        item.ReleaseYear() = objectCard.ReleaseYear();

        item.Directors() = JoinStringArray(objectCard.Directors().Get()->GetArray());
        item.Actors() = JoinStringArray(objectCard.Actors().Get()->GetArray());
    }

    if (clip->HasExtra()) {
        item.Related() = std::move(clip->Extra().Related());
    }

    ui64 views = 0;
    if (TryFromString(clip->Views(), views)) {
        item.ViewCount() = views;
    } else {
        LOG(WARNING) << "Bad views value: " << clip->Views() << Endl;
    }

    item.Available() = true;
    item.Source() = sourceTag;
    return true;
}

NVideoCommon::TResult ParseJsonResponse(const NSc::TValue& jsonData,
                                        TYaVideoContentGetterDelegate& contentGetterDelegate,
                                        TVideoGalleryScheme* response, TStringBuf sourceTag) {
    auto logBadResponse = [&jsonData](TStringBuf err) {
        LOG(WARNING) << err << Endl;
        LOG(WARNING) << "Bad response: " << jsonData << Endl;
    };
    contentGetterDelegate.FillTunnellerResponse(jsonData);

    const NSc::TValue& searchData = jsonData["searchdata"];
    if (searchData.IsNull()) {
        TStringBuf err = "yavideo json parse error: searchdata is not found in the YaVideo answer";
        logBadResponse(err);
        return NVideoCommon::TError{err};
    }
    if (searchData.Has("err_code")) {
        TString err = TStringBuilder{} << TStringBuf("yavideo json returned an error: code ")
                                       << searchData["err_code"].GetIntNumber()
                                       << ", text: " << searchData["err_text"].GetString();
        logBadResponse(err);
        return NVideoCommon::TError{err};
    }

    const NSc::TValue clips = searchData["clips"];
    if (!clips.IsArray()) {
        TStringBuf err = "yavideo json parse error: searchdata/clips is not an array";
        logBadResponse(err);
        return NVideoCommon::TError{err};
    }

    for (const NSc::TValue& clip : clips.GetArray()) {
        TVideoItem item;
        if (ParseVideoItemJsonResponse(clip, contentGetterDelegate, item.Scheme(), sourceTag))
            response->Items().Add() = item.Scheme();
    }
    return {};
}

} // namespace NBASS:NVideo

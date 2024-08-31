#include "youtube_utils.h"

#include <library/cpp/timezone_conversion/civil.h>

#include <regex>

namespace NVideoCommon {

namespace {

constexpr int SECS_IN_MINUTE = 60;
constexpr int SECS_IN_HOUR = 60 * SECS_IN_MINUTE;
constexpr int SECS_IN_DAY = 24 * SECS_IN_HOUR;
constexpr int SECS_IN_WEEK = 7 * SECS_IN_DAY;
constexpr TStringBuf HOST_YOUTUBE = "www.youtube.com";

}; // namespace

const TYouTubeCredentials& GetYouTubeCredentials() {
    return *Singleton<TYouTubeCredentials>();
}

ui64 GetYouTubeVideoDuration(const NSc::TValue& rawDuration) {
    ui64 result = 0;
    std::regex regexPTS("PT([\\d]+)S");
    std::regex regexPTM("PT([\\d]+)M$");
    std::regex regexPTMS("PT([\\d]+)M([\\d]+)S");
    std::regex regexPTHM("PT([\\d]+)H([\\d]+)M$");
    std::regex regexPTHS("PT([\\d]+)H([\\d]+)S");
    std::regex regexPTHMS("PT([\\d]+)H([\\d]+)M([\\d]+)S");
    std::regex regexPDTHMS("P([\\d]+)DT([\\d]+)H([\\d]+)M([\\d]+)S");
    std::regex regexPWDTHMS("P([\\d]+)W([\\d]+)DT([\\d]+)H([\\d]+)M([\\d]+)S");
    std::smatch match;

    NSc::TValue parts;
    const TString& strRawDuration = rawDuration.ForceString();
    if (std::regex_search(strRawDuration.c_str(), match, regexPTS)) {
        parts["S"] = match[1].str();
    } else if (std::regex_search(strRawDuration.c_str(), match, regexPTM)) {
        parts["M"] = match[1].str();
    } else if (std::regex_search(strRawDuration.c_str(), match, regexPTMS)) {
        parts["M"] = match[1].str();
        parts["S"] = match[2].str();
    } else if (std::regex_search(strRawDuration.c_str(), match, regexPTHM)) {
        parts["H"] = match[1].str();
        parts["M"] = match[2].str();
    } else if (std::regex_search(strRawDuration.c_str(), match, regexPTHS)) {
        parts["H"] = match[1].str();
        parts["S"] = match[2].str();
    } else if (std::regex_search(strRawDuration.c_str(), match, regexPTHMS)) {
        parts["H"] = match[1].str();
        parts["M"] = match[2].str();
        parts["S"] = match[3].str();
    } else if (std::regex_search(strRawDuration.c_str(), match, regexPDTHMS)) {
        parts["D"] = match[1].str();
        parts["H"] = match[2].str();
        parts["M"] = match[3].str();
        parts["S"] = match[4].str();
    } else if (std::regex_search(strRawDuration.c_str(), match, regexPWDTHMS)) {
        parts["W"] = match[1].str();
        parts["D"] = match[2].str();
        parts["H"] = match[3].str();
        parts["M"] = match[4].str();
        parts["S"] = match[5].str();
    }

    if (parts.Has("W")) {
        result += SECS_IN_WEEK * parts["W"].ForceIntNumber();
    }
    if (parts.Has("D")) {
        result += SECS_IN_DAY * parts["D"].ForceIntNumber();
    }
    if (parts.Has("H")) {
        result += SECS_IN_HOUR * parts["H"].ForceIntNumber();
    }
    if (parts.Has("M")) {
        result += SECS_IN_MINUTE * parts["M"].ForceIntNumber();
    }
    if (parts.Has("S")) {
        result += parts["S"].ForceIntNumber();
    }

    if (result > UINT32_MAX) {
        result = 0;
    }

    return result;
}

TMaybe<TVideoItem> TryParseYouTubeNode(const NSc::TValue& elem) {
    if (elem.IsNull()) {
        LOG(ERR) << "Empty YouTube videoItem node" << Endl;
        return Nothing();
    }

    TVideoItem response;
    // Required VideoItem params.
    const auto& idNode = elem["id"];
    const auto& thumbnailUrl16X9Node = elem["snippet"]["thumbnails"]["high"]["url"];
    const auto& nameNode = elem["snippet"]["title"];
    if (idNode.IsString() && thumbnailUrl16X9Node.IsString() && nameNode.IsString()) {
        response->ProviderItemId() = idNode.GetString();
        response->ThumbnailUrl16X9() = thumbnailUrl16X9Node.GetString();
        response->ThumbnailUrl16X9Small() = thumbnailUrl16X9Node.GetString();
        response->Name() = nameNode.GetString();
    } else {
        LOG(ERR) << "Empty or not sufficient YouTube videoItem node:\n" << elem << Endl;
        return Nothing();
    }
    // Additional VideoItem params.
    if (const auto& viewsNode = elem["statistics"]["viewCount"]; viewsNode.IsIntNumber())
        response->ViewCount() = viewsNode.GetIntNumber();
    if (const auto& durationNode = elem["contentDetails"]["duration"]; durationNode.IsString())
        response->Duration() = GetYouTubeVideoDuration(durationNode);
    if (const auto& idReleaseNode = elem["snippet"]["publishedAt"]; idReleaseNode.IsString()) {
        const auto when = TInstant::ParseIso8601(idReleaseNode);
        NDatetime::TCivilSecond cs = NDatetime::Convert(when, "UTC");
        response->ReleaseYear() = cs.year();
    }
    // Default YouTube VideoItem params.
    response->ProviderName() = PROVIDER_YOUTUBE;
    response->Type() = ToString(EItemType::Video);
    response->SourceHost() = HOST_YOUTUBE;
    response->Available() = true;

    return response;
}

TYouTubeContentRequestHandle::TYouTubeContentRequestHandle(const TSourceRequestFactory& source,
                                                           NHttpFetcher::IMultiRequest::TRef multiRequest,
                                                           TStringBuf id, bool enableYouTubeUserToken,
                                                           TStringBuf authToken)
{
    NHttpFetcher::TRequestPtr req = source.AttachRequest(multiRequest);
    TCgiParameters cgi;
    GetYouTubeCredentials().AddGoogleAPIsKey(cgi);

    if (enableYouTubeUserToken && !authToken.empty()) {
        TStringBuilder bearer;
        bearer << "Bearer " << authToken;
        req->AddHeader(TStringBuf("Authorization"), bearer);
    }

    cgi.InsertUnescaped(TStringBuf("id"), id);
    cgi.InsertUnescaped(TStringBuf("part"), TStringBuf("snippet,contentDetails,statistics"));
    req->AddCgiParams(cgi);
    Handle = req->Fetch();
}

TResult TYouTubeContentRequestHandle::WaitAndParseResponse(TVideoItem& response) {
    NHttpFetcher::TResponse::TRef resp = Handle->Wait();
    if (resp->IsError()) {
        TStringBuf err = TStringBuilder() << "Cannot get youtube content info: " << resp->GetErrorText();
        LOG(ERR) << err << Endl;
        return NVideoCommon::TError{err};
    }

    const NSc::TValue json = NSc::TValue::FromJson(resp->Data);
    if (json.IsNull() || !json.IsDict()) {
        TStringBuf err = "Youtube answer is not correct:";
        LOG(ERR) << err << Endl << json << Endl;
        return NVideoCommon::TError{err};
    }

    const auto& elem = json["items"][0];

    TMaybe<TVideoItem> item = TryParseYouTubeNode(elem);
    if (!item)
        return NVideoCommon::TError{TStringBuf("Empty or not sufficient YouTube videoItem node")};
    response = std::move(*item);

    return Nothing();
}

} // namespace NVideoCommon

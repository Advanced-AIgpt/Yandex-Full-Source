#include "music_catalog_prepare_handle.h"

#include <alice/library/apphost_request/request_builder.h>

#include <kernel/alice/music_scenario/web_url_canonizer/lib/web_url_canonizer.h>

#include <library/cpp/string_utils/quote/quote.h>

namespace {

constexpr TStringBuf MUSIC_SEARCH_HTTP_RESPONSE_ITEM = "music_search_http_response";
constexpr TStringBuf MUSIC_CATALOG_HTTP_REQUEST_ITEM = "music_catalog_http_request";

constexpr size_t MAX_WEB_RESULTS_COUNT = 5;

constexpr bool RICH_TRACKS = true;

constexpr TStringBuf TYPE_FIELD = "type";
constexpr TStringBuf PATH_FIELD = "path";
constexpr TStringBuf ID_FIELD = "id";
constexpr TStringBuf USER_ID_FIELD = "userId";
constexpr TStringBuf KIND_FIELD = "kind";
constexpr TStringBuf BULK_DATA_FIELD = "bulk_data";
constexpr TStringBuf RICH_TRACKS_FIELD = "richTracks";

// Copy from https://a.yandex-team.ru/arc_vcs/alice/bass/forms/music/websearch.cpp?rev=r9149220#L413-472
NJson::TJsonValue GetMusicDataFromDocUrl(const TStringBuf url) {
    NJson::TJsonValue obj;
    NJson::TJsonValue bulkData;

    const auto parseMusicUrlResult = NAlice::NMusic::ParseMusicDataFromDocUrl(url);

    Y_ASSERT(parseMusicUrlResult.Type == EMusicUrlType::None || parseMusicUrlResult.Parts.size() >= 1);

    for (const TParseMusicUrlResultPart& part : parseMusicUrlResult.Parts) {
        obj[part.PartName] = part.PartValue;
    }

    switch (parseMusicUrlResult.Type) {
        case EMusicUrlType::None: {
            return NJson::TJsonValue(NJson::JSON_NULL);
        }
        case EMusicUrlType::Album: {
            Y_ENSURE(parseMusicUrlResult.Parts.size() == 1);
            const TParseMusicUrlResultPart& firstPart = parseMusicUrlResult.Parts.front();
            obj[TYPE_FIELD] = firstPart.PartName;
            obj[PATH_FIELD] = TString::Join("/albums/", firstPart.PartValue);

            bulkData[ID_FIELD] = firstPart.PartValue;
            break;
        }
        case EMusicUrlType::Track: {
            Y_ENSURE(parseMusicUrlResult.Parts.size() <= 2);
            obj[TYPE_FIELD] = parseMusicUrlResult.Parts.back().PartName;
            obj[PATH_FIELD] = TString::Join("/tracks/", parseMusicUrlResult.Parts.back().PartValue);

            bulkData[ID_FIELD] = parseMusicUrlResult.Parts.back().PartValue;
            break;
        }
        case EMusicUrlType::Playlist: {
            Y_ENSURE(parseMusicUrlResult.Parts.size() == 2);
            const TStringBuf firstPartId = parseMusicUrlResult.Parts[0].PartValue;
            const TStringBuf secondPartId = parseMusicUrlResult.Parts[1].PartValue;
            TString userLogin(firstPartId);
            Quote(userLogin, "");
            obj[TYPE_FIELD] = "playlist";
            obj[PATH_FIELD] =
                TString::Join(
                    "/users/",
                    userLogin,
                    "/playlists/",
                    secondPartId,
                    "?rich-tracks=",
                    (RICH_TRACKS ? "true" : "false")
                );

            bulkData[USER_ID_FIELD] = userLogin;
            bulkData[KIND_FIELD] = secondPartId;
            bulkData[RICH_TRACKS_FIELD] = RICH_TRACKS;
            break;
        }
        case EMusicUrlType::Artist: {
            Y_ASSERT(parseMusicUrlResult.Parts.size() == 1);
            const TParseMusicUrlResultPart& firstPart = parseMusicUrlResult.Parts.front();
            obj[TYPE_FIELD] = firstPart.PartName;
            obj[PATH_FIELD] = TString::Join("/artists/", firstPart.PartValue, "/similar");

            bulkData[ID_FIELD] = firstPart.PartValue;
            break;
        }
    }

    bulkData[TYPE_FIELD] = obj[TYPE_FIELD];

    obj[BULK_DATA_FIELD] = std::move(bulkData);
    return obj;
}


} // namespace


// Copy from https://a.yandex-team.ru/arc_vcs/alice/bass/forms/music/websearch.cpp?rev=r9156639#L318-371
TMaybe<NAlice::NAppHostRequest::TAppHostHttpProxyRequestBuilder> BuildMusicCatalogRequest(
    const NJson::TJsonValue searchResponse
) {
    if (!searchResponse.IsMap()) {
        return Nothing();
    }

    const auto& docs = searchResponse["tmpl_data"]["searchdata"]["docs"].GetArray();
    if (docs.empty()) {
        return Nothing();
    }

    TVector<NJson::TJsonValue> webDatas;

    TSet<TStringBuf> seenPaths;

    for (size_t i = 0; i < Max(docs.size(), MAX_WEB_RESULTS_COUNT); ++i) {
        const auto docurl = docs[i]["url"].GetString();
        NJson::TJsonValue data = GetMusicDataFromDocUrl(docurl);
        if (data.IsNull()) {
            continue;
        }

        // For analytics
        data["docurl"] = docurl;
        data["docpos"] = docs[i]["num"].IsInteger() ? docs[i]["num"].GetInteger() : i;
        // Do not duplicate requests
        if (seenPaths.insert(data[PATH_FIELD].GetString()).second) {
            webDatas.push_back(std::move(data));
        }
    }

    NJson::TJsonValue requestBody;

    for (auto& webData : webDatas) {
        if (!webData.IsNull()) {
            auto& bulkField = webData[BULK_DATA_FIELD];
            if (bulkField[TYPE_FIELD] == "track") {
                bulkField["withAllPartsContainer"] = true;
            }
            requestBody["ids"].AppendValue(bulkField);
        }
    }

    NAlice::NAppHostRequest::TAppHostHttpProxyRequestBuilder request;

    request.SetBody(NAlice::JsonToString(requestBody), "post");
    request.SetContentType(NAlice::NContentTypes::APPLICATION_JSON);

    return request;
}


namespace NAlice::NHollywood::NMusic {

void TMusicCatalogPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    if (auto responseJsonStr = GetRawHttpResponseMaybe(ctx, MUSIC_SEARCH_HTTP_RESPONSE_ITEM)) {
        if (auto musicCatalogRequestBuilder = BuildMusicCatalogRequest(JsonFromString(*responseJsonStr))) {
            return ctx.ServiceCtx.AddProtobufItem(musicCatalogRequestBuilder->CreateRequest(), MUSIC_CATALOG_HTTP_REQUEST_ITEM);
        }
    }
}

} // namespace NAlice::NHollywood::NMusic

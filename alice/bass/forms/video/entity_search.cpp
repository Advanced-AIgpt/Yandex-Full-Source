#include "entity_search.h"

#include <alice/bass/libs/video_common/defs.h>

#include <alice/library/analytics/common/names.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/video_common/defs.h>

namespace NBASS::NVideo {

namespace {

constexpr ui64 ENTITY_SEARCH_COUNT_OBJECTS_ON_PAGE = 30;
constexpr TStringBuf ENTITY_SEARCH_CLIENT_NAME = "bass";

} // namespace

// Deprecated
TVector<TString> GetKpidsFromEntitySearchResponse(const NSc::TValue& entitySearchResponse, const TString& pathBegin = "cards/0") {
    NSc::TValue objects = entitySearchResponse.TrySelect(pathBegin + "/parent_collection/object");
    if (!objects.IsArray()) {
        return {};
    }
    TVector<TString> kinopoiskIds;
    for (size_t i = 0; i < objects.GetArray().size(); ++i) {
        NSc::TValue kinopoiskIdValue = objects.Get(i).TrySelect("ids/kinopoisk");
        if (!kinopoiskIdValue.IsString())
            continue;
        TStringBuf kinopoiskIdView = kinopoiskIdValue.GetString();
        if (!kinopoiskIdView.AfterPrefix("film/", kinopoiskIdView)) {
            LOG(ERR) << "Kinopoisk id \"" << kinopoiskIdView << "\" from entity search is not valid." << Endl;
            continue;
        }
        kinopoiskIds.push_back(TString{kinopoiskIdView});
    }
    return kinopoiskIds;
}

void TryAddObject(
    TVector<TString>& allKpIds, THashMap<TString, TString>& kpidToEntref,
    THashMap<TString, TString>& kpidToHorizontalPoster, THashMap<TString, NSc::TValue>& kpidToLicenses,
    const NSc::TValue& object)
{
    TStringBuf kinopoiskIdView = object.TrySelect("ids/kinopoisk").GetString();
    if (!kinopoiskIdView.AfterPrefix("film/", kinopoiskIdView)) {
        LOG(ERR) << "Kinopoisk id \"" << kinopoiskIdView << "\" from entity search is not valid." << Endl;
        return;
    }
    TString kpId = TString(kinopoiskIdView);
    allKpIds.push_back(kpId);
    kpidToEntref[kpId] = object.TrySelect("entref").GetString();
    kpidToHorizontalPoster[kpId] = object.TrySelect("legal/vh_licenses/horizontal_poster").GetString();
    {
        NSc::TValue licenses;
        if (object.PathExists("legal/vh_licenses/est")) {
            licenses["est"]["discount_price"] = object.TrySelect("legal/vh_licenses/est/discount_price");
            licenses["est"]["price"] = object.TrySelect("legal/vh_licenses/est/price");
        }
        if (object.PathExists("legal/vh_licenses/tvod")) {
            licenses["tvod"]["discount_price"] = object.TrySelect("legal/vh_licenses/tvod/discount_price");
            licenses["tvod"]["price"] = object.TrySelect("legal/vh_licenses/tvod/price");
        }
        if (object.PathExists("legal/vh_licenses/svod")) {
            licenses["svod"]["subscriptions"] = object.TrySelect("legal/vh_licenses/svod/subscriptions");
        }
        kpidToLicenses[kpId] = licenses;
    }
}

bool TryAddKpItem(TVector<TString>& allKpIds, TStringBuf visibleUrl) {
    if (visibleUrl.StartsWith("https://www.kinopoisk.ru/film/")) {
        visibleUrl.AfterPrefix("https://www.kinopoisk.ru/film/", visibleUrl);
        visibleUrl = visibleUrl.Before('/');
        allKpIds.emplace_back(visibleUrl);
        return true;
    }
    return false;
}

void TryAddOrganicKpObjects(TVector<TString>& allKpIds, size_t checkTopOrganicResultsForKp,
                            size_t checkReducedTopOrganicResultsForKp, size_t checkChainedTopOrganicResultsForKp,
                            size_t checkScaledTopOrganicResultsForKp, const NSc::TValue& clips) {
    if (checkTopOrganicResultsForKp) {
        size_t itemsToCheck = Min(checkTopOrganicResultsForKp, clips.ArraySize());
        size_t firstKpItemIdx = 0;

        for (; firstKpItemIdx < itemsToCheck; ++firstKpItemIdx) {
            if (TryAddKpItem(allKpIds, clips.Get(firstKpItemIdx)["VisibleURL"])) {
                break;
            }
        }

        if (firstKpItemIdx < itemsToCheck) {
            itemsToCheck = clips.ArraySize();

            // we don't want to check all the results, because we can get irrelevant objects from the bottom
            if (checkReducedTopOrganicResultsForKp) {
                itemsToCheck = Min(itemsToCheck, checkReducedTopOrganicResultsForKp);
            }

            if (checkScaledTopOrganicResultsForKp) {
                // the further away the first kp object is, the fewer objects we want to check

                // scale = what part of total organic top we want to dismiss for each step of searching for the first kp object
                // scale can be set by the value of flag checkScaledTopOrganicResultsForKp, but it can't be less than
                // checkTopOrganicResultsForKp - 1, because firstKpItemIdx can be equal to checkTopOrganicResultsForKp - 1,
                // and we don't want itemsToCheck to become negative. Besides, itemsToCheck can't be greater than
                // checkReducedTopOrganicResultsForKp (if it is set)
                size_t scale = Max(checkScaledTopOrganicResultsForKp, checkTopOrganicResultsForKp - 1);
                itemsToCheck = Min(itemsToCheck, clips.ArraySize() * (scale - firstKpItemIdx) / scale);

                for (size_t i = firstKpItemIdx + 1; i < itemsToCheck; ++i) {
                    TryAddKpItem(allKpIds, clips.Get(i)["VisibleURL"]);
                }
            } else if (checkChainedTopOrganicResultsForKp) {
                // we check results in a chain to ensure their relevance
                size_t lastKpIdx = firstKpItemIdx;

                for (size_t i = lastKpIdx + 1; i < itemsToCheck; ++i) {
                    if (i > checkChainedTopOrganicResultsForKp + lastKpIdx) {
                        break;
                    }

                    if (TryAddKpItem(allKpIds, clips.Get(i)["VisibleURL"])) {
                        lastKpIdx = i;
                    }
                }
            } else {
                // current prod (if checkReducedTopOrganicResultsForKp was not set)
                for (size_t i = firstKpItemIdx + 1; i < itemsToCheck; ++i) {
                    TryAddKpItem(allKpIds, clips.Get(i)["VisibleURL"]);
                }
            }
        }
    }
}

TVector<TString> GetIdsFromEntitySearchResponse(
    const NSc::TValue& entitySearchResponse, THashMap<TString, TString>& kpidToEntref,
    THashMap<TString, TString>& kpidToHorizontalPoster, THashMap<TString, NSc::TValue>& kpidToLicenses,
    size_t checkTopOrganicResultsForKp, size_t checkReducedTopOrganicResultsForKp,
    size_t checkChainedTopOrganicResultsForKp, size_t checkScaledTopOrganicResultsForKp, bool addMainObject)
{
    TVector<TString> allKpIds;
    if (addMainObject) {
        const NSc::TValue mainObject = entitySearchResponse.TrySelect("searchdata/scheme/entity_data/base_info");
        if (!mainObject.IsNull()) {
            TryAddObject(allKpIds, kpidToEntref, kpidToHorizontalPoster, kpidToLicenses, mainObject);
        }
    }
    const NSc::TValue& objects = entitySearchResponse.TrySelect("searchdata/scheme/entity_data/parent_collection/object");
    if (!objects.IsArray()) {
        //if there is no parent collection objects
        TryAddOrganicKpObjects(allKpIds, checkTopOrganicResultsForKp, checkReducedTopOrganicResultsForKp,
                               checkChainedTopOrganicResultsForKp, checkScaledTopOrganicResultsForKp,
                               entitySearchResponse.TrySelect("searchdata/clips"));
        return allKpIds;
    }
    for (size_t i = 0; i < objects.GetArray().size(); ++i) {
        TryAddObject(allKpIds, kpidToEntref, kpidToHorizontalPoster, kpidToLicenses, objects.Get(i));
    }
    return allKpIds;
}

TVector<NHttpFetcher::THandle::TRef> AddEntitySearchFilmListRequests(TContext& ctx,
                                                                     const TVideoSlots& videoSlots,
                                                                     ui64 pagesCount) {
    TString query = videoSlots.BuildSearchQueryForWeb();
    TVector<NHttpFetcher::THandle::TRef> resultRequests;
    for (ui64 pageNum = 0; pageNum < pagesCount; ++pageNum) {
        NHttpFetcher::TRequestPtr r = ctx.GetSources().EntitySearch().Request();
        r->AddCgiParam("client", ENTITY_SEARCH_CLIENT_NAME);
        r->AddCgiParam("film_list_predict", "[\"100\"]"); // To get list of films.
        r->AddCgiParam("text", query);
        // Param to skip some objects in answers list.
        r->AddCgiParam("wizextra", "entlistskip="sv + ToString(ENTITY_SEARCH_COUNT_OBJECTS_ON_PAGE*pageNum));
        resultRequests.push_back(r->Fetch());
    }
    return resultRequests;
}

NHttpFetcher::THandle::TRef AddEntitySearchSingleFilmRequest(TContext& ctx,
                                                             const TVideoSlots& videoSlots) {
    TString query = videoSlots.BuildSearchQueryForWeb();
    NHttpFetcher::TRequestPtr r = ctx.GetSources().EntitySearch().Request();
    r->AddCgiParam("client", ENTITY_SEARCH_CLIENT_NAME);
    r->AddCgiParam("text", query);
    return r->Fetch();
}

NSc::TValue GetEntitySearchResponseFromHandle(NHttpFetcher::THandle::TRef handle) {
    if (!handle) {
        LOG(ERR) << "Empty NHttpFetcher handle." << Endl;
        return {};
    }
    NHttpFetcher::TResponse::TRef httpResponse = handle->Wait();
    if (httpResponse->IsError()) {
        LOG(ERR) << httpResponse->GetErrorText() << Endl;
        return {};
    }
    NSc::TValue response;
    if (!NSc::TValue::FromJson(response, httpResponse->Data))
        LOG(ERR) << "Can not convert JSON to NSc::TValue." << Endl;
    return response;
}

TVector<NSc::TValue> GetEntitySearchResponsesFromHandles(TVector<NHttpFetcher::THandle::TRef>& handles) {
    TVector<NSc::TValue> resultValues;
    resultValues.reserve(handles.size());
    for (auto& handle : handles) {
        if (handle) {
            NSc::TValue value = GetEntitySearchResponseFromHandle(handle);
            resultValues.push_back(std::move(value));
        } else {
            resultValues.push_back({});
        }
    }
    return resultValues;
}

TVector<TVideoItem> GetVideoItemFromEntitySearchResponse(NSc::TValue& entitySearchResponse, TContext& ctx) {
    if (ctx.HasExpFlag(NAlice::NExperiments::TUNNELLER_ANALYTICS_INFO) &&
        entitySearchResponse.IsDict() && entitySearchResponse.Has(NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE)) {
        ctx.GetAnalyticsInfoBuilder().AddTunnellerRawResponse(
            TString{entitySearchResponse[NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE].GetString()});
    }

    NSc::TValue kinopoiskIdValue = entitySearchResponse.TrySelect("cards/0/base_info/ids/kinopoisk");
    if (!kinopoiskIdValue.IsString()) {
        LOG(WARNING) << "Entity search does not provide a single item for this request." << Endl;
        return {};
    }
    TStringBuf kinopoiskIdView = kinopoiskIdValue.GetString();
    if (!kinopoiskIdView.AfterPrefix("film/", kinopoiskIdView)) {
        LOG(ERR) << "Kinopoisk id \"" << kinopoiskIdView << "\" from entity search is not valid." << Endl;
        return {};
    }
    TVector<TVideoItem> videoItems;
    if (!TryGetVideoItemsFromYdbByKinopoiskIds(ctx, {TString(kinopoiskIdView)}, videoItems)) {
        LOG(WARNING) << "Can not get item in content db (kpid: \"" << kinopoiskIdView << "\")." << Endl;
        return {};
    }
    SetItemsSource(videoItems, NAlice::NVideoCommon::VIDEO_SOURCE_ENTITY_SEARCH);
    if (videoItems.empty()) {
        LOG(WARNING) << "No video items in content db with kpid: " << kinopoiskIdView << "." << Endl;
    }
    NSc::TValue entrefValue = entitySearchResponse.TrySelect("cards/0/base_info/entref");
    if (entrefValue.IsString()) {
        videoItems[0]->Entref() = entrefValue.GetString();
    }
    return videoItems;
}

TVector<TVideoItem> GetVideoItemsFromEntitySearchResponses(TVector<NSc::TValue>& entitySearchResponses, TContext& ctx) {
    TVector<TString> allKpids;
    // TODO Get rid of multiple requests to entitysearch.
    for (auto& response : entitySearchResponses) {
        if (ctx.HasExpFlag(NAlice::NExperiments::TUNNELLER_ANALYTICS_INFO) &&
            response.IsDict() && response.Has(NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE)) {
            ctx.GetAnalyticsInfoBuilder().AddTunnellerRawResponse(
                TString{response[NAlice::NAnalyticsInfo::TUNNELLER_RAW_RESPONSE].GetString()});
        }
        TVector<TString> kpids = GetKpidsFromEntitySearchResponse(response);
        std::move(kpids.begin(), kpids.end(), std::back_inserter(allKpids));
    }
    if (allKpids.empty()) {
        LOG(WARNING) << "No valid kinopoisk ids in entity search response." << Endl;
        return {};
    }
    TVector<TVideoItem> videoItems;
    if (!TryGetVideoItemsFromYdbByKinopoiskIds(ctx, allKpids, videoItems)) {
        LOG(WARNING) << "Incorrect result of GetVideoItemsListFromYdb." << Endl;
        return {};
    }
    SetItemsSource(videoItems, NAlice::NVideoCommon::VIDEO_SOURCE_ENTITY_SEARCH);
    return videoItems;
}

} // namespace NBASS::NVideo

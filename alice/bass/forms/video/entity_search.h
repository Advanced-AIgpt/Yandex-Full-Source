#pragma once

#include <alice/bass/util/error.h>
#include <alice/bass/forms/context/context.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/forms/video/video_slots.h>
#include <alice/bass/forms/video/video_provider.h>

namespace NBASS::NVideo {

TVector<TString> GetKpidsFromEntitySearchResponse(const NSc::TValue& entitySearchResponse, const TString& pathBegin);

TVector<TString> GetIdsFromEntitySearchResponse(
    const NSc::TValue& entitySearchResponse, THashMap<TString, TString>& kpidToEntref,
    THashMap<TString, TString>& kpidToHorizontalPoster, THashMap<TString, NSc::TValue>& kpidToLicenses,
    size_t checkTopOrganicResultsForKp, size_t checkReducedTopOrganicResultsForKp,
    size_t checkChainedTopOrganicResultsForKp, size_t checkScaledTopOrganicResultsForKp, bool addMainObject = false);

TVector<NHttpFetcher::THandle::TRef> AddEntitySearchFilmListRequests(TContext& ctx,
                                                                     const TVideoSlots& videoSlots,
                                                                     ui64 pagesCount);
NHttpFetcher::THandle::TRef AddEntitySearchSingleFilmRequest(TContext& ctx,
                                                             const TVideoSlots& videoSlots);

NSc::TValue GetEntitySearchResponseFromHandle(NHttpFetcher::THandle::TRef handle);
TVector<NSc::TValue> GetEntitySearchResponsesFromHandles(TVector<NHttpFetcher::THandle::TRef>& handles);

TVector<TVideoItem> GetVideoItemsFromEntitySearchResponses(TVector<NSc::TValue>& entitySearchResponses, TContext& ctx);
TVector<TVideoItem> GetVideoItemFromEntitySearchResponse(NSc::TValue& entitySearchResponse, TContext& ctx);

} // namespace NBASS::NVideo

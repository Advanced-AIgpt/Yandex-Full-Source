#pragma once

#include "defs.h"

#include <alice/bass/libs/fetcher/request.h>

namespace NBASS {
namespace NVideo {

class TVideoItemsMerger {
public:
    TVector<TVideoItem> GetResult() {
        return std::move(Items);
    }

    void Add(TVideoItem&& item) {
        if (item.Value().IsNull()) { // Item was already added and now is null
            return;
        }

        TStringBuf id = item->ProviderItemId();
        bool inserted = Ids.insert(id).second;
        if (inserted) {
            Items.push_back(std::move(item));
        }
    }

private:
    TVector<TVideoItem> Items;
    THashSet<TStringBuf> Ids;
};

using TWebSearchByProviderResponse = TVector<TVideoItem>;

struct TSearchByProviderResponses {
    TVector<TVideoItem> ProviderSearchItems;
    TVector<TVideoItem> WebSearchItems;

    TVector<TVideoItem> FinalGalleryItems;
    TVideoGalleryDebugInfo DebugInfo;
};

struct TWebSearchResponse {
    NHttpFetcher::TResponse::TRef HttpResponse;

    NSc::TValue FilmSnippet;
    TString NormalizedTitle;

    struct TCarouselItem {
        TString Name;
        TString KinopoiskId;
    };
    TVector<TCarouselItem> CarouselItems;
};

struct TVideoSearchResponses {
    THashMap<TStringBuf, TSearchByProviderResponses> ByProviders;
    TWebSearchResponse WebSearch;
    TVideoGalleryDebugInfo DebugInfo;
};

} // namespace NVideo
} // namespace NBASS

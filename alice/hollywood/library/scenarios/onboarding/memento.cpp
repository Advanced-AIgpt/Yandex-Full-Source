#include "memento.h"

#include <alice/memento/proto/api.pb.h>
#include <alice/protos/data/proactivity/tag_stats.pb.h>
#include <alice/protos/data/proactivity/last_views.pb.h>

namespace NAlice::NHollywoodFw::NOnboarding {

namespace {

constexpr TStringBuf EXP_PROACTIVITY_SHOWS_HISTORY_LEN = "mm_proactivity_shows_history_len";
constexpr size_t DEFAULT_SHOWS_HISTORY_LEN = 15;

NData::NProactivity::TItemAnalytics MakeItemAnalytics(const NDJ::NAS::TProtoItem& item, const ui32 itemType) {
    NData::NProactivity::TItemAnalytics result;
    result.SetItemId(item.GetId());
    result.SetBaseId(item.GetBaseItem());
    result.SetInfo(item.GetItemInfo());
    result.SetItemType(itemType);
    *result.MutableTags() = item.GetTags();
    return result;
}

NData::NProactivity::TItemView MakeItemView(const TRunRequest& request, const NDJ::NAS::TProtoItem& item, const ui32 itemType) {
    NData::NProactivity::TItemView itemView;
    *itemView.MutableItemAnalytics() = MakeItemAnalytics(item, itemType);
    *itemView.MutableSuccessConditions() = item.GetSuccessConditions();
    itemView.SetRequestId(request.System().RequestId());
    return itemView;
}

size_t GetShowsHistoryLen(const TRunRequest& request) {
    return request.Flags().GetValue<size_t>(EXP_PROACTIVITY_SHOWS_HISTORY_LEN, DEFAULT_SHOWS_HISTORY_LEN);
}

} // namespace

void UpdateTagStats(const TRunRequest& request, TStorage& storage, const TProtoItems& items) {
    const auto currentTime = request.Client().GetClientTimeMsGMT().count() / 1000;
    const auto updateStats = [currentTime](NData::NProactivity::TLastShowStats& s, int count = 1) {
        s.SetShowTimestamp(currentTime);
        s.SetShowCount(s.GetShowCount() + count);
    };
    auto tagStatsStorage = storage.GetMementoUserConfig().GetProactivityTagStats();
    updateStats(*tagStatsStorage.MutableLastShowStats(), items.size());
    for (const auto& item : items) {
        for (const auto& tag : item.GetTags()) {
            updateStats((*tagStatsStorage.MutableTagStats())[tag]);
        }
    }
    storage.AddMementoUserConfig(ru::yandex::alice::memento::proto::CK_PROACTIVITY_TAG_STATS, tagStatsStorage);
}

void AddLastViews(const TRunRequest& request, TStorage& storage, const TProtoItems& items, const ui32 itemType) {
    auto lastViewsStorage = storage.GetMementoUserConfig().GetProactivityLastViews();
    auto& lastViews = (*lastViewsStorage.MutableLastViewsByItemType())[itemType];
    lastViews.SetLastViewRequestId(request.System().RequestId());
    auto& itemViews = *lastViews.MutableItemViews();
    for (const auto& item : items) {
        *itemViews.Add() = MakeItemView(request, item, itemType);
    }
    const auto showsHistoryLen = GetShowsHistoryLen(request);
    if (static_cast<size_t>(itemViews.size()) > showsHistoryLen) {
        const auto first = itemViews.begin();
        const auto last = first + (itemViews.size() - showsHistoryLen);
        itemViews.erase(first, last);
    }
    storage.AddMementoUserConfig(ru::yandex::alice::memento::proto::CK_PROACTIVITY_LAST_VIEWS, lastViewsStorage);
}

} // namespace NAlice::NHollywoodFw::NOnboarding

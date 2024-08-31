#include "common.h"

#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>

namespace NAlice::NHollywood::NMusic::NCache {

namespace {

const THashMap<TStringBuf, TStringBuf> CACHEABLE_HTTP_REQUEST_ITEMS = {
    {MUSIC_LIKES_TRACKS_REQUEST_ITEM, MUSIC_LIKES_TRACKS_RESPONSE_ITEM},
    {MUSIC_DISLIKES_TRACKS_REQUEST_ITEM, MUSIC_DISLIKES_TRACKS_RESPONSE_ITEM},
    {MUSIC_REQUEST_ITEM, MUSIC_RESPONSE_ITEM},
};

} // namespace

THttpRequestInfo FindHttpRequestInfo(THwServiceContext& ctx) {
    for (const auto [inputItemName, outputItemName] : CACHEABLE_HTTP_REQUEST_ITEMS) {
        if (auto item = ctx.GetMaybeProto<NAppHostHttp::THttpRequest>(inputItemName)) {
            return {
                .HttpRequest = std::move(*item),
                .InputItemName = inputItemName,
                .OutputItemName = outputItemName,
            };
        }
    }
    ythrow yexception() << "Can't find any http request";
}

size_t CalculateHash(const TInputCacheMeta& meta, const THttpRequestInfo& requestInfo) {
    TStringBuf path = requestInfo.HttpRequest.GetPath();
    path = path.Before('?'); // drop query parameters

    // add subscription region id for content requests
    if (auto regionId = meta.GetMusicSubscriptionRegionId()) {
        return MultiHash(path, regionId);
    }

    // simple hash for other requests
    return MultiHash(path);
}

} // namespace NAlice::NHollywood::NMusic::NCache

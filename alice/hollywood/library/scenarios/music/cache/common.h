#pragma once

#include <alice/hollywood/library/scenarios/music/proto/cache_data.pb.h>

#include <alice/hollywood/library/hw_service_context/context.h>

#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NHollywood::NMusic::NCache {

const TString CACHE_STORAGE_TAG = "MusicScenario";

constexpr TStringBuf CACHE_GET_REQUEST_ITEM = "cache_get_request";
constexpr TStringBuf CACHE_GET_RESPONSE_ITEM = "cache_get_response";
constexpr TStringBuf CACHE_SET_REQUEST_ITEM = "cache_set_request";
constexpr TStringBuf CACHE_SET_RESPONSE_ITEM = "cache_set_response";

constexpr TStringBuf CACHE_LOOKUP_NODE_NAME = "MUSIC_CACHE_LOOKUP";
constexpr TStringBuf INPUT_CACHE_META_ITEM = "input_cache_meta";
constexpr TStringBuf OUTPUT_CACHE_META_ITEM = "output_cache_meta";


struct THttpRequestInfo {
    NAppHostHttp::THttpRequest HttpRequest;
    TStringBuf InputItemName;
    TStringBuf OutputItemName;
};

THttpRequestInfo FindHttpRequestInfo(THwServiceContext& ctx);
size_t CalculateHash(const TInputCacheMeta& meta, const THttpRequestInfo& requestInfo);

} // namespace NAlice::NHollywood::NMusic::NCache

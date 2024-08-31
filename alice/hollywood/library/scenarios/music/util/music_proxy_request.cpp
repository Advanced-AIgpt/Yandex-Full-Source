#include "music_proxy_request.h"

#include <alice/hollywood/protos/bass_request_rtlog_token.pb.h>
#include <alice/hollywood/library/music/create_search_request.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NHollywood::NMusic {

void AddMusicProxyRequest(TScenarioHandleContext& ctx, const NAppHostHttp::THttpRequest& musicRequest, const TStringBuf itemName) {
    AddHttpRequestItems(ctx, musicRequest, itemName);
}

TMaybeRawHttpResponse GetFirstOfRawHttpResponses(const TScenarioHandleContext& ctx,
                                                 const std::initializer_list<TStringBuf>& itemNames) {
    for (const auto& itemName : itemNames) {
        if (HaveHttpResponse(ctx, itemName)) {
            return std::make_pair(itemName, GetRawHttpResponse(ctx, itemName));
        }
    }
    return Nothing();
}

} // namespace NAlice::NHollywood::NMusic

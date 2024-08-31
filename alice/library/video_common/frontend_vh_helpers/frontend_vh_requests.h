#pragma once

#include <alice/library/video_common/hollywood_helpers/proxy_request_builder.h>

namespace NAlice::NVideoCommon {

NHollywood::THttpProxyRequest PrepareFrontendVhPlayerRequest(const TString& vhUuid,
                                                 const NHollywood::TScenarioRunRequestWrapper& request,
                                                 const NHollywood::TScenarioHandleContext& ctx,
                                                 TStringBuf from = QUASAR_FROM_ID,
                                                 TStringBuf service = QUASAR_SERVICE);
NHollywood::THttpProxyRequest PrepareFrontendVhPlayerRequest(const TString& vhUuid,
                                                 const NHollywoodFw::TRunRequest& request,
                                                 TStringBuf from = QUASAR_FROM_ID,
                                                 TStringBuf service = QUASAR_SERVICE);

NHollywood::THttpProxyRequest PrepareFrontendVhSeasonsRequest(const TString& vhUuid,
                                                              const NHollywood::TScenarioRunRequestWrapper& request,
                                                              const NHollywood::TScenarioHandleContext& ctx,
                                                              ui32 limit = 20,
                                                              ui32 offset = 0,
                                                              TStringBuf from = QUASAR_FROM_ID,
                                                              TStringBuf service = QUASAR_SERVICE);
NHollywood::THttpProxyRequest PrepareFrontendVhSeasonsRequest(const TString& vhUuid,
                                                              const NHollywoodFw::TRunRequest& request,
                                                              ui32 limit = 20,
                                                              ui32 offset = 0,
                                                              TStringBuf from = QUASAR_FROM_ID,
                                                              TStringBuf service = QUASAR_SERVICE);

NHollywood::THttpProxyRequest PrepareFrontendVhEpisodesRequest(const TString& vhUuid,
                                                               const NHollywood::TScenarioRunRequestWrapper& request,
                                                               const NHollywood::TScenarioHandleContext& ctx,
                                                               ui32 limit = 20,
                                                               ui32 offset = 0,
                                                               TStringBuf from = QUASAR_FROM_ID,
                                                               TStringBuf service = QUASAR_SERVICE);
NHollywood::THttpProxyRequest PrepareFrontendVhEpisodesRequest(const TString& vhUuid,
                                                               const NHollywoodFw::TRunRequest& request,
                                                               ui32 limit = 20,
                                                               ui32 offset = 0,
                                                               TStringBuf from = QUASAR_FROM_ID,
                                                               TStringBuf service = QUASAR_SERVICE);

} // namespace NAlice::NVideoCommon

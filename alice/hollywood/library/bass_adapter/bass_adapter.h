#pragma once

#include <alice/bass/libs/request/request.sc.h>

#include <alice/hollywood/library/base_scenario/fwd.h>
#include <alice/hollywood/library/frame/frame.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/request/fwd.h>
#include <alice/megamind/protos/scenarios/request_meta.pb.h>
#include <alice/hollywood/protos/bass_request_rtlog_token.pb.h>

#include <alice/library/logger/logger.h>

#include <alice/megamind/protos/scenarios/request.pb.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/util/scheme_holder.h>

#include <util/system/types.h>

#include <apphost/lib/proto_answers/http.pb.h>

#include <functional>

namespace NAlice::NHollywood {

constexpr TStringBuf BASS_REQUEST_ITEM = "hw_bass_request";
constexpr TStringBuf BASS_REQUEST_RTLOG_TOKEN_ITEM = "hw_bass_request_rtlog_token";
constexpr TStringBuf BASS_RESPONSE_ITEM = "hw_bass_response";

struct TBassMetaValidationError : public yexception {
    using yexception::yexception;
};

using TSourceTextProvider = std::function<TString(const TStringBuf key, const TString& value)>;

using TFormScheme = NBASSRequest::TForm<TSchemeTraits>;
using TMetaScheme = NBASSRequest::TMeta<TSchemeTraits>;
using TRequestScheme = NBASSRequest::TRequest<TSchemeTraits>;
using TRequestConstScheme = NBASSRequest::TRequestConst<TSchemeTraits>;
using TSlotScheme = TFormScheme::TSlot;
using TBassRequest = TSchemeHolder<TRequestScheme>;

using TBassMetaScheme = NBASSRequest::TMeta<TSchemeTraits>;
using TBassMeta = TSchemeHolder<TBassMetaScheme>;

using TBassMetaConstScheme = NBASSRequest::TMetaConst<TSchemeTraits>;
using TBassMetaConst = TSchemeHolder<TBassMetaConstScheme>;

using TBassActionScheme = NBASSRequest::TRequest<TSchemeTraits>::TAction;
using TBassAction = TSchemeHolder<TBassActionScheme>;

using TBassForm = TSchemeHolder<TFormScheme>;

namespace NImpl {

TBassForm CreateBassLikeForm(const TFrame& frame, const TSourceTextProvider* sourceTextProvider = nullptr);
TBassMeta CreateBassLikeMeta(const TScenarioBaseRequestWrapper& request,
                             const TScenarioInputWrapper& input,
                             bool imageSearch,
                             const bool forbidWebSearch = false,
                             const bool splitUuid = true,
                             const bool suppressFormChanges = true,
                             const TString& from = Default<TString>());

TBassRequest PrepareBassRequestBody(TRTLogger& logger,
                                    const TScenarioBaseRequestWrapper& request,
                                    const TScenarioInputWrapper& input,
                                    const TFrame& frame,
                                    const TSourceTextProvider* sourceTextProvider,
                                    bool imageSearch,
                                    const bool forbidWebSearch = false,
                                    const THashMap<EDataSourceType, const NScenarios::TDataSource*>& dataSources = {},
                                    const bool splitUuid = true);

THttpProxyRequest PrepareHttpBassRequest(TRTLogger& logger,
                                        const TString& path,
                                        const TString& body,
                                        const TString& formName,
                                        const NScenarios::TRequestMeta& meta);

// Must always be called after making the BASS request so that RTLog is correct;
// returns the response body, throws if the request failed
NJson::TJsonValue RetireHttpBassRequest(TRTLogger& logger,
                                        const TBassRequestRTLogToken& bassRequestRTLogToken,
                                        const NAppHostHttp::THttpResponse* bassResponse);

TBassAction CreateMusicPlayObjectAction(const TFrame& frame);

} // namespace NImpl

bool HasSlotForMusicPlayObject(const TFrame& frame);

// emulates VINS request
THttpProxyRequest PrepareBassVinsRequest(TRTLogger& logger,
                                        const TScenarioRunRequestWrapper& request,
                                        const TFrame& frame,
                                        const TSourceTextProvider* sourceTextProvider,
                                        const NScenarios::TRequestMeta& meta,
                                        const bool imageSearch,
                                        const NJson::TJsonValue& appHostParams,
                                        const bool forbidWebSearch = false,
                                        const TVector<EDataSourceType>& dataSourceTypes = {});
THttpProxyRequest PrepareBassVinsRequest(TRTLogger& logger,
                                        const TScenarioBaseRequestWrapper& request,
                                        const TScenarioInputWrapper& input,
                                        const TFrame& frame,
                                        const TSourceTextProvider* sourceTextProvider,
                                        const NScenarios::TRequestMeta& meta,
                                        const bool imageSearch,
                                        const NJson::TJsonValue& appHostParams,
                                        const bool forbidWebSearch = false,
                                        const THashMap<EDataSourceType, const NScenarios::TDataSource*>& dataSources = {},
                                        const bool splitUuid = true);

// emulates MM prepare request
THttpProxyRequest PrepareBassRunRequest(TRTLogger& logger,
                                       const TScenarioRunRequestWrapper& request,
                                       const TFrame& frame,
                                       const TSourceTextProvider* sourceTextProvider,
                                       const NScenarios::TRequestMeta& meta,
                                       const bool imageSearch,
                                       const NJson::TJsonValue& appHostParams);

// emulates MM apply request
THttpProxyRequest PrepareBassApplyRequest(TRTLogger& logger,
                                         const TScenarioApplyRequestWrapper& request,
                                         const NJson::TJsonValue& state,
                                         const NScenarios::TRequestMeta& meta,
                                         const TString& continuationName,
                                         const bool imageSearch,
                                         const NJson::TJsonValue& appHostParams);

THttpProxyRequest PrepareBassRadioSimilarToObjContinueRequest(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& request,
    const TString& contentType, const TString& contentId,
    bool isMuspult, const TString& radioStationId, const TString& startFromTrackId,
    const NScenarios::TRequestMeta& meta, const TString& continuationName,
    const NJson::TJsonValue& appHostParams
);

THttpProxyRequest PrepareBassRadioStationIdContinueRequest(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& request,
    const TString& radioStationId, const TString& from,
    bool isMuspult, const TString& startFromTrackId,
    const NScenarios::TRequestMeta& meta, const TString& continuationName,
    const NJson::TJsonValue& appHostParams
);

NJson::TJsonValue RetireBassRequest(const TScenarioHandleContext& ctx);
void AddBassRequestItems(TScenarioHandleContext& ctx, const THttpProxyRequest& bassRequest);

} // namespace NAlice::NHollywood

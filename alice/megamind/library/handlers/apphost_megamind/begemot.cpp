#include "begemot.h"

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/request_builder.h>
#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/begemot/begemot.h>
#include <alice/megamind/library/context/wizard_response.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/util/status.h>
#include <alice/megamind/library/worldwide/language/is_alice_worldwide_language.h>

#include <alice/begemot/lib/api/experiments/flags.h>
#include <alice/library/experiments/experiments.h>
#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/compiler/source_text_collection.h>

#include <search/begemot/rules/alice/response/proto/alice_response.pb.h>

namespace NAlice::NMegamind {
namespace {

void AddNativeRequestToAppHostContext(IAppHostCtx& ahCtx, const IContext& ctx, const ELanguage language,
                                         const NJson::TJsonValue& request, const TStringBuf ahItemBegemotNativeRequestName) {
    auto nativeRequest = SetRequiredRules(request, ctx, language);
    LOG_DEBUG(ahCtx.Log()) << "AppHost item '" << ahItemBegemotNativeRequestName << "' content '" << nativeRequest << '\'';
    ahCtx.ItemProxyAdapter().PutJsonIntoContext(std::move(nativeRequest), ahItemBegemotNativeRequestName);
}

void LogRequestCgiParams(TRTLogger& logger, const IContext& ctx, const TString& utterance, const ELanguage language,
                   const TStringBuf ahItemRequestName) {
    TAppHostHttpProxyMegamindRequestBuilder requestBuilder;
    CreateBegemotRequest(utterance, language, ctx, requestBuilder);
    auto& item = requestBuilder.CreateRequest();
    LOG_DEBUG(logger) << "AppHost Http proxy item '" << ahItemRequestName << "' path '" << item.GetPath() << '\'';
}

} // namespace

namespace NImpl {

ELogPriority GetLogPolicy(bool logPolicyInfo) {
    if (logPolicyInfo)
        return TLOG_INFO;
    else
        return TLOG_DEBUG;
}

} // namespace NImpl

TStatus AppHostBegemotSetup(IAppHostCtx& ahCtx, const TString& utterance, const IContext& ctx) {
    TSourcePrepareStatus result = ESourcePrepareType::NotNeeded;

    const ELanguage language = ConvertAliceWorldWideLanguageToOrdinar(ctx.Language());
    NJson::TJsonValue request(NJson::JSON_MAP);
    result = CreateNativeBegemotRequest(utterance, language, ctx, request);

    if (result.IsSuccess()) {
        AddNativeRequestToAppHostContext(ahCtx, ctx, language, request, AH_ITEM_BEGEMOT_NATIVE_REQUEST_NAME);
        ahCtx.ItemProxyAdapter().PutJsonIntoContext(NJson::TJsonValue(request), AH_ITEM_BEGEMOT_NATIVE_BEGGINS_REQUEST_NAME);
        ahCtx.ItemProxyAdapter().PutJsonIntoContext(CreateBegemotMergerRequest(request, ctx), AH_ITEM_BEGEMOT_MERGER_REQUEST);
    }

    // for debugging log cgi request
    LogRequestCgiParams(ahCtx.Log(), ctx, utterance, language, AH_ITEM_BEGEMOT_NATIVE_REQUEST_NAME);
    return result.Status();
}

TStatus AppHostPolyglotBegemotSetup(IAppHostCtx& ahCtx, const TString& utterance, const IContext& ctx) {
    if (utterance.empty()) {
        return TError{TError::EType::Logic} << "Couldn't fetch wizard response: utterance is empty";
    }

    const ELanguage language = ctx.Language();
    NJson::TJsonValue request(NJson::JSON_MAP);
    TSourcePrepareStatus result = CreateNativeBegemotRequest(utterance, language, ctx, request);

    if (result.IsSuccess()) {
        AddNativeRequestToAppHostContext(ahCtx, ctx, language, request, AH_ITEM_POLYGLOT_BEGEMOT_NATIVE_REQUEST_NAME);
        ahCtx.ItemProxyAdapter().PutJsonIntoContext(NJson::TJsonValue(request), AH_ITEM_POLYGLOT_BEGEMOT_NATIVE_BEGGINS_REQUEST_NAME);
        ahCtx.ItemProxyAdapter().PutJsonIntoContext(CreateBegemotMergerRequest(request, ctx), AH_ITEM_POLYGLOT_BEGEMOT_MERGER_REQUEST);
        ahCtx.ItemProxyAdapter().PutJsonIntoContext(CreatePolyglotBegemotMergerMergerRequest(request), AH_ITEM_POLYGLOT_BEGEMOT_MERGER_MERGER_REQUEST);
    }
    // for debugging log cgi request
    LogRequestCgiParams(ahCtx.Log(), ctx, utterance, language, AH_ITEM_POLYGLOT_BEGEMOT_NATIVE_REQUEST_NAME);
    return result.Status();
}

TStatus AppHostBegemotPostSetup(IAppHostCtx& ahCtx, const IContext& ctx, TWizardResponse& wizardResponse) {
    NBg::NProto::TAlicePolyglotMergeResponseResult responseProto;
    if (auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_BEGEMOT_ALICE_POLYGLOT_MERGER_RESPONSE_NAME, responseProto)) {
        LOG_WARN(ahCtx.Log()) << "Unable to find item: " << AH_ITEM_BEGEMOT_ALICE_POLYGLOT_MERGER_RESPONSE_NAME;
        return std::move(*err);
    }

    if (auto err = ParseBegemotAliceResponse(std::move(responseProto), ctx.HasExpFlag(EXP_BEGEMOT_GRANET_LOG)).MoveTo(wizardResponse)) {
        LOG_WARN(ahCtx.Log()) << "Unable to parse native begemot response";
        return std::move(*err);
    }

    wizardResponse.DumpQualityInfo(ctx.Logger(), NImpl::GetLogPolicy(ctx.HasExpFlag(EXP_SHOW_WIZARD_LOGS)));

    LOG_WITH_TYPE(ahCtx.Log(), ctx.HasExpFlag(EXP_LOG_BEGEMOT_RESPONSE) ? TLOG_INFO : TLOG_DEBUG, ELogMessageType::MegamindPreClasification)
        << TLogMessageTag{"Begemot response"}
        << "Native begemot response: " << wizardResponse.RawResponse();

    // TODO (petrk) Possibly add apphost item.

    return Success();
}


void AppHostBegemotResponseRewrittenRequestSetup(IAppHostCtx& ahCtx, const TString& rewrittenRequest) {
    NMegamindAppHost::NBegemotResponseParts::TRewrittenRequest rewrittenRequestProto;
    rewrittenRequestProto.SetData(rewrittenRequest);
    ahCtx.ItemProxyAdapter().PutIntoContext(rewrittenRequestProto, AH_ITEM_BEGEMOT_RESPONSE_REWRITTEN_REQUEST_PART);
}

TStatus AppHostBegemotResponseRewrittenRequestPostSetup(
    IAppHostCtx& ahCtx,
    NMegamindAppHost::NBegemotResponseParts::TRewrittenRequest& rewrittenRequestProto
) {
    if (auto err = GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_BEGEMOT_RESPONSE_REWRITTEN_REQUEST_PART, rewrittenRequestProto)) {
        LOG_WARN(ahCtx.Log()) << "Unable to find item: " << AH_ITEM_BEGEMOT_RESPONSE_REWRITTEN_REQUEST_PART;
        return std::move(*err);
    }
    return Success();
}

} // namespace NAlice::NMegamind

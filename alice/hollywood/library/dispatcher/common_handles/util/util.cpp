#include "util.h"

#include <library/cpp/uri/uri.h>


namespace NAlice::NHollywood {

TString GetPath(const NJson::TJsonValue& httpRequest) {
    const auto& uriString = httpRequest[URI_KEY].GetString();

    // TODO(vitvlkv): support other than http/https schemes...
    NUri::TUri uri;
    Y_ENSURE(uri.Parse(uriString) == NUri::TUri::EParsed::ParsedOK);
    return TString{uri.GetField(NUri::TUri::TField::FieldPath)};
}

NJson::TJsonValue GetAppHostParams(const NAppHost::IServiceContext& ctx) {
    return ctx.GetOnlyItem(NAppHost::APP_HOST_PARAMS_TYPE);
}

NScenarios::TRequestMeta GetMeta(const NAppHost::IServiceContext& ctx, TRTLogger& logger) {
    const auto items = ctx.GetProtobufItemRefs(REQUEST_META_ITEM);

    if (items.empty()) {
        LOG_ERR(logger) << "This is an exceptional behaviour, meta existance should be enforced by graph config."
                        << "Most likely you forgot to add UNPACK_HTTP_* dependency to your node."
                        << "Apphost RequestId: " << GetGuidAsString(ctx.GetRequestID()) << ", Apphost RUID: " << ctx.GetRUID()
                        << " Apphost DebugInfo: " << ctx.GetDebugInfo() << " Location: " << ctx.GetLocation().Path;
        // Since this is exceptional behaviour, we will still throw an exception
        ythrow yexception() << "meta is missing, most likely you forgot to add UNPACK_HTTP_* dependency to your node.";
    }

    const auto rawMeta = items.front().Raw();
    return ParseProto<NScenarios::TRequestMeta>(rawMeta);
}

TRTLogger CreateLogger(TGlobalContext& globalContext, const TString& token) {
    return globalContext.CreateLogger(token);
}

TString GetRTLogToken(const NJson::TJsonValue& appHostParams, const ui64 ruid) {
    const TString& baseRTLogToken = appHostParams["reqid"].GetString();

    return TStringBuilder{} << baseRTLogToken << '-' << ruid;
}

void UpdateMiscHandleSensors(IGlobalContext& globalContext, const EMiscHandle handle,
                             const ERequestResult requestResult, const i64 timeMs) {
    globalContext.Sensors().AddHistogram(MiscResponseTime(handle, requestResult), timeMs, NMetrics::TIME_INTERVALS);
    globalContext.Sensors().AddHistogram(MiscResponseTime(handle, ERequestResult::TOTAL), timeMs, NMetrics::TIME_INTERVALS);

    globalContext.Sensors().IncRate(MiscResponse(handle, requestResult));
    globalContext.Sensors().IncRate(MiscResponse(handle, ERequestResult::TOTAL));
}

} // namespace NAlice::NHollywood

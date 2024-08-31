#include "runner.h"

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <search/app_host_ops/parser.h>
#include <apphost/lib/converter/converter.h>

namespace NBASS {

namespace {
constexpr TStringBuf SERVICE_RESPONSE = "service_response";
}

TAppHostRunner::TAppHostRunner(const TSourceRequestFactory& appHostGraph)
    : GraphSourceFactory(appHostGraph)
{
}

TAppHostResultContext TAppHostRunner::Fetch(const TAppHostInitContext& appHostContext) {
    NHttpFetcher::TRequestPtr request = GraphSourceFactory.Request();
    request->AddHeader(TStringBuf("Content-Type"), TStringBuf("application/json"));
    request->SetBody(appHostContext.GetInitData().ToJson(), TStringBuf("POST"));

    NHttpFetcher::THandle::TRef handle = request->Fetch();
    if (!handle) {
        return TAppHostResultContext(NSc::Null());
    }
    NHttpFetcher::TResponse::TRef response = handle->Wait();
    if (response->IsError()) {
        LOG(ERR) << "Apphost error: " << response->GetErrorText() << Endl;
        return TAppHostResultContext(NSc::Null());
    }

    NAppHost::NConverter::TConverterFactory converterFactory;
    auto converter = converterFactory.Create(SERVICE_RESPONSE);

    NAppHost::NCompression::TCodecs codecs;
    auto converterResult = converter->ConvertFromJSON(response->Data,
                                                      codecs.SelectBest());

    const TString appHostOpsResult = NAppHostOps::ConvertToStringJSON(converterResult, SERVICE_RESPONSE);

    const NSc::TValue data = NSc::TValue::FromJson(appHostOpsResult);
    return TAppHostResultContext(data);
}

} // NBASS

#include "service_with_eventlog.h"
#include "callbacks_with_eventlog.h"

using namespace NAlice::NTts;

TAtomicCounter TServiceWithEventlog::TRequestProcessor::NextProcNumber_ = 0;

TServiceWithEventlog::TServiceWithEventlog()
    : TService()
{}

TServiceWithEventlog::TRequestProcessor::TRequestProcessor(TService& service, NAlice::NCuttlefish::TLogContext&& logContext)
    : TService::TRequestProcessor(service)
    , LogContext_(std::move(logContext))
    , Number_(NextProcNumber_.Inc())
{
    LogContext_.LogEventInfoCombo<NEvClass::AppHostProcessor>(Number_);
}

TIntrusivePtr<TInterface::TCallbacks> TServiceWithEventlog::TRequestProcessor::CreateTtsCallbacks() {
    return new TCallbacksWithEventlog(
        RequestHandler_,
        NAlice::NCuttlefish::TLogContext(NCuttlefish::SpawnLogFrame(), LogContext_.RtLogPtr(), LogContext_.Options()),  // callback work in another thread so has own frame
        Number_
    );
}

void TServiceWithEventlog::TRequestProcessor::OnBackendRequest(const NProtobuf::TBackendRequest& backendRequest, const TStringBuf& itemType) {
    LogContext_.LogEventInfoCombo<NEvClass::TRecvFromAppHostTtsBackendRequest>(NCuttlefish::GetCensoredTtsRequestStr(backendRequest), TString(itemType));
    TService::TRequestProcessor::OnBackendRequest(backendRequest, itemType);
}

void TServiceWithEventlog::TRequestProcessor::OnAppHostEmptyInput() {
    LogContext_.LogEventInfoCombo<NEvClass::AppHostEmptyInput>();
    TService::TRequestProcessor::OnAppHostEmptyInput();
}

void TServiceWithEventlog::TRequestProcessor::OnAppHostClose() {
    LogContext_.LogEventInfoCombo<NEvClass::ProcessCloseProcessor>();
    TService::TRequestProcessor::OnAppHostClose();
}

void TServiceWithEventlog::TRequestProcessor::OnUnknownItemType(const TString& tag, const TString& type) {
    LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>(TStringBuilder() << TStringBuf("unknown item tag=") << tag << TStringBuf(", type=") << type);
    TService::TRequestProcessor::OnUnknownItemType(tag, type);
}

void TServiceWithEventlog::TRequestProcessor::OnWarning(const TString& text) {
    LogContext_.LogEventInfoCombo<NEvClass::WarningMessage>(text);
    TService::TRequestProcessor::OnWarning(text);
}

void TServiceWithEventlog::TRequestProcessor::OnError(const TString& text) {
    LogContext_.LogEventErrorCombo<NEvClass::ErrorMessage>(text);
    TService::TRequestProcessor::OnError(text);
}

TIntrusivePtr<TService::TRequestProcessor> TServiceWithEventlog::CreateProcessor(NAppHost::IServiceContext& ctx, NAlice::NCuttlefish::TRtLogClient* rtLogClient) {
    return new TRequestProcessor(*this, NAlice::NCuttlefish::LogContextFor(ctx, rtLogClient));
}

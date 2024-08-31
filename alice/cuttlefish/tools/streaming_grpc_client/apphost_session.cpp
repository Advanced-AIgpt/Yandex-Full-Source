#include "apphost_session.h"

#include <alice/cuttlefish/library/logging/dlog.h>

namespace {

template <typename PromiseT, typename ValueT>
void SetValueAndReset(PromiseT& promise, ValueT&& value)
{
    PromiseT tmp;
    promise.Swap(tmp);
    tmp.SetValue(std::forward<ValueT>(value));
}

}  // anonymous namespace

//-------------------------------------------------------------------------------------------------
TIntrusivePtr<TAppHostSession> TAppHostSession::Start(NAppHost::NTransport::NGrpc::TGrpcPool& grpcPool, const TString& addr, bool secure)
{
    TIntrusivePtr<TAppHostSession> invoke(new TAppHostSession);
    DLOG("Constructed new TAppHostSession");

    grpcPool.Invoke(TIntrusivePtr<NAppHost::NGrpc::NClient::TChannel::TInvokeBase>(invoke.Get()), addr, secure);

    return invoke;
}

NThreading::TFuture<TAppHostSession::TMaybeServiceResponse> TAppHostSession::Read()
{
    if (!TInvokeBase::IsReady()) {
        ythrow yexception() << "session is not ready";
    }
    if (ResponsePromise.Initialized()) {
        ythrow yexception() << "duplicated read operation";
    }

    ResponsePromise = NThreading::NewPromise<TMaybeServiceResponse>();
    auto responseFut = ResponsePromise.GetFuture();

    TInvokeBase::ReadRaw();

    return responseFut;
}

NThreading::TFuture<bool> TAppHostSession::Send(TMaybeRequest&& request)
{
    if (!TInvokeBase::IsReady()) {
        ythrow yexception() << "session is not ready";
    }
    if (WritePromise.Initialized()) {
        ythrow yexception() << "duplicated write operation";
    }

    WritePromise = NThreading::NewPromise<bool>();
    auto writeFut = WritePromise.GetFuture();

    if (request.Defined()) {
        DLOG("Send TSessionRequest...");
        TInvokeBase::Write(std::move(*request), false);
    } else {
        TInvokeBase::WritesDone();
    }

    return writeFut;
}

void TAppHostSession::HandleWrite(bool ok)
{
    SetValueAndReset(WritePromise, ok);
}

void TAppHostSession::HandleWritesDone(bool ok)
{
    SetValueAndReset(WritePromise, ok);
}

void TAppHostSession::DoHandleRead(bool ok)
{
    if (ok) {
        SetValueAndReset(ResponsePromise, std::move(TInvokeBase::Response));
    } else {
        SetValueAndReset(ResponsePromise, Nothing());
    }
}

void TAppHostSession::HandleReady(bool ok)
{
    IsReadyPromise.SetValue(ok);
}

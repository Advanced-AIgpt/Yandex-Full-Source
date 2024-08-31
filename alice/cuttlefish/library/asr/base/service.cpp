#include "service.h"

#include <alice/cuttlefish/library/apphost/item_parser.h>

using namespace NAlice::NAsr;

// debug helpers
template <typename T>
inline void AppHostDumpItems(const T& items, TStringBuf title, IOutputStream& out) {
    for (auto it = items.begin(), end = items.end(); it != end; ++it) {
        out << title << ": " << it.GetTag() << " " << it.GetType() << Endl;
    }
}

inline void AppHostDump(NAppHost::IServiceContext& ctx, IOutputStream& out) {
    AppHostDumpItems(ctx.GetRawInputItemRefs(), "Raw", out);
    AppHostDumpItems(ctx.GetItemRefs(NAppHost::EContextItemSelection::Anything), "Json", out);
    AppHostDumpItems(ctx.GetProtobufItemRefs(NAppHost::EContextItemSelection::Anything), "Proto", out);
}

void TService::TRequestProcessor::ProcessInput(TRequestHandlerPtr rh) {
    // Cout << TInstant::Now() << " APP_HOST_SERVICE_INPUT_DUMP:" << Endl;
    // AppHostDump(*ctx, Cout);
    // Cout << Endl;
    RequestHandler_ = std::move(rh);
    ProcessInputImpl();
}

void TService::TRequestProcessor::ProcessInputImpl() {
    {
        if (!RequestHandler_->TryBeginProcessing()) {
            // processing finished
            return;
        }

        OnBeginProcessInput();
        auto items = RequestHandler_->Context().GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
        for (auto it = items.begin(); it != items.end();) {
            if (!OnAppHostProtoItem(TString(it.GetType()), *it)) {
                OnAppHostClose();
                RequestHandler_->EndProcessing();
                RequestHandler_->Finish();
                return;
            }

            ++it;
        }
        RequestHandler_->EndProcessing();
    }

    TIntrusivePtr<TRequestProcessor> self(this);
    RequestHandler_->Context().NextInput().Apply([processor = std::move(self)](auto hasData) mutable {
        if (!hasData.GetValue()) {
            processor->OnAppHostEmptyInput();
            processor->OnAppHostClose();
            return;
        }

        processor->ProcessInputImpl();
    });
}

// return false if processing request finished
bool TService::TRequestProcessor::OnAppHostProtoItem(
    const TString& type,
    const NAppHost::NService::TProtobufItem& item
) {
    if (type.StartsWith("audio")) {
        try {
            NAliceProtocol::TAudio audio;
            NCuttlefish::ParseProtobufItem(item, audio);
            return OnCuttlefishAudio(audio);
        } catch (...) {
            OnError(CurrentExceptionMessage());
            return false;
        }
    }
    return true; // continue (wait next input)
}

bool TService::TRequestProcessor::OnCuttlefishAudio(const NAliceProtocol::TAudio& audio) {
    NProtobuf::TRequest request;
    if (audio.HasAsrInitRequest()) {
        if (HasInitRequest_) {
            OnWarning("got more than one InitRequests (ignore extra)");
            return true;
        }

        HasInitRequest_ = true;
        auto& initRequest = audio.GetAsrInitRequest();
        TString requestId;
        if (initRequest.HasRequestId()) {
            requestId = initRequest.GetRequestId();
        }
        SpotterStream_ = initRequest.HasHasSpotterPart() && initRequest.GetHasSpotterPart();
        TIntrusivePtr<NAsr::TInterface::TCallbacks> callbacks = CreateAsrCallbacks(RequestHandler_, requestId);
        try {
            *request.MutableInitRequest() = initRequest;
            return OnInitRequest(request, callbacks, requestId);
        } catch (...) {
            OnError(CurrentExceptionMessage());
            return false;
        }
    }

    if (audio.HasEndSpotter()) {
        SpotterStream_ = false;
        return true;
    } else if (audio.HasEndStream()) {
        HasEndOfStream_ = true;
        request.MutableEndOfStream();
    } else if (audio.HasChunk()) {
        if (!HasInitRequest_) {
            OnWarning("got unexpected audio chunk message (before init request)");
            return false;
        }

        if (!audio.GetChunk().HasData()) {
            return true;  // ignore empty chunk
        }

        auto& addData = *request.MutableAddData();
        addData.SetSpotterChunk(SpotterStream_);
        addData.SetAudioData(audio.GetChunk().GetData());
    } else {
        return true;  // ignore unknow audio messages
    }

    if (Asr_) {
        return Asr_->ProcessAsrRequest(request);
    }

    return false;
}

void TService::TRequestProcessor::OnAppHostClose() {
    if (!Asr_) {
        RequestHandler_->Finish();
        return;
    }

    if (!HasEndOfStream_ || RequestHandler_->Context().IsCancelled()) {
        // lost input (apphost) stream before reach end of stream (intepret it as cancel request)
        Asr_->Close();
    } else {
        // need more time for finish recognition
        Asr_->SetNeedFinish();
        Asr_.Reset();
    }
}

void TService::TRequestProcessor::OnError(const TString& error) {
    if (!HasInitRequest_) {
        // can not send response for unknown request(format), only log problem
        OnWarning(error);
        return;
    }

    if (Asr_) {
        // for sending responses to apphost can be used another thread, so for avoid race we MUST entrust sending error to asr impl.
        Asr_->CauseError(error);
    } else {
        OnWarning(error);
    }
}

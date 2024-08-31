#include "service.h"

#include <alice/cuttlefish/library/apphost/item_parser.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cachalot/api/protos/cachalot.pb.h>

#include <util/string/builder.h>

#undef DLOG
#define DLOG(msg)

using namespace NAlice::NYabio;
using namespace NAlice::NCuttlefish;

namespace {
    void FillInitRequestContext(NProtobuf::TInitRequest& req, NProtobuf::TContext& ctx, bool hasData) {
        if (hasData && ctx.has_group_id()) {
            DLOG("FillInitRequestContext use data");
            req.Mutablecontext()->Swap(&ctx);
        } else {
            // init empty context
            DLOG("FillInitRequestContext init empty context: group_id=" << req.group_id());
            req.Mutablecontext()->set_group_id(req.group_id());
        }
    }
}

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

        auto items = RequestHandler_->Context().GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);
        for (auto it = items.begin(); it != items.end();) {
            if (!OnAppHostProtoItem(TString(it.GetType()), *it)) {
                OnAppHostClose();
                RequestHandler_->EndProcessing();
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
    bool typeAudio = false;
    if (type == ITEM_TYPE_BIO_CLASSIFY_AUDIO) {
        typeAudio = true;
        EnabledBiometryMethod_ = MethodClassify;
    } else if (type == ITEM_TYPE_BIO_SCORE_AUDIO) {
        typeAudio = true;
        EnabledBiometryMethod_ = MethodScore;
    } else if (type == ITEM_TYPE_AUDIO) {
        typeAudio = true;
    }

    if (typeAudio) {
        try {
            NAliceProtocol::TAudio audio;
            ParseProtobufItem(item, audio);
            return OnCuttlefishAudio(audio);
        } catch (...) {
            OnError(NProtobuf::RESPONSE_CODE_BAD_REQUEST, CurrentExceptionMessage());
            return false;
        }
    } else if (type == ITEM_TYPE_YABIO_CONTEXT_RESPONSE) {
        if (HasContextResponse_) {
            OnWarning("got more than one YabioContext (ignore extra)");
            return true;
        }

        try {
            NCachalotProtocol::TYabioContextResponse resp;
            ParseProtobufItem(item, resp);
            HasContextResponse_ = true;

            if (resp.HasError()) {
                TStringStream err;
                err << "fail load yabio context: " << resp.GetError().GetText() << " status=" << int(resp.GetError().GetStatus());
                OnError(NProtobuf::RESPONSE_CODE_INTERNAL_ERROR, err.Str());
                return TryBeginScore();  // send error response here if can
            } else if (resp.HasSuccess()) {
                auto& success = resp.GetSuccess();
                return OnYabioContext(success.GetOk(), success.GetContext());
            } else { // else ignore unknown response
                return true;
            }
        } catch (...) {
            OnError(NProtobuf::RESPONSE_CODE_INTERNAL_ERROR, CurrentExceptionMessage());
            return TryBeginScore();  // send error response here if can
        }

    } else if (type == ITEM_TYPE_ASR_FINISHED) {
        try {
            NAliceProtocol::TAsrFinished asrFinished;
            ParseProtobufItem(item, asrFinished);
            OnAsrFinished(asrFinished);
            return false;   // asr_finished immediatly break request execution
        } catch (...) {
            OnError(NProtobuf::RESPONSE_CODE_INTERNAL_ERROR, CurrentExceptionMessage());
            return TryBeginScore();  // send error response here if can
        }
    } // else ignore unknown item

    return true; // continue (wait next input)
}

bool TService::TRequestProcessor::OnCuttlefishAudio(NAliceProtocol::TAudio& audio, bool postponed) {
    (void)postponed;
    if (PostponedMode_) {
        InputQueue_.emplace(new NAliceProtocol::TAudio(std::move(audio)));
        return true;
    }

    if (audio.HasYabioInitRequest()) {
        if (audio.HasBeginStream()) {
            auto& beginStream = audio.GetBeginStream();
            if (!Mime_ && beginStream.HasMime() && beginStream.GetMime()) {
                Mime_ = beginStream.GetMime();
            }
        }
        auto& initRequest = *audio.MutableYabioInitRequest();
        if (EnabledBiometryMethod_ == MethodClassify) {
            if (initRequest.method() != NProtobuf::METHOD_CLASSIFY) {
                OnIgnoreInitRequest(initRequest, "enabled only method=classify");
                return true;
            }
        } else if (EnabledBiometryMethod_ == MethodScore) {
            if (initRequest.method() != NProtobuf::METHOD_SCORE) {
                OnIgnoreInitRequest(initRequest, "enabled only method=score");
                return true;
            }
        }
        if (HasInitRequest_) {
            OnWarning("got more than one InitRequests (ignore extra)");
            return true;
        }

        return OnRecvInitRequest(initRequest);
    }

    NProtobuf::TAddData addData;
    addData.SetlastChunk(false);
    if (audio.HasBeginStream()) {
        auto& beginStream = audio.GetBeginStream();
        if (!Mime_ && beginStream.HasMime() && beginStream.GetMime()) {
            Mime_ = beginStream.GetMime();
        }
        return true;
    } else if (audio.HasEndSpotter() && SpotterStream_) {
        SpotterStream_ = false;
        addData.SetlastSpotterChunk(true);
    } else if (audio.HasEndStream()) {
        HasEndOfStream_ = true;
        DLOG("TODO: SetLastChunk");
        addData.SetlastChunk(true);
    } else if (audio.HasChunk()) {
        if (!HasInitRequest_) {
            OnWarning("got unexpected audio chunk message (before init request)");
            return false;
        }

        if (!audio.GetChunk().HasData()) {
            return true;  // ignore empty chunk
        }

        addData.SetaudioData(audio.GetChunk().GetData());
    } else {
        return true;  // ignore unknow audio messages
    }

    if (Yabio_) {
        DLOG("TODO: TService::TRequestProcessor::OnCuttlefishAudio HAS_EOS=" << int(HasEndOfStream_));
        return Yabio_->ProcessAddData(addData);
    }
    DLOG("TService::TRequestProcessor::OnCuttlefishAudio FINISH");
    return false;
}

bool TService::TRequestProcessor::OnYabioContext(bool hasData, const TString& data) {
    DLOG("bio context=" << data.Size());
    if (hasData) {
        DLOG("OnYabioContext hasData");
        NProtobuf::TContext context;
        try {
            if (!context.ParseFromString(data)) {
                throw yexception() << "fail parse yabio protobuf context";
            }
        } catch (...) {
            TStringStream err;
            err << "fail parse yabio context: " << CurrentExceptionMessage();
            OnError(NProtobuf::RESPONSE_CODE_INTERNAL_ERROR, err.Str());
            return TryBeginScore();
        }

        Context_.Swap(&context);
        HasContextData_ = true;
    }
    return TryBeginScore();
}

void TService::TRequestProcessor::OnAsrFinished(const NAliceProtocol::TAsrFinished&) {
    DLOG("OnAsrFinished y=" << int(bool(Yabio_)) << " eos=" << int(HasEndOfStream_));
    if (Yabio_) {
        if (!HasEndOfStream_) {
            DLOG("close on asr_finished [1]");
            HasEndOfStream_ = true;
            Yabio_->Close();
        }
    }
}

bool TService::TRequestProcessor::TryBeginScore() {
    if (HasInitRequest_ && PosponedInitRequestError_ != NProtobuf::RESPONSE_CODE_OK && !Yabio_) {
        // already has some error, so use it
        SendFastInitResponseError(PosponedInitRequestError_);
        return false;
    }

    if (HasContextResponse_ && HasInitRequest_) {
        DLOG("HasContextData_=" << int(HasContextData_));
        FillInitRequestContext(InitRequest_, Context_, HasContextData_);
        DLOG("TryBeginScore() ....");
        PostponedMode_ = false;
        if (!OnInitRequest()) {
            DLOG("TryBeginScore() !initreq");
            return false;
        }
        // process InputQueue
        while (InputQueue_.size()) {
            if (NAliceProtocol::TAudio* audioPtr = dynamic_cast<NAliceProtocol::TAudio*>(InputQueue_.front().get())) {
                if (!OnCuttlefishAudio(*audioPtr, /* postponed= */ true)) {
                    DLOG("TryBeginScore() !cutaudio");
                    return false;
                }
            }
            InputQueue_.pop();
        }
    }
    return true;
}

bool TService::TRequestProcessor::OnRecvInitRequest(NProtobuf::TInitRequest& initRequest) {
    InitRequest_.Swap(&initRequest);
    HasInitRequest_ = true;
    if (InitRequest_.method() == NProtobuf::METHOD_SCORE) {
        if (!HasContextResponse_) {
            PostponedMode_ = true;
            return true;
        }
        return TryBeginScore();
    } else if (InitRequest_.method() == NProtobuf::METHOD_CLASSIFY) {
        // continue (not need wait anything)
    } else {
        OnError(NProtobuf::RESPONSE_CODE_BAD_REQUEST, TStringBuilder() << "unkown yabio method=" << int(initRequest.method()));
        return false;
    }
    return OnInitRequest();
}

bool TService::TRequestProcessor::OnInitRequest() {
    TString messageId;
    if (InitRequest_.HasMessageId()) {
        messageId = InitRequest_.GetMessageId();
    }
    SpotterStream_ = InitRequest_.has_spotter() && InitRequest_.spotter();
    TIntrusivePtr<NYabio::TInterface::TCallbacks> callbacks = CreateYabioCallbacks(messageId);
    try {
        if (Mime_ && !InitRequest_.has_mime()) {
            // use mime from begin stream
            InitRequest_.set_mime(Mime_);
        }
        return OnInitRequest(callbacks, messageId);
    } catch (...) {
        OnError(NProtobuf::RESPONSE_CODE_INTERNAL_ERROR, CurrentExceptionMessage());
        return false;
    }
}

bool TService::TRequestProcessor::OnInitRequest(TIntrusivePtr<NYabio::TInterface::TCallbacks> callbacks, const TString& requestId) {
    (void)requestId; // can be used for mark log records
    // by default create fake yabio (emulator for testing/debug)
    Yabio_.Reset(new NYabio::TFake(callbacks));
    return Yabio_->ProcessInitRequest(InitRequest_);
}

void TService::TRequestProcessor::OnAppHostEmptyInput() {
    if (Yabio_) {
        if (InitRequest_.method() == NProtobuf::METHOD_SCORE && !HasContextResponse_) {
            Yabio_->Close();
        } else {
            Yabio_->SoftClose();
        }
    } else {
        RequestHandler_->Finish();
    }
}

void TService::TRequestProcessor::OnAppHostClose() {
    if (Yabio_) {
        Yabio_->Close();
    } else {
        RequestHandler_->Finish();
    }
}

void TService::TRequestProcessor::OnError(NProtobuf::EResponseCode responseCode, const TString& error) {
    if (!HasInitRequest_) {
        // can not send response for unknown request(format), only log problem
        OnWarning(error);
        if (PosponedInitRequestError_ == NProtobuf::RESPONSE_CODE_OK) {
            PosponedInitRequestError_ = responseCode;
        }
        return;
    }

    if (Yabio_) {
        // for sending responses to apphost can be used another thread, so for avoid race we MUST entrust sending error to yabio impl.
        Yabio_->CauseError(responseCode, error);
    } else {
        OnWarning(error);
    }
}

void TService::TRequestProcessor::SendFastInitResponseError(NProtobuf::EResponseCode responseCode) {
    NProtobuf::TResponse response;
    auto& initResponse = *response.MutableInitResponse();
    NProtobuf::FillRequiredDefaults(initResponse);
    initResponse.SetresponseCode(responseCode);
    RequestHandler_->Context().AddProtobufItem(response, ITEM_TYPE_YABIO_PROTO_RESPONSE);
    RequestHandler_->Context().Flush();
}

#include "music_match_client.h"

#include <util/string/builder.h>

using namespace NAlice::NMusicMatchAdapter;

namespace {
    static const auto GetClassifyingMusicResult = []() {
        NJson::TJsonValue result = NJson::TJsonMap();
        result["result"] = "music";

        return result;
    };
    static const auto GetClassifyingNotMusicResult = []() {
        NJson::TJsonValue result = NJson::TJsonMap();
        result["result"] = "not-music";

        return result;
    };

    static constexpr TStringBuf CLASSIFYING_DIRECTIVE_NAME = "classifying";
    static const TString CLASSIFYING_MUSIC_RESULT = GetClassifyingMusicResult().GetStringRobust();
    static const TString CLASSIFYING_NOT_MUSIC_RESULT = GetClassifyingNotMusicResult().GetStringRobust();


    NAlice::NMusicMatch::NProtobuf::TStreamResponse CreateStreamResponse(const TString& rawJsonString, bool isFinish) {
        NAlice::NMusicMatch::NProtobuf::TStreamResponse response;
        NAlice::NMusicMatch::NProtobuf::TMusicResult* musicResult = response.MutableMusicResult();

        musicResult->SetIsOk(true);
        musicResult->SetIsFinish(isFinish);
        musicResult->SetRawMusicResultJson(rawJsonString);

        return response;
    }

    // Сopied as is from https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/musicstream/__init__.py?rev=r7723675#L22
    static constexpr size_t MAX_WS_MESSAGE_SIZE = 32768;

} // namespace

TMusicMatchClient::TMusicMatchClient()
    : UnistatCounterGuard_(Unistat().MusicMatchClientCounter())
{
    Unistat().OnCreateMusicMatchClient();
}

TMusicMatchClient::~TMusicMatchClient() {
    if (FinishPromise_.Defined()) {
        FinishPromise_->SetValue();
    }

    // Order is important
    // Request may be canceled after success
    // and we want to get canceled status in such case
    if (RequestCanceled_) {
        Unistat().OnMusicMatchRequestCancel();
    } else if (RequestSucceeded_) {
        Unistat().OnMusicMatchRequestSuccess();
    } else {
        // No cancel or success == some errors occured
        Unistat().OnMusicMatchRequestError();
    }

    DLOG("TMusicMatchClient::~TMusicMatchClient");
}

void TMusicMatchClient::SendStreamRequest(const NMusicMatch::NProtobuf::TStreamRequest& streamRequest) {
    if (WebsocketClosed_) {
        DLOG("Websocket already closed but got extra AddData streamRequest");

        // TODO(chegoryu) log it?
        return;
    }

    auto self = TIntrusivePtr<TMusicMatchClient>(this);
    if (streamRequest.HasAddData()) {
        TString audioData = streamRequest.GetAddData().GetAudioData();

        DLOG("client.Post AddData");
        GetIOService().Post([self = std::move(self), audioData = std::move(audioData)]() mutable {
            if (self->WebsocketClosed_ || self->Finished_) {
                DLOG("client.Send AddData canceled: websocketClosed=" << (bool)self->WebsocketClosed_ << ", finished=" << (bool)self->Finished_);
                return;
            }

            Unistat().OnMusicMatchReceivedAudioData(audioData.size());
            if (self->AudioConverter_) {
                DLOG("client.Send AddData to audio converter size=" << audioData.size());
                self->AudioConverter_->Write(audioData.c_str(), audioData.size());
            } else {
                for (size_t i = 0; i < audioData.size(); i += MAX_WS_MESSAGE_SIZE) {
                    size_t currentSize = Min(MAX_WS_MESSAGE_SIZE, audioData.size() - i);
                    DLOG("client.Send AddData directrly to websocket size=" << currentSize);

                    Unistat().OnMusicMatchSendedAudioData(currentSize);
                    self->NVoicetech::TWsHandler::SendBinaryMessage(audioData.substr(i, currentSize));
                }
            }
        });
    } else {
        DLOG("Unexpected TStreamRequest type");
        // TODO(chegoryu) log it?
    }
}

void TMusicMatchClient::EnableAudioConverter(const TString& fromMime, const TString& toMime) {
    // Init buffer size
    AudioConverterBuffer_.resize(MAX_WS_MESSAGE_SIZE);

    NVoicetech::NASConv::TConverterOptions options;
    AudioConverter_ = MakeHolder<TAudioConverter>(
        fromMime,
        toMime,
        options,
        // WARNING
        // Here we have cyclic pointer dependence TMusicMatchClient -> TThreadedConverter -> OnOutputCallback -> TMusicMatchClient
        // So we use raw poiter to trigger client destruction
        // In client destructor we join audio converter thread => client lives all the time while converter is alive
        [client = (TMusicMatchClient*)this](bool isEof) mutable {
            DLOG("audioConverter new data isEof=" << isEof);
            client->SendDataFromAudioConverter(isEof);
        }
    );
}

void TMusicMatchClient::SafeCauseError(const TString& error) {
    if (WebsocketClosed_) {
        return;
    }

    DLOG("client.Post SafeCauseError: " << error)
    GetIOService().Post([self = TIntrusivePtr<TMusicMatchClient>(this), error = TString(error)]() mutable {
        if (self->WebsocketClosed_ || self->Finished_) {
            DLOG("client.Send SafeCauseError canceled: error=" << error << ", websocketClosed=" << (bool)self->WebsocketClosed_ << ", finished=" << (bool)self->Finished_);
            return;
        }
        DLOG("client.Send SafeCauseError: " << error)

        self->WebsocketClosed_ = true;
        self->OnAnyError(error);

        self->ProcessSendCloseMessage();
    });
}

void TMusicMatchClient::SafeClose() {
    RequestCanceled_ = true;

    if (WebsocketClosed_ || WebsocketCloseSended_) {
        return;
    }

    DLOG("client.Post SafeClose");
    GetIOService().Post([self = TIntrusivePtr<TMusicMatchClient>(this)]() mutable {
        if (self->WebsocketClosed_ || self->WebsocketCloseSended_) {
            DLOG("client.Send SafeClosed canceled: websocketClosed=" << (bool)self->WebsocketClosed_ << ", websocketCloseSended_=" << (bool)self->WebsocketCloseSended_);
            return;
        }
        DLOG("client.Send SafeClose");

        if (!self->Finished_) {
            DLOG("client.Send close before normal finished");
            self->Finished_ = true;
        }

        self->ProcessSendCloseMessage();
    });
}

void TMusicMatchClient::SetFinishPromise(NThreading::TPromise<void>& promise) {
    FinishPromise_ = promise;
}

void TMusicMatchClient::OnUpgradeResponse(const THttpParser& p, const TString& error) {
    TWsHandler::OnUpgradeResponse(p, error);
    if (!error) {
        DLOG("client.OnUpgradeResponse ok");
        NVoicetech::TWsHandler::OnUpgradeResponse(std::move(p), std::move(error));
        RequestUpgraded_ = true;

        {
            NMusicMatch::NProtobuf::TInitResponse initResponse;
            initResponse.SetIsOk(true);
            OnInitResponse(initResponse);
        }

    } else {
        DLOG("client.OnUpgradeResponse error=" << error);
        Unistat().OnMusicMatchWebsocketUpgradeResponseError();
        NVoicetech::TWsHandler::OnUpgradeResponse(std::move(p), std::move(error));

        WebsocketClosed_ = true;
        Finished_ = true;

        {
            NMusicMatch::NProtobuf::TInitResponse initResponse;
            initResponse.SetIsOk(false);
            initResponse.SetErrorMessage(TStringBuilder() << "RequestWebSocket failed with error=" << error);
            OnInitResponse(initResponse);
        }
    }
}

void TMusicMatchClient::OnTextMessage(const char* data, size_t size) {
    DLOG("client.OnTextMessage size=" << size);
    // TODO(chegoryu) logging
    try {
        NJson::TJsonValue responseJson;
        NJson::ReadJsonTree(TStringBuf(data, size), &responseJson, /*throwOnError = */ true);

        DLOG("client.OnTextMessage responseJson=" << responseJson.GetStringRobust());

        NJson::TJsonValue directiveName;
        if (!responseJson.GetValueByPath("directive.header.name", directiveName)) {
            Unistat().OnMusicMatchWebsocketAnswerParseError();
            OnFail("receive from Music API WebSocket event without directive.header.name");
            return;
        }

        if (directiveName.GetStringSafe() == CLASSIFYING_DIRECTIVE_NAME) {
            NJson::TJsonValue musicDetected;
            if (!responseJson.GetValueByPath("directive.payload.musicDetected", musicDetected)) {
                Unistat().OnMusicMatchWebsocketAnswerParseError();
                OnFail("receive from Music API WebSocket classifying event without directive.payload.musicDectected");
                return;
            }

            if (musicDetected.GetBooleanSafe()) {
                Unistat().OnMusicMatchClassifyingMusicResult();
                ProcessOnStreamResponse(CLASSIFYING_MUSIC_RESULT, false);
            } else {
                Unistat().OnMusicMatchClassifyingNotMusicResult();
                ProcessOnStreamResponse(CLASSIFYING_NOT_MUSIC_RESULT, true);
            }

            return;
        }

        NJson::TJsonValue result;
        if (!responseJson.GetValueByPath("directive.payload.result", result)) {
            Unistat().OnMusicMatchWebsocketAnswerParseError();
            OnFail("receive from Music API WebSocket event without directive.payload.result");
            return;
        }

        if (result.GetType() == NJson::EJsonValueType::JSON_MAP) {
            NJson::TJsonValue finalResult = NJson::TJsonMap();
            finalResult["result"] = "success";
            finalResult["data"] = result;
            Unistat().OnMusicMatchFinalResult();
            ProcessOnStreamResponse(finalResult.GetStringRobust(), true);
        } else {
            NJson::TJsonValue finalResult = NJson::TJsonMap();
            finalResult["result"] = result;
            Unistat().OnMusicMatchFinalResult();
            ProcessOnStreamResponse(finalResult.GetStringRobust(), true);
        }
    } catch (...) {
        Unistat().OnMusicMatchWebsocketAnswerParseError();
        OnFail(CurrentExceptionMessage());
    }
}

void TMusicMatchClient::OnBinaryMessage(const void* data, size_t size) {
    Y_UNUSED(data);
    DLOG("client.OnBinaryMessage size=" << size);

    Unistat().OnMusicMatchWebsocketAnswerParseError();
    OnFail(TStringBuilder() << "Music API WebSocket returned some binary data size=" << size);
}

void TMusicMatchClient::OnCloseMessage(ui16 code, const TString& reason) {
    DLOG("client.OnCloseMessage code=" << code << " reason=" << reason);

    if (WebsocketClosed_) {
        DLOG("client.OnCloseMessage websocket already closed");
        return;
    }

    if (!Finished_) {
        WebsocketClosed_ = true;
        Unistat().OnMusicMatchWebsocketCloseError();
        OnAnyError(TStringBuilder() << "receive from Music API WebSocket unexpected close event, code=" << code << " reason=" << reason);
        Finished_ = true;
    } else {
        WebsocketClosed_ = true;
        if (code != NRfc6455::CSNormal) {
            Unistat().OnMusicMatchWebsocketCloseError();
            OnAnyError(TStringBuilder() << "receive from Music API WebSocket bad close event, code=" << code << " reason=" << reason);
        } else {
            OnClosed();
        }
    }

    // Confirm receiving close in any case
    ProcessSendCloseMessage();
}

void TMusicMatchClient::OnError(const NVoicetech::TNetworkError& error) {
    WebsocketClosed_ = true;

    Unistat().OnMusicMatchWebsocketNetworkError();
    OnAnyError(TStringBuilder() << "network op=" << int(error.Operation) << " errno=" << error.Value() << ": " << error.Text());

    Finished_ = true;
    // Network error => there is no need to send close
}

void TMusicMatchClient::OnError(const NRfc6455::TWsError& error) {
    WebsocketClosed_ = true;

    Unistat().OnMusicMatchWebsocketWsError();
    OnAnyError(TStringBuilder() << "server WS error: code=" << error.Code << " (" << error.what() << ")");

    Finished_ = true;

    ProcessSendCloseMessage();
}

void TMusicMatchClient::OnError(const NVoicetech::TTypedError& error) {
    WebsocketClosed_ = true;

    Unistat().OnMusicMatchWebsocketTypedError();
    OnAnyError(error.Text);

    Finished_ = true;

    ProcessSendCloseMessage();
}


void TMusicMatchClient::ProcessOnStreamResponse(const TString& rawJsonResult, bool isFinish) {
    OnStreamResponse(CreateStreamResponse(rawJsonResult, isFinish));
    if (isFinish) {
        Finished_ = true;
        RequestSucceeded_ = true;
        ProcessSendCloseMessage();
    }
}

void TMusicMatchClient::ProcessSendCloseMessage() {
    if (!WebsocketCloseSended_) {
        DLOG("client.ProcessSendCloseMessage");
        WebsocketCloseSended_ = true;
        SendCloseMessage();
    } else {
        DLOG("client.ProcessSendCloseMessage already sended");
    }
}

void TMusicMatchClient::SendDataFromAudioConverter(bool isEof) {
    if (WebsocketClosed_) {
        if (!isEof) {
            DLOG("Websocket already closed but got extra data from audio converter");
        }
        return;
    }

    if (isEof) {
        DLOG("client.SendDataFromAudioConverter got eof");
        return;
    }

    DLOG("clinet.Post SendDataFromAudioConverter");
    GetIOService().Post([self = TIntrusivePtr<TMusicMatchClient>(this)]() mutable {
        if (self->WebsocketClosed_ || self->Finished_ || !self->AudioConverter_) {
            DLOG("client.Send SendDataFromAudioConverter canceled: websocketClosed="
                << (bool)self->WebsocketClosed_
                << ", finished=" << (bool)self->Finished_
                << ", audioConverter=" << (bool)self->AudioConverter_
            );
            return;
        }

        ssize_t currentSize = self->AudioConverter_->Read(&self->AudioConverterBuffer_[0], MAX_WS_MESSAGE_SIZE);
        DLOG("client.SendDataFromAudioConverter data size=" << currentSize);

        if (currentSize == -1) {
            TString converterError;
            if (!self->AudioConverter_->GetError(&converterError)) {
                converterError = "Unspecified error";
            }

            Unistat().OnMusicMatchAudioConverterError();
            self->OnFail(TStringBuilder() << "Audio converter error: " << converterError);
        } else if (currentSize == 0) {
            // This is valid case because IOService Post could be called several times with small chunks of data
            // but we read everything in the first Post request and then 0 bytes in all other requests
            DLOG("client.SendDataFromAudioConverter empty data");
        } else { // currentSize > 0
            Unistat().OnMusicMatchConvertedAudioData(currentSize);
            Unistat().OnMusicMatchSendedAudioData(currentSize);
            self->NVoicetech::TWsHandler::SendBinaryMessage(TString(TStringBuf(&self->AudioConverterBuffer_[0], currentSize)));

            // Extra data in Converter internal buffer
            if (currentSize == MAX_WS_MESSAGE_SIZE) {
                DLOG("client.SendDataFromAudioConverter currentSize == MAX_WS_MESSAGE_SIZE, call add extra SendDataFromAudioConverter");
                self->SendDataFromAudioConverter(false);
            }
        }
    });
}

void TMusicMatchClient::OnFail(const TString& error) {
    TIntrusivePtr<TMusicMatchClient> self(this);
    OnAnyError(error);

    if (!WebsocketClosed_) {
        Finished_ = true;
        WebsocketClosed_ = true;
        ProcessSendCloseMessage();
    }
}

TMusicMatchClient::TAudioConverter::TAudioConverter(
    const TString& fromMime,
    const TString& toMime,
    const NVoicetech::NASConv::TConverterOptions& options,
    TOnOutputCallback onOutputCallback
)
    : NVoicetech::NASConv::TThreadedConverter(fromMime, toMime, options, onOutputCallback)
    , UnistatCounterGuard_(Unistat().MusicMatchAudioConverterCounter())
{
    Unistat().OnMusicMatchCreateAudioConverter();
}

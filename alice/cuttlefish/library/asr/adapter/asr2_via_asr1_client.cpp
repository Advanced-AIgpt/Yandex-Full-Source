#include "asr2_via_asr1_client.h"

#include <alice/cuttlefish/library/proto_censor/asr.h>

#include <library/cpp/json/json_writer.h>
#include <util/charset/utf8.h>
#include <util/string/builder.h>
#include <util/string/subst.h>
#include <util/string/strip.h>

#include <string_view>

#undef DLOG
#define DLOG(msg)

using namespace std::literals;
using namespace NAlice::NAsr;
using namespace NAlice::NAsrAdapter;
using namespace NAlice::NCuttlefish;

namespace {
    void StartHttpUpgradeRequest(NVoicetech::THttpClient& client, const TString& asrUrl, const TString rtLogToken, const NVoicetech::TUpgradedHttpHandlerRef& handler) {
        TString headers;
        if (rtLogToken) {
            TStringOutput so(headers);
            so << "\r\nX-RTLog-Token: " << rtLogToken;
        }
        client.RequestUpgrade(asrUrl, NVoicetech::TProtobufHandler::HttpUpgradeType, handler, headers);
    }
}

TAsr2ViaAsr1Client::TAsr2ViaAsr1Client(
    NVoicetech::THttpClient& httpClient,
    TIntrusivePtr<TCallbacks>& callbacks,
    const NCuttlefish::TLogContext& logContextForCallbacks,
    const NCuttlefish::TLogContext& logContext,
    TDuration spotterDeadline
)
    : NAsr::TInterface(callbacks)
    , HttpClient_(httpClient)
    , IOService_(httpClient.GetIOService())
    , CallbacksLog_(logContextForCallbacks)
    , Log_(logContext)
    , SpotterDeadline_(spotterDeadline)
    , UnistatCounterGuard_(Unistat().Asr2ViaAsr1ClientCounter())
{
    (void)MagicKey_;
}

void TAsr2ViaAsr1Client::StartRequest(
    const TString& asrUrl,
    const NProtobuf::TInitRequest& initRequest,
    const TAsrInfo& asrInfo,
    bool ignoreParsingProtobufError
) {
    try {
        MessageId_ = initRequest.GetRequestId();
        InitImpl(initRequest, asrInfo, ignoreParsingProtobufError);
    } catch (...) {
        // break smart_ptr's cross-ref
        SetMainAsrClient(nullptr);
        SetSpotterAsrClient(nullptr);
        throw;
    }
    if (auto mainAsrRequestHandler = Handler()) {
        Log_.LogEventInfoCombo<NEvClass::AsrRequest>(asrUrl);
        StartHttpUpgradeRequest(HttpClient_, asrUrl, MainRtLogToken(), mainAsrRequestHandler);
    }
    if (auto spotterAsrRequestHandler = SpotterHandler()) {
        try {
            Log_.LogEventInfoCombo<NEvClass::SpotterRequest>(asrUrl);
            StartHttpUpgradeRequest(HttpClient_, asrUrl, SpotterRtLogToken(), spotterAsrRequestHandler);
        } catch (...) {
            Log_.LogEventInfoCombo<NEvClass::WarningMessage>(TStringBuilder() << "spotter request failed: " << CurrentExceptionMessage());
            IOService_.Post([self = TIntrusivePtr<TAsr2ViaAsr1Client>(this)]() mutable {
                self->OnSpotterClosed(TUnistatN::SpotterRequestFailed);
            });
        }
    }
}

void TAsr2ViaAsr1Client::InitImpl(
    const NProtobuf::TInitRequest& initRequest,
    const TAsrInfo& asrInfo,
    bool ignoreParsingProtobufError
) {
    bool spotterValidation = initRequest.HasHasSpotterPart() && initRequest.GetHasSpotterPart();
    {
        YaldiProtobuf::InitRequest yaldiInitRequest;
        Convert(initRequest, yaldiInitRequest);
        if (spotterValidation) {
            auto& advancedOptions = *yaldiInitRequest.mutable_advanced_options();
            advancedOptions.clear_spotter_validation();
        }
        if (Log_.Options().WriteInfoToEventLog || Log_.Options().WriteInfoToRtLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
            {
                TString s = TStringBuilder() << "There are " << yaldiInitRequest.user_info().GetContactBookItems().size()  << " contact book items in the ASR InitRequest";
                Log_.LogEventInfoCombo<NEvClass::DebugMessage>(s);
            }
            {
                TString s = TStringBuilder() << "build asr InitRequest: " << GetCensoredYaldiInitRequest(yaldiInitRequest).ShortUtf8DebugString();
                Log_.LogEventInfoCombo<NEvClass::DebugMessage>(s);
            }
        }
        // topic hack for abezhin@ VOICESERV-3818
        if (yaldiInitRequest.topic() == "quasar-general-gpu-monolith") {
            yaldiInitRequest.set_topic("quasar-general-gpu");
        }
        auto client = MakeIntrusive<TMainAsr1Client>(yaldiInitRequest, Callbacks_, this, asrInfo, CallbacksLog_);
        SetMainAsrClient(client);
        client->SetIgnoreProtobufParsingError(ignoreParsingProtobufError);
        client->SetSingleUtterance(initRequest.GetEouMode() != NProtobuf::MultiUtterance);
    }
    if (spotterValidation) {
        YaldiProtobuf::InitRequest yaldiInitRequest;
        Convert(initRequest, yaldiInitRequest);
        if (initRequest.HasRecognitionOptions()) {
            const auto& recognitionOptions = initRequest.GetRecognitionOptions();
            if (recognitionOptions.HasSpotterPhrase()) {
                SpotterPhrase_ = ToLowerUTF8(recognitionOptions.GetSpotterPhrase());
                SubstGlobal(SpotterPhrase_, TStringBuf(" "), TStringBuf(""));
            }
        }
        // python_uniproxy now ignore empty spotter_phrase params, so do some
        // if (!SpotterPhrase_) {
        //     ythrow yexception() << TStringBuf("request with spotter validation MUST contain not empty spotter_phrase");
        // }

        yaldiInitRequest.set_punctuation(false);
        yaldiInitRequest.clear_normalizer_options();  // disable normalizer
        auto& advancedOptions = *yaldiInitRequest.mutable_advanced_options();
        // inherit this from v2::initRequest  advancedOptions.set_spotter_validation(true);
        advancedOptions.set_allow_multi_utt(false);
        advancedOptions.set_partial_results(false);
        advancedOptions.set_capitalize(false);
        advancedOptions.set_manual_punctuation(false);
        advancedOptions.set_early_eou_message(false);
        advancedOptions.set_enable_e2e_eou(false);
        if (!advancedOptions.has_request_front()) {
            advancedOptions.set_request_front(1000);
        }
        if (!advancedOptions.has_spotter_back()) {
            advancedOptions.set_spotter_back(2000);
        }
        if (Log_.Options().WriteInfoToEventLog || Log_.Options().WriteInfoToRtLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
            TString s = TStringBuilder() << "build spotter InitRequest: " << yaldiInitRequest.ShortUtf8DebugString();
            Log_.LogEventInfoCombo<NEvClass::DebugMessage>(s);
        }
        // special hack for abezhin@ (emulate python_uniproxy behaviuor)
        if (yaldiInitRequest.topic() == "quasar-general-gpu") {
            yaldiInitRequest.set_topic("quasar-spotter-gpu");
        }
        // another topic hack for abezhin@ VOICESERV-3818
        if (yaldiInitRequest.topic() == "quasar-general-gpu-monolith") {
            yaldiInitRequest.set_topic("quasar-spotter-gpu");
        }
        auto client = MakeIntrusive<TSpotterAsr1Client>(IOService_, yaldiInitRequest, this, CallbacksLog_);
        SetSpotterAsrClient(client);
        client->SetIgnoreProtobufParsingError(ignoreParsingProtobufError);
    }
}

bool TAsr2ViaAsr1Client::ProcessAsrRequest(const NProtobuf::TRequest& request) {
    bool needMoreRequests = true;
    bool spotterChunk = false;
    YaldiProtobuf::AddData addData1;
    if (request.HasAddData()) {
        auto& addData = request.GetAddData();
        spotterChunk = addData.HasSpotterChunk() && addData.GetSpotterChunk();
        size_t audioSize = 0;
        if (addData.HasAudioData()) {
            audioSize = addData.GetAudioData().size();
        }
        Log_.LogEventInfoCombo<NEvClass::ProcessAddData>(audioSize, spotterChunk);
        Convert(addData, addData1);
    } else if (request.HasEndOfStream()) {
        Log_.LogEventInfoCombo<NEvClass::ProcessEndOfStream>();
        Convert(request.GetEndOfStream(), addData1);
    } else if (request.HasCloseConnection()) {
        Log_.LogEventInfoCombo<NEvClass::ProcessCloseConnection>();
        Convert(request.GetCloseConnection(), addData1);
        needMoreRequests = false;
    } else {
        Log_.LogEventInfoCombo<NEvClass::WarningMessage>("unknown ASRRequest content");
    }

    if (!spotterChunk) {
        auto client = GetMainAsrClient();
        if (client) {
            DLOG("send chunk to asr");
            client->SafeSend(IOService_, addData1);
        }
    }
    if (!SpotterClosed_) {
        auto client = GetSpotterAsrClient();
        if (client) {
            DLOG("send chunk to spotter");
            client->SafeSend(IOService_, addData1);
            ChunksSendedToSpotterClient_++;
        }
    }
    return needMoreRequests;
}

void TAsr2ViaAsr1Client::CauseError(const TString& error) {
    auto client = GetMainAsrClient();
    if (client) {
        client->SafeCauseError(IOService_, error);
    }
}

void TAsr2ViaAsr1Client::Close() {
    Log_.LogEventInfoCombo<NEvClass::CancelRequest>();
    {
        auto client = GetMainAsrClient();
        if (client) {
            client->SafeClose(IOService_);
            SetMainAsrClient(nullptr);
        }
    }
    {
        auto client = GetSpotterAsrClient();
        if (client) {
            client->SafeClose(IOService_);
            SetSpotterAsrClient(nullptr);
        }
    }
}

void TAsr2ViaAsr1Client::SafeInjectAsrResponse(NProtobuf::TResponse&& response) {
    auto client = GetMainAsrClient();
    if (client) {
        IOService_.Post([client, r = std::move(response)]() mutable {
            client->OnAsrResponse(r);
        });
    }
}

bool TAsr2ViaAsr1Client::OnMainEou() {
    // if has spotter validation, check result, - if not has result, - start deadline timer
    auto client = GetSpotterAsrClient();
    if (client && ValidationResult_.Empty()) {
        static const TString s = "enforce lastChunk for spotter";
        CallbacksLog_.LogEventInfoCombo<NEvClass::DebugMessage>(s);
        client->SendLastChunk();
        client->SetDeadline(SpotterDeadline_);
        return false;
    }

    return true;
}

void TAsr2ViaAsr1Client::OnSpotterInitResponse(const YaldiProtobuf::InitResponse& initResponse) {
    if (initResponse.GetresponseCode() != YaldiProtobuf::ResponseCode::OK) {
        OnSpotterClosed(TUnistatN::SpotterFailInit);
    }
}

void TAsr2ViaAsr1Client::OnSpotterAddDataResponse(const YaldiProtobuf::AddDataResponse& addDataResponse) {
    if (!ValidationResult_.Empty()) {
        return;  // too late (likely already handle validation timeout)
    }

    if (addDataResponse.has_core_debug() && addDataResponse.core_debug()) {
        try {
            NJson::TJsonValue rec;
            rec["type"] = "AsrCoreDebug";
            rec["backend"] = "spotter";
            rec["ForEvent"] = MessageId_;
            rec["debug"] = addDataResponse.core_debug();
            NAliceProtocol::TSessionLogRecord sessionLogRecord;
            sessionLogRecord.SetName("Directive");
            sessionLogRecord.SetValue(NJson::WriteJson(rec, false, true));
            sessionLogRecord.SetAction("response");
            Callbacks_->OnSessionLog(sessionLogRecord);
        } catch (...) {
            CallbacksLog_.LogEventInfoCombo<NEvClass::WarningMessage>("fail build/send AsrCoreDebug" + CurrentExceptionMessage());
            // TODO: inc counter
        }
    }

    SetSpotterAsrClient(nullptr);
    // get words, merge to single line, compare front with spotter phrase
    TString utt;
    if (addDataResponse.recognition().size()) {
        TStringOutput uttSo(utt);
        const YaldiProtobuf::Result& result = addDataResponse.recognition()[0];
        for (auto& word : result.words()) {
            if (word.has_value()) {
                uttSo << word.value();
            }
        }
        try {
            utt = ToLowerUTF8(utt);
            SubstGlobal(utt, TStringBuf(" "), TStringBuf(""));
        } catch (...) {
            CallbacksLog_.LogEventErrorCombo<NEvClass::SpotterError>(CurrentExceptionMessage());
            OnSpotterClosed(TUnistatN::SpotterInvalidTextResponse);
            return;
        }
    }
    CallbacksLog_.LogEventInfoCombo<NEvClass::SpotterResultPhrase>(SpotterPhrase_, utt);
    NProtobuf::TResponse response;
    Convert(addDataResponse, response);
    auto& addDataResponse2 = *response.MutableAddDataResponse();
    NProtobuf::FillRequiredDefaults(addDataResponse2);
    auto asrClient = GetMainAsrClient();
    if (!utt.StartsWith(SpotterPhrase_)) {
        ValidationResult_ = false;
        Callbacks_->OnSpotterValidation(*ValidationResult_);
        // return validation_failed, but continue recognition
        addDataResponse2.SetResponseStatus(NProtobuf::ValidationFailed);
        addDataResponse2.SetValidationInvoked(true);
        if (Log_.Options().WriteInfoToEventLog || Log_.Options().WriteInfoToRtLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
            CallbacksLog_.LogEventInfoCombo<NEvClass::InfoMessage>(TStringBuilder() << "SPOTTER_FAILED: " << response.ShortUtf8DebugString());
        }
        Callbacks_->OnAddDataResponse(response);
    } else {
        ValidationResult_ = true;
        Callbacks_->OnSpotterValidation(*ValidationResult_);
        if (asrClient) {
            asrClient->SetValidationSuccess();
        }
    }
    if (asrClient) {
        asrClient->ResumeResponses();
    }
}

void TAsr2ViaAsr1Client::OnSpotterClosed(int errorCode) {
    SetSpotterAsrClient(nullptr);
    SpotterClosed_ = true;
    if (errorCode == TUnistatN::SpotterCancel) {
        Unistat().OnAsr1SpotterCancel();
    } else {
        Unistat().OnAsr1SpotterError(errorCode);
        if (errorCode) {
            CallbacksLog_.LogEventErrorCombo<NEvClass::SpotterErrorResponseCode>(errorCode);
        }
    }
    if (ValidationResult_.Empty()) {
        ValidationResult_ = true;  // abnornal closing & other errors interpret as validation success
        Callbacks_->OnSpotterValidation(*ValidationResult_);
        auto client = GetMainAsrClient();
        if (client) {
            client->SetValidationSuccess();
            client->ResumeResponses();
        }
    }
}

namespace {
    const TDuration HEART_BEAT_AFTER = TDuration::Seconds(3);
}

TAsr2ViaAsr1Client::TMainAsr1Client::TMainAsr1Client(
    const YaldiProtobuf::InitRequest& initRequest,
    const TIntrusivePtr<NAsr::TInterface::TCallbacks>& callbacks,
    const TIntrusivePtr<TAsr2ViaAsr1Client>& asr,
    const TAsrInfo& asrInfo,
    const NAlice::NCuttlefish::TLogContext& log
)
    : TAsr1Client(initRequest, MainClientNum_)
    , Asr2Callbacks_(callbacks)
    , Asr_(asr)
    , AsrInfo_(asrInfo)
    , Log_(log)
    , NextHeartBeatTime_(TInstant::Now() + HEART_BEAT_AFTER)
    , ResponseFakeTopic_(initRequest.topic())
{
    // hack for replace requested topic to returned from asr-sever
    // (asr v2 MUST return topic in InitResponse, bu we not has it in InitResponse v1)
    if (ResponseFakeTopic_ == "dialog-general-gpu") {
        ResponseFakeTopic_ = "dialogeneral-gpu";
    } else if (ResponseFakeTopic_ == "quasar-general-gpu") {
        ResponseFakeTopic_ = "quasargeneral-gpu";
    } else if (ResponseFakeTopic_ == "dialogmapsgpu") {
        ResponseFakeTopic_ = "dialog-maps-gpu";
    }
    if (Log_.RtLogPtr()) {
        RtLogChild_ = TRTLogActivation(Log_.RtLogPtr(), "yaldi-server", /*newRequest=*/false);
    }
}

void TAsr2ViaAsr1Client::TMainAsr1Client::OnSend() {
    TInstant now = TInstant::Now();
    if (NextHeartBeatTime_ < now) {
        NextHeartBeatTime_ = now + HEART_BEAT_AFTER;
        if (Asr2Callbacks_) {
            NProtobuf::TResponse response;
            response.MutableHeartBeat();
            Asr2Callbacks_->OnAddDataResponse(response);
        }
    }
}

void TAsr2ViaAsr1Client::TMainAsr1Client::OnAsrResponse(const NProtobuf::TResponse& response) {
    if (!Asr2Callbacks_) {
        return;
    }

    if (response.HasInitResponse()) {
        Asr2Callbacks_->OnInitResponse(response);
        if (!response.GetInitResponse().GetIsOk()) {
            Asr2Callbacks_->OnClosed();
            Asr2Callbacks_.Reset();
            Asr_.Reset();
        }
    } else if (response.HasAddDataResponse()) {
        NextHeartBeatTime_ = TInstant::Now() + HEART_BEAT_AFTER;
        Asr2Callbacks_->OnAddDataResponse(response);
        if (!response.GetAddDataResponse().GetIsOk()) {
            Asr2Callbacks_->OnClosed();
            Asr2Callbacks_.Reset();
            Asr_.Reset();
        }
    } else if (response.HasCloseConnection()) {
        Asr2Callbacks_->OnClosed();
        Asr2Callbacks_.Reset();
        Asr_.Reset();
    }
}

// impl. asr1_client callbacks
void TAsr2ViaAsr1Client::TMainAsr1Client::OnInitResponse(const YaldiProtobuf::InitResponse& initResponse) {
    if (Log_.Options().WriteInfoToEventLog || Log_.Options().WriteInfoToRtLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
        TString s = initResponse.ShortUtf8DebugString();
        Log_.LogEventInfoCombo<NEvClass::RecvAsr1InitResponse>(s);
    }
    NProtobuf::TResponse response;
    Convert(initResponse, response);
    auto& initResponse2 = *response.MutableInitResponse();
    if (AsrInfo_.UseFakeTopic) {
        initResponse2.SetTopic(ResponseFakeTopic_);  // hack for fill metainfo used in ftests
    } else {
        initResponse2.SetTopic(AsrInfo_.Topic);
    }
    initResponse2.SetTopicVersion(AsrInfo_.TopicVersion);
    initResponse2.SetServerVersion(AsrInfo_.ServerVersion);
    Asr2Callbacks_->OnInitResponse(response);
    CheckResponseCode(initResponse.GetresponseCode());
}

void TAsr2ViaAsr1Client::TMainAsr1Client::OnAddDataResponse(const YaldiProtobuf::AddDataResponse& addDataResponse) {
    if (Log_.Options().WriteInfoToEventLog || Log_.Options().WriteInfoToRtLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
        Log_.LogEventInfoCombo<NEvClass::RecvAsr1AddDataResponse>(addDataResponse.ShortUtf8DebugString());
    }
    if (!Asr_ || !Asr2Callbacks_) {
        return;
    }

    auto response = std::make_unique<NProtobuf::TResponse>();
    Convert(addDataResponse, *response);

    if (addDataResponse.GetresponseCode() != YaldiProtobuf::OK) {
        ResumeResponses();
        Asr2Callbacks_->OnAddDataResponse(*response);
        CheckResponseCode(addDataResponse.GetresponseCode());
        return;
    };

    response->MutableAddDataResponse()->SetValidationInvoked(ValidationSuccess_);
    // suspend only EOU=true response
    if (SuspendResponses_) {
        SuspendResponses_->push(std::move(response));
        return;
    }

    if (addDataResponse.HasendOfUtt() && addDataResponse.GetendOfUtt()) {
        // if has spotter mode & not has spotter result use queue for suspend all asr responses (+start spotter Deadline) for later result
        if (!ReceiveEou_) {
            ReceiveEou_ = true;
            if (!Asr_->OnMainEou()) {
                // yet not ready for EOU (wait spotter result)
                SuspendResponses_ = std::make_unique<TSuspendedResponsesQueue>();
                SuspendResponses_->push(std::move(response));
                Asr_.Reset();
                return;
            }
            if (SingleUtterance_) {
                Asr2Callbacks_->OnAddDataResponse(*response);
                Asr2Callbacks_->OnClosed();  // in single utterance mode we not need send anything after first EOU=true response
                CheckResponseCode(addDataResponse.GetresponseCode());
                Asr2Callbacks_.Reset();
                Asr_.Reset();
                return;
            }
        } else {
            if (SingleUtterance_) {
                // ignore all extra eou responses
                CheckResponseCode(addDataResponse.GetresponseCode());
                Asr_.Reset();
                return;
            }
        }
    }
    Asr2Callbacks_->OnAddDataResponse(*response);
    CheckResponseCode(addDataResponse.GetresponseCode());
}

void TAsr2ViaAsr1Client::TMainAsr1Client::ResumeResponses() {
    if (!SuspendResponses_) {
        return;
    }

    while (SuspendResponses_->size()) {
        std::unique_ptr<NProtobuf::TResponse> response = std::move(SuspendResponses_->front());
        SuspendResponses_->pop();
        if (!Asr2Callbacks_) {
            break;
        }

        // update messages with current spotter validation status
        if (response->HasAddDataResponse()) {
            response->MutableAddDataResponse()->SetValidationInvoked(ValidationSuccess_);
        }
        if (Asr2Callbacks_) {
            Asr2Callbacks_->OnAddDataResponse(*response);
            if (SingleUtterance_) {
                // we always suspend only EOU=true response & not need send anything after this response in SingleUtterance mode
                Asr2Callbacks_->OnClosed();
                Asr2Callbacks_.Reset();
            }
        }
        Asr_.Reset();
    }
    SuspendResponses_.reset();
    if (PostponedClose_) {
        OnClosed();
    }
}

void TAsr2ViaAsr1Client::TMainAsr1Client::ResetResponses() {
    SuspendResponses_.reset();
    if (PostponedClose_) {
        OnClosed();
    }
}

void TAsr2ViaAsr1Client::TMainAsr1Client::CheckResponseCode(int responseCode) {
    if (responseCode != YaldiProtobuf::OK) {
        if (Asr_) {
            Unistat().OnAsr1Error(responseCode);
            Log_.LogEventErrorCombo<NEvClass::AsrErrorResponseCode>(responseCode);
        }
        OnClosed();
        Cancel();
    }
}

void TAsr2ViaAsr1Client::TMainAsr1Client::OnClosed() {
    Log_.LogEventInfoCombo<NEvClass::InfoMessage>("(main) asr1 connection closed");
    if (SuspendResponses_ && SuspendResponses_->size()) {
        PostponedClose_ = true;
        return;
    }

    PostponedClose_ = false;
    if (Asr2Callbacks_) {
        Asr2Callbacks_->OnClosed();
        Asr2Callbacks_.Reset();
    }
    Asr_.Reset();
}

void TAsr2ViaAsr1Client::TMainAsr1Client::OnAnyError(const TString& error, bool fastError, int errorCode) {
    if (RtLogChild_ && Asr2Callbacks_) {
        RtLogChild_.Finish(/* ok= */ false, error);
    }
    if (Asr2Callbacks_) {
        Unistat().OnAsr1Error(errorCode);
        Log_.LogEventErrorCombo<NEvClass::AsrError>(error);
        Asr2Callbacks_->OnAnyError(TStringBuilder() << "asr1_client: "sv << error, fastError);
        Asr2Callbacks_.Reset();
    }
    Asr_.Reset();
}

TAsr2ViaAsr1Client::TSpotterAsr1Client::TSpotterAsr1Client(
    NAsio::TIOService& ioService,
    const YaldiProtobuf::InitRequest& initRequest,
    const TIntrusivePtr<TAsr2ViaAsr1Client>& asr,
    const NAlice::NCuttlefish::TLogContext& log
)
    : TAsr1Client(initRequest, SpotterClientNum_)
    , IOService_(ioService)
    , Asr_(asr)
    , Log_(log)
{
    if (log.RtLogPtr()) {
        RtLogChild_ = TRTLogActivation(log.RtLogPtr(), "yaldi-server/spotter", /*newRequest=*/false);
    }
}

void TAsr2ViaAsr1Client::TSpotterAsr1Client::SendLastChunk() {
    YaldiProtobuf::AddData addData;
    addData.SetlastChunk(true);
    SafeSend(IOService_, addData);
}

void TAsr2ViaAsr1Client::TSpotterAsr1Client::SetDeadline(TDuration dur) {
    ValidationDeadlineTimer_.reset(new NAsio::TDeadlineTimer(GetIOService()));
    ValidationDeadlineTimer_->AsyncWaitExpireAt(
        dur,
        [self = TIntrusivePtr<TAsr2ViaAsr1Client::TSpotterAsr1Client>(this)](const NAsio::TErrorCode& ec, NAsio::IHandlingContext&) {
            if (ec) {
                return;  // ignore canceling
            }

            self->OnDeadline();
        }
    );
}

void TAsr2ViaAsr1Client::TSpotterAsr1Client::OnDeadline() {
    if (Asr_) {
        ReleaseAsr()->OnSpotterClosed(TUnistatN::SpotterErrorDeadline);
    }
    Cancel();
}

// impl. asr1_client callbacks
void TAsr2ViaAsr1Client::TSpotterAsr1Client::OnInitResponse(const YaldiProtobuf::InitResponse& initResponse) {
    if (Log_.Options().WriteInfoToEventLog || Log_.Options().WriteInfoToRtLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
        Log_.LogEventInfoCombo<NEvClass::RecvSpotter1InitResponse>(initResponse.ShortUtf8DebugString());
    }
    if (!Asr_) {
        return;
    }

    if (initResponse.GetresponseCode() != YaldiProtobuf::OK) {
        ReleaseAsr()->OnSpotterClosed(initResponse.GetresponseCode());
        Cancel();
    } else {
        Asr_->OnSpotterInitResponse(initResponse);
    }
}

void TAsr2ViaAsr1Client::TSpotterAsr1Client::OnAddDataResponse(const YaldiProtobuf::AddDataResponse& addDataResponse) {
    if (Log_.Options().WriteInfoToEventLog || Log_.Options().WriteInfoToRtLog) {  // NOTE: CPU optimisation (avoid call ShortUtf8DebugString)
        Log_.LogEventInfoCombo<NEvClass::RecvAsr1SpotterAddDataResponse>(addDataResponse.ShortUtf8DebugString());
    }
    if (Asr_) {
        if (addDataResponse.GetresponseCode() != YaldiProtobuf::OK) {
            ReleaseAsr()->OnSpotterClosed(addDataResponse.GetresponseCode());
        } else {
            ReleaseAsr()->OnSpotterAddDataResponse(addDataResponse);
        }
    }
    UnsafeSendCloseConnection(IOService_);  // soft closing connection to asr (for avoid 'invalid socket' error on asr-server)
}

void TAsr2ViaAsr1Client::TSpotterAsr1Client::OnClosed() {
    if (ValidationDeadlineTimer_) {
        ValidationDeadlineTimer_->Cancel();
        ValidationDeadlineTimer_.reset();
    }
    if (Asr_) {
        if (Closing_) {
            Log_.LogEventInfoCombo<NEvClass::InfoMessage>("spotter connection canceled");
            ReleaseAsr()->OnSpotterClosed(TUnistatN::SpotterCancel);
        } else {
            Log_.LogEventErrorCombo<NEvClass::ErrorMessage>("spotter connection unexpectedly closed");
            ReleaseAsr()->OnSpotterClosed();
        }
    } else {
        Log_.LogEventInfoCombo<NEvClass::InfoMessage>("spotter connection closed");
    }
}

void TAsr2ViaAsr1Client::TSpotterAsr1Client::OnAnyError(const TString& error, bool fastError, int /*errorCode*/) {
    (void)fastError;
    if (RtLogChild_) {
        if (Asr_) {
            RtLogChild_.Finish(/* ok= */ false, error);
        } else {
            RtLogChild_.Finish(/* ok= */ true);  // ignore cancelling request error
        }
    }
    if (Asr_) {
        Log_.LogEventErrorCombo<NEvClass::SpotterError>(error);
        ReleaseAsr()->OnSpotterClosed();
    }
}

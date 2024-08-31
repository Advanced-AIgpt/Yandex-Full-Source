#include "test_functions.h"

#include <util/string/builder.h>

using namespace NAlice::NMusicMatchAdapter::NTestLib;

void TMusicMatchFakeProxyHandler::CheckNoErrors() {
    const TString fullError = BuildFullErrorMessage();
    ResetSomeErrors();

    UNIT_ASSERT_C(!CustomError_.Defined(), GetErrorMessage(*CustomError_, fullError));

    UNIT_ASSERT_C(!UpgradeRequestError_.Defined(), GetErrorMessage(*UpgradeRequestError_, fullError));
    UNIT_ASSERT_C(!CloseMessageError_.Defined(), GetErrorMessage(*CloseMessageError_, fullError));

    UNIT_ASSERT_C(!NetworkError_.Defined(), GetErrorMessage(*NetworkError_, fullError));
    UNIT_ASSERT_C(!WsError_.Defined(), GetErrorMessage(*WsError_, fullError));
    UNIT_ASSERT_C(!TypedError_.Defined(), GetErrorMessage(*TypedError_, fullError));

    UNIT_ASSERT_C(RequestUpgraded_, GetErrorMessage("Request not upgraded", fullError));
    UNIT_ASSERT_C(HasCloseMessage_, GetErrorMessage("Close message not received", fullError));
}


TString TMusicMatchFakeProxyHandler::BuildFullErrorMessage() const {
    TStringBuilder errorMessage;

    errorMessage << "CustomError: " << CustomError_.GetOrElse("All ok") << "\n";
    errorMessage << "UpgradeRequestError: " << UpgradeRequestError_.GetOrElse("All ok") << "\n";
    errorMessage << "CloseMessageError: " << CloseMessageError_.GetOrElse("All ok") << "\n";
    errorMessage << "NetworkError:  " << NetworkError_.GetOrElse("All ok") << "\n";
    errorMessage << "WsError: " << WsError_.GetOrElse("All ok") << "\n";
    errorMessage << "TypedError: " << TypedError_.GetOrElse("All ok") << "\n";

    if (!RequestUpgraded_) {
        errorMessage << "Request not upgraded";
    }
    if (!HasCloseMessage_) {
        errorMessage << "Close message not received";
    }

    return errorMessage;
}

TString TMusicMatchFakeProxyHandler::GetErrorMessage(const TString& mainError, const TString& fullError) const {
    TStringBuilder errorMessage;

    errorMessage << "Main error: " << mainError << "\n";

    errorMessage << "Other errors:\n";
    errorMessage << fullError;

    return errorMessage;
}

void TMusicMatchFakeProxyHandler::SendCloseMessageIfNotSended() {
    if (!CloseMessageSended_) {
        CloseMessageSended_ = true;
        TWsHandler::SendCloseMessage();
    }
}

void TMusicMatchFakeProxyHandler::SafeSendTextMessage(const TString& message, bool sendCloseMessage) {
    GetIOService().Post([self = TIntrusivePtr<TMusicMatchFakeProxyHandler>(this), message, sendCloseMessage]() {
        if (self->HasCloseMessage_ || self->CloseMessageSended_) {
            return;
        }
        self->TWsHandler::SendTextMessage(message);
        if (sendCloseMessage) {
            self->CloseMessageSended_ = true;
            self->TWsHandler::SendCloseMessage();
        }
    });
}

void TMusicMatchFakeProxyHandler::OnMessage(TMusicMatchFakeProxyHandler::EMessageType lastMessageType) {
    if (CheckSendClassifying(lastMessageType)) {
        NJson::TJsonValue ret = NJson::TJsonMap();
        ret["directive"]["header"]["name"] = "classifying";
        ret["directive"]["payload"]["musicDetected"] = true;
        SafeSendTextMessage(ret.GetStringRobust(), false);
    }
    if (CheckSendNotMusic(lastMessageType)) {
        NJson::TJsonValue ret = NJson::TJsonMap();
        ret["directive"]["header"]["name"] = "classifying";
        ret["directive"]["payload"]["musicDetected"] = false;
        SafeSendTextMessage(ret.GetStringRobust(), true);
    }
    SendCustomAnswer(lastMessageType);
}

void TMusicMatchFakeProxyHandler::OnUpgradeRequest(const THttpParser& httpParser, int httpCode, const TString& error) {
    RequestUpgraded_ = true;
    TWsHandler::OnUpgradeRequest(httpParser, httpCode, error);
    if (error) {
        UpgradeRequestError_ = TStringBuilder() << "UpdateRequestError: error=" << error;
    }
}

void TMusicMatchFakeProxyHandler::OnTextMessage(const char* data, size_t size) {
    TextMessages_.push_back(TString(TStringBuf(data, size)));
    OnMessage(TMusicMatchFakeProxyHandler::EMessageType::TEXT);
}

void TMusicMatchFakeProxyHandler::OnBinaryMessage(const void* data, size_t size) {
    BinaryMessages_.push_back(TString(TStringBuf(static_cast<const char*>(data), size)));
    OnMessage(TMusicMatchFakeProxyHandler::EMessageType::BINARY);
}

void TMusicMatchFakeProxyHandler::OnCloseMessage(ui16 code, const TString& reason) {
    HasCloseMessage_ = true;
    if (code != NRfc6455::CSNormal) {
        CloseMessageError_ = TStringBuilder() << "CloseMessageError: code=" << code << ", " << "reason=" << reason;
    }
    SendCloseMessageIfNotSended();
}

void TMusicMatchFakeProxyHandler::OnError(const NVoicetech::TNetworkError& error) {
    NetworkError_ = TStringBuilder() << "NetworkError: op=" << int(error.Operation) << " errno=" << error.Value() << ": " << error.Text();
}

void TMusicMatchFakeProxyHandler::OnError(const NRfc6455::TWsError& error) {
    WsError_ = TStringBuilder() << "WsError: code=" << error.Code << " (" << error.what() << ")";
    SendCloseMessageIfNotSended();
}

void TMusicMatchFakeProxyHandler::OnError(const NVoicetech::TTypedError& error) {
    TypedError_ = TStringBuilder() << "TypedError: text=" << error.Text;
    SendCloseMessageIfNotSended();
}


// ------------------------------------------------------------------------------------------------
bool THttpClientRunner::TryWaitAsioClients(const TDuration& period, size_t tries) {
    return TryWait(period, tries, [this]() {
        return !HttpClient_.AsioClients().Val();
    });
}


// ------------------------------------------------------------------------------------------------
bool NAlice::NMusicMatchAdapter::NTestLib::TryWait(const TDuration& period, size_t tries, std::function<bool()>&& breakHook) {
    for (size_t i = 0; i < tries; ++i) {
        if (breakHook()) {
            return true;
        }
        if (i + 1 != tries) {
            Sleep(period);
        }
    }

    return false;
}

#include "personal_data.h"

#include <variant>

namespace NAlice::NHollywood {

namespace {

class TDataSyncDelegate : public NAlice::NDataSync::TDataSyncAPI::IDelegate {
public:
    TDataSyncDelegate(const TClientInfo& clientInfo)
        : ClientInfo_{clientInfo} {}

    NHttpFetcher::TRequestPtr Request(TStringBuf /* path */) const override {
        ythrow yexception() << "Not implemented NAlice::NHollywood::TDataSyncDelegate::Request";
    };

    NHttpFetcher::TRequestPtr AttachRequest(TStringBuf /* path */,
                                            NHttpFetcher::IMultiRequest::TRef /* multiRequest */) const override
    {
        ythrow yexception() << "Not implemented NAlice::NHollywood::TDataSyncDelegate::AttachRequest";
    };

    void AddTVM2AuthorizationHeaders(TStringBuf /* userId */, NHttpFetcher::TRequestPtr& /* request */) const override {
        ythrow yexception() << "Not implemented NAlice::NHollywood::TDataSyncDelegate::AddTVM2AuthorizationHeaders";
    };

    TString GetDeviceModel() const override {
        return ClientInfo_.DeviceModel;
    };

    TString GetDeviceId() const override {
        return ClientInfo_.DeviceId;
    };

    TString GetPersId() const override {
        ythrow yexception() << "Not implemented NAlice::NHollywood::TDataSyncDelegate::GetPersId. "
                            << "Use NAlice::NHollywood::TDataSyncDelegateEnrollmentSpecificWrapper";
    }

private:
    const TClientInfo& ClientInfo_;
};

class TDataSyncDelegateEnrollmentSpecificWrapper : public NAlice::NDataSync::TDataSyncAPI::IDelegate {
public:
    TDataSyncDelegateEnrollmentSpecificWrapper(const NAlice::NDataSync::TDataSyncAPI::IDelegate& wrappeeDelegate, const TString& persId)
        : WrappeeDelegate_{wrappeeDelegate}
        , PersId_(persId)
    {}

    NHttpFetcher::TRequestPtr Request(TStringBuf /* path */) const override {
        ythrow yexception() << "Not implemented NAlice::NHollywood::TDataSyncDelegateEnrollmentSpecificWrapper::Request";
    };

    NHttpFetcher::TRequestPtr AttachRequest(TStringBuf /* path */,
                                            NHttpFetcher::IMultiRequest::TRef /* multiRequest */) const override
    {
        ythrow yexception() << "Not implemented NAlice::NHollywood::TDataSyncDelegateEnrollmentSpecificWrapper::AttachRequest";
    };

    void AddTVM2AuthorizationHeaders(TStringBuf /* userId */, NHttpFetcher::TRequestPtr& /* request */) const override {
        ythrow yexception() << "Not implemented NAlice::NHollywood::TDataSyncDelegateEnrollmentSpecificWrapper::AddTVM2AuthorizationHeaders";
    };

    TString GetDeviceModel() const override {
        return WrappeeDelegate_.GetDeviceModel();
    };

    TString GetDeviceId() const override {
        return WrappeeDelegate_.GetDeviceId();
    };

    TString GetPersId() const override {
        return PersId_;
    }

private:
    const NAlice::NDataSync::TDataSyncAPI::IDelegate& WrappeeDelegate_;
    const TString& PersId_;
};

TMaybe<TString> GetValueFromDataSync(const TScenarioBaseRequestWrapper& request, const TString& key) {
    const auto* valuePtr = request.GetPersonalDataString(key);
    if (valuePtr) {
        return *valuePtr;
    } else {
        return Nothing();
    }
}

} // namespace

TString ToPersonalDataKey(const TClientInfo& clientInfo, NAlice::NDataSync::EKey key, TMaybe<TString> persId) {
    TDataSyncDelegate delegate{clientInfo};
    if (std::holds_alternative<NAlice::NDataSync::EEnrollmentSpecificKey>(key)) {
        Y_ENSURE(persId && !persId->Empty(), "Non-empty persId for enrollment-specific DataSync key is expected");
        return NAlice::NDataSync::ToDataSyncKey(TDataSyncDelegateEnrollmentSpecificWrapper{delegate, *persId}, key);
    }
    return NAlice::NDataSync::ToDataSyncKey(delegate, key);
}

TMaybe<TString> GetOwnerNameFromDataSync(const TScenarioBaseRequestWrapper& request) {
    auto userNameKey = ToPersonalDataKey(request.ClientInfo(), NAlice::NDataSync::EUserSpecificKey::UserName);
    return GetValueFromDataSync(request, userNameKey);
}

TMaybe<TString> GetKolonkishUidFromDataSync(const TScenarioBaseRequestWrapper& request) {
    auto guestUidKey = ToPersonalDataKey(request.ClientInfo(), NAlice::NDataSync::EUserSpecificKey::GuestUID);
    return GetValueFromDataSync(request, guestUidKey);
}

TMaybe<TString> GetGenderFromDataSync(const TScenarioBaseRequestWrapper& request) {
    auto genderKey = ToPersonalDataKey(request.ClientInfo(), NAlice::NDataSync::EUserSpecificKey::Gender);
    return GetValueFromDataSync(request, genderKey);
}

}  // namespace NAlice::NHollywood

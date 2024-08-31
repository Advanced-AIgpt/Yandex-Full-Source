#pragma once

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/hollywood/library/scenarios/messenger_call/proto/state.pb.h>
#include <alice/protos/data/contacts.pb.h>

#include <util/generic/fwd.h>
#include <util/generic/set.h>

using namespace NAlice::NScenarios;
using namespace NAlice::NHollywood::NMessengerCall;

namespace NAlice::NHollywood {

struct TPhone {
    TStringBuf Phone;
    TStringBuf IdString;
    long Id;

    static TPhone FromProto(const NAlice::NData::TContactsList::TPhone& phone);

    bool operator<(const TPhone& other) const;
};

struct TContact {
    TStringBuf DisplayName;
    TStringBuf LookupKey;
    TSet<TPhone> Phones;

    static TContact FromProto(const NAlice::NData::TContactsList::TContact& contact);
};

class TTokenAccumulator {
public:
    THashMap<TDynBitMap, TVector<TString>> GetUniqueTokens() const;
    THashMap<TDynBitMap, TVector<TString>> GetCommonTokens() const;
    void AddTokens(const TString name, const ELang lang, uint contactPosition);
private:
    uint contactCount = 0;
    THashMap<TDynBitMap, TVector<TString>> GetTokenSets(bool uniqueTokens) const;
    TVector<TString> CombineTokens(const TVector<TString>& tokens);
    THashMap<TString, TDynBitMap> tokenMasks;
    THashMap<TString, uint> names;
};

class TPhoneCall {
public:
    TPhoneCall(TRTLogger& logger,
               TResponseBodyBuilder& bodyBuilder,
               TNlgData& nlgData,
               TNlgWrapper& nlgWrapper,
               IRng& rng,
               const TScenarioRunRequestWrapper& request,
               TState& requestState);

    bool TryHandlePhoneCall();

private:
    void HandleUnauthorizedUser();

    bool HasValidFrame() const;
    const TPtrWrapper<TSemanticFrame> FindPhoneCallFrame() const;
    const TPtrWrapper<TSemanticFrame> FindPhoneCallExtendedFrame() const;
    bool HasNoTargetCallFrame() const;

    void HandlePhoneCall(const TFrame& frame, const bool callAllowed = true);
    void HandleRequestPermissions(const THashSet<NAlice::TPermissions_EValue>& permissions);
    void HandleWaitingForContacts();
    bool TryHandleCallback(const TCallbackDirective& callback);

    void SetSensitiveContentProperties();

    size_t GetMaxContactsListSize() const;

    THashSet<NAlice::TPermissions_EValue> GetMissingPermissions() const;
    void LogRequestedPermissions(const THashSet<NAlice::TPermissions_EValue>& permissions) const;
    bool IsWaitingForContacts() const;

    void HandlePhoneCall(const TVector<TContact>& matchedContacts, const bool callAllowed = true);

    TVector<TContact> BuildContacts(const TVector<TString>& lookupKeys) const;

    TVector<TContact> BuildContactsFromPayload(const TCallbackDirective& callback) const;
    TMaybe<TContact> BuildContact(const long phoneId, const TString& phoneIdString) const;

    void HandleSingleMatchedContact(const TContact& callee);

    void ListContacts(const TVector<TContact>& matchedContacts);
    void ListContactsDiv(const TVector<TContact>& matchedContacts);
    void ListContactsSuggests(const TVector<TContact>& matchedContacts);

    void AddListContactsAnalyticsInfo(const TVector<TContact>& matchedContacts);
    void AddPhoneBookAnalyticsInfo(const NAlice::NData::TContactsList& contacts);
    NAlice::NData::TContactsList ToAnalyticsContactsList(const TVector<TContact>& contacts) const;

    void HandleListPhoneNumbers(const TContact& contact);
    void ListPhoneNumbersDiv(const TContact& contact);
    void ListPhoneNumbersSuggests(const TContact& contact);

    void HandleContactNotFound();

    IAnalyticsInfoBuilder& GetAnalyticsInfoBuilder();

    void AddRenderedPhrase(const TStringBuf phrase, const TVector<TLayout::TButton>& buttons = {});

    void AddRenderedDiv2Card(const TStringBuf card, const TStringBuf voicePhrase);

    void SetStubResponse();

    void AddRandomOrdinalSuggest(const size_t minOrdinal, const size_t maxOrdinal);
    void AddRandomContactNameSuggest(const TVector<TContact>& contacts);
    void TryAddOpenAddressBookSuggest();
    TString RenderOrdinalPhrase(const size_t ordinal) const;
    TString RenderPhrase(const TStringBuf phrase) const;

    TDirective MakeCallDirective(const TStringBuf phone) const;
    TDirective MakeOpenAddressBookDirective() const;
    TDirective MakeChooseContactCallback(const TVector<TContact>& contact) const;
    TDirective MakeChoosePhoneCallback(const TPhone& phone) const;
    TDirective MakeRequestPermissionsDirective(const THashSet<NAlice::TPermissions_EValue>& permissions) const;

    void AddOpenAddressBookAction();
    void AddAction(const TStringBuf actionId, TDirective&& directive, TFrameNluHint&& nluHint);
    void AddContactOrdinalAction(const TContact& contact, size_t contactPosition);
    void AddContactVoiceAction(const TContact& contact, const TVector<TString>& tokens, size_t contactPosition);
    void AddContactSetVoiceAction(const TVector<TContact>& contacts, const TDynBitMap mask, const TVector<TString>& tokens);
    TString AddPhoneAction(const TPhone& phone, size_t phonePosition);

private:
    TRTLogger& Logger;
    TResponseBodyBuilder& BodyBuilder;
    const TScenarioRunRequestWrapper& Request;
    TNlgData& NlgData;
    TNlgWrapper& NlgWrapper;
    IRng& Rng;
    const ELang Lang;
    const NAlice::NData::TContactsList ContactsList;
    TState& State;
    const bool CanOpenAddressBook = false;
};

} // namespace NAlice::NHollywood

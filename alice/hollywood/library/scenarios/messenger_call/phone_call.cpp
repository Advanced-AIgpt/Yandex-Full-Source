#include "phone_call.h"

#include "nlu_hint.h"

#include <alice/megamind/protos/common/content_properties.pb.h>
#include <alice/protos/div/div2card.pb.h>
#include <alice/megamind/protos/common/permissions.pb.h>
#include <alice/megamind/protos/scenarios/analytics_info.pb.h>

#include <alice/hollywood/library/request/request.h>
#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/contacts/contacts.h>
#include <alice/library/experiments/experiments.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/proto/proto.h>
#include <alice/library/url_builder/url_builder.h>
#include <alice/nlu/libs/frame/slot.h>
#include <alice/nlu/libs/normalization/normalize.h>
#include <alice/nlu/libs/tokenization/tokenizer.h>
#include <alice/protos/data/contacts.pb.h>

#include <util/generic/algorithm.h>
#include <util/string/ascii.h>
#include <util/string/cast.h>
#include <util/string/join.h>

using namespace NAlice::NScenarios;
using namespace NAlice::NData;

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf PHONE_CALL = "phone_call";

// Frames
constexpr TStringBuf PHONE_CALL_FRAME = "alice.phone_call";
constexpr TStringBuf PHONE_CALL_ONE_WORD_EXTENSION_FRAME = "alice.phone_call.one_word_extension";
constexpr TStringBuf CALL_TO_FRAME = "alice.messenger_call.call_to";
constexpr TStringBuf PHONE_CALL_CONTACT_FROM_ADDRESS_BOOK = "alice.phone_call.contact_from_address_book";

// Directive names
constexpr TStringBuf CHOOSE_CONTACT_CALLBACK = "choose_contact_callback";
constexpr TStringBuf ON_REQUEST_READ_CONTACTS_PERMISSIONS_SUCCESS = "on_request_read_contacts_permissions_success";
constexpr TStringBuf ON_REQUEST_READ_CONTACTS_PERMISSIONS_FAIL = "on_request_read_contacts_permissions_fail";

// NLG
constexpr TStringBuf CONTACT_NOT_FOUND = "contact_not_found";
constexpr TStringBuf CHOOSE_CALLEE = "choose_callee";
constexpr TStringBuf CHOOSE_PHONE_NUMBER = "choose_phone_number";
constexpr TStringBuf CALL = "call";
constexpr TStringBuf REQUEST_READ_CONTACTS_PERMISSION = "request_read_contacts_permissions";
constexpr TStringBuf GRANTED_READ_CONTACTS_PERMISSION = "granted_read_contacts_permissions";
constexpr TStringBuf DENIED_READ_CONTACTS_PERMISSION = "denied_read_contacts_permissions";
constexpr TStringBuf WAITING_FOR_CONTACTS = "waiting_for_contacts";
constexpr TStringBuf COULD_NOT_LOAD_CONTACTS = "could_not_load_contacts";
constexpr TStringBuf COULD_NOT_LOAD_CONTACTS_STATIONS = "could_not_load_contacts_stations";
constexpr TStringBuf OPEN_ADDRESS_BOOK_SUGGEST = "open_address_book_suggest";
constexpr TStringBuf ORDINAL = "ordinal";
constexpr TStringBuf CALL_COMMON_CONTACT = "call_common_contact";
constexpr TStringBuf USER_NOT_AUTHORIZED = "user_not_authorized";
constexpr TStringBuf STUB_RESPONSE = "stub_response";

// Div2 cards
constexpr TStringBuf CONTACTS_CARD = "contacts_card";
constexpr TStringBuf LIST_CONTACTS = "list_contacts";
constexpr TStringBuf LIST_PHONE_NUMBERS = "list_phone_numbers";

// Misc
constexpr TStringBuf CHOOSE_CONTACT = "choose_contact";
constexpr TStringBuf CHOOSE_CONTACT_DIV = "choose_contact_div";
constexpr TStringBuf CHOOSE_CONTACT_ORDINAL = "choose_contact_ordinal";
constexpr TStringBuf LOOKUP_KEYS = "lookup_keys";
constexpr TStringBuf PHONE_ID = "phone_id";
constexpr TStringBuf PHONE_ID_STRING = "phone_id_string";
constexpr TStringBuf OPEN_ADDRESS_BOOK_ACTION_ID = "open_address_book";

constexpr size_t DEFAULT_MAX_CONTACTS_LIST_SIZE = 5;
constexpr ui32 WAITING_CONTACTS_TIMEOUT = 3;

const THashMap<TStringBuf, NAlice::TPermissions_EValue> REQUIRED_PERMISSIONS = {
    { "read_contacts", NAlice::TPermissions::ReadContacts },
};


template <typename TContainer>
const auto& Choice(const TContainer& seq, IRng& rng) {
    return seq[rng.RandomInteger(seq.size())];
}

template<typename TContainer>
void Trim(TContainer& container, const size_t maxSize) {
    if (container.size() > maxSize) {
        container.resize(maxSize);
    }
}

TVector<TString> ParseMatchedContactKeys(const TPtrWrapper<TSlot>& source) {
    TVector<TString> contactKeys;
    if (source->Type != "variants") {
        contactKeys.push_back(source->Value.AsString());
    } else {
        for (const auto& slotItem : NAlice::UnPackVariantsValue(source->Value.AsString())) {
            contactKeys.push_back(slotItem.Value);
        }
    }
    return contactKeys;
}

auto& PrepareChooseContactPayload(TDirective& directive) {
    auto& callback = *directive.MutableCallbackDirective();
    callback.SetName(TString{CHOOSE_CONTACT_CALLBACK});

    return *callback.MutablePayload()->mutable_fields();
}

// Simple & stupid, O(1) insertion, O(1) random access, preserves the order of insertion
template<typename K, typename V>
class TOrderedMap {
public:
    // Useful for predefining the order
    TOrderedMap(const TVector<K>& initialKeys) {
        for (const auto& key : initialKeys) {
            (*this)[key] = {};
        }
    }

    bool contains(const K& key) const {
        return Indices.contains(key);
    }

    V& operator[](const K& key) {
        if (!contains(key)) {
            Indices[key] = Values.size();
            Values.push_back(V());
        }
        return Values[Indices[key]];
    }

    TVector<V> MoveValues() && {
        return std::move(Values);
    }

private:
    THashMap<K, uint> Indices;
    TVector<V> Values;
};

NAlice::NData::TContactsList GetContactsList(const TDataSource* contactsListSource) {
    NAlice::NData::TContactsList contactsList;
    if (contactsListSource != nullptr) {
        contactsList = contactsListSource->GetContactsList();
    }

    if (const auto& lookupKeyMapSerialized = contactsList.GetLookupKeyMapSerialized(); !lookupKeyMapSerialized.empty()) {
        const auto lookupKeyMap = NContacts::DeSerializeLookupKeyMap(lookupKeyMapSerialized);
        for (auto& contact : *contactsList.MutableContacts()) {
            if (const auto lookupKey = lookupKeyMap.FindPtr(contact.GetLookupKeyIndex())) {
                contact.SetLookupKey(*lookupKey);
            }
        }
        for (auto& phone : *contactsList.MutablePhones()) {
            if (const auto lookupKey = lookupKeyMap.FindPtr(phone.GetLookupKeyIndex())) {
                phone.SetLookupKey(*lookupKey);
            }
        }
    }

    return contactsList;
}

} // namespace

// ~~~~ TTokenAccumulator ~~~~

void TTokenAccumulator::AddTokens(const TString name, const ELang lang, uint contactPosition) {
    names[NNlu::NormalizeText(name, static_cast<ELanguage>(lang))]++;
    const auto tokens = NNlu::TSmartTokenizer(name, static_cast<ELanguage>(lang)).GetNormalizedTokens();
    for (const auto& token : CombineTokens(tokens)) {
        tokenMasks[token].Set(contactPosition);
        if (contactPosition >= contactCount) {
            contactCount = contactPosition + 1;
        }
    }
}

THashMap<TDynBitMap, TVector<TString>> TTokenAccumulator::GetUniqueTokens() const {
    return GetTokenSets(true);
}

THashMap<TDynBitMap, TVector<TString>> TTokenAccumulator::GetCommonTokens() const {
    return GetTokenSets(false);
}

THashMap<TDynBitMap, TVector<TString>> TTokenAccumulator::GetTokenSets(bool uniqueTokens) const {
    THashMap<TDynBitMap, TVector<TString>> tokenSets;
    for (const auto& [token, mask] : tokenMasks) {
        if (uniqueTokens) {
            if (mask.Count() == 1 || names.contains(token) && names.at(token) == 1) {
                tokenSets[mask].push_back(token);
            }
        } else if (mask.Count() > 1 && (!names.contains(token) || names.at(token) != 1)) {
            tokenSets[mask].push_back(token);
        }
    }
    return tokenSets;
}

TVector<TString> TTokenAccumulator::CombineTokens(const TVector<TString>& tokens) {
    if (tokens.size() < 2) {
        return tokens;
    }

    TVector<TString> tokenCombinations(tokens);
    tokenCombinations.push_back(
        JoinSeq(" ", tokens)
    );
    for (size_t i = 0; i < tokens.size(); ++i) {
        for (size_t j = i + 1; j < tokens.size(); ++j) {
            tokenCombinations.push_back(
                TString::Join(tokens[i], " ", tokens[j])
            );
        }
    }

    return tokenCombinations;
}

// ~~~~ TPhone ~~~~

TPhone TPhone::FromProto(const TContactsList::TPhone& phone) {
    return {
        .Phone = phone.GetPhone(),
        .IdString = phone.GetIdString(),
        .Id = phone.GetId(),
    };
}

bool TPhone::operator<(const TPhone& other) const {
    return Phone < other.Phone;
}

// ~~~~ TContact ~~~~

TContact TContact::FromProto(const TContactsList::TContact& contact) {
    return {
        .DisplayName = contact.GetDisplayName(),
        .LookupKey = contact.GetLookupKey(),
    };
};

// ~~~~ TPhoneCall ~~~~

TPhoneCall::TPhoneCall(TRTLogger& logger,
                       TResponseBodyBuilder& bodyBuilder,
                       TNlgData& nlgData,
                       TNlgWrapper& nlgWrapper,
                       IRng& rng,
                       const TScenarioRunRequestWrapper& request,
                       TState& state)
    : Logger(logger)
    , BodyBuilder(bodyBuilder)
    , Request(request)
    , NlgData(nlgData)
    , NlgWrapper(nlgWrapper)
    , Rng(rng)
    , Lang(request.Proto().GetBaseRequest().GetUserLanguage())
    , ContactsList(
        GetContactsList(
            request.GetDataSource(NAlice::EDataSourceType::CONTACTS_LIST)
        )
    )
    , State(state)
    , CanOpenAddressBook(Request.BaseRequestProto().GetInterfaces().GetOpenAddressBook())
{
}

bool TPhoneCall::TryHandlePhoneCall() {
    if (!Request.BaseRequestProto().GetInterfaces().GetOutgoingPhoneCalls()) {
        LOG_WARNING(Logger) << "Skipping PhoneCall: client does not support outgoing phone calls";
        return false;
    }
    if (!Request.BaseRequestProto().GetInterfaces().GetPhoneAddressBook()) {
        LOG_WARNING(Logger) << "Skipping PhoneCall: client does not support phone address book";
        return false;
    }

    NlgData.Context["open_address_book"] = CanOpenAddressBook;

    bool success = false;
    bool userSpecificInfoRendered = false;

    // Handle callbacks first, because callbacks are intentionally produced by us
    if (const auto* callback = Request.Input().GetCallback()) {
        success = TryHandleCallback(*callback);
        userSpecificInfoRendered = success;
        if (!success) {
            LOG_WARNING(Logger) << "Skipping PhoneCall: failed to handle callback";
        }

    } else if (!HasValidFrame()) {
        LOG_WARNING(Logger) << "Skipping PhoneCall: no valid semantic frame or callback found";

    } else if (!GetUid(Request)) {
        HandleUnauthorizedUser();
        success = true;

    } else if (const auto missingPermissions = GetMissingPermissions()) {
        HandleRequestPermissions(missingPermissions);
        success = true;

    } else if (IsWaitingForContacts()) {
        HandleWaitingForContacts();
        success = true;

    } else if (const auto frame = FindPhoneCallFrame()) {
        HandlePhoneCall(TFrame::FromProto(*frame));
        success = true;
        userSpecificInfoRendered = true;

    } else if (const auto frame = FindPhoneCallExtendedFrame()) {
        HandlePhoneCall(TFrame::FromProto(*frame), /* callAllowed = */ false);
        success = true;
        userSpecificInfoRendered = true;

    } else if (HasNoTargetCallFrame()) {
        HandleContactNotFound();
        success = true;

    } else {
        LOG_WARNING(Logger) << "Skipping PhoneCall: failed to handle PhoneCall";
    }

    if (success) {
        SetSensitiveContentProperties();
        if (CanOpenAddressBook) {
            AddOpenAddressBookAction();
        }
        auto& analyticsInfoBuilder = GetAnalyticsInfoBuilder();
        analyticsInfoBuilder.SetProductScenarioName(NAlice::NProductScenarios::CALL);
        analyticsInfoBuilder.SetIntentName(TString{PHONE_CALL});

        if (userSpecificInfoRendered && Request.HasExpFlag(NExperiments::EXP_HW_PHONE_CALLS_STUB_RESPONSE)) {
            SetStubResponse();
        }
        if (Request.HasExpFlag(NExperiments::EXP_HW_ENABLE_PHONE_BOOK_ANALYTICS)) {
            AddPhoneBookAnalyticsInfo(ContactsList);
        }
    }

    return success;
}

void TPhoneCall::HandleUnauthorizedUser() {
    BodyBuilder.TryAddAuthorizationDirective(
        Request.Interfaces().GetCanOpenYandexAuth()
    );
    AddRenderedPhrase(USER_NOT_AUTHORIZED);
}

bool TPhoneCall::HasValidFrame() const {
    return FindPhoneCallFrame() != nullptr
        || FindPhoneCallExtendedFrame() != nullptr
        || HasNoTargetCallFrame();
}

const TPtrWrapper<TSemanticFrame> TPhoneCall::FindPhoneCallFrame() const {
    return Request.Input().FindSemanticFrame(PHONE_CALL_FRAME)
        ?: Request.Input().FindSemanticFrame(PHONE_CALL_CONTACT_FROM_ADDRESS_BOOK);
}

const TPtrWrapper<TSemanticFrame> TPhoneCall::FindPhoneCallExtendedFrame() const {
    return Request.Input().FindSemanticFrame(PHONE_CALL_ONE_WORD_EXTENSION_FRAME);
}

bool TPhoneCall::HasNoTargetCallFrame() const {
    return Request.Input().FindSemanticFrame(CALL_TO_FRAME) != nullptr;
}

void TPhoneCall::HandlePhoneCall(const TFrame& frame, const bool callAllowed) {
    if (const auto slot = frame.FindSlot("item_name")) {
        const auto contactKeys = ParseMatchedContactKeys(slot);
        auto matchedContacts = BuildContacts(contactKeys);

        LOG_INFO(Logger) << "PhoneCall: matched " << matchedContacts.size() << " contacts";
        Trim(matchedContacts, GetMaxContactsListSize());
        HandlePhoneCall(matchedContacts, callAllowed);
    } else {
        HandleContactNotFound();
    }
}

void TPhoneCall::HandleRequestPermissions(const THashSet<NAlice::TPermissions_EValue>& permissions) {
    LogRequestedPermissions(permissions);

    BodyBuilder.AddDirective(
        MakeRequestPermissionsDirective(permissions)
    );
    BodyBuilder.AddRenderedText(PHONE_CALL, REQUEST_READ_CONTACTS_PERMISSION, NlgData);
}

void TPhoneCall::HandleWaitingForContacts() {
    const ui32 waitCount = State.GetWaitingContactsCount();
    LOG_INFO(Logger) << "PhoneCall: waiting for contacts, waitCount: " << waitCount;
    if (waitCount <= WAITING_CONTACTS_TIMEOUT) {
        State.SetWaitingContactsCount(waitCount + 1);
        NlgData.Context[WAITING_FOR_CONTACTS] = waitCount;
        AddRenderedPhrase(WAITING_FOR_CONTACTS);
        return;
    }

    if (Request.HasExpFlag(NExperiments::EXP_ENABLE_OUTGOING_DEVICE_CALLS)) {
        AddRenderedPhrase(COULD_NOT_LOAD_CONTACTS_STATIONS);
    } else {
        AddRenderedPhrase(COULD_NOT_LOAD_CONTACTS);
    }
}

bool TPhoneCall::TryHandleCallback(const TCallbackDirective& callback) {
    if (callback.GetName() == CHOOSE_CONTACT_CALLBACK) {
        const auto contacts = BuildContactsFromPayload(callback);
        if (contacts.size() == 0) {
            LOG_ERROR(Logger) << "PhoneCall: failed to handle callback, "
                              << "can't find contacts from payload in the contacts list";
            return false;
        }
        AddListContactsAnalyticsInfo(contacts);
        HandlePhoneCall(contacts, /* callAllowed = */ true);
        return true;
    }
    if (callback.GetName() == ON_REQUEST_READ_CONTACTS_PERMISSIONS_SUCCESS) {
        AddRenderedPhrase(GRANTED_READ_CONTACTS_PERMISSION);
        return true;
    }
    if (callback.GetName() == ON_REQUEST_READ_CONTACTS_PERMISSIONS_FAIL) {
        AddRenderedPhrase(DENIED_READ_CONTACTS_PERMISSION);
        return true;
    }
    LOG_WARNING(Logger) << "PhoneCall: failed to handle callback, no valid callback found";
    return false;
}

void TPhoneCall::SetSensitiveContentProperties() {
    auto& contentProperties = *BodyBuilder.GetResponseBody().MutableLayout()->MutableContentProperties();
    contentProperties.SetContainsSensitiveDataInRequest(true);
    contentProperties.SetContainsSensitiveDataInResponse(true);
}

size_t TPhoneCall::GetMaxContactsListSize() const {
    const auto maxContactsListSizeExp = GetExperimentValueWithPrefix(
        Request.ExpFlags(),
        NExperiments::EXP_HW_PHONE_CALLS_MAX_DISPLAY_CONTACTS_PREFIX
    );
    if (!maxContactsListSizeExp.Defined()) {
        return DEFAULT_MAX_CONTACTS_LIST_SIZE;
    }
    return IntFromString<size_t, 10>(*maxContactsListSizeExp);
}

THashSet<NAlice::TPermissions_EValue> TPhoneCall::GetMissingPermissions() const {
    THashMap<TStringBuf, bool> permissionsInRequest;
    for (const auto& permissions : Request.Proto().GetBaseRequest().GetOptions().GetPermissions()) {
        permissionsInRequest[permissions.GetName()] = permissions.GetGranted();
    }

    THashSet<NAlice::TPermissions_EValue> missingPermissions;
    for (const auto& [permissionName, permission] : REQUIRED_PERMISSIONS) {
        if (!permissionsInRequest[permissionName]) {
            missingPermissions.insert(permission);
        }
    }
    return missingPermissions;
}

void TPhoneCall::LogRequestedPermissions(const THashSet<NAlice::TPermissions_EValue>& permissions) const {
    TVector<TStringBuf> permissionNames;
    for (const auto& permission : permissions) {
        permissionNames.push_back(NAlice::TPermissions::EValue_Name(permission));
    }
    LOG_INFO(Logger) << "PhoneCall: requesting permissions " << JoinSeq(", ", permissionNames);
}

bool TPhoneCall::IsWaitingForContacts() const {
    return !ContactsList.GetIsKnownUuid();
}

void TPhoneCall::HandlePhoneCall(const TVector<TContact>& matchedContacts, const bool callAllowed) {
    // If only one contact found - call it
    if (matchedContacts.size() == 1 && callAllowed) {
        AddListContactsAnalyticsInfo(matchedContacts);
        HandleSingleMatchedContact(matchedContacts[0]);

    // if several contacts are found (or the call is not allowed, i.e. when the match is fuzzy and likely wrong) -
    // let the user decide which one to call
    } else if (matchedContacts.size() >= 1) {
        AddListContactsAnalyticsInfo(matchedContacts);
        ListContacts(matchedContacts);

    // if no contacts are found - ask user to try again
    } else {
        HandleContactNotFound();
    }
}

TVector<TContact> TPhoneCall::BuildContacts(const TVector<TString>& contactKeys) const {
    TOrderedMap<TString, TContact> contacts(contactKeys);

    THashMap<TStringBuf, THashSet<TStringBuf>> accountTypes;

    // Searching contacts by lookup_key or lookup_index
    for (const auto& contactProto : ContactsList.GetContacts()) {
        if (NAlice::NContacts::IsContactFromMessenger(contactProto)) {
            continue;
        }

        TString contactIndex;
        if (contacts.contains(contactProto.GetLookupKey())) {
            contactIndex = contactProto.GetLookupKey();
        } else if (contacts.contains(ToString(contactProto.GetLookupKeyIndex()))) {
            contactIndex = ToString(contactProto.GetLookupKeyIndex());
        } else {
            continue;
        }

        contacts[contactIndex] = TContact::FromProto(contactProto);
        accountTypes[contactIndex].insert(
            contactProto.GetAccountType()
        );
    }

    // Matching phones with found contacts
    for (const auto& phone : ContactsList.GetPhones()) {
        TString contactIndex = ToString(phone.GetLookupKeyIndex());
        if (contacts.contains(phone.GetLookupKey())) {
            contactIndex = phone.GetLookupKey();
        } else if (contacts.contains(ToString(phone.GetLookupKeyIndex()))) {
            contactIndex = ToString(phone.GetLookupKeyIndex());
        } else {
            continue;
        }
        
        if (!accountTypes[contactIndex].contains(phone.GetAccountType())) {
            continue;
        }

        contacts[contactIndex].Phones.insert(
            TPhone::FromProto(phone)
        );
    }

    auto phoneContacts = std::move(contacts).MoveValues();
    EraseIf(phoneContacts, [](const TContact& contact) {
        return contact.Phones.empty();
    });

    return phoneContacts;
}

TVector<TContact> TPhoneCall::BuildContactsFromPayload(const TCallbackDirective& callback) const {
    const auto& payload = callback.GetPayload().fields();
    if (payload.contains(LOOKUP_KEYS)) {
        const auto& rawLookupKeys = payload.at(LOOKUP_KEYS).list_value().values();
        TVector<TString> extractedLookupKeys;
        for (const auto& key : rawLookupKeys) {
            extractedLookupKeys.push_back(key.string_value());
        }
        return BuildContacts(extractedLookupKeys);
    }
    if (payload.contains(PHONE_ID)) {
        const auto contact = BuildContact(
            payload.at(PHONE_ID).number_value(),
            payload.at(PHONE_ID_STRING).string_value()
        );
        return { *contact };
    }
    return {};
}

TMaybe<TContact> TPhoneCall::BuildContact(const long phoneId, const TString& phoneIdString) const {
    const auto* phoneProto = FindIfPtr(ContactsList.GetPhones(),
        [&phoneId, &phoneIdString](const TContactsList::TPhone& phone) {
            return phone.GetId() == phoneId && phone.GetIdString() == phoneIdString;
        }
    );
    if (phoneProto == nullptr) {
        return Nothing();
    }

    const auto* contactProto = FindIfPtr(ContactsList.GetContacts(),
        [phoneProto](const TContactsList::TContact& contact) {
            return contact.GetLookupKey() == phoneProto->GetLookupKey();
        }
    );
    if (contactProto == nullptr) {
        return Nothing();
    }

    auto contact = TContact::FromProto(*contactProto);
    contact.Phones.insert(
        TPhone::FromProto(*phoneProto)
    );
    return contact;
}

void TPhoneCall::HandleSingleMatchedContact(const TContact& contact) {
    LOG_INFO(Logger) << "PhoneCall: handling single matched contact with " << contact.Phones.size() << " phones";

    // If the contact thas only one phone - call it
    if (contact.Phones.size() == 1) {
        BodyBuilder.AddDirective(
            MakeCallDirective(contact.Phones.begin()->Phone)
        );
        NlgData.Context["calee_name"] = contact.DisplayName;
        AddRenderedPhrase(CALL);
        TryAddOpenAddressBookSuggest();

    // If the contact has multiple phones - list its phones
    } else if (contact.Phones.size() > 1) {
        HandleListPhoneNumbers(contact);
    }
}

void TPhoneCall::ListContacts(const TVector<TContact>& matchedContacts) {
    TTokenAccumulator tokenAcc;
    uint contactPosition = 0;
    for (const auto& contact : matchedContacts) {
        tokenAcc.AddTokens(TString{contact.DisplayName}, Lang, contactPosition);
        AddContactOrdinalAction(contact, contactPosition);
        ++contactPosition;
    }

    for (const auto& [mask, tokens] : tokenAcc.GetUniqueTokens()) {
        const int contactPos = mask.FirstNonZeroBit();
        AddContactVoiceAction(matchedContacts[contactPos], tokens, contactPos);
    }
    for (const auto& [mask, tokens] : tokenAcc.GetCommonTokens()) {
        AddContactSetVoiceAction(matchedContacts, mask, tokens);
    }

    if (Request.BaseRequestProto().GetInterfaces().GetCanRenderDiv2Cards()) {
        ListContactsDiv(matchedContacts);
    } else {
        ListContactsSuggests(matchedContacts);
    }
}

void TPhoneCall::ListContactsDiv(const TVector<TContact>& matchedContacts) {
    LOG_INFO(Logger) << "Rendering contacts list as div2";
    NJson::TJsonArray divCardContacts;

    for (const auto& contact : matchedContacts) {
        auto actionId = TString::Join(CHOOSE_CONTACT_DIV, "_", contact.LookupKey);
        BodyBuilder.AddAction(MakeChooseContactCallback({contact}), actionId);
        divCardContacts.AppendValue(
            NJson::TJsonMap({
                { "name", TString{contact.DisplayName} },
                { "action_id", actionId },
            })
        );
    }

    NlgData.Context["all_contacts_action_id"] = TString{OPEN_ADDRESS_BOOK_ACTION_ID};
    NlgData.Context["contacts"] = std::move(divCardContacts);

    AddRenderedDiv2Card(LIST_CONTACTS, CHOOSE_CALLEE);
    AddRandomOrdinalSuggest(1, 1);
    if (matchedContacts.size() > 1) {
        AddRandomOrdinalSuggest(2, matchedContacts.size());
    }
    AddRandomContactNameSuggest(matchedContacts);
    BodyBuilder.SetShouldListen(true);
}

void TPhoneCall::ListContactsSuggests(const TVector<TContact>& matchedContacts) {
    LOG_INFO(Logger) << "Rendering contacts list as suggests";

    AddRenderedPhrase(CHOOSE_CALLEE);
    for (const auto& contact : matchedContacts) {
        auto actionId = TString::Join(CHOOSE_CONTACT_DIV, "_", contact.LookupKey);
        BodyBuilder.AddAction(MakeChooseContactCallback({contact}), actionId);
        BodyBuilder.AddActionSuggest(actionId).Title(TString{contact.DisplayName});
    }
    TryAddOpenAddressBookSuggest();
    BodyBuilder.SetShouldListen(true);
}

void TPhoneCall::AddContactOrdinalAction(const TContact& contact, size_t contactPosition) {
    const auto actionId = TString::Join(CHOOSE_CONTACT_ORDINAL, "_", contact.LookupKey);
    AddAction(
        actionId,
        MakeChooseContactCallback({contact}),
        MakeOrdinalNluHint(
            actionId,
            RenderOrdinalPhrase(contactPosition + 1),
            contactPosition,
            Lang
        )
    );
}

void TPhoneCall::AddContactVoiceAction(const TContact& contact, const TVector<TString>& tokens, size_t contactPosition) {
    const auto actionId = TString::Join(CHOOSE_CONTACT, "_", contact.LookupKey);
    AddAction(
        actionId,
        MakeChooseContactCallback({contact}),
        MakeChooseContactNluHint(
            actionId,
            tokens,
            RenderOrdinalPhrase(contactPosition + 1),
            contactPosition,
            Lang,
            false
        )
    );
}

void TPhoneCall::AddContactSetVoiceAction(const TVector<TContact>& contacts, const TDynBitMap mask, const TVector<TString>& tokens) {
    TVector<TContact> contactSet;
    TStringBuilder maskId;
    for (uint i = 0; i < contacts.size(); ++i) {
        if (mask.Get(i)) {
            contactSet.push_back(contacts[i]);
            maskId << "1";
        } else {
            maskId << "0";
        }
    }
    const auto actionId = TString::Join(CHOOSE_CONTACT, "_SET_", maskId);
    AddAction(
        actionId,
        MakeChooseContactCallback(contactSet),
        MakeChooseContactNluHint(
            actionId,
            tokens,
            "",
            -1,
            Lang,
            false
        )
    );
}

void TPhoneCall::AddListContactsAnalyticsInfo(const TVector<TContact>& matchedContacts) {
    TAnalyticsInfo::TObject matchedContactsObject;
    matchedContactsObject.SetId("phone_contacts");
    matchedContactsObject.SetName("matched phone contacts");
    matchedContactsObject.SetHumanReadable("Найденные телефонные контакты");
    *matchedContactsObject.MutableMatchedContacts() = ToAnalyticsContactsList(matchedContacts);
    GetAnalyticsInfoBuilder().AddObject(matchedContactsObject);
}

void TPhoneCall::AddPhoneBookAnalyticsInfo(const TContactsList& contacts) {
    TAnalyticsInfo::TObject phoneBookObject;
    phoneBookObject.SetId("phone_book");
    phoneBookObject.SetName("phone book");
    phoneBookObject.SetHumanReadable("Контактная книга");
    *phoneBookObject.MutablePhoneBook()->MutableContacts() = contacts.GetContacts();
    GetAnalyticsInfoBuilder().AddObject(phoneBookObject);
}

TContactsList TPhoneCall::ToAnalyticsContactsList(const TVector<TContact>& contacts) const {
    TContactsList analyticsContactsList;
    for (const auto& contact : contacts) {
        auto& analyticsContact = *analyticsContactsList.AddContacts();
        analyticsContact.SetLookupKey(TString{contact.LookupKey});
    }
    return analyticsContactsList;
}

void TPhoneCall::HandleListPhoneNumbers(const TContact& contact) {
    NlgData.Context["calee_name"] = contact.DisplayName;
    if (Request.BaseRequestProto().GetInterfaces().GetCanRenderDiv2Cards()) {
        ListPhoneNumbersDiv(contact);
    } else {
        ListPhoneNumbersSuggests(contact);
    }
}

void TPhoneCall::ListPhoneNumbersDiv(const TContact& contact) {
    NJson::TJsonArray divCardPhones;

    for (const auto& phone : contact.Phones) {
        const auto phonePosition = divCardPhones.GetArray().size();
        const auto actionId = AddPhoneAction(phone, phonePosition);
        divCardPhones.AppendValue(
            NJson::TJsonMap({
                { "phone", TString{phone.Phone} },
                { "action_id", actionId },
            })
        );
    }

    NlgData.Context["phones"] = std::move(divCardPhones);

    AddRenderedDiv2Card(LIST_PHONE_NUMBERS, CHOOSE_PHONE_NUMBER);
    AddRandomOrdinalSuggest(1, 1);
    AddRandomOrdinalSuggest(2, contact.Phones.size());
    TryAddOpenAddressBookSuggest();
    BodyBuilder.SetShouldListen(true);
}

void TPhoneCall::ListPhoneNumbersSuggests(const TContact& contact) {
    size_t phonePosition = 0;
    for (const auto& phone : contact.Phones) {
        const auto actionId = AddPhoneAction(phone, phonePosition);
        BodyBuilder.AddActionSuggest(actionId).Title(TString{phone.Phone});
        ++phonePosition;
    }

    AddRenderedPhrase(CHOOSE_PHONE_NUMBER);
    TryAddOpenAddressBookSuggest();
    BodyBuilder.SetShouldListen(true);
}

TString TPhoneCall::AddPhoneAction(const TPhone& phone, size_t phonePosition) {
    const auto actionId = TString::Join(PHONE_CALL, "_", ToString(phonePosition));
    AddAction(
        actionId,
        MakeChoosePhoneCallback(phone),
        MakeOrdinalNluHint(
            actionId,
            RenderOrdinalPhrase(phonePosition + 1),
            phonePosition,
            Lang
        )
    );
    return actionId;
}

void TPhoneCall::HandleContactNotFound() {
    AddRenderedPhrase(CONTACT_NOT_FOUND);
    BodyBuilder.AddTypeTextSuggest(
        RenderPhrase(CALL_COMMON_CONTACT)
    );
    TryAddOpenAddressBookSuggest();
    BodyBuilder.SetShouldListen(true);
    BodyBuilder.AddNluHint(
        MakeFrameNluHint(PHONE_CALL_CONTACT_FROM_ADDRESS_BOOK)
    );
}

IAnalyticsInfoBuilder& TPhoneCall::GetAnalyticsInfoBuilder() {
    if (!BodyBuilder.HasAnalyticsInfoBuilder()) {
        BodyBuilder.CreateAnalyticsInfoBuilder();
    }
    return BodyBuilder.GetAnalyticsInfoBuilder();
}

void TPhoneCall::AddRenderedPhrase(const TStringBuf phrase, const TVector<TLayout::TButton>& buttons) {
    BodyBuilder.AddRenderedTextWithButtonsAndVoice(PHONE_CALL, phrase, buttons, NlgData);
}

void TPhoneCall::AddRenderedDiv2Card(const TStringBuf card, const TStringBuf voicePhrase) {
    if (Request.BaseRequestProto().GetInterfaces().GetCanRenderDiv2Cards()) {
        BodyBuilder.AddRenderedVoice(PHONE_CALL, voicePhrase, NlgData);
        BodyBuilder.AddRenderedDiv2Card(CONTACTS_CARD, card, NlgData);

    // Fallback to the regular NLG if the client doesn't support div2 cards
    } else {
        AddRenderedPhrase(voicePhrase);
        BodyBuilder.AddRenderedText(PHONE_CALL, card, NlgData);
    }
}

void TPhoneCall::SetStubResponse() {
    auto& layout = *BodyBuilder.GetResponseBody().MutableLayout();
    layout.ClearCards();
    layout.ClearOutputSpeech();
    layout.ClearDirectives();
    layout.ClearSuggestButtons();
    AddRenderedPhrase(STUB_RESPONSE);
}

void TPhoneCall::AddRandomOrdinalSuggest(const size_t minOrdinal, const size_t maxOrdinal) {
    BodyBuilder.AddTypeTextSuggest(
        RenderOrdinalPhrase(Rng.RandomInteger(minOrdinal, maxOrdinal + 1))
    );
}

void TPhoneCall::AddRandomContactNameSuggest(const TVector<TContact>& contacts) {
    BodyBuilder.AddTypeTextSuggest(
        TString{Choice(contacts, Rng).DisplayName}
    );
}

void TPhoneCall::TryAddOpenAddressBookSuggest() {
    if (CanOpenAddressBook) {
        BodyBuilder.AddTypeTextSuggest(
            RenderPhrase(OPEN_ADDRESS_BOOK_SUGGEST)
        );
    }
}

TString TPhoneCall::RenderOrdinalPhrase(const size_t ordinal) const {
    NlgData.Context[ORDINAL] = ordinal;
    return RenderPhrase(ORDINAL);
}

TString TPhoneCall::RenderPhrase(const TStringBuf phrase) const {
    return NlgWrapper.RenderPhrase(PHONE_CALL, phrase, NlgData).Text;
}

TDirective TPhoneCall::MakeCallDirective(const TStringBuf phone) const {
    TDirective directive;
    directive.MutableOpenUriDirective()->SetUri(
        GeneratePhoneUri(Request.ClientInfo(), TString{phone}, true, true)
    );
    return directive;
}

TDirective TPhoneCall::MakeOpenAddressBookDirective() const {
    TDirective directive;
    directive.MutableOpenUriDirective()->SetUri("contacts://address_book");
    return directive;
}

TDirective TPhoneCall::MakeChooseContactCallback(const TVector<TContact>& contacts) const {
    TDirective directive;
    auto& payload = PrepareChooseContactPayload(directive);
    for (const auto& contact : contacts) {
        auto lookupKey = payload[LOOKUP_KEYS].mutable_list_value()->mutable_values()->Add();
        lookupKey->set_string_value(TString{contact.LookupKey});
    }
    return directive;
}

TDirective TPhoneCall::MakeChoosePhoneCallback(const TPhone& phone) const {
    TDirective directive;
    auto& payload = PrepareChooseContactPayload(directive);
    payload[PHONE_ID].set_number_value(phone.Id);
    payload[PHONE_ID_STRING].set_string_value(TString{phone.IdString});

    return directive;
}

TDirective TPhoneCall::MakeRequestPermissionsDirective(const THashSet<NAlice::TPermissions_EValue>& permissions) const {
    TDirective directive;
    auto& requestPermissionsDirective = *directive.MutableRequestPermissionsDirective();
    for (const auto& permission : permissions) {
        requestPermissionsDirective.AddPermissions(permission);
    }
    requestPermissionsDirective.MutableOnSuccess()->MutableCallbackDirective()->SetName(
        TString{ON_REQUEST_READ_CONTACTS_PERMISSIONS_SUCCESS}
    );
    requestPermissionsDirective.MutableOnFail()->MutableCallbackDirective()->SetName(
        TString{ON_REQUEST_READ_CONTACTS_PERMISSIONS_FAIL}
    );

    return directive;
}

void TPhoneCall::AddOpenAddressBookAction() {
    AddAction(
        OPEN_ADDRESS_BOOK_ACTION_ID,
        MakeOpenAddressBookDirective(),
        MakeOpenAddressBookNluHint()
    );
}

void TPhoneCall::AddAction(const TStringBuf actionId, TDirective&& directive, TFrameNluHint&& nluHint) {
    TString actionIdStr{actionId};
    BodyBuilder.AddAction(std::move(directive), std::move(nluHint), actionIdStr);
}

} // namespace NAlice::NHollywood

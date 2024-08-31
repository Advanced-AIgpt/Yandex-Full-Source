#include "avatars.h"
#include "contacts.h"
#include "contacts_finder.h"
#include "translit.h"

#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/string_utils/levenshtein_diff/levenshtein_diff.h>
#include <library/cpp/telfinder/text_telfinder.h>

#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>
#include <util/generic/ylimits.h>
#include <util/string/ascii.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/join.h>

namespace {

using NBASS::TContext;

constexpr TStringBuf REQUEST_FULL_MATCH_FIELD = "request_full_match";
constexpr TStringBuf SEARCH_TAG_FIELD = "search_tag";

NSc::TValue GetFailCallback(const TContext& ctx) {
    TContext callback(ctx, ctx.FormName());
    callback.CopySlotsFrom(ctx, {"recipient"});
    callback.CreateSlot("permission", "string", true, NSc::TValue("READ_CONTACTS"));

    return callback.ToJson(TContext::EJsonOut::FormUpdate | TContext::EJsonOut::Resubmit);
}

TString NormalizePhone(TStringBuf phone) {
    TString phoneClean(Reserve(phone.size()));
    for (size_t i = 0; i < phone.size(); ++i) {
        char ch = phone[i];
        if (IsAsciiDigit(ch))
            phoneClean.push_back(ch);
    }
    return phoneClean;
}

TString ParsePhoneNumber(TStringBuf str) {
    constexpr size_t validRussianPhoneNumberDigitsCount = 11;

    TTelFinder telFinder;
    TTextTelProcessor finder(&telFinder);
    finder.ProcessText(TUtf16String::FromAscii(str));
    TFoundPhones phones;
    finder.GetFoundPhones(phones);

    LOG(DEBUG) << "found " << phones.size() << " phone numbers by telfinder: " << str << Endl;
    switch (phones.size()) {
        case 0: {
            // https://wiki.yandex-team.ru/users/igoshkin/russia-phones-format/#rezjume
            TString phone = NormalizePhone(str);
            if (phone.length() == validRussianPhoneNumberDigitsCount && phone[0] == '8')
                return phone.substr(1);

            LOG(WARNING) << "unusual phone number found (can't parse by telfinder and len != 11 or not prefixed by 8): " << str << Endl;
            return TString{str};
        }
        case 1: {
            return phones[0].Phone.ToPhoneWithArea();
        }
        default: {
            LOG(WARNING) << "more than one phone found by telfinder in str: " << str << Endl;
            return TString{str};
        }
    }
}

TString MakeGroupKey(const NSc::TValue& contact) {
    TVector<TString> phones;
    for (auto& item : contact.GetArray()) {
        phones.push_back(NormalizePhone(ParsePhoneNumber(item["phone"].GetString())));
    }

    SortUnique(phones);
    return JoinSeq("" /* delim */, phones);
}

void RemoveGroupDuplicates(TVector<NSc::TValue>& contacts) {
    THashMap<TString, NSc::TValue> uniqContacts;
    for (auto& contact : contacts) {
        uniqContacts.insert(std::make_pair(MakeGroupKey(contact), contact));
    }

    contacts.clear();
    for (auto& item : uniqContacts) {
        contacts.push_back(item.second);
    }
}

} // namespace

namespace NBASS {

NSc::TValue GetFindContactsPayload(const TContext& ctx) {
    NSc::TValue payload = NContactsFinder::PrepareClientRequestData(ctx.GetSlot("recipient"), ctx.GetSlot("personal_asr_value"));
    payload["form"] = ctx.FormName();
    payload["on_permission_denied_payload"] = GetFailCallback(ctx);
    return payload;
}

TContacts::TContacts(const NSc::TValue& items, const TContext& ctx)
    : Items(items)
    , Ctx(ctx)
{
}

void TContacts::MakePhonesUnique(NSc::TValue& phones) {
    THashMap<TString, NSc::TValue> uniquePhones;
    for (const auto& item: phones.GetArray()) {
        TStringBuf phoneNumber = item["phone"].GetString();
        auto number = NormalizePhone(ParsePhoneNumber(phoneNumber));
        LOG(DEBUG) << "normalized phone: " << number << Endl;
        uniquePhones[number] = item;
    }

    NSc::TValue values;
    for (const auto& item: uniquePhones) {
        values.GetArrayMutable().push_back(item.second);
    }

    phones = values;
}

bool TContacts::IsGrouped() const {
    return Items.GetArray().back().Has("phones");
}

THashMap<i64, NSc::TValue> TContacts::GroupBy(TStringBuf attribute) const {
    THashMap<i64, NSc::TValue> groups;
    for (const auto& item : Items.GetArray()) {
        const i64 key = item[attribute].GetIntNumber();
        NSc::TValue contact;
        contact.CopyFrom(item);
        if (TStringBuf phone = item["phone"].GetString())
            contact["phone_uri"] = GeneratePhoneUri(Ctx.MetaClientInfo(), phone, false /* don't normalize */, false /* prefix free */);

        groups[key].Push(contact);
    }
    return groups;
}

void TContacts::Reorder(TVector<NSc::TValue>& items, TStringBuf relevantText) const {
    auto f = [relevantText](NSc::TValue& value) {
        auto& firstItem = value.GetArrayMutable().front();
        const auto& name = firstItem["name"].GetString();
        firstItem[REQUEST_FULL_MATCH_FIELD].SetBool(ToLowerUTF8(name) == relevantText);
    };

    ForEach(items.begin(), items.end(), f);
    std::stable_partition(items.begin(), items.end(), [](const NSc::TValue& v){ return v.GetArray()[0][REQUEST_FULL_MATCH_FIELD].GetBool(); });
}

template <typename T>
std::pair<size_t, size_t> GetWordLimits(const T& sentence, const T& word) {
    static const auto SPACE = u" ";

    size_t start = sentence.find(word, 0);
    if (start == T::npos)
        return std::make_pair(T::npos, T::npos);

    size_t end = sentence.find(SPACE, start + word.length());
    return std::make_pair(start, end);
}

template <typename T>
bool IsWord(const T& sentence, const T& substr) {
    auto limits = GetWordLimits(sentence, substr);
    if (sentence.length() == substr.length())
        return true;
    // 'ivanov' -> 'ivanov ivan'
    if (limits.first == 0 && limits.second != T::npos && limits.second + 1 == substr.length())
        return true;
    // 'ivanov' -> 'ivan ivanov'
    if (limits.first != 0 && limits.second == T::npos && limits.first + substr.length() == sentence.length())
        return true;
    // 'ivanov' -> 'sir ivanov ivan'
    if (limits.first != 0 && limits.second != T::npos && limits.second - limits.first + 1 == substr.length())
        return true;

    return false;
}

void TContacts::Filter(const TUtf16String& surname) {
    static const size_t maxDistance = 3;

    if (!surname || surname.length() <= NContactsFinder::SURNAME_PREFIX_LEN || Items.GetArray().empty())
        return;

    NSc::TValue filtered;
    filtered.SetArray();
    size_t minDistance = Max<size_t>();
    TUtf16String surnamePrefix = surname.substr(0, NContactsFinder::SURNAME_PREFIX_LEN);
    TMultiMap<size_t, NSc::TValue> extra;

    for (const auto& item : Items.GetArray()) {
        TUtf16String contactName = UTF8ToWide(item["name"].GetString());
        contactName.to_lower();

        if (contactName.Contains(surname) && IsWord(contactName, surname))
            filtered.Push(item);
        else {
            if (!filtered.GetArray().empty())
                continue;

            auto limits = GetWordLimits(contactName, surnamePrefix);
            size_t start = limits.first;
            size_t end = limits.second;

            if (start == TUtf16String::npos) {
                filtered.Push(item);
                continue;
            }

            size_t len = (end == TUtf16String::npos) ? contactName.length() - start : end - start;
            auto substr = contactName.substr(start, len);
            size_t distance = NLevenshtein::Distance(surname, substr);
            LOG(DEBUG) << "Distance between " << surname << " and " << substr << " = " << distance << Endl;
            extra.insert(std::make_pair(distance, item));
            minDistance = std::min(distance, minDistance);
        }
    }

    if (filtered.GetArray().empty() && minDistance <= maxDistance) {
        auto range = extra.equal_range(minDistance);
        auto currIt = range.first;
        auto lastIt = range.second;
        for (; currIt != lastIt; ++currIt) {
            filtered.Push(std::move(currIt->second));
        }
    }

    Items = filtered;
}

NSc::TValue TContacts::Preprocess(const TSlot& recipient) {
    NSc::TValue result;
    if (Items.GetArray().empty() || IsGrouped())
        return Items;

    TStringBuf text = recipient.SourceText;

    auto* searchTag = Items.GetArrayMutable()[0].GetNoAdd(SEARCH_TAG_FIELD);
    if (searchTag == nullptr || searchTag->GetString() == NContactsFinder::SEARCH_TAG_PREFIX) {
        TStringBuf surname = NContactsFinder::GetPossibleSurname(recipient, SplitString(text.data(), " ").size());
        Filter(UTF8ToWide(surname));
    }

    TVector<NSc::TValue> groups;
    for (const auto& item : GroupBy("contact_id")) {
        groups.push_back(item.second);
    }

    RemoveGroupDuplicates(groups);
    Reorder(groups, text);

    for (const auto& group : groups) {
        NSc::TValue& contact = result.Push();
        contact["name"] = group.GetArray()[0]["name"].GetString();
        contact["phones"] = group;
        contact["avatar_url"] = NContacts::NAvatars::MakeUrl(contact, Ctx);
        contact[REQUEST_FULL_MATCH_FIELD] = group.GetArray()[0][REQUEST_FULL_MATCH_FIELD];

        MakePhonesUnique(contact["phones"]);
    }

    return result;
}

} // namespace NBASS

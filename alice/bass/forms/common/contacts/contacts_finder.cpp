#include "contacts_finder.h"
#include "synonyms.h"
#include "translit.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <dict/dictutil/str.h>

#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/generic/strbuf.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>
#include <util/string/split.h>

namespace {

using NBASS::NCall::FioTranslit;

constexpr std::array<TStringBuf, 3> FIO_COMPONENTS = {
    TStringBuf("name"),
    TStringBuf("patronym"),
    TStringBuf("surname")
};

struct TContactsMimeWhitelist {
public:
    TContactsMimeWhitelist() {
        NSc::TValue types;
        NSc::TValue columns;

        types.AppendAll(
            {
                "vnd.android.cursor.item/phone_v2",
                "vnd.android.cursor.item/vnd.com.viber.voip.viber_out_call_none_viber",
                "vnd.android.cursor.item/com.skype4life.phone",
                "vnd.android.cursor.item/vnd.org.telegram.messenger.android.profile"
            }
        );

        columns.AppendAll({"data1", "data1", "data1", "data3"});

        // https://st.yandex-team.ru/ASSISTANT-2932#5c139fe6863a5f001c920915
        Data["name"] = types;
        Data["column"] = columns;
    }

    NSc::TValue Data;
};

const TStringBuf TYPE_FIO = "fio";
const TStringBuf TYPE_STRING = "string";

template<typename TItemsType>
void AddStringItemsToArray(NSc::TValue& values, const TItemsType& items) {
    values.SetArray();
    for (auto& item: items) {
        values.Push().SetString(item);
    }
}

bool RemoveLastSymbol(TString& value) {
    static constexpr int minWordLen = 3;

    TUtf16String wideValue = UTF8ToWide(value);
    if (wideValue.length() <= minWordLen)
        return false;

    TWtringBuf buf = wideValue;
    buf.Chop(1);
    value = WideToUTF8(buf);
    return true;
}

class TTokenizer {
public:
    explicit TTokenizer(const TString& data) {
        Split(data, " ", Tokens);
    }

    size_t GetTokensCount() const {
        return Tokens.size();
    }

    const TVector<TString>& GetTokens() const {
        return Tokens;
    }
private:
    TVector<TString> Tokens;
};

TVector<TString> GetContactSearchQueryTexts(TStringBuf request, int numTokens) {
    TVector<TString> result;
    TString req = TString{request};
    if (numTokens > 1)
        ReplaceAll(req, ' ', '%');

    // simulate word boundared search by like operator
    result.push_back(req);
    result.push_back(TStringBuilder() << request << " %");
    result.push_back(TStringBuilder() << "% " << request);

    if (numTokens == 1)
        result.push_back(TStringBuilder() << "% " << request << " %");

    return result;
}

TString GetTitle(const NBASS::TSlot& recipient) {
    auto result = (recipient.Type == TYPE_FIO) ? recipient.SourceText : recipient.Value;
    return TString{result.GetString()};
}

TString FormatFio(const NBASS::TSlot& slot) {
    if (slot.Type != TYPE_FIO)
        return "";

    auto& fio = slot.Value.GetDict();
    TVector<TString> fioWords;
    for (const auto& component : FIO_COMPONENTS) {
        const NSc::TValue value = fio.Get(component);
        if (value != NSc::TValue::DefaultValue())
            fioWords.push_back(TString{value.GetString()});
    }
    return JoinStrings(fioWords, " ");
}

void TreatGenderError(TStringBuf value, THashSet<TString>& values) {
    TString copy = TString{value};
    // fix for male/female ending mismatch
    if (!RemoveLastSymbol(copy))
        return;

    values.insert(copy); // позвони александру -> type=fio, name=александра (bug in vins (inflector)) -> александр
    values.insert(copy + "_"); // позвони михаилу -> type=fio, name=михаил -> михаи_
}

TVector<TString> GetSurnamePrefixValues(TStringBuf name, TStringBuf surname) {
    using NBASS::NContactsFinder::SURNAME_PREFIX_LEN;

    TVector<TString> result;
    if (surname && UTF8ToWide(surname).length() > SURNAME_PREFIX_LEN) {
        TString substr = TString{SubstrUTF8(surname, 0, SURNAME_PREFIX_LEN)} + '%';
        if (name) {
            result.push_back(substr + " " + name);
            result.push_back(TString{name} + " " + substr);
        } else {
            result.push_back(substr);
            result.push_back("% " + substr);
        }
    }
    return result;
}

void TreatWrongDetectedSurname(const NBASS::TSlot& slot, size_t numTokens, THashSet<TString>& values) {
    TStringBuf name, surname;

    if (slot.Type == TYPE_FIO) {
        name = slot.Value.GetDict().Get("name").GetString();
        surname = slot.Value.GetDict().Get("surname").GetString();
    } else if (slot.Type == TYPE_STRING && numTokens == 1) {
        surname = slot.Value;
    }

    auto texts = GetSurnamePrefixValues(name, surname);
    values.insert(texts.begin(), texts.end());
}

THashSet<TString> GetBasicValues(const NBASS::TSlot& recipientSlot, const TTokenizer& tokenizer, TMaybe<TString> personalAsrValue = {}) {
    THashSet<TString> values;
    const TString title = GetTitle(recipientSlot);
    auto strFio = FormatFio(recipientSlot);

    if (tokenizer.GetTokensCount() == 1) {
        auto synonyms = NBASS::TClusters::Instance().GetCluster(title);
        // typical situation: позвони гошану -> normalize_to(nomn) = гошана (source_text) -> type fio, name = гошан
        // GetCluster(гошана) -> empty, GetCluster(гошан) - size > 0
        if (synonyms.empty() && !strFio.empty() && strFio != title)
            synonyms = NBASS::TClusters::Instance().GetCluster(strFio);

        for (auto& item: synonyms) {
            const auto texts = GetContactSearchQueryTexts(item, 1);
            values.insert(texts.begin(), texts.end());
        }

        const auto texts = GetContactSearchQueryTexts(title, 1);
        values.insert(texts.begin(), texts.end());

        for (auto& item: FioTranslit(title)) {
            const auto texts = GetContactSearchQueryTexts(item, 1);
            values.insert(texts.begin(), texts.end());
        }
    } else {
        auto titleTexts = GetContactSearchQueryTexts(title, tokenizer.GetTokensCount());
        values.insert(titleTexts.begin(), titleTexts.end());

        if (tokenizer.GetTokensCount() == 2) {
            const auto& tokens = tokenizer.GetTokens();
            auto lastToken = tokens.back();
            if (lastToken.length() == 1 && IsAsciiDigit(lastToken[0]))
                values.insert(tokens.front() + tokens.back());

            values.insert(tokens.back() + " " + tokens.front());

            TString iso1, iso2;
            if (NBASS::IsoTranslit(tokens[0], iso1)) {
                if (NBASS::IsoTranslit(tokens[1], iso2))
                    values.insert(iso1 + ' ' + iso2);

                TString secondWord = tokens[1];
                if (RemoveLastSymbol(secondWord) && NBASS::IsoTranslit(secondWord, iso2))
                    values.insert(iso1 + ' ' + iso2);
            }
        }
    }

    if (!strFio.empty()) {
        const auto texts = GetContactSearchQueryTexts(strFio, tokenizer.GetTokensCount());
        values.insert(texts.begin(), texts.end());
    }

    if (recipientSlot.Type == TYPE_STRING)
        TreatGenderError(title, values);

    if (personalAsrValue)
        values.insert(*personalAsrValue);

    return values;
}

} // anonymous namespace

namespace NBASS {
namespace NContactsFinder {

TStringBuf GetPossibleSurname(const NBASS::TSlot& slot, size_t numTokens) {
    NSc::TValue value;
    if (slot.Type == TYPE_STRING && numTokens == 1)
        value = slot.Value;
    else if (slot.Type == TYPE_FIO)
        value = slot.Value.GetDict().Get("surname");

    return value.GetString();
}

NSc::TValue PrepareClientRequestData(const TSlot* recipient, const TSlot* personalAsr) {
    static const TContactsMimeWhitelist whitelist;

    if (IsSlotEmpty(recipient)) {
        LOG(WARNING) << "Empty mandatory slot 'recipient'" << Endl;
        return NSc::Null();
    }

    NSc::TValue data;
    data["mimetypes_whitelist"] = whitelist.Data;

    TTokenizer tokenizer(GetTitle(*recipient));

    TMaybe<TString> asrValue;
    if (!IsSlotEmpty(personalAsr))
        asrValue = personalAsr->Value.GetString();

    THashSet<TString> values = GetBasicValues(*recipient, tokenizer, asrValue);

    AddStringItemsToArray(data["values"], values);

    auto& request = data["request"].SetArray();

    NSc::TValue requestData;
    requestData["values"] = data["values"].Clone();
    requestData["tag"].SetString(SEARCH_TAG_BASIC);
    request.Push(requestData);

    values.clear();
    TreatWrongDetectedSurname(*recipient, tokenizer.GetTokensCount(), values);
    if (!values.empty()) {
        AddStringItemsToArray(data["values"], values);

        NSc::TValue requestData;
        AddStringItemsToArray(requestData["values"], values);
        requestData["tag"].SetString(SEARCH_TAG_PREFIX);
        request.Push(requestData);
    }

    if (tokenizer.GetTokensCount() == 2) {
        TVector<TStringBuf> tokens;
        if (recipient->Type == TYPE_FIO) {
            const auto& fio = recipient->Value.GetDict();
            for (const auto& item: {TStringBuf("surname"), TStringBuf("name")}) {
                const NSc::TValue& value = fio.Get(item);
                if (value != NSc::TValue::DefaultValue())
                    tokens.push_back(value.GetString());
            }
        } else if (recipient->Type == TYPE_STRING) {
            for (const auto& token : tokenizer.GetTokens()) {
                tokens.push_back(token);
            }
        }

        for (const auto& token: tokens) {
            NSc::TValue requestData;

            TVector<TString> partialValues = FioTranslit(token);
            TString isoValue;
            if (NBASS::IsoTranslit(token, isoValue))
                partialValues.push_back(isoValue);

            partialValues.push_back(TString{token});

            for (const auto& item: partialValues) {
                // we using tokens count = 2 to get all request variants despite real tokens count value
                const auto& values = GetContactSearchQueryTexts(item, 2);
                AddStringItemsToArray(requestData["values"], values);
            }

            requestData["tag"].SetString(PARTIAL_TAG_PREFIX);
            request.Push(requestData);
        }
    }

    return data;
}

bool HandlePermission(TStringBuf permission, TContext& ctx) {
    auto slot = ctx.GetSlot(permission);
    const bool empty = IsSlotEmpty(slot);

    if (!empty) {
        TString permission = TString{slot->Value.GetString()};
        permission.to_lower();

        ctx.AddAttention(TStringBuilder() << "permission_" << permission << "_denied");
    }

    return empty;

}

} // namespace NContactsFinder
} // namespace NBASS

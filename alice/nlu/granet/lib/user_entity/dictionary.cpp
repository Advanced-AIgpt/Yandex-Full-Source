#include "dictionary.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/stream/buffer.h>

namespace NGranet::NUserEntity {

namespace NScheme {
    static const TStringBuf EntityName = "entity_name";
    static const TStringBuf EnablePartialMatch = "enable_partial_match";
    static const TStringBuf Values = "values";
    static const TStringBuf Texts = "texts";
    static const TStringBuf EntityFlags = "entity_flags";
}

// ~~~~ TEntityDict ~~~~

TEntityDict::TEntityDict(TStringBuf entityName, EEntityDictFlags flags)
    : EntityName(entityName)
    , Flags(flags)
{
}

NSc::TValue TEntityDict::ToTValue() const {
    NSc::TValue result;
    result[NScheme::EntityName] = EntityName;
    if (Flags.HasFlags(EDF_ENABLE_PARTIAL_MATCH)) {
        result[NScheme::EnablePartialMatch] = true;
    }
    // For more compact representation store items as two parallel arrays and map (unique_entity_flags->indexes_of_items).
    NSc::TArray& values = result[NScheme::Values].GetArrayMutable();
    NSc::TArray& texts = result[NScheme::Texts].GetArrayMutable();
    for (const TEntityDictItem& item : Items) {
        values.push_back(NSc::TValue(item.Value));
        texts.push_back(NSc::TValue(item.Text));
    }
    for (size_t i = 0; i < Items.size(); ++i) {
        const TString& entityFlags = Items[i].EntityFlags;
        if (!entityFlags.empty()) {
            result[NScheme::EntityFlags][entityFlags].Push(i);
        }
    }
    return result;
}

void TEntityDict::FromTValue(const NSc::TValue& dict, const bool validate) {
    EntityName = dict[NScheme::EntityName].GetString("");

    Flags = 0;
    SetFlags(&Flags, EDF_ENABLE_PARTIAL_MATCH, dict[NScheme::EnablePartialMatch].GetBool(false));

    Items.clear();
    const NSc::TArray& values = dict[NScheme::Values].GetArray();
    const NSc::TArray& texts = dict[NScheme::Texts].GetArray();
    if (values.size() != texts.size()) {
        Y_ENSURE(!validate, "Invalid TEntityDict scheme.");
        return;
    }
    for (size_t i = 0; i < values.size(); ++i) {
        TEntityDictItem& item = Items.emplace_back();
        item.Value = values[i].GetString();
        item.Text = texts[i].GetString();
    }
    for (const auto& [entityFlags, indexes] : dict[NScheme::EntityFlags].GetDict()) {
        for (const size_t index : indexes.GetArray()) {
            if (index >= Items.size() || !Items[index].EntityFlags.empty()) {
                Y_ENSURE(!validate, "Invalid TEntityDict scheme.");
                continue;
            }
            Items[index].EntityFlags = entityFlags;
        }
    }
}

void TEntityDict::Dump(IOutputStream* out, const TString& indent) const {
    Y_ENSURE(out);
    *out << indent << "TEntityDict:" << Endl;
    *out << indent << "  EntityName: " << EntityName << Endl;
    *out << indent << "  Flags: " << FormatFlags(Flags) << Endl;
    *out << indent << "  Items (value extra: text):" << Endl;
    for (const TEntityDictItem& item : Items) {
        *out << indent << "    " << item.Value << ' ' << item.EntityFlags << ": " << item.Text << Endl;
    }
}

// ~~~~ TEntityDicts ~~~~

TEntityDict& TEntityDicts::AddDict(TStringBuf entityName, EEntityDictFlags flags) {
    return *Dicts.emplace_back(MakeHolder<TEntityDict>(entityName, flags));
}

NSc::TValue TEntityDicts::ToTValue() const {
    NSc::TValue value;
    value.SetArray();
    for (const TEntityDictPtr& dict : Dicts) {
        value.Push(dict->ToTValue());
    }
    return value;
}

void TEntityDicts::FromTValue(const NSc::TValue& value, const bool validate) {
    Dicts.clear();
    if (value.IsNull()) {
        return;
    }
    Y_ENSURE(!validate || value.IsArray(), "not valid input scheme");
    for (const NSc::TValue& dictValue : value.GetArray()) {
        NJsonConverters::FromTValue(dictValue, AddDict(), validate);
    }
}

TString TEntityDicts::ToBase64() const {
    return Base64EncodeUrl(ToJson(true));
}

void TEntityDicts::FromBase64(TStringBuf base64) {
    FromJson(Base64DecodeUneven(base64), true);
}

void TEntityDicts::Dump(IOutputStream* out, const TString& indent) const {
    Y_ENSURE(out);
    *out << indent << "TEntityDicts:" << Endl;
    *out << indent << "  Dicts:" << Endl;
    for (const TEntityDictPtr& dict : Dicts) {
        dict->Dump(out, indent + "    ");
    }
}

} // namespace NGranet::NUserEntity

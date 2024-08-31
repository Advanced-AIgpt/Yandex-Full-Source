#pragma once

#include <alice/nlu/granet/lib/utils/flag_utils.h>
#include <library/cpp/scheme/scheme_cast.h>
#include <library/cpp/scheme/scheme.h>
#include <util/generic/cast.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGranet::NUserEntity {

// ~~~~ EEntityDictFlag ~~~~

enum EEntityDictFlag : ui32 {
    // Enable match by meaningful prefixes and suffixes of item.
    EDF_ENABLE_PARTIAL_MATCH = FLAG32(0),
};

Y_DECLARE_FLAGS(EEntityDictFlags, EEntityDictFlag);
Y_DECLARE_OPERATORS_FOR_FLAGS(EEntityDictFlags);

// ~~~~ TEntityDictItem ~~~~

struct TEntityDictItem {
    // Value associated with Text. For TEntity::Value.
    TString Value;
    // For TEntity::Flags
    TString EntityFlags;
    // Search text.
    TString Text;
};

// ~~~~ TEntityDict ~~~~

class TEntityDict : public NJsonConverters::IJsonSerializable, public TMoveOnly {
public:
    // Entity type name for TEntity::Type.
    TString EntityName;
    // Options for entity finder.
    EEntityDictFlags Flags;
    // Search texts with associated values.
    TVector<TEntityDictItem> Items;

public:
    explicit TEntityDict(TStringBuf entityName = {}, EEntityDictFlags flags = 0);

    NSc::TValue ToTValue() const override;
    void FromTValue(const NSc::TValue& value, const bool validate) override;

    void Dump(IOutputStream* out, const TString& indent = "") const;
};

using TEntityDictPtr = THolder<TEntityDict>;

// ~~~~ TEntityDicts ~~~~

class TEntityDicts : public NJsonConverters::IJsonSerializable, public TMoveOnly {
public:
    TVector<TEntityDictPtr> Dicts;

public:
    TEntityDicts() = default;

    TEntityDict& AddDict(TStringBuf entityName = {}, EEntityDictFlags flags = 0);

    NSc::TValue ToTValue() const override;
    void FromTValue(const NSc::TValue& value, const bool validate) override;

    TString ToBase64() const;
    void FromBase64(TStringBuf base64);

    void Dump(IOutputStream* out, const TString& indent = "") const;
};

using TEntityDictsPtr = THolder<TEntityDicts>;

} // namespace NGranet::NUserEntity

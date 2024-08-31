#pragma once

#include "string_matcher.h"
#include <alice/hollywood/library/scenarios/food/proto/cart.pb.h>
#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>
#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/json/writer/json_value.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood::NFood {

// ~~~~ TNluCartItem ~~~~

struct TNluCartItem {
    TString SpokenName;
    TString NameId;
    bool IsQuantityDefinedByUser = false;
    ui32 Quantity = 1;

    DECLARE_TUPLE_LIKE_TYPE(TNluCartItem, SpokenName, NameId, IsQuantityDefinedByUser, Quantity);
};

TVector<TNluCartItem> ReadNluCartItemsFromSlots(const THashMap<TString, TString>& slots);

// ~~~~ TMatcherCartItemOption ~~~~

struct TMatcherCartItemOption {
    ui64 GroupId = 0;
    ui64 OptionId = 0;
    TString Name;
    double Price = 0;
    bool IsExplicit = false;

    DECLARE_TUPLE_LIKE_TYPE(TMatcherCartItemOption, GroupId, OptionId, Name, Price, IsExplicit);
};

// ~~~~ TMatcherCartItem ~~~~

struct TMatcherCartItem {
    bool IsAvailable = false;
    ui64 Id = 0;
    TString Name;
    double Price = 0;
    ui64 Quantity = 1;
    TString Description;
    TString Weight;
    TVector<TMatcherCartItemOption> Options;
    TVector<TString> OptionsNlg;

    DECLARE_TUPLE_LIKE_TYPE(TMatcherCartItem, IsAvailable, Id, Name, Price, Quantity,
        Description, Weight, Options, OptionsNlg);
};

size_t CountOptionsOfGroup(const TMatcherCartItem& item, ui64 groupId);
bool HasOption(const TMatcherCartItem& item, ui64 groupId, ui64 optionId);

// ~~~~ TMenuMatcher ~~~~

NJson::TJsonValue ReadHardcodedMenuMatcherConfig();
NJson::TJsonValue ReadHardcodedMenuSample();

class TMenuMatcher {
public:
    TMenuMatcher();

    void AddMenuToNormalizationCache(const NJson::TJsonValue& menu);

    void Convert(const TVector<TNluCartItem>& srcItems, const NJson::TJsonValue& menu,
        TVector<TMatcherCartItem>* dstItems, TVector<TString>* unknownItems) const;

    void UpdateCart(NApi::TCart& cart, TVector<TString>& unavailableItems, const NJson::TJsonValue& menu) const;

    void Dump(IOutputStream* log, bool verbose = true, const TString& indent = "") const;

private:
    void ReadConfig(const NJson::TJsonValue& config);
    void ReadNlgOptions(const NJson::TJsonValue& config);
    void ReadEntitiesConfig(const NJson::TJsonValue& config);
    void ReadNameVariantsFromConfig(const NJson::TJsonValue& pattern, TVector<TUtf16String>* result) const;
    void Convert(const TNluCartItem& srcItem, const NJson::TJsonValue& menu, TVector<TMatcherCartItem>* dstItems,
        TVector<TString>* unknownItems) const;
    bool TryConvertBySpokenName(const TNluCartItem& srcItem, const NJson::TJsonValue& menu,
        TVector<TMatcherCartItem>* dstItems) const;
    bool TryConvertByNluId(const TNluCartItem& srcItem, const TString& nluId, const NJson::TJsonValue& menu,
        TVector<TMatcherCartItem>* dstItems, TVector<TString>* triedNluIds) const;
    bool TryConvertToOption(const TNluCartItem& srcItem, const TUtf16String& normalizedOptionName,
        const NJson::TJsonValue& menu, TVector<TMatcherCartItem>* dstItems) const;
    bool TryConvertToOption(const TUtf16String& normalizedOptionName, const NJson::TJsonValue& menu,
        TVector<TMatcherCartItem>* dstItems) const;
    bool TryConvertToItem(const TNluCartItem& srcItem, const TUtf16String& normalizedItemName,
        const NJson::TJsonValue& menu, TVector<TMatcherCartItem>* dstItems) const;
    void Postprocess(const NJson::TJsonValue& menu, TVector<TMatcherCartItem>* items) const;
    void AddRequiredItemOptions(const NJson::TJsonValue& menu, TVector<TMatcherCartItem>* dstItems) const;
    void AddRequiredItemOptions(const NJson::TJsonValue& menu, TMatcherCartItem* dstItem) const;
    static const NJson::TJsonValue* FindItemInMenu(const NJson::TJsonValue& menu, const ui64 id);
    static const NJson::TJsonValue* FindOptionsGroupInItem(const NJson::TJsonValue& item, const ui64 id);
    static const NJson::TJsonValue* FindOptionInOptionsGroup(const NJson::TJsonValue& optionsGroup, const ui64 id);
    static bool UpdateItem(NApi::TCart::TItem& item, const NJson::TJsonValue& menu);
    const NJson::TJsonValue* FindItemInMenuByName(const NJson::TJsonValue& menu, const TUtf16String& normalizedItemName) const;
    void GenerateItemOptionsNlg(TMatcherCartItem* item) const;

private:
    struct TItemConfig {
        TVector<TUtf16String> MenuItemNameVariants;
        TVector<TUtf16String> MenuOptionNameVariants;
        TVector<TString> Alternatives;
    };

private:
    TStringMatcher StringMatcher;
    THashMap<TString, TItemConfig> NluIdToItemConfig;
    THashSet<TUtf16String> OptionsToHide;
};

// ~~~~ Functions for working with cart and menu ~~~~

void AddRequiredItemOptions(const NJson::TJsonValue& menu, TVector<TMatcherCartItem>* cartItems);
void AddRequiredItemOptions(const NJson::TJsonValue& menu, TMatcherCartItem* cartItem);

NApi::TCart::TItem ToApiCartItem(const TMatcherCartItem& item);
void AddItemsToApiCart(const TVector<TMatcherCartItem>& items, NApi::TCart* cart, TVector<TString>* unavailableItems);

// All-in-one function
void BuildApiCartFromSlots(const TMenuMatcher& menuMatcher, const NJson::TJsonValue& menu,
    const THashMap<TString, TString>& slots, NApi::TCart* cart, TVector<TString>* unknownItems,
    TVector<TString>* unavailableItems);

// ~~~~ NApi::TCart::TItem helpers ~~~~

bool operator==(const NApi::TCart::TItem& item1, const NApi::TCart::TItem& item2);

// ~~~~ NApi::TCart helpers ~~~~

ui32 CalculateSubtotal(const NApi::TCart& cart);

}  // namespace NAlice::NHollywood::NFood

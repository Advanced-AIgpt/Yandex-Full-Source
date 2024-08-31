#include "matcher.h"
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/resource/resource.h>
#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/xrange.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/subst.h>

namespace NAlice::NHollywood::NFood {

// ~~~~ TNluCartItem ~~~~

static const size_t MAX_ITEMS_IN_FRAME = 8;

TVector<TNluCartItem> ReadNluCartItemsFromSlots(const THashMap<TString, TString>& slots) {
    TVector<TNluCartItem> items;
    for (size_t i : xrange(MAX_ITEMS_IN_FRAME)) {
        const TString suffix = ToString<size_t>(i + 1);
        TNluCartItem item;
        item.SpokenName = slots.Value("item_text" + suffix, "");
        item.NameId = slots.Value("item_name" + suffix, "");
        if (item.NameId.empty() && item.SpokenName.empty()) {
            continue;
        }
        item.IsQuantityDefinedByUser = TryFromString(slots.Value("item_count" + suffix, ""), item.Quantity);
        if (!item.IsQuantityDefinedByUser) {
            item.Quantity = 1;
        }
        items.push_back(item);
    }
    return items;
}

// ~~~~ TMatcherCartItem ~~~~

size_t CountOptionsOfGroup(const TMatcherCartItem& item, ui64 groupId) {
    ui64 count = 0;
    for (const TMatcherCartItemOption& option : item.Options) {
        if (option.GroupId == groupId) {
            count++;
        }
    }
    return count;
}

bool HasOption(const TMatcherCartItem& item, ui64 groupId, ui64 optionId) {
    for (const TMatcherCartItemOption& option : item.Options) {
        if (option.GroupId == groupId && option.OptionId == optionId) {
            return true;
        }
    }
    return false;
}

// ~~~~ TMenuMatcher ~~~~

NJson::TJsonValue ReadHardcodedMenuMatcherConfig() {
    const TString str = NResource::Find("alice/hollywood/library/scenarios/food/menu_matcher/config.json");
    return NJson::ReadJsonFastTree(str, true);
}

NJson::TJsonValue ReadHardcodedMenuSample() {
    const TString str = NResource::Find("alice/hollywood/library/scenarios/food/menu_matcher/menu_sample_mcdonalds_komsomolskyprospect_evening.json");
    return NJson::ReadJsonFastTree(str, true);
}

TMenuMatcher::TMenuMatcher() {
    ReadConfig(ReadHardcodedMenuMatcherConfig());
}

void TMenuMatcher::ReadConfig(const NJson::TJsonValue& config) {
    StringMatcher.ReadOptionsJson(config["matching_options"]);
    ReadNlgOptions(config);
    ReadEntitiesConfig(config);
}

void TMenuMatcher::ReadNlgOptions(const NJson::TJsonValue& config) {
    const NJson::TJsonValue& nlgOptions = config["nlg_options"];

    for (const NJson::TJsonValue& optionNameJson : nlgOptions["options_to_hide"].GetArray()) {
        OptionsToHide.insert(StringMatcher.NormalizeForMatch(optionNameJson.GetString()));
    }
}

void TMenuMatcher::ReadEntitiesConfig(const NJson::TJsonValue& config) {
    for (const NJson::TJsonValue& groupJson : config["groups"].GetArray()) {
        Y_UNUSED(groupJson);
        for (const NJson::TJsonValue& itemJson : groupJson["items"].GetArray()) {
            const TString nluId = itemJson["nlu_id"].GetString();
            if (nluId.empty()) {
                continue;
            }
            TItemConfig& itemConfig = NluIdToItemConfig[nluId];
            ReadNameVariantsFromConfig(itemJson["menu_item"], &itemConfig.MenuItemNameVariants);
            ReadNameVariantsFromConfig(itemJson["with_option"], &itemConfig.MenuOptionNameVariants);
            for (const NJson::TJsonValue& alternative : itemJson["specify"].GetArray()) {
                itemConfig.Alternatives.push_back(alternative.GetString());
            }
            for (const NJson::TJsonValue& alternative : itemJson["substitute"].GetArray()) {
                itemConfig.Alternatives.push_back(alternative.GetString());
            }
        }
    }
}

void TMenuMatcher::ReadNameVariantsFromConfig(const NJson::TJsonValue& pattern, TVector<TUtf16String>* result) const {
    for (const TStringBuf variant : StringSplitter(pattern.GetString()).Split('|').SkipEmpty()) {
        const TUtf16String normalized = StringMatcher.NormalizeForMatch(StripString(variant));
        if (!normalized.empty()) {
            result->push_back(normalized);
        }
    }
}

void TMenuMatcher::AddMenuToNormalizationCache(const NJson::TJsonValue& menu) {
    for (const NJson::TJsonValue& category : menu["categories"].GetArray()) {
        for (const NJson::TJsonValue& item : category["items"].GetArray()) {
            StringMatcher.AddToNormalizationCache(item["name"].GetString());
            for (const NJson::TJsonValue& group : item["optionsGroups"].GetArray()) {
                for (const NJson::TJsonValue& option : group["options"].GetArray()) {
                    StringMatcher.AddToNormalizationCache(option["name"].GetString());
                }
            }
        }
    }
}

void TMenuMatcher::Convert(const TVector<TNluCartItem>& srcItems, const NJson::TJsonValue& menu,
    TVector<TMatcherCartItem>* dstItems, TVector<TString>* unknownItems) const
{
    for (const TNluCartItem& srcItem : srcItems) {
        Convert(srcItem, menu, dstItems, unknownItems);
    }
    Postprocess(menu, dstItems);
}

void TMenuMatcher::Convert(const TNluCartItem& srcItem, const NJson::TJsonValue& menu,
    TVector<TMatcherCartItem>* dstItems, TVector<TString>* unknownItems) const
{
    TVector<TString> triedNluIds;
    if (TryConvertByNluId(srcItem, srcItem.NameId, menu, dstItems, &triedNluIds)) {
        return;
    }
    if (TryConvertBySpokenName(srcItem, menu, dstItems)) {
        return;
    }
    unknownItems->push_back(srcItem.SpokenName);
}

bool TMenuMatcher::TryConvertBySpokenName(const TNluCartItem& srcItem, const NJson::TJsonValue& menu,
    TVector<TMatcherCartItem>* dstItems) const
{
    const TUtf16String normalizedName = StringMatcher.NormalizeForMatch(srcItem.SpokenName);
    return TryConvertToOption(srcItem, normalizedName, menu, dstItems)
        || TryConvertToItem(srcItem, normalizedName, menu, dstItems);
}

bool TMenuMatcher::TryConvertByNluId(const TNluCartItem& srcItem, const TString& nluId,
    const NJson::TJsonValue& menu, TVector<TMatcherCartItem>* dstItems,
    TVector<TString>* triedNluIds) const
{
    if (nluId.empty()) {
        return false;
    }

    Y_ENSURE(triedNluIds);
    if (IsIn(*triedNluIds, nluId)) {
        return false;
    }
    triedNluIds->push_back(nluId);

    const TItemConfig* itemConfig = NluIdToItemConfig.FindPtr(nluId);
    if (itemConfig == nullptr) {
        return false;
    }

    for (const TUtf16String& nameVariant : itemConfig->MenuItemNameVariants) {
        if (TryConvertToOption(srcItem, nameVariant, menu, dstItems)) {
            return true;
        }
        if (TryConvertToItem(srcItem, nameVariant, menu, dstItems)) {
            for (const TUtf16String& optionVariant : itemConfig->MenuOptionNameVariants) {
                if (TryConvertToOption(optionVariant, menu, dstItems)) {
                    break;
                }
            }
            return true;
        }
    }

    for (const TString& alternative : itemConfig->Alternatives) {
        if (TryConvertByNluId(srcItem, alternative, menu, dstItems, triedNluIds)) {
            return true;
        }
    }
    return false;
}

bool TMenuMatcher::TryConvertToOption(const TNluCartItem& srcItem, const TUtf16String& normalizedOptionName,
    const NJson::TJsonValue& menu, TVector<TMatcherCartItem>* dstItems) const
{
    if (srcItem.IsQuantityDefinedByUser) {
        return false;
    }
    return TryConvertToOption(normalizedOptionName, menu, dstItems);
}

bool TMenuMatcher::TryConvertToOption(const TUtf16String& normalizedOptionName, const NJson::TJsonValue& menu,
    TVector<TMatcherCartItem>* dstItems) const
{
    Y_ENSURE(dstItems);
    if (dstItems->empty()) {
        return false;
    }
    TMatcherCartItem& dstItem = dstItems->back();
    const NJson::TJsonValue* menuItem = FindItemInMenu(menu, dstItem.Id);
    if (menuItem == nullptr) {
        return false;
    }
    for (const NJson::TJsonValue& group : (*menuItem)["optionsGroups"].GetArray()) {
        const ui64 groupId = group["id"].GetUIntegerSafe();
        const size_t groupLimit = group["maxSelected"].GetUIntegerSafe(0);
        if (CountOptionsOfGroup(dstItem, groupId) >= groupLimit) {
            continue;
        }

        for (const NJson::TJsonValue& option : group["options"].GetArray()) {
            const ui64 optionId = option["id"].GetUIntegerSafe();
            if (HasOption(dstItem, groupId, optionId)) {
                continue;
            }
            const TString optionName = option["name"].GetString();
            if (!StringMatcher.MatchNames(normalizedOptionName, optionName)) {
                continue;
            }
            dstItem.Options.push_back({
                .GroupId = groupId,
                .OptionId = optionId,
                .Name = optionName,
                .Price = option["price"].GetDoubleSafe(0),
                .IsExplicit = true,
            });
            return true;
        }
    }
    return false;
}

bool TMenuMatcher::TryConvertToItem(const TNluCartItem& srcItem, const TUtf16String& normalizedItemName,
    const NJson::TJsonValue& menu, TVector<TMatcherCartItem>* dstItems) const
{
    Y_ENSURE(dstItems);

    TMatcherCartItem dstItem;
    bool isFound = false;

    for (const NJson::TJsonValue& menuCategory : menu["categories"].GetArray()) {
        for (const NJson::TJsonValue& menuItem : menuCategory["items"].GetArray()) {
            const TString menuItemName = menuItem["name"].GetString();
            if (!StringMatcher.MatchNames(normalizedItemName, menuItemName)) {
                continue;
            }
            const bool isAvailable = menuItem["available"].GetBooleanSafe(true);
            if (isFound && !isAvailable) {
                continue;
            }
            dstItem = TMatcherCartItem{
                .IsAvailable = isAvailable,
                .Id = menuItem["id"].GetUIntegerSafe(),
                .Name = menuItemName,
                .Price = menuItem["price"].GetDoubleSafe(0),
                .Quantity = srcItem.Quantity,
                .Description = menuItem["description"].GetString(),
                .Weight = menuItem["weight"].GetString(),
            };
            isFound = true;
        }
    }
    if (!isFound) {
        return false;
    }
    dstItems->push_back(std::move(dstItem));
    return true;
}

void TMenuMatcher::Postprocess(const NJson::TJsonValue& menu, TVector<TMatcherCartItem>* items) const {
    Y_ENSURE(items);

    for (TMatcherCartItem& item : *items) {
        AddRequiredItemOptions(menu, &item);
        GenerateItemOptionsNlg(&item);
    }
}

void TMenuMatcher::AddRequiredItemOptions(const NJson::TJsonValue& menu, TMatcherCartItem* dstItem) const {
    Y_ENSURE(dstItem);

    const NJson::TJsonValue* menuItem = FindItemInMenu(menu, dstItem->Id);
    if (menuItem == nullptr) {
        return;
    }
    for (const NJson::TJsonValue& group : (*menuItem)["optionsGroups"].GetArray()) {
        const ui64 groupId = group["id"].GetUIntegerSafe();
        const size_t minSelected = group["minSelected"].GetUIntegerSafe(0);
        size_t currSelected = CountOptionsOfGroup(*dstItem, groupId);
        if (currSelected >= minSelected) {
            continue;
        }

        // Move cheaper options (like "Без соуса") on top.
        TVector<const NJson::TJsonValue*> orderedOptions;
        for (const NJson::TJsonValue& option : group["options"].GetArray()) {
            orderedOptions.push_back(&option);
        }
        StableSortBy(orderedOptions, [](const NJson::TJsonValue* option) { return (*option)["price"].GetDoubleSafe(0); });

        for (const NJson::TJsonValue* option : orderedOptions) {
            const ui64 optionId = (*option)["id"].GetUIntegerSafe();
            if (HasOption(*dstItem, groupId, optionId)) {
                continue;
            }
            dstItem->Options.push_back({
                .GroupId = groupId,
                .OptionId = optionId,
                .Name = (*option)["name"].GetString(),
                .Price = (*option)["price"].GetDoubleSafe(0),
                .IsExplicit = false,
            });
            currSelected++;
            if (currSelected >= minSelected) {
                break;
            }
        }
    }
}

const NJson::TJsonValue* TMenuMatcher::FindItemInMenuByName(const NJson::TJsonValue& menu, const TUtf16String& normalizedItemName) const {
    for (const NJson::TJsonValue& category : menu["categories"].GetArray()) {
        for (const NJson::TJsonValue& item : category["items"].GetArray()) {
            const TString menuItemName = item["name"].GetString();
            if (StringMatcher.MatchNames(normalizedItemName, menuItemName)) {
                return &item;
            }
        }
    }
    return nullptr;
}

// static
const NJson::TJsonValue* TMenuMatcher::FindItemInMenu(const NJson::TJsonValue& menu, const ui64 id) {
    // TODO(samoylovboris) Prebuild hash table
    for (const NJson::TJsonValue& category : menu["categories"].GetArray()) {
        for (const NJson::TJsonValue& item : category["items"].GetArray()) {
            if (item["id"].GetUIntegerSafe() == id) {
                return &item;
            }
        }
    }
    return nullptr;
}

const NJson::TJsonValue* TMenuMatcher::FindOptionsGroupInItem(const NJson::TJsonValue& item, const ui64 id) {
    for (const NJson::TJsonValue& optionsGroup : item["optionsGroups"].GetArray()) {
        if (optionsGroup["id"].GetUIntegerSafe() == id) {
            return &optionsGroup;
        }
    }
    return nullptr;
}

const NJson::TJsonValue* TMenuMatcher::FindOptionInOptionsGroup(const NJson::TJsonValue& optionsGroup, const ui64 id) {
    for (const NJson::TJsonValue& option : optionsGroup["options"].GetArray()) {
        if (option["id"].GetUIntegerSafe() == id) {
            return &option;
        }
    }
    return nullptr;
}

void TMenuMatcher::GenerateItemOptionsNlg(TMatcherCartItem* item) const {
    Y_ENSURE(item);

    item->OptionsNlg.clear();

    for (const TMatcherCartItemOption& option : item->Options) {
        if (!option.IsExplicit) {
            continue;
        }
        if (OptionsToHide.contains(StringMatcher.NormalizeForMatch(option.Name))) {
            continue;
        }
        item->OptionsNlg.push_back(option.Name);
    }
}

void TMenuMatcher::Dump(IOutputStream* log, bool verbose, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << "TMenuMatcher:" << Endl;

    *log << indent << "  StringMatcher:" << Endl;
    StringMatcher.Dump(log, verbose, indent + "    ");

    *log << indent << "  NluIdToItemConfig:" << Endl;
    for (const TString& id : NGranet::OrderedSetOfKeys(NluIdToItemConfig)) {
        const TItemConfig& config = NluIdToItemConfig.at(id);
        *log << indent << "    " << id << ":" << Endl;
        *log << indent << "      TItemConfig:" << Endl;
        *log << indent << "        MenuItemNameVariants:" << Endl;
        for (const TUtf16String& variant : config.MenuItemNameVariants) {
            *log << indent << "          " << WideToUTF8(variant) << Endl;
        }
        *log << indent << "        MenuOptionNameVariants:" << Endl;
        for (const TUtf16String& variant : config.MenuOptionNameVariants) {
            *log << indent << "          " << WideToUTF8(variant) << Endl;
        }
        *log << indent << "        Alternatives:" << Endl;
        for (const TString& alternative : config.Alternatives) {
            *log << indent << "          " << alternative << Endl;
        }
    }
}

// ~~~~ Functions for working with cart and menu ~~~~

NApi::TCart::TItem ToApiCartItem(const TMatcherCartItem& srcItem) {
    NApi::TCart::TItem dstItem;
    dstItem.SetItemId(srcItem.Id);
    dstItem.SetName(srcItem.Name);
    dstItem.SetQuantity(srcItem.Quantity);
    dstItem.SetDescription(srcItem.Description);
    dstItem.SetWeight(srcItem.Weight);

    double priceWithOptions = srcItem.Price;
    for (const TMatcherCartItemOption& option : srcItem.Options) {
        priceWithOptions += option.Price;
    }
    dstItem.SetPrice(static_cast<ui32>(ceil(priceWithOptions * srcItem.Quantity)));
    for (const TString& name : srcItem.OptionsNlg) {
        dstItem.AddItemOptionNames(name);
    }

    if (!srcItem.Options.empty()) {
        TMap<ui64, TVector<TMatcherCartItemOption>> groups;
        for (const TMatcherCartItemOption& option : srcItem.Options) {
            groups[option.GroupId].push_back(option);
        }
        for (const auto& [groupId, srcGroup] : groups) {
            NApi::TCart::TItem::TItemOption* destGroup = dstItem.AddItemOptions();
            destGroup->SetGroupId(groupId);
            for (const TMatcherCartItemOption& srcOption : srcGroup) {
                destGroup->AddGroupOptions(srcOption.OptionId);
                NApi::TCart::TItem::TItemOption::TModifier* destModifier = destGroup->AddModifiers();
                destModifier->SetOptionId(srcOption.OptionId);
                destModifier->SetQuantity(1);
                destModifier->SetName(srcOption.Name);
            }
        }
    }
    return dstItem;
}

void AddItemsToApiCart(const TVector<TMatcherCartItem>& items, NApi::TCart* cart, TVector<TString>* unavailableItems) {
    Y_ENSURE(cart);
    Y_ENSURE(unavailableItems);

    for (const TMatcherCartItem& item : items) {
        if (!item.IsAvailable) {
            unavailableItems->push_back(item.Name);
            continue;
        }
        *cart->AddItems() = ToApiCartItem(item);
    }
}

void BuildApiCartFromSlots(const TMenuMatcher& menuMatcher, const NJson::TJsonValue& menu,
    const THashMap<TString, TString>& slots, NApi::TCart* cart, TVector<TString>* unknownItems,
    TVector<TString>* unavailableItems)
{
    Y_ENSURE(cart);
    Y_ENSURE(unknownItems);
    Y_ENSURE(unavailableItems);

    const TVector<TNluCartItem> nluCartItems = ReadNluCartItemsFromSlots(slots);

    TVector<TMatcherCartItem> items;
    menuMatcher.Convert(nluCartItems, menu, &items, unknownItems);

    AddItemsToApiCart(items, cart, unavailableItems);
}

bool TMenuMatcher::UpdateItem(NApi::TCart::TItem& item, const NJson::TJsonValue& menu) {
    const NJson::TJsonValue* menuItem = FindItemInMenu(menu, item.GetItemId());
    const bool isAvailable = menuItem != nullptr && (*menuItem)["available"].GetBooleanSafe(false);
    if (!isAvailable) {
        return false;
    }
    double priceWithOptions = (*menuItem)["price"].GetDoubleSafe(0);
    for (auto& itemOption : *item.MutableItemOptions()) {
        const NJson::TJsonValue* optionsGroup = FindOptionsGroupInItem(*menuItem, itemOption.GetGroupId());
        if (!optionsGroup) {
            return false;
        }
        for (auto& modifier: *itemOption.MutableModifiers()) {
            const NJson::TJsonValue* option = FindOptionInOptionsGroup(*optionsGroup, modifier.GetOptionId());
            if (!option) {
                return false;
            }
            priceWithOptions += (*option)["price"].GetDoubleSafe(0) * modifier.GetQuantity();
        }
    }
    item.SetPrice(static_cast<ui32>(ceil(priceWithOptions * item.GetQuantity())));
    return true;
}

void TMenuMatcher::UpdateCart(NApi::TCart& cart, TVector<TString>& unavailableItems, const NJson::TJsonValue& menu) const {
    NApi::TCart updatedCart;
    for (auto& item : *cart.MutableItems()) {
        if (UpdateItem(item, menu)) {
            *updatedCart.AddItems() = item;
        } else {
            unavailableItems.push_back(item.GetName());
        }
    }
    cart = updatedCart;
}

// ~~~~ NApi::TCart::TItem helpers ~~~~

bool operator==(const NApi::TCart::TItem& item1, const NApi::TCart::TItem& item2) {
    if (item1.GetItemId() != item2.GetItemId()) {
        return false;
    }
    THashSet<TString> optionsList1(item1.GetItemOptionNames().begin(), item1.GetItemOptionNames().end()); 
    THashSet<TString> optionsList2(item2.GetItemOptionNames().begin(), item2.GetItemOptionNames().end());
    return optionsList1 == optionsList2;
}

// ~~~~ NApi::TCart helpers ~~~~

ui32 CalculateSubtotal(const NApi::TCart& cart) {
    ui32 subtotal = 0;
    for (const NApi::TCart::TItem& item : cart.GetItems()) {
        subtotal += item.GetPrice();
    }
    return subtotal;
}

} // namespace NAlice::NHollywood::NFood

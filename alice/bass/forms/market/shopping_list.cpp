#include "shopping_list.h"

#include "client/pers_basket_client.h"
#include "dynamic_data.h"
#include "forms.h"
#include "market_url_builder.h"
#include "login.h"
#include "util/serialize.h"

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/algorithm.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/string/subst.h>

namespace NBASS {

namespace NMarket {

TShoppingListImpl::TShoppingListImpl(TMarketContext& ctx)
    : Ctx(ctx)
    , Form(FromString<EShoppingListForm>(Ctx.FormName()))
    , PersonalData(Ctx.Ctx())
{
    PersonalData.GetUserInfo(BlackboxInfo);
}

TResultValue TShoppingListImpl::Do()
{
    Ctx.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::SHOPPING_LIST);
    const bool isLoginForm = Form == EShoppingListForm::Login || Form == EShoppingListForm::LoginFixlist;
    EShoppingListForm form = Form;
    if (isLoginForm) {
        TryFromString<EShoppingListForm>(Ctx.GetStringSlot(TStringBuf("form_name")), form);
    }

    if (BlackboxInfo.GetUid().empty()) {
        Ctx.SetStringSlot(TStringBuf("form_name"), ToString(form));
        return HandleGuest(Ctx, isLoginForm);
    }

    switch (form) {
        case EShoppingListForm::Show:
        case EShoppingListForm::ShowFixlist:
        case EShoppingListForm::Show_Show:
        case EShoppingListForm::Show_ShowFixlist:
            return HandleShow();
        case EShoppingListForm::Show_Add:
        case EShoppingListForm::Add:
        case EShoppingListForm::Show_AddFixlist:
        case EShoppingListForm::AddFixlist:
            return HandleAdd();
        case EShoppingListForm::Show_DeleteIndex:
        case EShoppingListForm::Show_DeleteIndexFixlist:
            return HandleDeleteIndex();
        case EShoppingListForm::Show_DeleteItem:
        case EShoppingListForm::DeleteItem:
        case EShoppingListForm::Show_DeleteItemFixlist:
        case EShoppingListForm::DeleteItemFixlist:
            return HandleDeleteItem();
        case EShoppingListForm::Show_DeleteAll:
        case EShoppingListForm::DeleteAll:
        case EShoppingListForm::Show_DeleteAllFixlist:
        case EShoppingListForm::DeleteAllFixlist:
            return HandleDeleteAll();
        case EShoppingListForm::Login:
        case EShoppingListForm::LoginFixlist:
            return HandleLogin();
    }
}

TResultValue TShoppingListImpl::HandleAdd()
{
    const NSc::TValue itemSlot = Ctx.GetSlot(TStringBuf("item"));

    SetResponseFormToShow();

    TPersBasketClient client(Ctx);
    auto entries = client.GetAllEnties(BlackboxInfo.GetUid()).GetResponse();

    NSc::TValue data;
    data["entries"].SetArray();
    data["duplicates"].SetArray();

    if (itemSlot.IsString()) {
        const TStringBuf item = itemSlot.GetString();
        if (item.empty()) {
            Ctx.AddTextCardBlock(TStringBuf("shopping_list__add_item__empty"));
            return TResultValue();
        }
        if (Ctx.GetExperiments().ShopplingListFuzzy()) {
            TMaybe<int> index = MatchEntriesWithText(entries, item);
            if (index.Defined()) {
                data["duplicates"].Push() = entries[index.GetRef()].Text;
                Ctx.AddTextCardBlock(TStringBuf("shopping_list__add_item__duplicate"), data);
                AddSuggestShoppingList();
                return TResultValue();
            }
        } else {
            for (const auto& entry : entries) {
                if (entry.Text == item) {
                    data["duplicates"].Push() = item;
                    Ctx.AddTextCardBlock(TStringBuf("shopping_list__add_item__duplicate"), data);
                    AddSuggestShoppingList();
                    return TResultValue();
                }
            }
        }
        const TPersBasketEntry entry(item);
        auto result = client.AddEntry(BlackboxInfo.GetUid(), entry);
        result.GetResponse();
        data["entries"].Push() = item;
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__add_item__success"), data);
        TryAddFactCard({entry});
        AddSuggestShoppingList();
        return TResultValue();
    } else if(itemSlot.IsArray()) {
        TVector<TPersBasketEntry> newEntries;

        const NSc::TArray& items = itemSlot.GetArray();
        if (items.empty()) {
            Ctx.AddTextCardBlock(TStringBuf("shopping_list__add_item__empty"));
            return TResultValue();
        }
        for (const NSc::TValue& itemValue : items) {
            const TStringBuf item = itemValue.GetString();
            if (item.empty()) {
                continue;
            }
            if (Ctx.GetExperiments().ShopplingListFuzzy()) {
                if (TMaybe<int> index = MatchEntriesWithText(entries, item)) {
                    data["duplicates"].Push() = entries[index.GetRef()].Text;
                    continue;
                }
                if (MatchEntriesWithText(newEntries, item)) {
                    continue;
                }
            } else {
                bool isDuplicate = false;
                for (const auto& entry : entries) {
                    if (entry.Text == item) {
                        data["duplicates"].Push() = item;
                        isDuplicate = true;
                        break;
                    }
                }
                if (isDuplicate) {
                    continue;
                }
                for (const auto& entry : newEntries) {
                    if (entry.Text == item) {
                        isDuplicate = true;
                        break;
                    }
                }
                if (isDuplicate) {
                    continue;
                }
            }
            const TPersBasketEntry entry(item);
            newEntries.push_back(entry);
            data["entries"].Push() = item;
        }
        if (newEntries.empty()) {
            Ctx.AddTextCardBlock(TStringBuf("shopping_list__add_item__duplicate"), data);
            AddSuggestShoppingList();
            return TResultValue();
        }
        auto result = client.AddEntries(BlackboxInfo.GetUid(), newEntries);
        result.GetResponse();
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__add_item__success"), data);
        TryAddFactCard(newEntries);
        AddSuggestShoppingList();
        return TResultValue();
    } else {
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__add_item__empty"));
        return TResultValue();
    }
}

bool TShoppingListImpl::TryAddFactCard(const TVector<TPersBasketEntry>& entries)
{
    if (Ctx.IsScreenless()) {
        return false;
    }
    if (entries.empty()) {
        return false;
    }
    TVector<TString> factCandidates;
    if (Ctx.GetExperiments().ShopplingListFuzzy()) {
        TVector<TStringBuf> factProducts = TDynamicDataFacade::GetFactProducts();
        for (const auto& factProduct : factProducts) {
            TMaybe<int> index = MatchEntriesWithText(entries, factProduct, 0.33 /*entryInTextThreshold*/);
            if (index.Empty()) {
                continue;
            }
            const auto* facts = TDynamicDataFacade::GetFacts(factProduct);
            if (facts) {
                factCandidates.reserve(factCandidates.size() + facts->size());
                factCandidates.insert(factCandidates.end(), facts->begin(), facts->end());
            }
        }
    } else {
        for (const auto& entry : entries) {
            const auto* facts = TDynamicDataFacade::GetFacts(entry.Text);
            if (facts) {
                factCandidates.reserve(factCandidates.size() + facts->size());
                factCandidates.insert(factCandidates.end(), facts->begin(), facts->end());
            }
        }
    }
    if (factCandidates.empty()) {
        return false;
    }

    auto& rng = Ctx.GetRng();
    TString fact = factCandidates[rng.RandomInteger(factCandidates.size())];
    SubstGlobal(fact, TStringBuf("\n"), TStringBuf("<br/>"));

    NSc::TValue data;
    data["fact"] = fact;
    Ctx.AddDivCardBlock(TStringBuf("shopping_list__fact"), data);
    return true;
}

TResultValue TShoppingListImpl::HandleDeleteAll()
{
    SetResponseFormToShow();

    TPersBasketClient client(Ctx);
    auto entries = client.GetAllEnties(BlackboxInfo.GetUid()).GetResponse();
    if (entries.empty()) {
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_all__empty"));
        return TResultValue();
    }
    TVector<TResponseHandle<void>> responces;
    for (const auto& entry : entries) {
        responces.push_back(client.DeleteEntryAsync(BlackboxInfo.GetUid(), entry.Id));
    }
    for (auto&& r : responces) {
        r.Wait();
    }
    Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_all__success"));
    return TResultValue();
}

THashMap<TPersBasketEntryId, NSc::TValue> TShoppingListImpl::RequestBeruInfo(
    const TVector<TPersBasketEntryWithId>& entries) const
{
    if (Ctx.GetExperiments().ShoppingListDisableBeru()) {
        return {};
    }
    TMarketClient client(Ctx);

    TVector<std::pair<const TPersBasketEntryWithId&, TReportRequest>> requests(Reserve(entries.size()));
    TRearFlags flags;
    flags.Add(TStringBuf("market_alice_shopping_list"), TStringBuf("1"));
    for (const auto& entry : entries) {
        requests.emplace_back(
            entry,
            client.MakeSearchRequestAsync(
                entry.Text,
                NSc::TValue() /* price */,
                EMarketType::BLUE,
                false /* allowRedirects */,
                {} /* fesh */,
                {} /* redirectParams */,
                flags)
        );
    }

    THashMap<TPersBasketEntryId, NSc::TValue> result;
    for (auto& [entry, request] : requests) {
        try {
            const auto response = request.Wait();
            if (response.HasError()) {
                LOG(ERR) << "Error in blue report response for request '" << entry.Text << "': " << response.GetError() << Endl;
                continue;
            }
            if (response.GetTotal()) {
                NSc::TValue& entryBeruStats = result[entry.Id];
                entryBeruStats["total"] = response.GetTotal();
                entryBeruStats["url"] = TMarketUrlBuilder(Ctx).GetMarketSearchUrl(
                    EMarketType::BLUE,
                    entry.Text,
                    Ctx.UserRegion());

                const auto& responseResults = response.GetResults();
                if (!responseResults.empty()) {
                    const NSc::TValue& mostRelevantDoc = responseResults[0].GetRawData();
                    NBassApi::TPicture<TBoolSchemeTraits> pictureScheme(&entryBeruStats["picture"]);
                    SerializePicture(TPicture::GetMostSuitablePicture(mostRelevantDoc), pictureScheme);
                } else {
                    LOG(ERR) << "Got empty report response results, but total != 0" << Endl;
                }
            }
        } catch (const NMarket::THttpTimeoutException& e) {
            LOG(ERR) << "Timeout in blue report response for request '" << entry.Text << "': " << e.what() << Endl;
        }
    }
    return result;
}

TResultValue TShoppingListImpl::HandleShow()
{
    SetResponseFormToShow();

    TPersBasketClient client(Ctx);
    auto entries = client.GetAllEnties(BlackboxInfo.GetUid()).GetResponse();
    if (entries.empty()) {
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__show__empty"));
        return TResultValue();
    }
    SortBy(entries.begin(), entries.end(), [] (const auto& entry) { return entry.Id; });
    const auto& beruInfo = RequestBeruInfo(entries);
    NSc::TValue data;
    NSc::TValue dataEntries = data["entries"].SetArray();
    bool hasBeruOffers = false;
    for (const auto& entry : entries) {
        auto& dataEntry = dataEntries.Push();
        dataEntry["id"] = entry.Id;
        dataEntry["text"] = entry.Text;
        if (beruInfo.contains(entry.Id)) {
            dataEntry["beru"] = beruInfo.at(entry.Id);
            hasBeruOffers = true;
        }
        const auto& facts = TDynamicDataFacade::GetFacts(entry.Text);
        dataEntry["has_fact"] = facts && facts->size() > 0;
    }
    data["has_beru_offers"] = hasBeruOffers;

    Ctx.SetSlot(TStringBuf("list"), dataEntries);
    if (Ctx.IsScreenless()) {
      Ctx.AddTextCardBlock(TStringBuf("shopping_list__show__success"), data);
    } else {
      Ctx.AddTextCardBlock(TStringBuf("shopping_list__show__success"));
      Ctx.AddDivCardBlock(TStringBuf("shopping_list__list"), data);
    }
    return TResultValue();
}

TResultValue TShoppingListImpl::HandleDeleteIndex()
{
    if (!Ctx.HasSlot(TStringBuf("list"))) {
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_index__slot_list_empty"));
        AddSuggestShoppingList();
        return TResultValue();
    }
    const NSc::TArray& listSlot = Ctx.GetSlot(TStringBuf("list")).GetArray();
    const size_t listSize = listSlot.size();

    const TStringBuf indexString = Ctx.GetStringSlot(TStringBuf("index"));
    TVector<TStringBuf> indexStringParts = StringSplitter(indexString).Split(' ').SkipEmpty();
    TVector<size_t> indexes;
    for (const auto& part : indexStringParts) {
        size_t index = 0;
        if (TryFromString(part, index) && index > 0 && index <= listSize) {
            indexes.push_back(index - 1);
        }
    }
    if (indexes.empty()) {
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_index__slot_index_empty"));
        return TResultValue();
    }

    TPersBasketClient client(Ctx);
    auto entries = client.GetAllEnties(BlackboxInfo.GetUid()).GetResponse();

    if (entries.empty()) {
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_index__empty"));
        return TResultValue();
    }

    NSc::TValue data;
    NSc::TValue dataEntries = data["entries"].SetArray();

    TVector<TResponseHandle<void>> responces;
    for (size_t index : indexes) {
        NSc::TValue listEntry = listSlot[index];
        i64 id = listEntry["id"].GetIntNumber();
        for (const auto& entry : entries) {
            if (entry.Id != id) {
                continue;
            }
            responces.push_back(client.DeleteEntryAsync(BlackboxInfo.GetUid(), entry.Id));
            dataEntries.Push(listEntry);
            break;
        }
    }

    if (responces.empty()) {
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_index__already_deleted"));
        AddSuggestShoppingList();
        return TResultValue();
    }

    for (auto&& r : responces) {
        r.Wait();
    }

    Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_index__success"), data);
    AddSuggestShoppingList();
    return TResultValue();
}

TResultValue TShoppingListImpl::HandleDeleteItem()
{
    constexpr float entryInTextThreshold = 0.33;
    const NSc::TValue itemSlot = Ctx.GetSlot(TStringBuf("item"));

    SetResponseFormToShow();

    TPersBasketClient client(Ctx);
    auto entries = client.GetAllEnties(BlackboxInfo.GetUid()).GetResponse();

    if (entries.empty()) {
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_item__empty"));
        return TResultValue();
    }

    NSc::TValue data;
    NSc::TValue dataEntries = data["entries"].SetArray();

    if (itemSlot.IsString()) {
        const TStringBuf item = itemSlot.GetString();
        if (item.empty()) {
            Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_item__slot_empty"));
            return TResultValue();
        }
        if (Ctx.GetExperiments().ShopplingListFuzzy()) {
            TMaybe<int> index = MatchEntriesWithText(entries, item, entryInTextThreshold);
            if (index.Defined()) {
                const auto& entry = entries[index.GetRef()];
                const auto result = client.DeleteEntry(BlackboxInfo.GetUid(), entry.Id);
                result.GetResponse();
                dataEntries.Push() = entry.Text;
                Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_item__success"), data);
                AddSuggestShoppingList();
                return TResultValue();
            }
        } else {
            for (const auto& entry : entries) {
                if (entry.Text == item) {
                    const auto result = client.DeleteEntry(BlackboxInfo.GetUid(), entry.Id);
                    result.GetResponse();
                    dataEntries.Push() = item;
                    Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_item__success"), data);
                    AddSuggestShoppingList();
                    return TResultValue();
                }
            }
        }
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_item__not_found"));
        AddSuggestShoppingList();
        return TResultValue();
    } else if(itemSlot.IsArray()) {
        const NSc::TArray& items = itemSlot.GetArray();
        if (items.empty()) {
            Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_item__slot_empty"));
            return TResultValue();
        }
        TVector<TResponseHandle<void>> responces;
        for (const NSc::TValue& itemValue : items) {
            const TStringBuf item = itemValue.GetString();
            if (item.empty()) {
                continue;
            }
            if (Ctx.GetExperiments().ShopplingListFuzzy()) {
                TMaybe<int> index = MatchEntriesWithText(entries, item, entryInTextThreshold);
                if (index.Defined()) {
                    const auto& entry = entries[index.GetRef()];
                    dataEntries.Push() = entry.Text;
                    responces.push_back(client.DeleteEntryAsync(BlackboxInfo.GetUid(), entry.Id));
                    entries.erase(entries.begin() + index.GetRef());
                }
            } else {
                for (const auto& entry : entries) {
                    if (entry.Text == item) {
                        dataEntries.Push() = item;
                        responces.push_back(client.DeleteEntryAsync(BlackboxInfo.GetUid(), entry.Id));
                        break;
                    }
                }
            }
        }
        if (responces.empty()) {
            Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_item__not_found"));
            AddSuggestShoppingList();
            return TResultValue();
        }
        for (auto&& r : responces) {
            r.Wait();
        }
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_item__success"), data);
        AddSuggestShoppingList();
        return TResultValue();
    } else {
        Ctx.AddTextCardBlock(TStringBuf("shopping_list__delete_item__slot_empty"));
        return TResultValue();
    }
}

TResultValue TShoppingListImpl::HandleLogin()
{
    NSc::TValue data;
    data[TStringBuf("name")] = BlackboxInfo.GetFirstName();
    Ctx.AddTextCardBlock(TStringBuf("shopping_list__login__no_intent"), data);
    return TResultValue();
}

void TShoppingListImpl::AddSuggestShoppingList()
{
    Ctx.AddSuggest(TStringBuf("shopping_list"));
    NSc::TValue data;
    data["attach_to_card"].SetBool(true);
    Ctx.AddSuggest(TStringBuf("shopping_list_button"), data);
}

void TShoppingListImpl::SetResponseFormToShow()
{
    if (Form == EShoppingListForm::Show) {
        return;
    }
    Ctx.SetResponseFormAndCopySlots(ToString(EShoppingListForm::Show), {TStringBuf("list"), TStringBuf("item")});
}

} // namespace NMarket

} // namespace NBASS

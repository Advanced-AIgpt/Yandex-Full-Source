#pragma once

#include "client/pers_basket_entry.h"
#include "context.h"
#include "forms.h"
#include <alice/bass/forms/common/personal_data.h>
#include <alice/library/parsed_user_phrase/parsed_sequence.h>

namespace NBASS {

namespace NMarket {

class TShoppingListImpl {
public:
    explicit TShoppingListImpl(TMarketContext& ctx);

    TResultValue Do();

private:
    TMarketContext& Ctx;
    EShoppingListForm Form;
    TPersonalDataHelper::TUserInfo BlackboxInfo;
    TPersonalDataHelper PersonalData;

    TResultValue HandleAdd();
    TResultValue HandleShow();
    TResultValue HandleDeleteIndex();
    TResultValue HandleDeleteItem();
    TResultValue HandleDeleteAll();
    TResultValue HandleLogin();

    THashMap<TPersBasketEntryId, NSc::TValue> RequestBeruInfo(const TVector<TPersBasketEntryWithId>& entries) const;
    bool TryAddFactCard(const TVector<TPersBasketEntry>& entries);
    void AddSuggestShoppingList();
    void SetResponseFormToShow();

    template<typename PersBasketEntry>
    static TMaybe<int> MatchEntriesWithText(const TVector<PersBasketEntry>& entries, TStringBuf text,
        float entryInTextThreshold = 0.4, float textInEntryThreshold = 0.7);
};


template<class PersBasketEntry>
TMaybe<int> TShoppingListImpl::MatchEntriesWithText(const TVector<PersBasketEntry>& entries, TStringBuf text,
    float entryInTextThreshold, float textInEntryThreshold)
{
    struct TMatchingResult {
        int Index;
        float EntryInText;
        float TextInEntry;
    };

    NParsedUserPhrase::TParsedSequence parsedText{text};

    TMaybe<int> result;
    float bestScore = 0;

    for (int index = 0; index < entries.ysize(); index++) {
        NParsedUserPhrase::TParsedSequence parsedEntry{entries[index].Text};
        const float entryInTextScore = parsedEntry.Match(parsedText);
        const float textInEntryScore = parsedText.Match(parsedEntry);
        if (entryInTextScore < entryInTextThreshold || textInEntryScore < textInEntryThreshold) {
            continue;
        }
        const float score = entryInTextScore + textInEntryScore;
        if (result.Empty() || score > bestScore) {
            result = index;
            bestScore = score;
        }
    }
    return result;
}

}

}

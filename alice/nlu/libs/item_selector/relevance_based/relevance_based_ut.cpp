#include "relevance_based.h"

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

using namespace NAlice::NItemSelector;

constexpr double THRESHOLD = 0.5;

void RunSimpleTest(const TRelevanceBasedItemSelector& selector) {
    TSelectionRequest request;
    request.Phrase.Text = "вася";
    request.Phrase.Language = ELanguage::LANG_RUS;
    request.NonsenseTagging = {
        {"вася", 0.105507873}
    };

    TVector<TSelectionItem> items(2);
    items[0].Synonyms = {{"василий", ELanguage::LANG_RUS}, {"вася", ELanguage::LANG_RUS}};
    items[1].Synonyms = {{"петя", ELanguage::LANG_RUS}, {"петр", ELanguage::LANG_RUS}};

    const TVector<TSelectionResult> selected = selector.Select(request, items);
    UNIT_ASSERT(selected.size() == items.size());
    UNIT_ASSERT(selected[0].IsSelected == true);
    UNIT_ASSERT(selected[1].IsSelected == false);
}

void RunEmptyTest(const TRelevanceBasedItemSelector& selector) {
    TSelectionRequest request;
    request.Phrase.Text = "вася";
    request.Phrase.Language = ELanguage::LANG_RUS;
    request.NonsenseTagging = {
        {"вася", 0.105507873}
    };

    TVector<TSelectionItem> items;
    auto selected = selector.Select(request, items);
    UNIT_ASSERT(selected.empty());
}


class TExactMatchRelevanceComputer : public IRelevanceComputer {
public:
    TExactMatchRelevanceComputer() {}

    float ComputeRelevance(const TString& request, const TVector<TString>& synonyms) const override {
        return Find(synonyms, request) != synonyms.end();
    }
};


Y_UNIT_TEST_SUITE(NNItemSelectorTestSuite) {
    Y_UNIT_TEST(SimpleTestSuite) {
        TRelevanceBasedItemSelector itemSelector(MakeHolder<TExactMatchRelevanceComputer>(), THRESHOLD, false);
        RunSimpleTest(itemSelector);
    }

    Y_UNIT_TEST(EmptyTestSuite) {
        TRelevanceBasedItemSelector itemSelector(MakeHolder<TExactMatchRelevanceComputer>(), THRESHOLD, false);
        RunEmptyTest(itemSelector);
    }
}

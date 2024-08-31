#include "default.h"

#include <alice/library/parsed_user_phrase/stopwords.h>

#include <library/cpp/langs/langs.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/folder/path.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/split.h>

using namespace NAlice::NItemSelector;

namespace {

constexpr TStringBuf EMBEDDER_DIRECTORY = "alice/nlu/data/ru/boltalka_dssm";
constexpr TStringBuf EMBEDDER_CONFIG_FILE = "boltalka_dssm_config.json";
constexpr TStringBuf EMBEDDER_FILE = "boltalka_dssm";

NAlice::TBoltalkaDssmEmbedder LoadEmbedder() {
    const TFsPath modelDirectory = BinaryPath(EMBEDDER_DIRECTORY);
    const TBlob modelBlob = TBlob::PrechargedFromFile(modelDirectory / EMBEDDER_FILE);
    TFileInput modelConfigStream(modelDirectory / EMBEDDER_CONFIG_FILE);

    return NAlice::TBoltalkaDssmEmbedder(modelBlob, &modelConfigStream);
}

struct TConfusionMatrix {
    double F1Score() {
        return 2. * TruePositive / (2. * TruePositive + FalsePositive + FalseNegative);
    }

    int TruePositive = 0;
    int FalsePositive = 0;
    int TrueNegative = 0;
    int FalseNegative = 0;
};

void UpdateMetrics(const bool shouldSelect, const bool selected, int weight, TConfusionMatrix& confusionMatrix) {
    if (shouldSelect) {
        if (selected) {
            confusionMatrix.TruePositive += weight;
        } else {
            confusionMatrix.FalseNegative += weight;
        }
    } else {
        if (selected) {
            confusionMatrix.FalsePositive += weight;
        } else {
            confusionMatrix.TrueNegative += weight;
        }
    }
}

//Should replace with real tagging
TNonsenseTagging GenerateFakeNonsenseTagging(const TString& text) {
    TVector<TString> tokens;
    Split(text, "\t", tokens);
    TNonsenseTagging tagging;
    for (const TString& token : tokens) {
        tagging.push_back({token, 0});
    }
    return tagging;
}

struct TDataEntry {
    int Count = 0;
    TString Query;
    TVector<bool> Actions;
};

TDataEntry ParseToDataEntry(const TString& line) {
    TVector<TString> tokens;
    Split(line, "\t", tokens);

    TDataEntry entry {
        FromString<int>(tokens[0]),
        tokens[1],
        {FromString<bool>(tokens[2]), FromString<bool>(tokens[3]), FromString<bool>(tokens[4])}
    };
    entry.Actions.push_back(!AnyOf(entry.Actions, [](const bool x){ return x; }));
    return entry;
}

// Also obtaining fake "last" item name corresponding to the case when NONE should be selected
TVector<TString> ObtainItemNames(const TString& header) {
    TVector<TString> tokens;
    Split(header, "\t", tokens);
    TVector<TString> names(tokens.begin() + 2, tokens.end());
    names.push_back("NONE");
    return names;
}

void RunSimpleTest(const TDefaultItemSelector& selector) {
    TSelectionRequest request;
    request.Phrase = {"вася", ELanguage::LANG_RUS};
    request.NonsenseTagging = {
        // {"вася", 0.105507873}
    };

    TVector<TSelectionItem> items(2);
    items[0].Synonyms = {{"василий", ELanguage::LANG_RUS}, {"вася", ELanguage::LANG_RUS}};
    items[1].Synonyms = {{"петя", ELanguage::LANG_RUS}, {"петр", ELanguage::LANG_RUS}};

    const TVector<TSelectionResult> selected = selector.Select(request, items);
    UNIT_ASSERT(selected.size() == items.size());
    UNIT_ASSERT(selected[0].IsSelected == true);
    UNIT_ASSERT(selected[1].IsSelected == false);
}

void RunSimpleNegativeTest(const TDefaultItemSelector& selector) {
    TSelectionRequest request;
    request.Phrase = {"васе", ELanguage::LANG_RUS};
    request.NonsenseTagging = {
        {"васе", 0.105507873}
    };

    TVector<TSelectionItem> items(1);
    items[0].Synonyms = {{"василий", ELanguage::LANG_RUS}, {"вася", ELanguage::LANG_RUS}};
    items[0].Negatives = {{"васе", ELanguage::LANG_RUS}};

    const TVector<TSelectionResult> selected = selector.Select(request, items);
    UNIT_ASSERT(selected.size() == items.size());
    UNIT_ASSERT(selected[0].IsSelected == false);
}

void RunWantTest(const TDefaultItemSelector& selector) {
    TSelectionRequest request;
    request.Phrase = {"хочу", ELanguage::LANG_RUS};
    request.NonsenseTagging = {
        {"хочу", 0.105507873}
    };

    TVector<TSelectionItem> items(2);
    items[0].Synonyms = {{"не хочу", ELanguage::LANG_RUS}};
    items[1].Synonyms = {{"хочу", ELanguage::LANG_RUS}};

    const TVector<TSelectionResult> selected = selector.Select(request, items);
    UNIT_ASSERT(selected.size() == items.size());
    UNIT_ASSERT(selected[0].IsSelected == false);
    UNIT_ASSERT(selected[1].IsSelected == true);
}

void RunUppercaseTest(const TDefaultItemSelector& selector) {
    TVector<TSelectionItem> items(2);
    items[0].Synonyms = {{"не хочу", ELanguage::LANG_RUS}};
    items[1].Synonyms = {{"хОчУ", ELanguage::LANG_RUS}, {"давай", ELanguage::LANG_RUS}};
    items[1].Negatives = {{"Не ДаВаЙ", ELanguage::LANG_RUS}};

    TSelectionRequest request;
    request.Phrase = {"XоЧу", ELanguage::LANG_RUS};
    request.NonsenseTagging = {
        {"ХоЧу", 0.105507873}
    };

    const TVector<TSelectionResult> selected = selector.Select(request, items);
    UNIT_ASSERT(selected.size() == items.size());
    UNIT_ASSERT(selected[0].IsSelected == false);
    UNIT_ASSERT(selected[1].IsSelected == true);
}

void RunUppercaseNegativesTest(const TDefaultItemSelector& selector) {
    TVector<TSelectionItem> items(1);
    items[0].Synonyms = {{"хОчУ", ELanguage::LANG_RUS}, {"давай", ELanguage::LANG_RUS}};
    items[0].Negatives = {{"Не ДаВаЙ", ELanguage::LANG_RUS}};

    TSelectionRequest request;
    request.Phrase = {"Не давай", ELanguage::LANG_RUS};
    request.NonsenseTagging = {
        {"не", 0.5375658274},
        {"давай", 0.002787783509}
    };

    const TVector<TSelectionResult> selected = selector.Select(request, items);
    UNIT_ASSERT(selected.size() == items.size());
    UNIT_ASSERT(selected[0].IsSelected == false);
}

void RunWantNegativesTest(const TDefaultItemSelector& selector) {
    TSelectionRequest request;
    request.Phrase = {"не хочу", ELanguage::LANG_RUS};
    request.NonsenseTagging = {
        {"не", 0.5375658274},
        {"хочу", 0.002787783509}
    };

    TVector<TSelectionItem> items(1);
    items[0].Synonyms = {{"хочу", ELanguage::LANG_RUS}, {"давай", ELanguage::LANG_RUS}};
    items[0].Negatives = {{"не хочу", ELanguage::LANG_RUS}};

    const TVector<TSelectionResult> selected = selector.Select(request, items);

    UNIT_ASSERT(selected.size() == items.size());
    UNIT_ASSERT(selected[0].IsSelected == false);
}

void RunCovidTest(const TDefaultItemSelector& selector) {
    const TVector<double> f1ScoreThresholds = {0.9, 0.75, 0.65, 0.9};

    TVector<TSelectionItem> items(3);
    items[0].Synonyms = {{"симптомы", ELanguage::LANG_RUS}, {"расскажи о симптомах", ELanguage::LANG_RUS}};
    items[1].Synonyms = {
        {"рекомендации", ELanguage::LANG_RUS},
        {"расскажи о рекомендациях", ELanguage::LANG_RUS},
        {"советы", ELanguage::LANG_RUS},
        {"меры профилактики", ELanguage::LANG_RUS}
    };
    items[2].Synonyms = {
        {"расскажи", ELanguage::LANG_RUS},
        {"давай", ELanguage::LANG_RUS},
        {"да", ELanguage::LANG_RUS},
        {"расскажи о симптомах и рекомендациях", ELanguage::LANG_RUS}
    };

    TFileInput fin(ArcadiaSourceRoot() + "/alice/nlu/libs/item_selector/default/ut/covid.tsv");

    TString header;
    fin.ReadLine(header);
    const TVector<TString> itemNames = ObtainItemNames(header);

    TVector<TConfusionMatrix> confusionMatrices(4);
    TString line;
    while(fin.ReadLine(line)) {
        const TDataEntry entry = ParseToDataEntry(line);

        TSelectionRequest request;
        request.Phrase = {entry.Query, ELanguage::LANG_RUS};
        request.NonsenseTagging = GenerateFakeNonsenseTagging(request.Phrase.Text);

        const TVector<TSelectionResult> selected = selector.Select(request, items);

        const bool selectedNothing = !AnyOf(selected, [](const TSelectionResult& r){ return r.IsSelected; });

        for (size_t i = 0; i < items.size() + 1; ++i) {
            const bool shouldSelect = entry.Actions[i];
            const bool hasSelected = i == items.size() ? selectedNothing : selected[i].IsSelected;

            UpdateMetrics(shouldSelect, hasSelected, entry.Count, confusionMatrices[i]);
        }
    }

    for (size_t i = 0; i < items.size() + 1; ++i) {
        const double f1Score = confusionMatrices[i].F1Score();
        const auto message = TStringBuilder() << "F1 score for item " << itemNames[i] << " is " << f1Score << "\n"
            << "TP: " << confusionMatrices[i].TruePositive << " FP: " << confusionMatrices[i].FalsePositive
            << " TN: " << confusionMatrices[i].TrueNegative << " FN: " << confusionMatrices[i].FalseNegative << "\n";
        UNIT_ASSERT_GT_C(f1Score, f1ScoreThresholds[i], message);
    }
}

} // namespace anonymous

Y_UNIT_TEST_SUITE(DefaultItemSelectorTestSuite) {
    const auto embedder = LoadEmbedder();
    const TDefaultItemSelector simplestSelector(nullptr, ELanguage::LANG_UNK, Nothing());
    const TDefaultItemSelector simpleSelector(&embedder, ELanguage::LANG_RUS, Nothing());
    const TDefaultItemSelector idfSelector(&embedder, ELanguage::LANG_RUS, LoadIDFs());

    Y_UNIT_TEST(SimpleTestSuite) {
        RunSimpleTest(simplestSelector);
        RunSimpleTest(simpleSelector);
        RunSimpleTest(idfSelector);
    }

    Y_UNIT_TEST(SimpleNegativeTestSuite) {
        RunSimpleNegativeTest(simplestSelector);
        RunSimpleNegativeTest(simpleSelector);
        RunSimpleNegativeTest(idfSelector);
    }

    Y_UNIT_TEST(WantTestSuite) {
        RunWantTest(simpleSelector);
        RunWantTest(idfSelector);
    }

    Y_UNIT_TEST(UppercaseTestSuite) {
        RunUppercaseTest(simpleSelector);
        RunUppercaseTest(idfSelector);
    }

    Y_UNIT_TEST(UppercaseNegativesTestSuite) {
        RunUppercaseNegativesTest(simpleSelector);
        RunUppercaseNegativesTest(idfSelector);
    }

    Y_UNIT_TEST(WantTestSuiteNegatives) {
        RunWantNegativesTest(simpleSelector);
        RunWantNegativesTest(idfSelector);
    }

    Y_UNIT_TEST(CovidMetric) {
        RunCovidTest(simpleSelector);
        RunCovidTest(idfSelector);
    }
}

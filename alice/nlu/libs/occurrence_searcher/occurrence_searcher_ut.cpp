#include <alice/nlu/libs/occurrence_searcher/occurrence_searcher.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/algorithm.h>
#include <utility>
#include <tuple>

namespace NAlice {
namespace NNlu {

template<>
void Update(int* value, const int& newValue) {
    Y_ASSERT(value);
    (*value) += newValue;
}

bool operator==(const TOccurrence<int>& lhs, const TOccurrence<int>& rhs) {
    return std::tie(lhs.Begin, lhs.End, lhs.Value) == std::tie(rhs.Begin, rhs.End, rhs.Value);
}

IOutputStream& operator<<(IOutputStream& os, const TOccurrence<int> item) {
    return os << "<(" << item.Begin << ", " << item.End << "): " << item.Value << ">";
}


Y_UNIT_TEST_SUITE(OccurrenceSearcherTestSuite) {

Y_UNIT_TEST(TestOccurrenceSearcher) {
    TVector<std::pair<TString, int>> expected{{
        {"nirvana", 100},
        {"smells like teen spirit", 12345},
        {"nevermind", 9},
        {"nothing else matters", 42},
        {"metallica", 13},
        {"spirit of god", 1}
    }};

    TOccurrenceSearcherDataBuilder<int> builder;
    for (const auto& item : expected) {
        builder.Add(item.first, item.second);
    }
    builder.Add("nirvana", 15);
    expected[0].second += 15;
    TOccurrenceSearcher<int> searcher(builder.Build());

    const TString string = "ab nothing else matters c metallica ddd nevermind ! nirvana smells like teen spirit of god";
    TVector<std::pair<TString, int>> got(Reserve(expected.size()));
    for (const auto& occurence : searcher.Search(string)) {
        const auto name = string.substr(occurence.Begin, occurence.End - occurence.Begin);
        got.push_back({name, occurence.Value});
    }

    Sort(std::begin(expected), std::end(expected));
    Sort(std::begin(got), std::end(got));

    UNIT_ASSERT_VALUES_EQUAL(got, expected);
}

Y_UNIT_TEST(TestTokenizedOccurrenceSearcher) {
    const TVector<TString> tokens{
        "каждый",
        "охотник",
        "желает",
        "знать",
        "где",
        "сидит",
        "фазан"
    };

    TVector<TOccurrence<int>> expected{
        {.Value = 3, .Begin = 0, .End = 3},
        {.Value = 2, .Begin = 1, .End = tokens.size()},
        {.Value = 1, .Begin = 0, .End = tokens.size()},
        {.Value = 0, .Begin = 4, .End = 5},
    };

    TOccurrenceSearcherDataBuilder<int> builder;
    for (const auto& item : expected) {
        builder.Add(JoinRange(" ", std::begin(tokens) + item.Begin, std::begin(tokens) + item.End), item.Value);
    }
    TTokenizedOccurrenceSearcher<int> searcher(builder.Build());

    TVector<TOccurrence<int>> got = searcher.Search(tokens);

    const auto keyGetter = [](const TOccurrence<int>& item) { return std::tie(item.Begin, item.End, item.Value); };
    SortBy(expected, keyGetter);
    SortBy(got, keyGetter);

    UNIT_ASSERT_VALUES_EQUAL(got, expected);
}

Y_UNIT_TEST(NormalizeString) {
    UNIT_ASSERT_VALUES_EQUAL(NormalizeString(LANG_RUS, "ВесёлыЙ"), "веселый");
}

}

} // NNlu
} // NAlice

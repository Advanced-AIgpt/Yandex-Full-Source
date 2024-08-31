#include <alice/nlu/libs/annotator/annotator.h>
#include <alice/nlu/libs/annotator/annotator_with_mapping.h>
#include <library/cpp/text_processing/tokenizer/tokenizer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/output.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>


using namespace NAnnotator;
using namespace NTextProcessing::NTokenizer;


namespace {
    TTokenizer GetTokenizer(bool lemmatizing = false) {
        TTokenizerOptions options;
        options.SeparatorType = ESeparatorType::BySense;
        options.Lemmatizing = lemmatizing;
        return TTokenizer(options);
    }

    template <class TAnnotation>
    bool AnnotationLess(const TAnnotation& lhs, const TAnnotation& rhs) {
        return lhs.ClassData < rhs.ClassData;
    }

    TVector<TOccurencePosition> SearchPatternsTrivially(const TVector<TString>& patterns,
                                                        const TString& text,
                                                        const TTokenizer& tokenizer) {
        TVector<TOccurencePosition> positions;

        const auto textTokens = tokenizer.Tokenize(text);
        for (const auto& pattern : patterns) {
            const auto patternTokens = tokenizer.Tokenize(pattern);
            for (size_t start = 0; start + patternTokens.size() <= textTokens.size(); ++start) {
                const size_t end = start + patternTokens.size();
                bool equal = true;
                for (size_t textTokensPos = start; equal && textTokensPos < end; ++textTokensPos) {
                    const size_t patternTokensPos = textTokensPos - start;
                    equal = patternTokens[patternTokensPos] == textTokens[textTokensPos];
                }
                if (equal) {
                    positions.emplace_back(start, end);
                }
            }
        }

        Sort(positions);
        positions.erase(Unique(positions.begin(), positions.end()), positions.end());
        return positions;
    }
} // anonymous

template <>
void Out<TOccurencePosition>(IOutputStream& out, const TOccurencePosition& occurencePosition) {
    out << "[" << occurencePosition.StartToken << ", " << occurencePosition.EndToken << ")";
}


Y_UNIT_TEST_SUITE(AnnotatorTestSuite) {

Y_UNIT_TEST(Annotator) {
    const THashMap<TString, TVector<TString>> classes = {
        {"vowel", {"a", "e", "i", "o", "u", "y"}},
        {"consonant", {"b", "c", "d", "f", "g", "h", "j", "k", "l", "m",
                       "n", "p", "q", "r", "s", "t", "v", "w", "x", "z",
                       // intentional duplicates:
                       "b", "c", "d", "f", "g", "h", "j", "k", "l", "m"}},
        {"letter", {"b", "c", "d", "f", "g", "h", "j", "k", "l", "m",
                    "n", "p", "q", "r", "s", "t", "v", "w", "x", "z",
                    "a", "e", "i", "o", "u", "y"}},
        {"word 'ab'", {"ab", "ab", "ab"}}
    };

    TAnnotatorBuilder builder;

    TClassId expectedClassId = 0;
    for (const auto& [name, patterns] : classes) {
        for (const auto& pattern : patterns) {
            builder.AddPattern(pattern);
        }
        TClassId gotClassId = builder.FinishClass();
        UNIT_ASSERT_VALUES_EQUAL(gotClassId, expectedClassId);
        ++expectedClassId;
    }

    TBufferOutput buffer;
    builder.Save(&buffer);

    TAnnotator annotator(TBlob::FromBuffer(buffer.Buffer()));

    const TVector<TString> samples = {"a ab abc cab", "a a a", "d a s d f g g s h f s v o o o o o o o o o o s g s d", ""};

    const auto tokenizer = GetTokenizer();
    for (const auto& sample : samples) {
        TVector<TVector<TOccurencePosition>> expected;

        const auto sampleTokens = tokenizer.Tokenize(sample);
        for (const auto& [className, patterns] : classes) {
            expected.emplace_back(SearchPatternsTrivially(patterns, sample, tokenizer));
        }

        const auto annotations = annotator.Annotate(sample);

        TVector<TVector<TOccurencePosition>> got(expected.size());
        for (const auto& [classId, positions] : annotations) {
            for (const auto& position : positions) {
                got[classId].emplace_back(position.StartToken, position.EndToken);
            }
        }

        for (auto& occurences : got) {
            Sort(occurences);
        }

        UNIT_ASSERT_VALUES_EQUAL(expected, got);
    }
}

Y_UNIT_TEST(AnnotatorWithMapping) {
    const THashMap<TString, TVector<TString>> classes = {
        {"alice",      {"Алиса", "Эй Алиса", "Алисочка"}},
        {"please",     {"Пожалуйста"}},
        {"check mood", {"Как дела", "Как у тебя дела", "Всё хорошо"}},
        {"repeat",     {"Повтори за мной", "Повтори мои слова"}},
    };

    NAnnotator::TAnnotatorWithMappingBuilder<TString> builder(/*lemmatizing*/true);

    for (const auto& [name, patterns] : classes) {
        for (const auto& pattern : patterns) {
            builder.AddPattern(pattern);
        }
        builder.FinishClass(name);
    }

    TBufferOutput buffer;
    builder.Save(&buffer);

    NAnnotator::TAnnotatorWithMapping<TString> annotator(TBlob::FromBuffer(buffer.Buffer()));

    const TVector<TString> samples = {
        "Привет Алиса Как дела Надеюсь у тебя всё хорошо",
        "Алисочка повторишь за мной пожалуйста пару слов",
        ""
    };

    using TAnnotation = NAnnotator::TComprehensiveAnnotation<TString>;

    const auto tokenizer = GetTokenizer(/*lemmatizing*/true);
    for (const auto& sample : samples) {
        TVector<TAnnotation> expected;
        for (const auto& [name, patterns] : classes) {
            auto occurencePositions = SearchPatternsTrivially(patterns, sample, tokenizer);
            if (!occurencePositions.empty()) {
                expected.emplace_back(TAnnotation{name, occurencePositions});
            }
        }
        Sort(expected, AnnotationLess<TAnnotation>);

        auto got = annotator.Annotate(sample);

        Sort(got, AnnotationLess<TAnnotation>);
        UNIT_ASSERT_VALUES_EQUAL(got.size(), expected.size());
        for (size_t classIdx = 0; classIdx < got.size(); ++classIdx) {
            UNIT_ASSERT_VALUES_EQUAL(got[classIdx].ClassData, expected[classIdx].ClassData);
            UNIT_ASSERT_VALUES_EQUAL(got[classIdx].OccurencePositions, expected[classIdx].OccurencePositions);
        }
    }
}

Y_UNIT_TEST(AnnotatorWithMappingEmpty) {
    NAnnotator::TAnnotatorWithMappingBuilder<TString> builder(/*lemmatizing*/true);

    TBufferOutput buffer;
    builder.Save(&buffer);

    NAnnotator::TAnnotatorWithMapping<TString> annotator(TBlob::FromBuffer(buffer.Buffer()));

    TVector<TString> samples = {"does it work?", ""};

    for (const auto& sample : samples) {
        auto annotations = annotator.Annotate(sample);
        UNIT_ASSERT(annotations.empty());
    }
}

}

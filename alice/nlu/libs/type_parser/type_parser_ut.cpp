#include <alice/nlu/libs/annotator/annotator_with_mapping.h>
#include <alice/nlu/libs/type_parser/dictionary.h>
#include <alice/nlu/libs/type_parser/digital.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/buffer.h>
#include <util/string/split.h>

namespace {
    constexpr TStringBuf TIME_DICTIONARY_PATH = "type_parser_time_rus.dict";

    void TestByValue(const NAlice::TTypeParser& parser,
                     const TStringBuf text,
                     const TVector<TString>& expectedValues) {
        TVector<TString> sortedExpectedValues = expectedValues;
        Sort(sortedExpectedValues);

        const TVector<TString> tokens = StringSplitter(text).Split(' ');
        const auto entitiesByType = parser.Parse(tokens);
        TVector<TString> foundValues;
        for (const auto& [type, entities] : entitiesByType) {
            for (const auto& entity : entities) {
                foundValues.push_back(entity.Value);
            }
        }
        TVector<TString>& sortedFoundValues = foundValues;
        Sort(sortedFoundValues);

        UNIT_ASSERT_VALUES_EQUAL(sortedFoundValues, sortedExpectedValues);
    }

    void TestBySegment(const NAlice::TTypeParser& parser,
                       const TStringBuf text,
                       const TVector<NNlu::TInterval>& expectedSegments) {
        TVector<NNlu::TInterval> sortedExpectedSegments = expectedSegments;
        Sort(sortedExpectedSegments);

        const TVector<TString> tokens = StringSplitter(text).Split(' ');
        const auto entitiesByType = parser.Parse(tokens);
        TVector<NNlu::TInterval> foundSegments;
        for (const auto& [type, entities] : entitiesByType) {
            for (const auto& entity : entities) {
                foundSegments.push_back(entity.Interval);
            }
        }
        TVector<NNlu::TInterval>& sortedFoundSegments = foundSegments;
        Sort(sortedFoundSegments);

        UNIT_ASSERT_VALUES_EQUAL(sortedFoundSegments, sortedExpectedSegments);
    }

    TVector<NNlu::TInterval> GenerateAllSubsegments(const NNlu::TInterval& segment) {
        TVector<NNlu::TInterval> result;
        result.reserve(segment.Length() * (segment.Length() + 1));
        for (size_t start = segment.Begin; start < segment.End; ++start) {
            for (size_t end = start + 1; end <= segment.End; ++end) {
                result.push_back({start, end});
            }
        }
        return result;
    }

    template <class T>
    TVector<T> JoinVectors(const TVector<T>& lhs, const TVector<T>& rhs) {
        TVector<T> result;
        result.reserve(lhs.size() + rhs.size());
        result.insert(result.end(), lhs.begin(), lhs.end());
        result.insert(result.end(), rhs.begin(), rhs.end());
        return result;
    }
} // namespace anonymous

Y_UNIT_TEST_SUITE(TypeParserTestSuite) {

Y_UNIT_TEST(Digital) {
    NAlice::TDigitalTypeParser parser;

    TestBySegment(parser, "1 23 456 78 9", GenerateAllSubsegments({0, 5}));
    TestBySegment(parser, "1 thing 2 say 3 words 4 you", {{0, 1}, {2, 3}, {4, 5}, {6, 7}});
    TestBySegment(parser, "00 01 or 00 02", JoinVectors(GenerateAllSubsegments({0, 2}), GenerateAllSubsegments({3, 5})));
}

Y_UNIT_TEST(Dictionary) {
    NAnnotator::TAnnotatorWithMappingBuilder<TString> annotator;

    annotator.AddPattern("привет");
    annotator.AddPattern("как дела");
    annotator.FinishClass("greeting");

    annotator.AddPattern("привет");
    annotator.AddPattern("хорошо");
    annotator.FinishClass("greeting_response");

    TBufferOutput annotatorOut;
    annotator.Save(&annotatorOut);

    NAlice::TDictionaryTypeParser parser(TBlob::FromBuffer(annotatorOut.Buffer()));

    TestByValue(parser, "привет", {"greeting", "greeting_response"});
    TestByValue(parser, "как дела", {"greeting"});
    TestByValue(parser, "хорошо", {"greeting_response"});
    TestByValue(parser, "привет как дела",
        {"greeting", "greeting_response", // "привет"
         "greeting"});                    // "как дела"
    TestByValue(parser, "алиса у меня всё хорошо а у тебя как дела",
        {"greeting_response", // "хорошо"
         "greeting"});        // "как дела"
}

Y_UNIT_TEST(Time) {
    NAlice::TDictionaryTypeParser parser(TBlob::FromString(NResource::Find(TIME_DICTIONARY_PATH)));

    TestBySegment(parser, "в 12 часов дня будет полдень", {{1, 2}, {1, 3}, {1, 4}, {5, 6}});
    TestBySegment(parser, "поставь будильник на завтра в 5 30 утра", {{5, 6}, {5, 7}, {5, 8}});
    TestBySegment(parser, "солнце ярче всего в полдень а в полночь его совсем не видно", {{4, 5}, {7, 8}});
}

}

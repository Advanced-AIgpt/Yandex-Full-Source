#include "phrases.h"

#include <google/protobuf/text_format.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>
#include <util/string/join.h>

using namespace NAlice;

namespace {

TPhrasesCorpus LoadTestData() {
    TPhrasesCorpus data;
    const auto path = SRC_("test_data.pb.txt");
    UNIT_ASSERT_C(NProtoBuf::TextFormat::ParseFromString(TFileInput{path}.ReadAll(), &data),
                  TString::Join(path, " parse failed"));
    return data;
}

bool TagChecker(TStringBuf tag) {
    const bool invert = tag.AfterPrefix("!", tag);
    if (tag == "trueTag") {
        return !invert;
    }
    if (tag == "falseTag") {
        return invert;
    }
    UNIT_FAIL("unexpected tag");
    return false;
}

} // namespace

Y_UNIT_TEST_SUITE(Phrases) {
    Y_UNIT_TEST(TestData) {
        TPhraseCollection collection{LoadTestData()};

        TString buffer;
        TStringOutput out{buffer};
        const auto consumer = [&](const TPhraseGroup::TPhrase& phrase, const TVector<TStringBuf>& tags, double p) {
            out << phrase.GetText() << " " << JoinSeq(" ", tags) << " " << p << "\n";
            return true;
        };
        collection.FindPhrases("group1", TagChecker, consumer);
        UNIT_ASSERT_EQUAL(buffer, R"___(text1  1
text2 trueTag !falseTag 1
group2/text trueTag 1
nestedGroup1/text trueTag 0.2
nestedGroup1/nested2/text trueTag 0.04
nestedGroup1/nested3/text trueTag 0.06
)___");

        buffer.clear();
        collection.FindPhrases("group_with_fallback1", TagChecker, consumer);
        UNIT_ASSERT_EQUAL(buffer, "text  1\n");

        const auto consumer2 = [&](const auto& phrase, const auto& tags, double p) {
            if (phrase.GetText() == "text") {
                return false;
            }
            return consumer(phrase, tags, p);
        };
        buffer.clear();
        collection.FindPhrases("group_with_fallback1", TagChecker, consumer2);
        UNIT_ASSERT_EQUAL(buffer, "fallback  1\n");

        buffer.clear();
        collection.FindPhrases("group_with_fallback2", TagChecker, consumer);
        UNIT_ASSERT_EQUAL(buffer, "fallback  1\n");
    }
}

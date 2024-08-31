#include "tags.h"

#include <google/protobuf/text_format.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/stream/file.h>
#include <util/string/join.h>

using namespace NAlice;

namespace {

TTagConditionsCorpus LoadTestData() {
    TTagConditionsCorpus data;
    const auto path = SRC_("test_data.pb.txt");
    UNIT_ASSERT_C(NProtoBuf::TextFormat::ParseFromString(TFileInput{path}.ReadAll(), &data),
                  TString::Join(path, " parse failed"));
    return data;
}

} // namespace

Y_UNIT_TEST_SUITE(Tags) {
    Y_UNIT_TEST(TestData) {
        TTagConditionCollection collection{LoadTestData()};
        TProtoEvaluator evaluator;
        TTagEvaluator ctx{collection, evaluator};

        UNIT_ASSERT(ctx.CheckTag("true"));
        UNIT_ASSERT(!ctx.CheckTag("!true"));
        UNIT_ASSERT(!ctx.CheckTag("false"));
        UNIT_ASSERT(ctx.CheckTag("!false"));
        UNIT_ASSERT(ctx.CheckTag("true3"));
        UNIT_ASSERT(ctx.CheckTag("!false3"));

        UNIT_ASSERT(ctx.CheckTag("or_true1"));
        UNIT_ASSERT(ctx.CheckTag("or_true2"));
        UNIT_ASSERT(ctx.CheckTag("and_true1"));
        UNIT_ASSERT(ctx.CheckTag("and_true2"));
        UNIT_ASSERT(ctx.CheckTag("!or_false1"));
        UNIT_ASSERT(ctx.CheckTag("!or_false2"));
        UNIT_ASSERT(ctx.CheckTag("!and_false1"));
        UNIT_ASSERT(ctx.CheckTag("!and_false2"));

        UNIT_ASSERT(!ctx.CheckTag("loop1"));
        UNIT_ASSERT(!ctx.CheckTag("!loop1"));

        UNIT_ASSERT(ctx.CheckTag("5 < 6"));
        UNIT_ASSERT(ctx.CheckTag("!5 < 4"));
        UNIT_ASSERT(!ctx.CheckTag("6 < 6"));
        UNIT_ASSERT(ctx.CheckTag("!6 < 4"));
        UNIT_ASSERT(ctx.CheckTag("10 > 6"));
        UNIT_ASSERT(ctx.CheckTag("10 > 4"));
        UNIT_ASSERT(ctx.CheckTag("no_tag_suffix"));
    }
}

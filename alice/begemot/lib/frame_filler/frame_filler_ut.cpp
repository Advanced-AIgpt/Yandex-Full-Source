#include <alice/begemot/lib/frame_filler/frame_filler.h>
#include <alice/nlu/libs/token_aligner/aligner.h>
#include <library/cpp/testing/unittest/registar.h>
#include <search/begemot/rules/external_markup/proto/external_markup.pb.h>
#include <search/begemot/rules/occurrences/custom_entities/rule/proto/custom_entities.pb.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/string/split.h>

namespace {
    const THashMap<TString, THashSet<TString>> ACCEPTED_TYPES_BY_SLOT = {
        {"geo_slot", {"GeoAddr.Location"}},
        {"fst_slot", {"fst.time"}},
        {"custom_entities_slot", {"custom.song"}},
        {"frame_entities_slot", {"when"}},
        {"untyped_slot", {"string"}},
        {"all_types_slot", {"GeoAddr.City", "fst.time", "custom.song", "when", "string"}}};

    NAlice::TSemanticFrame GetSemanticFrame() {
        NAlice::TSemanticFrame frame;
        frame.SetName("test_frame");
        for (const auto& [slotName, types] : ACCEPTED_TYPES_BY_SLOT) {
            auto& slot = *frame.AddSlots();
            slot.SetName(slotName);
            for (const auto& type : types) {
                slot.AddAcceptedTypes(type);
            }
        }

        return frame;
    }

    TVector<TString> Tokenize(const TString& text) {
        return StringSplitter(text).Split(' ');
    }

    NBg::TFrameFiller CreateFrameFiller(const TString& request,
                                        const TString& resolvedRequest,
                                        const TString& slotName) {
        const auto requestTokens = Tokenize(request);
        const auto resolvedRequestTokens = Tokenize(resolvedRequest);
        return NBg::TFrameFiller(GetSemanticFrame(),
                                 slotName,
                                 ACCEPTED_TYPES_BY_SLOT.at(slotName),
                                 NNlu::TTokenAligner().Align(resolvedRequestTokens, requestTokens),
                                 requestTokens.size());
    }
} // namespace anonymous

Y_UNIT_TEST_SUITE(TFrameFillerSuite) {

Y_UNIT_TEST(CustomEntities) {
    {
        const auto frameFiller = CreateFrameFiller("мурку давай",         // unresolved
                                                   "поставь песню мурку", // text
                                                   "custom_entities_slot");
        auto match = frameFiller.TryFillWithEntity({
            NGranet::TEntity{{2, 3}, "cat_name", "murka the cat", "", "", 0., 0.},
            NGranet::TEntity{{2, 3}, "custom.song", "murka the song", "", "", 0., 0.}
        });
        UNIT_ASSERT(match.Defined());
        UNIT_ASSERT_VALUES_EQUAL(1, match->NumTokens);
        UNIT_ASSERT_VALUES_EQUAL("custom.song", match->Type);
        UNIT_ASSERT(match->Value == "murka the song");
    }
}

Y_UNIT_TEST(Fst) {
    {
        const auto frameFiller = CreateFrameFiller("на 9 утра пожалуйста",                      // unresolved
                                                   "поставь-ка будильник на 9 утра пожалуйста", // text
                                                   "fst_slot");
        auto match = frameFiller.TryFillWithEntity({
            NGranet::TEntity{{3, 5}, "fst.time", "{\"hours\":9,\"minutes\":0,\"period\":\"am\"}", "", "", 0., 0.}
        });
        UNIT_ASSERT(match.Defined());
        UNIT_ASSERT_VALUES_EQUAL(2, match->NumTokens);
        UNIT_ASSERT_VALUES_EQUAL("fst.time", match->Type);
        UNIT_ASSERT(match->Value == "{\"hours\":9,\"minutes\":0,\"period\":\"am\"}");
    }

    {
        const auto frameFiller = CreateFrameFiller("9 утра и 8 вечера",                      // unresolved
                                                   "поставь будильник на 9 утра и 8 вечера", // text
                                                   "fst_slot");
        auto match = frameFiller.TryFillWithEntity({
            NGranet::TEntity{{3, 5}, "fst.time", "{\"hours\":9,\"minutes\":0,\"period\":\"am\"}", "", "", 0., 0.},
            NGranet::TEntity{{6, 8}, "fst.time", "{\"hours\":8,\"minutes\":0,\"period\":\"pm\"}", "", "", 0., 0.}
        });
        UNIT_ASSERT(!match.Defined());
    }
}

Y_UNIT_TEST(GeoAddr) {
    {
        const auto frameFiller = CreateFrameFiller("в санкт-петербург",         // unresolved
                                                   "поехали в санкт-петербург", // text
                                                   "geo_slot");
        auto match = frameFiller.TryFillWithEntity({
            NGranet::TEntity{{2, 3}, "GeoAddr.Location", "санкт-петербург", "", "", 0., 0.}
        });
        UNIT_ASSERT(match.Defined());
        UNIT_ASSERT_VALUES_EQUAL(1, match->NumTokens);
        UNIT_ASSERT_VALUES_EQUAL("GeoAddr.Location", match->Type);
        UNIT_ASSERT(match->Value == "санкт-петербург");
    }

    {
        const auto frameFiller = CreateFrameFiller("в питер тогда",         // unresolved
                                                   "поехали в питер тогда", // text
                                                   "geo_slot");
        auto match = frameFiller.TryFillWithEntity({
            NGranet::TEntity{{2, 3}, "GeoAddr.Location", "санкт-петербург", "", "", 0., 0.}
        });
        UNIT_ASSERT(!match.Defined()); // bad case:(
    }

    {
        const auto frameFiller = CreateFrameFiller("питер", // unresolved
                                                   "питер", // text
                                                   "geo_slot");
        auto match = frameFiller.TryFillWithEntity({
            NGranet::TEntity{{0, 1}, "GeoAddr.Location", "санкт-петербург", "", "", 0., 0.}
        });
        UNIT_ASSERT(match.Defined());
        UNIT_ASSERT_VALUES_EQUAL(1, match->NumTokens);
        UNIT_ASSERT_VALUES_EQUAL("GeoAddr.Location", match->Type);
        UNIT_ASSERT(match->Value == "санкт-петербург");
    }
}

Y_UNIT_TEST(Untyped) {
    {
        const TString unresolvedRequest = "песню про белого бычка";
        const auto frameFiller = CreateFrameFiller(unresolvedRequest,
                                                   "включи песню про белого бычка",  // text
                                                   "geo_slot");
        auto match = frameFiller.TryFillWithUntypedValue(
            {NGranet::TEntity{{2, 3}, "GeoAddr.Location", "санкт-петербург", "", "", 0., 0.}},
            Tokenize(unresolvedRequest)
        );
        UNIT_ASSERT(!match.Defined());
    }

    {
        const TString unresolvedRequest = "песню про белого бычка";
        const auto frameFiller = CreateFrameFiller(unresolvedRequest,
                                                   "включи песню про белого бычка",  // text
                                                   "untyped_slot");
        auto match = frameFiller.TryFillWithUntypedValue(
            {},
            Tokenize(unresolvedRequest)
        );
        UNIT_ASSERT(match.Defined());
        UNIT_ASSERT_VALUES_EQUAL(4, match->NumTokens);
        UNIT_ASSERT_VALUES_EQUAL("string", match->Type);
        UNIT_ASSERT_VALUES_EQUAL("песню про белого бычка", match->Value);
    }

    {
        const TString unresolvedRequest = "пожалуйста песню про как его белого бычка";
        const auto frameFiller = CreateFrameFiller(unresolvedRequest,
                                                   "включи пожалуйста песню про как его белого бычка",  // text
                                                   "untyped_slot");
        auto match = frameFiller.TryFillWithUntypedValue(
            {
                NGranet::TEntity{{1, 2}, "nonsense", "", "", "", 0., 0.},
                NGranet::TEntity{{4, 6}, "nonsense", "", "", "", 0., 0.}
            },
            Tokenize(unresolvedRequest)
        );
        UNIT_ASSERT(match.Defined());
        UNIT_ASSERT_VALUES_EQUAL(4, match->NumTokens);
        UNIT_ASSERT_VALUES_EQUAL("string", match->Type);
        UNIT_ASSERT_VALUES_EQUAL("песню про белого бычка", match->Value);
    }
}

}

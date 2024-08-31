#include "vins.h"

#include <alice/bass/ut/helpers.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NBASS;

namespace {

const NSc::TValue REQUEST = NSc::TValue::FromJson(R"({
    "form": {
        "name": "foo"
    },
    "meta": {
        "epoch": 1578676778,
        "tz": "Europe/Moscow",
        "utterance": "превед"
    }
})");

class TTestContinuation final : public THollywoodContinuation {
public:
    using THollywoodContinuation::THollywoodContinuation;

    TStringBuf GetName() const override {
        return TStringBuf("TTestContinuation");
    }

    TResultValue Apply() override {
        return ResultSuccess();
    }
};

Y_UNIT_TEST_SUITE(HollywoodContinuation) {
    Y_UNIT_TEST(NotFinished) {
        const auto context = NTestingHelpers::MakeContext(REQUEST);

        const TTestContinuation continuation{context, {} /* applyArguments */, {} /* featuresData */};
        UNIT_ASSERT(!continuation.IsFinished());
    }

    Y_UNIT_TEST(Name) {
        const auto context = NTestingHelpers::MakeContext(REQUEST);

        const TTestContinuation continuation{context, {} /* applyArguments */, {} /* featuresData */};

        const auto json = continuation.ToJson();
        UNIT_ASSERT_VALUES_EQUAL(continuation.GetName(), json["ObjectTypeName"].GetString());
    }

    Y_UNIT_TEST(Serialization) {
        const auto context = NTestingHelpers::MakeContext(REQUEST);

        const NSc::TValue applyArguments = NSc::TValue::FromJson(R"({
            "hello": "world"
        })");

        const NSc::TValue featuresData = NSc::TValue::FromJson(R"({
            "bar": 1
        })");
        TTestContinuation continuation{context, NSc::TValue{applyArguments}, NSc::TValue{featuresData}};

        const auto json = continuation.ToJson();

        auto globalCtx = IGlobalContext::MakePtr<NTestingHelpers::TTestGlobalContext>();
        const NSc::TValue configPatch{};
        TTestContinuation restoredContinuation{json,
                                               globalCtx,
                                               REQUEST["meta"],
                                               context->UserAuthorizationHeader(),
                                               context->UserTicket(),
                                               configPatch};

        UNIT_ASSERT_VALUES_EQUAL(applyArguments, restoredContinuation.ApplyArguments());
        UNIT_ASSERT_VALUES_EQUAL(featuresData, restoredContinuation.FeaturesData());
    }
}

} // namespace

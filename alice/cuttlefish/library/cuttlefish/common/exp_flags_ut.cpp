#include <library/cpp/testing/unittest/registar.h>
#include <alice/cuttlefish/library/cuttlefish/common/exp_flags.h>


Y_UNIT_TEST_SUITE(GetExpValue) {

    Y_UNIT_TEST(GetFail) {
        NAliceProtocol::TRequestContext reqCtx;
        reqCtx.MutableExpFlags()->insert({"hello", "1"});
        reqCtx.MutableExpFlags()->insert({"world", "1"});
        reqCtx.MutableExpFlags()->insert({"sun", "1"});
        reqCtx.MutableExpFlags()->insert({"moon", "1"});

        UNIT_ASSERT(!NAlice::NCuttlefish::NExpFlags::GetExperimentValue(reqCtx, "world").Defined());
    }

    Y_UNIT_TEST(GetSuccess) {
        NAliceProtocol::TRequestContext reqCtx;
        reqCtx.MutableExpFlags()->insert({"hello=world", "1"});
        reqCtx.MutableExpFlags()->insert({"helloo=woorld", "1"});
        reqCtx.MutableExpFlags()->insert({"wrong=", "1"});
        reqCtx.MutableExpFlags()->insert({"temp=14", "1"});

        UNIT_ASSERT_VALUES_EQUAL(NAlice::NCuttlefish::NExpFlags::GetExperimentValue(reqCtx, "hello").GetRef(), "world");
        UNIT_ASSERT_VALUES_EQUAL(NAlice::NCuttlefish::NExpFlags::GetExperimentValue(reqCtx, "helloo").GetRef(), "woorld");
        UNIT_ASSERT_VALUES_EQUAL(NAlice::NCuttlefish::NExpFlags::GetExperimentValue(reqCtx, "wrong").GetRef(), "");
        UNIT_ASSERT_VALUES_EQUAL(NAlice::NCuttlefish::NExpFlags::GetExperimentValue(reqCtx, "temp").GetRef(), "14");
    }

    Y_UNIT_TEST(Conducting) {
        NAliceProtocol::TRequestContext reqCtx;
        reqCtx.MutableExpFlags()->insert({"hello", "1"});
        reqCtx.MutableExpFlags()->insert({"world", "1"});
        reqCtx.MutableExpFlags()->insert({"sun", "1"});
        reqCtx.MutableExpFlags()->insert({"moon", "1"});

        UNIT_ASSERT(NAlice::NCuttlefish::NExpFlags::ConductingExperiment(reqCtx, "hello"));
        UNIT_ASSERT(NAlice::NCuttlefish::NExpFlags::ConductingExperiment(reqCtx, "world"));
        UNIT_ASSERT(NAlice::NCuttlefish::NExpFlags::ConductingExperiment(reqCtx, "sun"));
        UNIT_ASSERT(NAlice::NCuttlefish::NExpFlags::ConductingExperiment(reqCtx, "moon"));
        UNIT_ASSERT(!NAlice::NCuttlefish::NExpFlags::ConductingExperiment(reqCtx, "sunday"));
        UNIT_ASSERT(!NAlice::NCuttlefish::NExpFlags::ConductingExperiment(reqCtx, "wednesday"));
    }

    Y_UNIT_TEST(TestExperimentFlagHasTrueValue) {
        NAliceProtocol::TRequestContext requestContext;

        {
            auto& expFlags = *requestContext.MutableExpFlags();
            expFlags["exp1"] = "true";
            expFlags["exp2"] = "1";
            expFlags["exp3"] = "false";
            expFlags["exp4"] = "0";
        }

        UNIT_ASSERT(NAlice::NCuttlefish::NExpFlags::ExperimentFlagHasTrueValue(requestContext, "exp1"));
        UNIT_ASSERT(NAlice::NCuttlefish::NExpFlags::ExperimentFlagHasTrueValue(requestContext, "exp2"));
        UNIT_ASSERT(!NAlice::NCuttlefish::NExpFlags::ExperimentFlagHasTrueValue(requestContext, "exp3"));
        UNIT_ASSERT(!NAlice::NCuttlefish::NExpFlags::ExperimentFlagHasTrueValue(requestContext, "exp4"));
        UNIT_ASSERT(!NAlice::NCuttlefish::NExpFlags::ExperimentFlagHasTrueValue(requestContext, "random_exp"));
    }
}

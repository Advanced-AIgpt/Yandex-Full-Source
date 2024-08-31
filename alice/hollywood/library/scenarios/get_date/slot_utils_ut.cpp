#include "get_date.h"

#include <alice/hollywood/library/scenarios/get_date/proto/get_date.pb.h>
#include <alice/hollywood/library/scenarios/get_date/slot_utils/slot_utils.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/timezone_conversion/civil.h>

namespace NAlice::NHollywoodFw::NGetDate {

class TGetDateFixture : public NUnitTest::TBaseFixture {
public:
    TGetDateFixture()
    : Request_(RequestProto_, Ctx_)
    {
    }

private:
    NScenarios::TScenarioRunRequest RequestProto_;
    NAppHost::NService::TTestContext Ctx_;
    NHollywood::TScenarioRunRequestWrapper Request_;
};

Y_UNIT_TEST_SUITE_F(GetDate, TGetDateFixture) {
    Y_UNIT_TEST(TestGetDateTense) {

        TVector<std::pair<TStringBuf, NAlice::TSysDatetimeParser::ETense> > slots = {
            std::pair("past", NAlice::TSysDatetimeParser::ETense::TensePast),
            std::pair("future", NAlice::TSysDatetimeParser::ETense::TenseFuture),
            std::pair("present", NAlice::TSysDatetimeParser::ETense::TenseDefault),
            std::pair("xxx", NAlice::TSysDatetimeParser::ETense::TenseDefault),
            std::pair("", NAlice::TSysDatetimeParser::ETense::TenseDefault)
        };

        for (const auto& it : slots) {
            UNIT_ASSERT(FillTenseInfo(it.first) == it.second);
        }
    }

    Y_UNIT_TEST(TestGetTargetQuestionInfo) {

        TVector<std::pair<TString, ETargetQuestion> > slots = {
            std::pair("date", ETargetQuestion::Date),
            std::pair("date_and_day_of_week", ETargetQuestion::All),
            std::pair("day_of_week", ETargetQuestion::DayOfWeek),
            std::pair("year", ETargetQuestion::Year),
            std::pair("", ETargetQuestion::Default),
            std::pair("xxx", ETargetQuestion::Unknown)
        };

        for (const auto& it : slots) {
            TGetDateSceneArgs state;
            state.SetQueryTarget(it.first);
            UNIT_ASSERT(GetTargetQuestionInfo(state) == it.second);
        }
    }
}

} // namespace NAlice::NHollywood::NGetDate

#include "get_date.h"

#include <alice/hollywood/library/scenarios/get_date/proto/get_date.pb.h>

#include <alice/hollywood/library/framework/framework_ut.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywoodFw::NGetDate {

namespace {


} // anonimous namespace

Y_UNIT_TEST_SUITE(GetDateRender) {

    Y_UNIT_TEST(GetDateRender1) {
        TTestEnvironment testData("get_date", "ru-ru");
        TGetDateRenderProto proto;

        proto.SetPhrase("day_of_week");
        proto.SetSourceDate("{\"days\":0,\"days_relative\":true}");
        proto.SetResultCity("Москва");
        proto.SetCityPreparse("{\"city\":\"Москва\"}");
        proto.SetIsCustomCity(false);
        proto.SetResultDayWeek(0); // Monday

        UNIT_ASSERT(testData >> TTestRender(&TGetDateScenario::Render, proto) >> testData);
        UNIT_ASSERT(!testData.IsIrrelevant());
        UNIT_ASSERT(testData.ContainsVoice("Сегодня "));
        UNIT_ASSERT(testData.ContainsVoice(" понедельник."));
    }
}

} // namespace NAlice::NHollywoodFw::NGetDate

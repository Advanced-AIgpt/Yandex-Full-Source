#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/ner/ner.h>
#include <alice/bass/ut/helpers.h>
#include <alice/library/unittest/fake_fetcher.h>

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/tests_data.h>

#include <util/generic/strbuf.h>


using namespace NBASS;

namespace {

const TString DATE_UTTERANCE = "1 апреля 2019 года.";
const TString DATE_FAKE_RESPONSE = R"-({"nlu":{"entities":[{"tokens":{"end":1,"start":0},"type":"YANDEX.NUMBER","value":1},{"tokens":{"end":4,"start":0},"type":"YANDEX.DATETIME","value":{"day":1,"day_is_relative":false,"month":4,"month_is_relative":false,"year":2019,"year_is_relative":false}},{"tokens":{"end":3,"start":2},"type":"YANDEX.NUMBER","value":2019}],"tokens":["1","апреля","2019","года"]}})-";
const TString EMPTY_UTTERANCE = "";
const TString EMPTY_FAKE_RESPONSE = "";
Y_UNIT_TEST_SUITE(Ner) {

Y_UNIT_TEST(Response) {
    TStringBuf skillId;
    TNerRequester nerRequester = TNerRequester{THolder(new NAlice::NTestingHelpers::TFakeRequest(DATE_FAKE_RESPONSE)), DATE_UTTERANCE, skillId};
    const NSc::TValue* nerData = nerRequester.Response();
    UNIT_ASSERT(nerData);
    UNIT_ASSERT_VALUES_EQUAL(nerData->ToJson(NSc::TValue::JO_SORT_KEYS), DATE_FAKE_RESPONSE);

    const NSc::TValue* nerDataAgain = nerRequester.Response();
    UNIT_ASSERT(nerData);
    UNIT_ASSERT_VALUES_EQUAL(nerDataAgain->ToJson(NSc::TValue::JO_SORT_KEYS), DATE_FAKE_RESPONSE);

    TNerRequester nerRequesterEmpty = TNerRequester{THolder(new NAlice::NTestingHelpers::TFakeRequest(EMPTY_FAKE_RESPONSE)), EMPTY_UTTERANCE, skillId};
    const NSc::TValue* nerDataEmpty = nerRequesterEmpty.Response();
    UNIT_ASSERT(!nerDataEmpty);
}
}

} // namespace

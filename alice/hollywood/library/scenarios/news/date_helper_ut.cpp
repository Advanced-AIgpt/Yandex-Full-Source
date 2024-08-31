#include "date_helper.h"

#include <library/cpp/testing/unittest/registar.h>

#include <google/protobuf/util/json_util.h>

#include <util/generic/strbuf.h>

namespace NAlice::NHollywood {

static TVector<std::pair<TStringBuf, bool>> VARIOUS_SLOTS = {
    std::pair<TStringBuf, bool>("", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":-1}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":-2}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":-3}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":-4}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":-5}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":-6}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":-7}", false),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":-8}", false),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":-9}", false),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":-10}", false),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":1}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":2}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":3}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":4}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":5}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":6}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":7}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":8}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":9}", true),
    std::pair<TStringBuf, bool>("{\"days_relative\":true, \"days\":10}", true),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":23}", true),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":22}", true),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":21}", true),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":20}", true),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":19}", true),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":18}", true),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":17}", true),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":16}", true),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":15}", true),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":14}", true),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":13}", false),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":12}", false),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":11}", false),
    std::pair<TStringBuf, bool>("{\"months\":10, \"days\":10}", false),
    std::pair<TStringBuf, bool>("{\"months\":9, \"days\":30}", false),
    std::pair<TStringBuf, bool>("{\"months\":11, \"days\":1}", true)
};

Y_UNIT_TEST_SUITE(NewsDateTests) {

    Y_UNIT_TEST(NewsDateTests1) {
        const TInstant localtime =TInstant::FromValue(1634729880UL*1000UL*1000UL); // 2021-10-20 11:38:00 in microseconds

        for (const auto& it : VARIOUS_SLOTS) {
            NAlice::TSysDateValue dateProto;
            google::protobuf::util::JsonStringToMessage(it.first.Data(), &dateProto);
            UNIT_ASSERT(IsActual(TSysDatetimeParser::Parse(dateProto), localtime) == it.second);
        }
    }

    Y_UNIT_TEST(NewsDateTests2) {
        const TInstant localtime = TInstant::Now();
        NAlice::TSysDateValue dateProto;
        UNIT_ASSERT(IsActual(TSysDatetimeParser::Parse(dateProto), localtime));
    }

}

} // namespace NAlice::NHollywood

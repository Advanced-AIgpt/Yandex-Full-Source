#include <alice/nlg/example/import_example/register.h>
#include <alice/nlg/library/testing/testing_helpers.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NNlg::NTesting;

namespace {

const auto REG = &NAlice::NNlg::NExample::NImportExample::RegisterAll;

} // namespace

Y_UNIT_TEST_SUITE(ToolFeatures) {
    Y_UNIT_TEST(TestInnerMacrosUsage) {
        TestPhrase(REG, "render_dt", "inner_macros_usage", "['\\xD0\\x9F\\xD0\\xBD', '\\xD0\\x92\\xD1\\x82'] 1 2;"
                   " ['orange', 'apple'] [3, 7] 2; ['orange', 'apple'] [3, 7] 2;"
                   " 2008 10 16 0 0 0 0 None октябрь четверг;");
    }

    Y_UNIT_TEST(TestTry) {
        TestPhrase(REG, "render_dt", "try", "сегодня 0:59:3 1970");
    }

    Y_UNIT_TEST(TestExtGlobal) {
        TestPhrase(REG, "render_dt", "external_global", "импортированный модуль всё ещё должен писать 123");
    }

    Y_UNIT_TEST(TesExtGlobalStrict) {
        TestPhrase(REG, "render_dt", "external_global_strict", "1995 1 5 12 15 40 0 None январь четверг");
    }

    Y_UNIT_TEST(TestCallExtNumStringValues) {
        TestPhrase(REG, "render_dt", "call_external_num_string_values", "1917-11-07; 07.11.1917; 00:00 1917-11-07;"
                        " 1961-04-12 09:07; 1961-04-12; 11:48:07 01.04.2020;");
    }

    Y_UNIT_TEST(TestCallExtListDictValues) {
        TestPhrase(REG, "render_dt", "call_external_list_dict_values",
                   "['melon', 'apple', 'orange'] None 2 [3, 7];"
                   " ['Sa', 'We', 'Fr', 'Tu'] 6 3 5 2;"
                   " ['orange', 'apple'] [3, 7] 2;"
                   " ['orange', 'apple'] [14, 4] ['Tu', [4, 8, 3]];"
                   " ['weekdays', 'months'] ['Mo', 'Tu', 'We', 'Th', 'Fr', 'Sa', 'Su'] {'Jan': 31};"
                   " ['\\xD0\\x9F\\xD0\\xBD', '\\xD0\\x92\\xD1\\x82'] 1 2;");
    }

    Y_UNIT_TEST(TestCallExtRangeValues) {
        TestPhrase(REG, "render_dt", "call_external_range_values", "xrange(3); 0 1 2");
    }

    Y_UNIT_TEST(TestPrintExtDefaults) {
        TestPhrase(REG, "render_dt", "print_external_defaults",
                   "13; 3.14159; sample; 0 1 2 3; True 1; 19 2.71828 0 str_in_list xrange(3) {'list': [1, 2]};"
                   " 23 0.577 str_in_dict 1 xrange(2) [29] {'key': 5}");
        TestPhrase(REG, "render_dt", "print_empty_list_and_dict", "[] {}");
    }

    Y_UNIT_TEST(TestExtNlgImport) {
        TestPhrase(REG, "render_dt", "give_rendered_date", "2020 4 5 20 40 0 0 None апрель воскресенье");
        TestPhrase(REG, "render_dt", "check_lib_test", "test");
        TestPhrase(REG, "render_dt", "test", "test");
        TestPhrase(REG, "render_dt", "hello", "Hello,...!");
    }
}

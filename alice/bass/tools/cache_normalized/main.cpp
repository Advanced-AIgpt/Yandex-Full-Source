/*
 * This program can be used to cache normalized names for IoT scenario. Names
 * can be collected by using a query like
 * https://yql.yandex-team.ru/Operations/Xjvg2QlcTodT0I1QTs4SFMquDR75mh2Yaic5Dy-Yfx0=
 * Supply the query output in json format through standard input. A map from
 * names to normalized names will be printed in standard output.
 */

#include <alice/bass/libs/iot/utils.h>

#include <library/cpp/scheme/scheme.h>

using namespace NBASS::NIOT;

int main() {
    NSc::TValue result;

    TString line;
    i64 total = 0;
    i64 cached = 0;
    while (Cin.ReadLine(line)) {
        const auto parsed = NSc::TValue::FromJson(line);
        if (parsed.IsDict()) {
            auto name = parsed["name"].GetString();
            auto count = parsed["count"].GetIntNumber(0);
            total += count;
            if (count >= 5) {
                result[name] = Normalize(name);
                cached += count;
            }
        }
    }

    Cout << result.ToJsonPretty();
    Cerr << "Cached " << 100.0 * cached / total << "%" << Endl;

    return 0;
}

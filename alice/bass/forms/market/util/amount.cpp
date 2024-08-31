#include <alice/bass/forms/context/context.h>
#include <util/system/types.h>

namespace NBASS {

namespace NMarket {

double NormalizeAmount(TStringBuf amount)
{
    double num;
    if (TryFromString(amount, num)) {
        return num;
    }

    auto list = StringSplitter(amount).Split(' ').SkipEmpty().ToList<TStringBuf>();
    if (list.size() != 2) {
        return 0;
    }

    TString first = TString{list[0]}, second = TString{list[1]};
    if (second == "1000" || second.StartsWith("тыс")) {
        if (first.StartsWith("полтор")) {
            return 1500;
        }
        auto pos = first.find(',');
        if (pos != TString::npos) {
            first[pos] = '.';
        }
        return FromString<double>(first) * 1000;
    }

    if (first.length() > 2 || second.length() != 3) {
        return 0;
    }
    if (TryFromString<double>(first + second, num)) {
        return num;
    }
    return 0;
}

} // End of namespace NMarket

} // End of namespace NBass

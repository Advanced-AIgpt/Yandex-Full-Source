#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/input.h>

namespace NCalendarParser {

class TReader final {
public:
    explicit TReader(IInputStream& input);

    bool NextLine(TString& line);

    template <typename TFn>
    static void ForEachLine(IInputStream& input, TFn&& fn) {
        TString line;
        TReader reader(input);
        while (reader.NextLine(line))
            fn(line);
    }

    static void ReadLines(IInputStream& input, TVector<TString>& lines);

private:
    // Returns true if line was successfully read from
    // Input. Otherwise returns false.
    static bool NextLine(IInputStream& input, TString& line);

private:
    IInputStream& Input;

    TString Curr;
    bool HasCurr = false;
};

} // namespace NCalendarParser

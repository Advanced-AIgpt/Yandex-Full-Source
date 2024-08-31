#include "reader.h"

#include <util/generic/yexception.h>

namespace NCalendarParser {

TReader::TReader(IInputStream& input)
    : Input(input) {
    HasCurr = NextLine(Input, Curr);
}

bool TReader::NextLine(TString& line) {
    if (!HasCurr)
        return false;

    line = Curr;
    HasCurr = false;
    while (NextLine(Input, Curr)) {
        if (Curr.StartsWith(' ') || Curr.StartsWith('\t')) {
            line.append(Curr.begin() + 1, Curr.end());
        } else {
            HasCurr = true;
            break;
        }
    }

    return true;
}

// static
void TReader::ReadLines(IInputStream& input, TVector<TString>& lines) {
    ForEachLine(input, [&lines](const TString& line) { lines.push_back(line); });
}

// static
bool TReader::NextLine(IInputStream& input, TString& line) {
    try {
        line = input.ReadLine();
        return true;
    } catch (const yexception&) {
        return false;
    }
}

} // namespace NCalendarParser

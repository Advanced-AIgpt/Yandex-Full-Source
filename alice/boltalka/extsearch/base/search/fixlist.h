#pragma once

#include <library/cpp/regex/pcre/regexp.h>

#include <util/generic/vector.h>

namespace NNlg {

struct TFixlistCase {
    TRegExMatch MatchPhrase;
    TVector<TString> Answers;
};

class TFixlist {
public:
    TFixlist();

    TVector<TString> MatchContext(const TVector<TString>& context) const;

private:
    TVector<TFixlistCase> Cases;
};

}

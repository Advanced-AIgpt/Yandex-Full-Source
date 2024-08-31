#include "fixlist.h"

namespace NNlg {

TFixlist::TFixlist() {}

TVector<TString> TFixlist::MatchContext(const TVector<TString>& context) const {
    if (context.empty()) {
        return {};
    }
    for (const auto& fixlistCase : Cases) {
        if (fixlistCase.MatchPhrase.Match(context[0].data())) {
            return fixlistCase.Answers;
        }
    }
    return {};
}

}

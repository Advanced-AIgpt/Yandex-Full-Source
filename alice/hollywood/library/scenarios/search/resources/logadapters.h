#pragma once

#include <alice/library/logger/logadapter.h>

namespace NAlice::NHollywood::NSearch {

class TEmptyLogAdapter final : public TLogAdapter {
private:
    void LogImpl(TStringBuf, const TSourceLocation&, ELogAdapterType) const override {
    }
};

} // namespace NAlice::NHollywood::NSearch

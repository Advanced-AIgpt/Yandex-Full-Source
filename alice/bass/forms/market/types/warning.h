#pragma once

#include <library/cpp/scheme/scheme.h>

namespace NBASS {

namespace NMarket {

class TWarning {
public:
    explicit TWarning(const NSc::TValue& data);

    TStringBuf GetType() const { return Type; }
    TStringBuf GetValue() const { return Value; }
    bool IsIllegal() const;

    static TVector<TWarning> InitVector(const NSc::TValue& data);
    static TStringBuf GetAge(TStringBuf type);

private:
    TString Type;
    TString Value;
};

} // namespace NMarket

} // namespace NBASS

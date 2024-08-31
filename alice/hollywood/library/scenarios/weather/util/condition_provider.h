#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NHollywood::NWeather {

class IConditionProvider {
public:
    virtual ~IConditionProvider() = default;
    virtual double GetTemp() const = 0;
    virtual size_t GetPrecType() const = 0;
    virtual double GetPrecStrength() const = 0;
    virtual double GetCloudness() const = 0;
    virtual TStringBuf GetCondition() const = 0;
};

template<typename TObj>
class TDefaultConditionProvider : public IConditionProvider {
public:
    TDefaultConditionProvider(const TObj& obj)
        : Obj_{obj}
    {}

    double GetTemp() const override {
        if constexpr (requires { Obj_.Temp; }) {
            return Obj_.Temp;
        } else {
            // some API object have only average temperature
            return Obj_.TempAvg;
        }
    };
    size_t GetPrecType() const override { return Obj_.PrecType; }
    double GetPrecStrength() const override { return Obj_.PrecStrength; }
    double GetCloudness() const override { return Obj_.Cloudness; }
    TStringBuf GetCondition() const override { return Obj_.Condition; }

private:
    const TObj& Obj_;
};

} // NAlice::NHollywood::NWeather

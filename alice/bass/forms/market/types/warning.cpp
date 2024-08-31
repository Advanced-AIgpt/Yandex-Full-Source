#include "warning.h"
#include <alice/bass/forms/market/settings.h>
#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {

namespace NMarket {

TWarning::TWarning(const NSc::TValue& data)
    : Type(data["type"])
    , Value(data["value"]["full"])
{
    if (TStringBuf age = GetAge(Type)) {
        Value += " " + TString{age} + "+";
    }
}

bool TWarning::IsIllegal() const
{
    static const THashSet<TStringBuf> ILLEGAL_WARNINGS {
        TStringBuf("adult"),
        TStringBuf("tobacco"),
        TStringBuf("weapons"),
    };
    return ILLEGAL_WARNINGS.contains(Type);
}

TVector<TWarning> TWarning::InitVector(const NSc::TValue& data)
{
    TVector<TWarning> warnings;
    if (!data.IsNull()) {
        for (const auto& jsonedWarning : data["common"].GetArray()) {
            warnings.push_back(TWarning(jsonedWarning));
        }
    }
    return warnings;
}

TStringBuf TWarning::GetAge(TStringBuf type)
{
    // https://github.yandex-team.ru/market/MarketNode/blob/master/client/desktop.blocks/n-warning/n-warning.yate
    if (type.substr(0, 4) == "age_") {
        TStringBuf age = type.substr(4);
        i32 val;
        if (!TryFromString(age, val) || val < 0 || val > 18) {
            LOG(WARNING) << " strange age_* disclaimer type: '" << type << "', assuming 18" << Endl;
            return "18";
        } else {
            return age;
        }
    } else if (type == "age" || type == "adult") {
        return "18";
    }
    return "";
}

} // namespace NMarket

} // namespace NBASS

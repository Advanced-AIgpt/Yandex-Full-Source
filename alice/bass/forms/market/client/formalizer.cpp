#include <alice/bass/forms/market/client.h>

#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {

namespace NMarket {

namespace {

TString GetPositionValue(const NSc::TValue& position)
{
    const auto type = position["type"].GetString();
    if (EqualToOneOf(type, TStringBuf("ENUM"), TStringBuf("BOOLEAN"))) {
        return position["value_id"].ForceString();
    }
    if (type == TStringBuf("NUMERIC")) {
        auto value = position["number_value"].GetNumber();
        return TStringBuilder() << value << "~" << value;
    }
    LOG(ERR) << "Got unexpected position type " << type
             << " (param_id=" << position["param_id"].GetString() << ")" << Endl;
    Y_ASSERT(false);
    return TString();
}

} // namespace anonymous

TFormalizerResponse::TFormalizerResponse(const NHttpFetcher::TResponse::TRef response)
    : TBaseJsonResponse(response)
{
}

THashMap<TString, TVector<TString>> TFormalizerResponse::GetFilters() const
{
    Y_ASSERT(Data.Defined());
    const auto& data = Data.GetRef();

    THashSet<ui64> consequentParamIds;
    for (const auto& param : data["consequent_param_value"].GetArray()) {
        consequentParamIds.insert(param["main_param_id"].GetIntNumber());
    }

    const auto& positions = data["position"].GetArray();
    THashMap<TString, TVector<TString>> filters;
    for (const auto& position : positions) {
        if (!consequentParamIds.contains(position["param_id"].GetIntNumber())) {
            filters[position["param_id"].ForceString()].push_back(GetPositionValue(position));
        }
    }
    return filters;
}

} // namespace NMarket

} // namespace NBASS

#include "client.h"
#include "context.h"
#include "filter_worker.h"

#include <util/charset/utf8.h>

#include <cmath>

namespace NBASS {

namespace NMarket {

namespace {

const size_t MAX_FILTER_EXAMPLES_AMOUNT = 5;
const size_t MAX_BOOL_FILTER_EXAMPLES_AMOUNT = 3;

TMaybe<TGlFilter> CreateGlFilter(
    TStringBuf id,
    const NBassApi::TFormalizedGlFilterConst<TBoolSchemeTraits>& rawFilter)
{
    if (rawFilter.Type() == TStringBuf("boolean")) {
        if (rawFilter.Values().Size() != 1) {
            LOG(ERR) << "Got bool filter with " << rawFilter.Values().Size()
                     << " (expected 1). Filter id - " << id << Endl;
            Y_ASSERT(false);
            return Nothing();
        }
        return TBoolGlFilter(id, rawFilter.Values()[0].Id());
    } else if (rawFilter.Type() == TStringBuf("enum")) {
        if (rawFilter.Values().Empty()) {
            LOG(ERR) << "Got enum filter without values. Filter id - " << id << Endl;
            Y_ASSERT(false);
            return Nothing();
        }
        TEnumGlFilter::TValues values;
        for (const auto& rawValue : rawFilter.Values()) {
            values.Add(TEnumGlFilter::TValue(ToString(rawValue.Id())));
        }
        return TEnumGlFilter(id, values);
    } else if (rawFilter.Type() == TStringBuf("number")) {
        if (rawFilter.Values().Size() == 1) {
            TMaybe<double> value = rawFilter.Values()[0].Num();
            return TNumberGlFilter(id, value, value);
        } else if (rawFilter.Values().Size() == 2) {
            TMaybe<double> valueMin = rawFilter.Values()[0].Num();
            TMaybe<double> valueMax = rawFilter.Values()[1].Num();
            if (valueMin && valueMax && (*valueMin > *valueMax)) {
                Swap(valueMin, valueMax);
            }
            return TNumberGlFilter(id, valueMin, valueMax);

        } else {
            LOG(ERR) << "Got number filter with " << rawFilter.Values().Size()
                     << " (expected 1 or 2). Filter id - " << id << Endl;
            Y_ASSERT(false);
            return Nothing();
        }

    }
    LOG(ERR) << "Got unexpected filter type \"" << rawFilter.Type() << "\". FilterId = " << id << Endl;
    Y_ASSERT(false);
    return Nothing();
}

THashMap<TStringBuf, const NSc::TValue*> GetMapById(const NSc::TArray& arr)
{
    THashMap<TStringBuf, const NSc::TValue*> map;
    for (const auto& elem : arr) {
        map[elem["id"].GetString()] = &elem;
    }
    return map;
}

class TFilterUpdater {
public:
    explicit TFilterUpdater(const NSc::TValue& filterData)
        : FilterData(filterData)
    {
    }

    TGlFilter operator()(TBoolGlFilter& filter) const
    {
        if (!CheckType(TStringBuf("boolean"), filter.Id)) {
            return filter;
        }
        filter.Name = FilterData["name"].ForceString();
        return filter;
    }
    TGlFilter operator()(TNumberGlFilter& filter) const
    {
        if (!CheckType(TStringBuf("number"), filter.Id)) {
            return filter;
        }
        filter.Name = FilterData["name"].ForceString();
        filter.Unit = FilterData["unit"].ForceString();
        return filter;
    }
    TGlFilter operator()(TEnumGlFilter& filter) const
    {
        if (!CheckType(TStringBuf("enum"), filter.Id)) {
            return filter;
        }
        const auto& valuesById = GetMapById(FilterData["values"].GetArray());
        filter.Name = FilterData["name"].ForceString();
        for (auto& [id, value] : filter.Values) {
            if (valuesById.contains(id)) {
                const auto& fullValue = *(valuesById.at(id));
                value.Name = fullValue["value"].ForceString();
            } else {
                LOG(ERR) << "Got unknown value id \"" << id << "\" for filter \"" << filter.Id << "\"" << Endl;
                Y_ASSERT(false);
            }
        }
        return filter;
    }
    TGlFilter operator()(TRawGlFilter& filter) const
    {
        TStringBuf type = FilterData["type"].GetString();
        if (type == "boolean") {
            return GetBoolRawFilter(filter);
        } else if (type == "number") {
            return GetNumberRawFilter(filter);
        } else if (type == "enum") {
            return GetEnumRawFilter(filter);
        } else {
            LOG(ERR) << "Got unexpected gl filter type - \"" << type
                     << "\" for filter \"" << filter.Id << "\"" << Endl;
            Y_ASSERT(false);
            return filter;
        }
    }

private:
    const NSc::TValue& FilterData;

    bool CheckType(TStringBuf expectedType, TStringBuf filterId) const
    {
        if (FilterData["type"].GetString() != expectedType) {
            LOG(ERR) << "Failed to update filter \"" << filterId << "\". Got \"" << FilterData["type"].GetString()
                     << "\" (expected \"" << expectedType << "\")" << Endl;
            Y_ASSERT(false);
            return false;
        }
        return true;
    }
    TGlFilter GetBoolRawFilter(const TRawGlFilter& filter) const
    {
        if (filter.Values.size() != 1) {
            LOG(ERR) << "Got bool filter \"" << filter.Id << "\" with "
                     << filter.Values.size() << " values (expected 1)" << Endl;
            Y_ASSERT(false);
            return filter;
        }

        TStringBuf valueStr = filter.Values[0];
        bool value;
        if (valueStr == TStringBuf("select")) {
            value = true;
        } else if (valueStr == TStringBuf("exclude")) {
            value = false;
        } else if (!TryFromString<bool>(valueStr, value)) {
            LOG(ERR) << "Got invalid bool filter: id - \"" << filter.Id
                     << "\" , value - \"" << valueStr << "\"" << Endl;
            Y_ASSERT(false);
            return filter;
        }
        return TBoolGlFilter(filter.Id, FilterData["name"].GetString(), value);
    }
    TGlFilter GetNumberRawFilter(TRawGlFilter& filter) const
    {
        TStringBuf fromStr, toStr;
        if (filter.Values.size() == 1) {
            TStringBuf valueStr = filter.Values[0];
            if (!valueStr.TrySplit('~', fromStr, toStr)) {
                fromStr = toStr = valueStr;
            }
        } else if (filter.Values.size() == 2) {
            fromStr = filter.Values[0];
            toStr = filter.Values[1];
        } else {
            LOG(ERR) << "Got number filter \"" << filter.Id << "\" with "
                     << filter.Values.size() << " values (expected 1)" << Endl;
            Y_ASSERT(false);
            return filter;
        }

        TMaybe<double> from, to;
        if (fromStr) {
            if (!TryCastToMaybeDouble(fromStr, filter, from)) {
                return filter;
            }
        }
        if (toStr) {
            if (!TryCastToMaybeDouble(toStr, filter, to)) {
                return filter;
            }
        }

        return TNumberGlFilter(
            filter.Id,
            FilterData["name"].GetString(),
            from,
            to,
            FilterData["unit"].GetString()
        );
    }
    TGlFilter GetEnumRawFilter(const TRawGlFilter& filter) const
    {
        const auto& valuesById = GetMapById(FilterData["values"].GetArray());
        TEnumGlFilter::TValues glValues;
        for (const auto& valueId: filter.Values) {
            TEnumGlFilter::TValue value(valueId);
            if (valuesById.contains(valueId)) {
                const auto& fullValue = *(valuesById.at(valueId));
                value.Name = fullValue["value"].ForceString();
            } else {
                LOG(ERR) << "Got unknown value id \"" << valueId << "\" for filter \"" << filter.Id << "\"" << Endl;
                Y_ASSERT(false);
            }
            glValues.Add(value);
        }
        return TEnumGlFilter(
            filter.Id,
            FilterData["name"].GetString(),
            glValues
        );
    }
    static bool TryCastToMaybeDouble(TStringBuf valueStr, const TRawGlFilter& filter, TMaybe<double>& valueMaybe)
    {
        double value;
        if (!TryFromString<double>(valueStr, value)) {
            LOG(ERR) << "Got invalid number filter: id - \"" << filter.Id
                     << "\" , value - \"" << valueStr << "\"" << Endl;
            Y_ASSERT(false);
            return false;
        }
        valueMaybe = value;
        return true;
    }
};

} // namespace

TFilterWorker::TFilterWorker(TMarketContext& ctx)
    : Ctx(ctx)
    , NumberFilterWorker(ctx)
{
}

void TFilterWorker::AddFormalizedFilters(const TFormalizedGlFilters& filters)
{
    for (const auto kv : filters->Filters()) {
        TString id = ToString(kv.Key());
        const auto& filter = CreateGlFilter(id, kv.Value());
        if (!filter.Defined()) {
            continue;
        }
        Ctx.AddGlFilter(filter.GetRef());
        LOG(DEBUG) << "Filter " << kv.Key() << " recognized by formalizer" << Endl;
    }
}

void TFilterWorker::UpdateFiltersDescription(const NSc::TArray& filters)
{
    auto glFilters = Ctx.GetGlFilters();
    for (const auto& fullFilter : filters) {
        auto id = fullFilter["id"].GetString();
        if (glFilters.contains(id)) {
            TFilterUpdater visitor(fullFilter);
            auto result = std::visit(visitor, glFilters[id]);
            Ctx.AddGlFilter(result);
        }
    }
    LOG(DEBUG) << "Filters description updated" << Endl;
}

TVector<const NSc::TValue*> GetFilterValues(const NSc::TValue& filter)
{
    TVector<const NSc::TValue*> vals;
    for (const auto& val : filter["values"].GetArray()) {
        if (val["found"].GetIntNumber()) {
            vals.push_back(&val);
        }
    }
    return vals;
}

bool IsFilterWithSuggests(const NSc::TValue& filter)
{
    return EqualToOneOf(filter["type"], TStringBuf("enum"), TStringBuf("boolean"));
}

bool DoesFilterExampleFit(const NSc::TValue& filter, const NSc::TDict& glFilters)
{
    i64 kind = filter["kind"].GetIntNumber();
    NSc::TValue filterId = filter["id"];

    return !glFilters.contains(filterId) &&
           (kind == 1 || filter["subType"] == TStringBuf("color")) &&
           filter["xslname"] != TStringBuf("vendor_line");
}

void TFilterWorker::SetFilterExamples(const NSc::TArray& filters)
{
    auto glFiltersData = Ctx.GetRawGlFilters();
    const NSc::TDict& glFilters = glFiltersData.GetDict();

    TVector<const NSc::TValue*> suitableFilters;
    for (const auto& filter : filters) {
        if (DoesFilterExampleFit(filter, glFilters)) {
            suitableFilters.push_back(&filter);
        }
    }

    TVector<NSc::TValue> filterExamples;
    auto booleanBlockExample = GetBooleanBlockFilterExample(suitableFilters);
    size_t maxNonBoolExamplesAmount = booleanBlockExample.IsNull()
            ? MAX_FILTER_EXAMPLES_AMOUNT
            : MAX_FILTER_EXAMPLES_AMOUNT - 1;
    for (const auto filter : suitableFilters) {
        if ((*filter)["type"] != "boolean" && TrySetFilterExample(*filter, filterExamples)) {
            if (filterExamples.size() >= maxNonBoolExamplesAmount) {
                break;
            }
        }
    }
    if (!booleanBlockExample.IsNull()) {
        filterExamples.push_back(booleanBlockExample);
    }
    if (!filterExamples.empty()) {
        Ctx.SetFilterExamples(filterExamples);
    }
}

bool TFilterWorker::TrySetFilterExample(const NSc::TValue& filter, TVector<NSc::TValue>& filterExamples)
{
    if (IsFilterWithSuggests(filter)) {
        TVector<const NSc::TValue*> vals = GetFilterValues(filter);

        if (vals.size() > 1) {
            auto getFoundAmount = [](const NSc::TValue* filterValue) {
                return (*filterValue)["found"].GetIntNumber();
            };
            auto mostPopularValue = *MaxElementBy(vals, getFoundAmount);

            NSc::TValue value;
            value["name"] = filter["name"];
            value["type"] = filter["type"];
            value["value"] = *mostPopularValue;
            filterExamples.push_back(value);
            return true;
        }
    } else if (filter["type"] == TStringBuf("number")) {
        NSc::TValue value;
        value["value"] = filter["values"][0];
        double minVal = value["value"]["min"].ForceNumber();
        double maxVal = value["value"]["max"].ForceNumber();
        if (minVal == maxVal) {
            return false;
        }
        value["name"] = filter["name"];
        value["type"] = filter["type"];
        value["unit"] = filter["unit"];
        value["value"]["value"] = std::round((minVal + maxVal) / 2);

        filterExamples.push_back(value);
        return true;
    }
    return false;
}

NSc::TValue TFilterWorker::GetBooleanBlockFilterExample(const TVector<const NSc::TValue*>& filters) const
{
    NSc::TValue exampleValues;
    exampleValues.SetArray();
    for (const auto filter : filters) {
        if ((*filter)["type"] == TStringBuf("boolean") && GetFilterValues(*filter).size() > 1) {
            NSc::TValue exampleValue;
            exampleValue["name"] = (*filter)["name"];
            exampleValues.Push(exampleValue);
            if (exampleValues.ArraySize() >= MAX_BOOL_FILTER_EXAMPLES_AMOUNT) {
                break;
            }
        }
    }

    if (exampleValues.ArrayEmpty()) {
        return NSc::TValue();
    }

    NSc::TValue example;
    example["type"] = TStringBuf("boolean_block");
    example["values"] = exampleValues;
    return example;
}

THashMap<TString, TString> TFilterWorker::ResolveFiltersNameAndValue(const NSc::TArray& glFilters)
{
    THashMap<TString, TString> result;
    for (const auto& filter: glFilters) {
        for (const auto& value: filter["values"].GetArray()) {
            if (value["checked"].GetBool()) {
                TStringBuilder resultStr;
                if (filter["type"].GetString() != TStringBuf("boolean")) {
                    resultStr << ToLowerUTF8(filter["name"].GetString())
                              << " \"" << value["value"].GetString() << '"';
                } else {
                    resultStr << '"' << ToLowerUTF8(filter["name"].GetString()) << '"';
                }
                result.emplace(filter["id"].GetString(), resultStr);
                LOG(DEBUG) << filter["id"].GetString() << " " << resultStr << Endl;
            }
        }
    }
    return result;
}

} // namespace NMarket

} // namespace NBASS

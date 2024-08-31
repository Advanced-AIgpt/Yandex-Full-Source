#include "filter.h"

#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NBASS {

namespace NMarket {

namespace {

class TGlFilterPrintVisitor {
public:
    TString operator()(const TBoolGlFilter &filter) const
    {
        return TStringBuilder() << "Boolean Id " << filter.Id
                                << ", Name \"" << filter.Name
                                << "\", Value " << filter.Value;
    }

    TString operator()(const TNumberGlFilter &filter) const
    {
        return TStringBuilder() << "Number Id " << filter.Id
                                << ", Name \"" << filter.Name
                                << "\", range "
                                << (filter.Min.Defined() ? ToString(filter.Min.GetRef()) : "<undefined>")
                                << " - " << (filter.Max.Defined() ? ToString(filter.Max.GetRef()) : "<undefined>")
                                << ", Unit \"" << filter.Unit << "\"";
    }

    TString operator()(const TEnumGlFilter &filter) const
    {
        TStringBuilder str;
        str << "Enum Id " << filter.Id << ", Name \"" << filter.Name << "\", Values: [ ";
        for (const auto&[id, value] : filter.Values) {
            str << "(Id " << id << ", Name \"" << value.Name << "\") ";
        }
        str << "]";
        return str;
    }

    TString operator()(const TRawGlFilter &filter) const
    {
        TStringBuilder str;
        str << "Raw Id " << filter.Id << ", Values: [ ";
        for (TStringBuf value : filter.Values) {
            str << value << " ";
        }
        str << "]";
        return str;
    }
};

} // namespace

TString ToDebugString(const TGlFilter& filter)
{
    TGlFilterPrintVisitor visitor;
    return std::visit(visitor, filter);
};

} // namespace NMarket

} // namespace NBASS

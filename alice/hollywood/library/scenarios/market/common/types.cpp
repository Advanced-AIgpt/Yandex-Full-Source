#include "types.h"

#include <util/string/builder.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NMarket {

TCategory::TCategory(const NProto::TCategory& proto)
{
    if (const auto nid = proto.GetNid()) {
        Nid = nid;
    }
    if (const auto hid = proto.GetHid()) {
        Hid = hid;
    }
    Slug = proto.GetSlug();
}

NProto::TCategory TCategory::ToProto() const
{
    NProto::TCategory proto;
    if (Hid) {
        proto.SetHid(Hid.GetRef());
    }
    if (Nid) {
        proto.SetNid(Nid.GetRef());
    }
    proto.SetSlug(Slug);

    return proto;
}

TCgiGlFilters::TCgiGlFilters(const NProto::TCgiGlFilters& proto)
{
    Reserve(proto.GlFiltersSize());
    for (const auto& glFilterProto : proto.GetGlFilters()) {
        emplace(
            std::piecewise_construct,
            std::forward_as_tuple(glFilterProto.GetId()),
            std::forward_as_tuple(
                glFilterProto.GetValues().begin(),
                glFilterProto.GetValues().end()
            )
        );
    }
}

void TCgiGlFilters::AddToCgi(TCgiParameters& cgi) const
{
    for (const auto& [id, values] : *this) {
        if (!values.empty()) {
            cgi.InsertUnescaped(TStringBuf("glfilter"), Join(":", id, JoinSeq(",", values)));
        }
    }
}

NProto::TCgiGlFilters TCgiGlFilters::ToProto() const
{
    NProto::TCgiGlFilters proto;
    for (const auto& [id, values] : *this) {
        auto* glFilter = proto.AddGlFilters();
        glFilter->SetId(id);
        for (const auto& value : values) {
            glFilter->AddValues(value);
        }
    }

    return proto;
}

TCgiGlFilters TCgiGlFilters::FromReportFilters(const NJson::TJsonValue::TArray& reportFilters)
{
    TCgiGlFilters filters;
    for (const auto& jsonedFilter : reportFilters) {
        if (jsonedFilter["kind"] == 2) {
            TVector<TString> checkedValues;
            const bool isNumber = jsonedFilter["type"].GetString() == TStringBuf("number");
            for (const auto& jsonedValue : jsonedFilter["values"].GetArray()) {
                if (jsonedValue["checked"].GetBoolean()) {
                    if (isNumber) {
                        TStringBuf min = jsonedValue["min"].GetString();
                        TStringBuf max = jsonedValue["max"].GetString();
                        checkedValues.push_back(ToString(
                            min == max
                                ? min
                                : TStringBuilder() << min << '~' << max));
                    } else {
                        checkedValues.push_back(jsonedValue["id"].GetString());
                    }
                }
            }
            if (!checkedValues.empty()) {
                filters[jsonedFilter["id"].GetString()] = checkedValues;
            }
        }
    }
    return filters;
}

void TCgiRedirectParameters::AddToCgi(TCgiParameters& cgi) const
{
    auto it = begin();
    while (it != end()) {
        cgi.erase(it->first);
        const auto next = upper_bound(it->first);
        cgi.insert(it, next);
        it = next;
    }
}

} // namespace NAlice::NHollywood::NMarket

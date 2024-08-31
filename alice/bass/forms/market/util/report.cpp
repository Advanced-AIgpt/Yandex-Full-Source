#include "report.h"

#include <alice/bass/libs/logging_v2/logger.h>


namespace NBASS {

namespace NMarket{

NSc::TValue GetFiltersForDetails(const NSc::TArray& reportFilters)
{
    NSc::TValue filters;
    filters.SetArray();
    for (const auto& reportFilter : reportFilters) {
        if (reportFilter["type"].GetString() == TStringBuf("enum")) {
            NSc::TValue values;
            values.SetArray();
            for (const auto& reportValue : reportFilter["values"].GetArray()) {
                if (reportValue["found"].GetIntNumber()) {
                    NSc::TValue value;
                    value["value"] = reportValue["value"];
                    values.Push(value);
                }
            }
            if (!values.ArrayEmpty()) {
                NSc::TValue filter;
                filter["name"] = reportFilter["name"];
                filter["values"] = values;
                filters.Push(filter);
            }
        } else if (reportFilter["type"].GetString() == TStringBuf("number")) {
            NSc::TValue values;
            values.SetArray();
            for (const auto& reportValue : reportFilter["values"].GetArray()) {
                NSc::TValue value;
                if (reportValue["min"].GetIntNumber() == reportValue["max"].GetIntNumber()) {
                    value["value"] = reportValue["min"];
                } else {
                    value["min"] = reportValue["min"];
                    value["max"] = reportValue["max"];
                }
                values.Push(value);
            }
            if (!values.ArrayEmpty()) {
                NSc::TValue filter;
                filter["name"] = reportFilter["name"];
                filter["values"] = values;
                filter["unit"] = reportFilter["unit"];
                filters.Push(filter);
            }
        } else if (reportFilter["type"].GetString() == TStringBuf("boolean")) {
            for (const auto& reportValue : reportFilter["values"].GetArray()) {
                if (reportValue["id"] == TStringBuf("1")) {
                    if (reportValue["found"].GetIntNumber()) {
                        NSc::TValue filter;
                        filter["name"] = reportFilter["name"];
                        filters.Push(filter);
                    }
                    break;
                }
            }
        }
    }
    return filters;
}

void SetRedirectModeCgiParam(bool allowRedirects, TCgiParameters& cgi)
{
    if (allowRedirects) {
        cgi.InsertUnescaped(TStringBuf("non-dummy-redirects"), TStringBuf("1"));
        cgi.InsertUnescaped(TStringBuf("cvredirect"), TStringBuf("1"));
    } else {
        cgi.InsertUnescaped(TStringBuf("cvredirect"), TStringBuf("0"));
    }
}

void SetRedirectCgiParams(const TRedirectCgiParams& redirectParams, TCgiParameters& cgi)
{
    if (!redirectParams.ReportState.empty()) {
        cgi.InsertUnescaped(TStringBuf("rs"), redirectParams.ReportState);
    }
    if (!redirectParams.WasRedir.empty()) {
        cgi.InsertUnescaped(TStringBuf("was_redir"), redirectParams.WasRedir);
    }
}

void SetPriceCgiParams(const NSc::TValue& price, TCgiParameters& cgi)
{
    if (!price["from"].IsNull()) {
        cgi.InsertUnescaped(TStringBuf("mcpricefrom"), price["from"].ForceString());
    }
    if (!price["to"].IsNull()) {
        cgi.InsertUnescaped(TStringBuf("mcpriceto"), price["to"].ForceString());
    }
}

void AddGlFilters(const TCgiGlFilters& glFilters, TCgiParameters& cgi)
{
    for (const auto& kv : glFilters) {
        if (!kv.second.empty()) {
            TStringBuilder glFilter;
            glFilter << kv.first << ":" << kv.second[0];
            for (size_t i = 1; i < kv.second.size(); i++) {
                glFilter << ',' << kv.second[i];
            }
            cgi.InsertUnescaped(TStringBuf("glfilter"), glFilter);
        }
    }
}

TString FormatPof(ui32 clid)
{
    return TStringBuilder() << "{\"clid\":[\"" << clid << "\"],\"mclid\":null,\"vid\":null,\"distr_type\":null,\"opp\":null}";
}

} // namespace NMarket

} // namespace NBASS

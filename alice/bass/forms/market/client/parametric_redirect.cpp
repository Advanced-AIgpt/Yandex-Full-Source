#include <alice/bass/forms/market/client.h>
#include <alice/bass/forms/market/util/string.h>

namespace NBASS {

namespace NMarket {

TReportResponse::TParametricRedirect::TParametricRedirect(NSc::TValue data)
    : TBaseRedirect(data)
{
    const auto& params = Data["redirect"]["params"];

    Category = TCategory(
        FromStringWithLogging<ui64>(params["hid"][0].GetString(), 0),
        FromStringWithLogging<ui64>(params["nid"][0].GetString(), 0),
        params["slug"][0].GetString(),
        GetCategoryName());

    const auto& reportState = params["rs"];
    if (!reportState.IsNull()) {
        ReportState = reportState.GetArray()[0].GetString();
    }

    const auto& fesh = params["fesh"];
    if (!fesh.IsNull()) {
        for (auto& shopId: fesh.GetArray()) {
            Fesh.push_back(FromString<i64>(shopId.GetString()));
        }
    }
}

const TCategory& TReportResponse::TParametricRedirect::GetCategory() const
{
    return Category;
}

const TString& TReportResponse::TParametricRedirect::GetReportState() const
{
    return ReportState;
}

const TVector<i64>& TReportResponse::TParametricRedirect::GetFesh() const {
    return Fesh;
}

TStringBuf TReportResponse::TParametricRedirect::GetCategoryName() const
{
    return Data["redirect"]["category_name"].GetString();
}

} // namespace NMarket

} // namespace NBASS

#include <alice/bass/forms/market/client.h>
#include <alice/bass/forms/market/util/string.h>

#include <util/string/split.h>

namespace NBASS {

namespace NMarket {

TReportResponse::TRedirect::TRedirect(NSc::TValue data)
    : TBaseRedirect(data)
{
    const auto& params = Data["redirect"]["params"];

    if (params.Has("hid") && params.Has("nid")) {
        Category = TCategory(
            FromStringWithLogging<ui64>(params["hid"][0].GetString(), 0),
            FromStringWithLogging<ui64>(params["nid"][0].GetString(), 0),
            params["slug"][0].GetString(),
            GetCategoryName());
    }

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

    if (params.Has("lr")) {
        UserRegion = FromStringWithLogging<i64>(params["lr"][0].GetString(), 0);
    }
}

TStringBuf TReportResponse::TRedirect::GetCategoryName() const
{
    return Data["redirect"]["category_name"].GetString();
}

bool TReportResponse::TRedirect::HasCategory() const {
    return Category.Defined();
}

const TCategory& TReportResponse::TRedirect::GetCategory() const
{
    return *Category;
}

const TString& TReportResponse::TRedirect::GetReportState() const
{
    return ReportState;
}

const TVector<i64>& TReportResponse::TRedirect::GetFesh() const {
    return Fesh;
}

bool TReportResponse::TRedirect::HasUserRegion() const {
    return UserRegion.Defined();
}

i64 TReportResponse::TRedirect::GetUserRegion() const
{
    return *UserRegion;
}

void TReportResponse::TRedirect::FillCtx(TMarketContext& ctx) const
{
    const auto& params = Data["redirect"]["params"];
    if (params.Has("suggest_text")) {
        ctx.SetSuggestTextRedirect(SuggestText);
    }
    ctx.SetTextRedirect(Text);
    if (!UserRegion.Empty()) {
        ctx.SetUserRegion(*UserRegion);
    }
    if (!Fesh.empty()) {
        ctx.SetFesh(Fesh);
    }
    if (!Category.Empty()) {
       ctx.SetCategory(*Category);
       LOG(DEBUG) << "Parametric redirect. hid: " << Category->GetHid() << " nid: " << Category->GetNid() << Endl;
    }
    for (const auto &kv : GlFilters) {
        ctx.AddGlFilter(kv.first, kv.second);
    }

    auto redirectParams = ctx.GetRedirectCgiParams();
    if (params.Has("was_redir")) {
        redirectParams.WasRedir = CgiParams.WasRedir;
    }
    if (params.Has("rs")) {
        redirectParams.ReportState = CgiParams.ReportState;
    }
    ctx.SetRedirectCgiParams(redirectParams);
}

} // namespace NMarket

} // namespace NBASS

#include "request.h"

#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NAlice::NHollywood::NMarket {

namespace {

void SetRedirectModeCgiParam(bool allowRedirects, TCgiParameters& cgi)
{
    if (allowRedirects) {
        cgi.InsertUnescaped(TStringBuf("non-dummy-redirects"), TStringBuf("1"));
        cgi.InsertUnescaped(TStringBuf("cvredirect"), TStringBuf("1"));
    } else {
        cgi.InsertUnescaped(TStringBuf("cvredirect"), TStringBuf("0"));
    }
}

TCgiParameters CreateReportCgiParams()
{
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("regset"), TStringBuf("2"));
    cgi.InsertUnescaped(TStringBuf("alice"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("onstock"), TStringBuf("1"));
    cgi.InsertUnescaped(TStringBuf("local-offers-first"), TStringBuf("0"));
    cgi.InsertUnescaped(TStringBuf("adult"), TStringBuf("0"));
    cgi.InsertUnescaped(TStringBuf("show-urls"), TStringBuf("external,encryptedmodel"));
    cgi.InsertUnescaped(TStringBuf("allow-collapsing"), TStringBuf("1"));
    return cgi;
}

const unsigned WATCHES = 91259;
const unsigned SEAT_COVERINGS = 90465;
const unsigned FAUCETS = 91610;
const unsigned DESKTOPS = 91011;
const unsigned WATCHES_AND_ACCESSORIES = 15064473;
const unsigned MATRESSES = 1003092;
const unsigned MOBILE_CASES = 91498;

/*
 * Запросы к этим категориям падают по таймауту
 * для них приходится добавлять prun-count, чтобы уменьший время ответа репорта
 */
bool IsBigCategory(const THid hid)
{
    return hid == MOBILE_CASES;
}

bool IsVeryBigCategory(const THid hid)
{
    return EqualToOneOf(
        hid,
        WATCHES,
        SEAT_COVERINGS,
        FAUCETS,
        DESKTOPS,
        WATCHES_AND_ACCESSORIES,
        MATRESSES);
}

TStringBuf GetTextlessPrunCount(const THid hid)
{
    if (IsVeryBigCategory(hid)) {
        return TStringBuf("1000");
    } else if (IsBigCategory(hid)) {
        return TStringBuf("5000");
    }
    return TStringBuf("20000");
}

} // namespace

TReportPrimeRequest::TReportPrimeRequest(TPpParam pp)
{
    Cgi = CreateReportCgiParams();
    Cgi.InsertUnescaped(TStringBuf("pp"), ToString(pp));
    Cgi.InsertUnescaped(TStringBuf("use-default-offers"), TStringBuf("1"));
    Cgi.InsertUnescaped(TStringBuf("place"), TStringBuf("prime"));
    Cgi.InsertUnescaped(TStringBuf("numdoc"), TStringBuf("12"));
}

TString FormatPof(EClid clid)
{
    const auto clidValue = static_cast<TClidValue>(clid);
    return TStringBuilder() << "{\"clid\":[\""<< clidValue << "\"],\"mclid\":null,\"vid\":null,\"distr_type\":null,\"opp\":null}";
}

TString TReportPrimeRequest::Path() const
{
    TCgiParameters cgi = Cgi;
    SetRedirectModeCgiParam(AllowRedirects, cgi);
    if (Text) {
        cgi.InsertUnescaped(TStringBuf("text"), Text);
    }
    if (Category) {
        if (Category->HasNid()) {
            cgi.InsertUnescaped(TStringBuf("nid"), ToString(Category->GetNid()));
        }
        if (Category->HasHid()) {
            cgi.InsertUnescaped(TStringBuf("hid"), ToString(Category->GetHid()));
        }
    }
    SetupPrunning(cgi);
    return TStringBuilder()
        << TStringBuf("/yandsearch?")
        << cgi.Print();
}

THttpHeaders TReportPrimeRequest::Headers() const
{
    return Headers_;
}

void TReportPrimeRequest::SetAllowRedirects(bool allow)
{
    AllowRedirects = allow;
}

void TReportPrimeRequest::SetRegionId(NGeobase::TId regionId)
{
    Cgi.ReplaceUnescaped(TStringBuf("rids"), ToString(regionId));
}

void TReportPrimeRequest::SetRedirectParams(const TCgiRedirectParameters& params)
{
    params.AddToCgi(Cgi);
}

void TReportPrimeRequest::SetCategory(const TCategory& category)
{
    Category = category;
}

void TReportPrimeRequest::SetText(TStringBuf text)
{
    Text = ToString(text);
}

void TReportPrimeRequest::SetGlFilters(const TCgiGlFilters& filters)
{
    Cgi.EraseAll(TStringBuf("glfilter"));
    filters.AddToCgi(Cgi);
}

void TReportPrimeRequest::SetUuid(TStringBuf uuid)
{
    Cgi.ReplaceUnescaped(TStringBuf("uuid"), uuid);
}

void TReportPrimeRequest::SetIp(TStringBuf ip)
{
    Cgi.ReplaceUnescaped(TStringBuf("ip"), ip);
}

void TReportPrimeRequest::SetReqid(TStringBuf reqid)
{
    Cgi.ReplaceUnescaped(TStringBuf("wprid"), reqid);
}

void TReportPrimeRequest::SetClid(EClid clid)
{
    Cgi.ReplaceUnescaped(TStringBuf("pof"), FormatPof(clid));
}

void TReportPrimeRequest::SetMarketReqId(ui64 timestamp, TStringBuf reqid, ui32 num)
{
    Headers_.AddHeader({
        "X-Market-Req-ID",
        TStringBuilder() << timestamp << '/' << reqid << '/' << num,
    });
}

void TReportPrimeRequest::SetupPrunning(TCgiParameters& cgi) const
{
    if (!Category || !Category->HasHid()) {
        return;
    }
    const THid hid = Category->GetHid();
    if (Text) {
        if (IsBigCategory(hid)) {
            cgi.InsertUnescaped(TStringBuf("prun-count"), TStringBuf("10000"));
        }
    } else {
        cgi.InsertUnescaped(TStringBuf("prun-count"), GetTextlessPrunCount(hid));
    }
}

} // namespace NAlice::NHollywood::NMarket

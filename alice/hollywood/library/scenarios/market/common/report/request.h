#pragma once

#include <alice/hollywood/library/scenarios/market/common/types.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/http/io/headers.h>

namespace NAlice::NHollywood::NMarket {

using TPpParam = ui32;

/*
    Класс, позволяющий создавать сырые prime запросы в репорт
*/
class TReportPrimeRequest {
public:
    TReportPrimeRequest(TPpParam pp);

    TString Path() const;
    THttpHeaders Headers() const;

    void SetAllowRedirects(bool allow);
    void SetRegionId(NGeobase::TId regionId);
    void SetRedirectParams(const TCgiRedirectParameters& params);
    void SetCategory(const TCategory& category);
    void SetText(TStringBuf text);
    void SetGlFilters(const TCgiGlFilters& filters);

    void SetUuid(TStringBuf uuid);
    void SetIp(TStringBuf ip);
    void SetReqid(TStringBuf reqid);
    void SetPp(TPpParam pp);
    void SetClid(EClid clid);
    void SetMarketReqId(ui64 timestamp, TStringBuf reqid, ui32 num);

private:
    TString Text = {};
    TMaybe<TCategory> Category = Nothing();
    bool AllowRedirects = false;
    TCgiParameters Cgi;
    THttpHeaders Headers_;

    void SetupPrunning(TCgiParameters& cgi) const;
};

} // namespace NAlice::NHollywood::NMarket

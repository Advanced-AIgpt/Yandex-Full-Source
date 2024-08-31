#pragma once

#include "request.h"
#include <alice/hollywood/library/scenarios/market/common/types.h>
#include <alice/hollywood/library/scenarios/market/common/proto/report.pb.h>

#include <alice/hollywood/library/request/request.h>

namespace NAlice::NHollywood::NMarket {

/*
    Класс, позволяющий создавать валидные запросы в репорт со всей необходимой информацией для
    аналитики
*/
class TReportClient {
public:
    TReportClient(
        TStringBuf uuid,
        TStringBuf ip,
        TStringBuf reqid,
        EClid clid,
        TPpParam pp,
        ui64 timestamp);
    TReportClient(const NProto::TReportClient& proto);

    TReportPrimeRequest CreateRequest(TStringBuf text);
    TReportPrimeRequest CreateRequest(const TCategory& category, TStringBuf text = {});

    NProto::TReportClient CreateProto() const;
private:
    TString Uuid;
    TString Ip;
    TString Reqid;
    EClid Clid;
    TPpParam Pp;
    ui64 Timestamp;
    ui32 SentRequestsCount = 0;

    void SetBaseParams(TReportPrimeRequest& request);
};

TReportClient CreateReportClient(
    EClid clid, TPpParam pp, const TScenarioBaseRequestWrapper& request);

} // namespace NAlice::NHollywood::NMarket

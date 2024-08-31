#include "client.h"

namespace NAlice::NHollywood::NMarket {

TReportClient::TReportClient(
        TStringBuf uuid,
        TStringBuf ip,
        TStringBuf reqid,
        EClid clid,
        TPpParam pp,
        ui64 timestamp)
    : Uuid(ToString(uuid))
    , Ip(ToString(ip))
    , Reqid(ToString(reqid))
    , Clid(clid)
    , Pp(pp)
    , Timestamp(timestamp)
{}

TReportClient::TReportClient(const NProto::TReportClient& proto)
    : TReportClient(
        proto.GetUuid(),
        proto.GetIp(),
        proto.GetReqid(),
        static_cast<EClid>(proto.GetClid()),
        proto.GetPp(),
        proto.GetTimestamp())
{
    SentRequestsCount = proto.GetSentRequestsCount();
}

TReportPrimeRequest TReportClient::CreateRequest(TStringBuf text)
{
    TReportPrimeRequest request { Pp };
    SetBaseParams(request);
    request.SetText(text);
    return request;
}

TReportPrimeRequest TReportClient::CreateRequest(
    const TCategory& category,
    TStringBuf text)
{
    TReportPrimeRequest request { Pp };
    SetBaseParams(request);
    request.SetCategory(category);
    if (text) {
        request.SetText(text);
    }
    return request;
}

NProto::TReportClient TReportClient::CreateProto() const
{
    NProto::TReportClient proto;
    proto.SetUuid(Uuid);
    proto.SetIp(Ip);
    proto.SetReqid(Reqid);
    proto.SetClid(static_cast<TClidValue>(Clid));
    proto.SetPp(Pp);
    proto.SetTimestamp(Timestamp);
    proto.SetSentRequestsCount(SentRequestsCount);
    return proto;
}

void TReportClient::SetBaseParams(TReportPrimeRequest& request)
{
    request.SetUuid(Uuid);
    request.SetIp(Ip);
    request.SetReqid(Reqid);
    request.SetClid(Clid);
    request.SetMarketReqId(Timestamp, Reqid, ++SentRequestsCount);
}

TReportClient CreateReportClient(
    EClid clid, TPpParam pp, const TScenarioBaseRequestWrapper& request)
{
    return {
        request.ClientInfo().Uuid,
        request.BaseRequestProto().GetOptions().GetClientIP(),
        request.RequestId(),
        clid,
        pp,
        request.ServerTimeMs(),
    };
}

} // namespace NAlice::NHollywood::NMarket

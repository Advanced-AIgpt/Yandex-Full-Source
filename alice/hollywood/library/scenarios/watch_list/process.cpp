#include "process.h"

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/protos/data/tv/watch_list/wl_answer.pb.h>

#include <contrib/libs/googleapis-common-protos/google/rpc/status.pb.h>

namespace NAlice::NHollywood::NWatchList {
    namespace NImpl {
        NTv::TTvWatchListSwitchSuccess BuildSuccessResponse() {
            return {};
        }

        NTv::TTvWatchListSwitchFailure BuildFailureResponse(const TString& reason) {
            NTv::TTvWatchListSwitchFailure failureResult;
            failureResult.SetReason(reason);
            return std::move(failureResult);
        }

        TString FailureReason(int code) {
            switch (code) {
                case 401:
                case 403:
                    return "Authorization failed";
                case 500:
                    return "OTT internal error";
                case 503:
                    return "Blackbox unavailable";
                default:
                    return TString::Join("Unexpected error ", ToString(code));
            }
        }

        void PutResponse(NAppHost::IServiceContext& srvCtx, NTv::TTvWatchListSwitchItemResultData& res) {
            google::protobuf::Any responseContainer;
            responseContainer.PackFrom(res);
            srvCtx.AddProtobufItem(responseContainer, "response");
        }

        void PutError(NAppHost::IServiceContext& srvCtx) {
            google::rpc::Status status;
            status.Setcode(grpc::UNAVAILABLE);
            status.Setmessage("No response from OTT");
            srvCtx.AddProtobufItem(status, "error");
        }
    }
    void TTvWatchListProcessHandle::Do(TScenarioHandleContext& ctx) const {
        LOG_INFO(ctx.Ctx.Logger()) << "TTvWatchListProcessHandle::Do" << Endl;

        const auto maybeResponse = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(ctx.ServiceCtx, "http_response");

        if (maybeResponse && maybeResponse->GetStatusCode() == 200) {
            LOG_INFO(ctx.Ctx.Logger()) << "Successful response from OTT" << Endl;
            NTv::TTvWatchListSwitchItemResultData result;
            *result.MutableSuccess() = NImpl::BuildSuccessResponse();
            NImpl::PutResponse(ctx.ServiceCtx, result);
        } else if (maybeResponse) {
            LOG_INFO(ctx.Ctx.Logger()) << "Error response from OTT, code = " << maybeResponse->GetStatusCode() << Endl;
            TString reason = NImpl::FailureReason(maybeResponse->GetStatusCode());
            NTv::TTvWatchListSwitchItemResultData result;
            *result.MutableFailure() = NImpl::BuildFailureResponse(reason);
            NImpl::PutResponse(ctx.ServiceCtx, result);
        } else {
            LOG_INFO(ctx.Ctx.Logger()) << "No response from OTT" << Endl;
            NImpl::PutError(ctx.ServiceCtx);
        }
    }
} //namespace NAlice::NHollywood::NWatchList

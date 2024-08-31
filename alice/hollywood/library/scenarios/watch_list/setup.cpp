#include "setup.h"
#include <alice/protos/data/tv/watch_list/wl_requests.pb.h>
#include <contrib/libs/googleapis-common-protos/google/rpc/status.pb.h>
#include <contrib/libs/googleapis-common-protos/google/rpc/error_details.pb.h>

namespace NAlice::NHollywood::NWatchList {

    namespace NImpl {
        void AddErrorItem(TScenarioHandleContext& ctx) {
            google::rpc::Status status;
            if (ctx.RequestMeta.GetUserTicket().empty()) {
                status.Setcode(grpc::UNAUTHENTICATED);
                status.Setmessage("Failed to setup http request: UserTicket is empty");
                google::protobuf::Any* detailWrapped = status.Mutabledetails()->Add();
                google::rpc::ErrorInfo errorInfo;
                errorInfo.Setreason("Possible reasons are: empty or invalid OAuth token in request; malfunctioning apphost_Voice");
                detailWrapped->PackFrom(errorInfo);
            } else {
                status.Setcode(grpc::INTERNAL);
                status.Setmessage("Failed to setup http request: reason unknown");
            }
            ctx.ServiceCtx.AddProtobufItem(status, "error");
        }
    }

    void TTvWatchListSetupHandle::Do(TScenarioHandleContext& ctx) const {
        LOG_DEBUG(ctx.Ctx.Logger()) << "TTvWatchListSetupHandle::Do" << Endl;

        const auto& uuid = ItemUuid(ctx.ServiceCtx);

        try {
            const auto httpRequest = SetupRequest(ctx, uuid);
            AddHttpRequestItems(ctx, httpRequest, "wl_http_request", "http_request_rtlog_token");
        } catch (const yexception& ye) {
            LOG_ERR(ctx.Ctx.Logger()) << "TTvWatchListSetupHandle::Exception happend during SetupRequest: " << ye.what();
            NImpl::AddErrorItem(ctx);
        }
    }

    NAlice::NHollywood::THttpProxyRequest TTvWatchListSetupHandle::SetupRequest(TScenarioHandleContext& ctx, const TString& uuid) const {
        const auto name = "Ott_Watch_List_Request";
        const auto path = "/v13/films-to-watch/" + uuid;

        THttpProxyRequestBuilder requestBuilder(path, ctx.RequestMeta, ctx.Ctx.Logger(), name);
        requestBuilder
            .SetUseTVMUserTicket()
            .SetMethod(HttpMethod());
        return std::move(requestBuilder.Build());
    }
} // namespace NAlice::NHollywood::NWatchList

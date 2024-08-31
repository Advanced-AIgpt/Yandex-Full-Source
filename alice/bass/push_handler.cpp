#include "push_handler.h"

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/push_notification/request.h>
#include <alice/bass/util/error.h>

#include <util/generic/yexception.h>

#include <filesystem>

namespace NBASS {
namespace {

constexpr TStringBuf CONTENT_TYPE = "application/json; charset=utf-8";

class TResultHandler {
public:
    TResultHandler(const TRequestReplier::TReplyParams& params)
        : Output_{params.Output}
    {
    }

    void operator()(const NPushNotification::TRequests& requests) {
        NSc::TValue response;
        TVector<NHttpFetcher::THandle::TRef> fetchingRequests;
        for (const auto& req : requests) {
            fetchingRequests.push_back(req.Request->Fetch());

            LOG(INFO) <<  "Push service request: " << req.Request->GetBody() << Endl;
            response["pushes"].Push()["type"] = req.Name;
        }

        for (size_t i = 0; i < fetchingRequests.size(); ++i) {
            NHttpFetcher::TResponse::TRef resp = fetchingRequests[i]->Wait();
            LOG(INFO) << "Push service response: " << resp->Data << Endl;
            NSc::TValue& v = response["pushes"].GetArrayMutable()[i];
            if (resp->IsHttpOk()) {
                v["status"] = "OK";
            } else {
                v["status"] = "FAIL";
                v["info"] = resp->GetErrorText();
            }
        }
        Output_ << THttpResponse{HTTP_OK}.SetContentType(CONTENT_TYPE).SetContent(response.ToJson());
    }

    void operator()(const TError& error) {
        THttpResponse response;
        switch (error.Type) {
            case TError::EType::BADREQUEST:
                response = THttpResponse{HTTP_BAD_REQUEST}.SetContentType(CONTENT_TYPE);
                break;
            case TError::EType::INVALIDPARAM:
                response = THttpResponse{HTTP_UNPROCESSABLE_ENTITY}.SetContentType(CONTENT_TYPE);
                break;
            default:
                response = THttpResponse{HTTP_INTERNAL_SERVER_ERROR}.SetContentType(CONTENT_TYPE);
        }

        LOG(ERR) << "Push request: " << error << Endl;
        Output_ << response << error.ToJson();
    }

private:
    THttpOutput& Output_;
};

} // namespace

bool TPushRequestHandler::DoHttpReply(TGlobalContextPtr globalCtx,
                                      const TParsedHttpFull& /* http */,
                                      const TRequestReplier::TReplyParams& params)
{
    static const THashMap<NTvmAuth::TTvmId, TStringBuf> clientIds = {
        { 2009295, "iot" },
        { 2000860, "bass-dev" }
    };

    TResultHandler handler{params};

    try {
        TStringStream logData;

        const THttpInputHeader* headerServiceTicket = nullptr;
        for (const auto& h : params.Input.Headers()) {
            if (AsciiEqualsIgnoreCase(h.Name(), "x-ya-service-ticket")) {
                headerServiceTicket = &h;
            } else {
                logData << Endl << h.ToString();
            }
        }

        const TString body = params.Input.ReadAll();
        const auto js = NSc::TValue::FromJson(body);
        if (!js.IsNull()) {
            logData << Endl << body;
        } else {
            logData << Endl << js.ToJson();
        }

        LOG(INFO) << "push_handler: " << logData.Str() << Endl;

        TMaybe<TString> error;
        if (!headerServiceTicket) {
            error = "no header 'X-Ya-Service-Ticket' found";
        } else {
            const NTvmAuth::TCheckedServiceTicket st = Tvm_.CheckServiceTicket(headerServiceTicket->Value());
            if (!st) {
                error = NTvmAuth::StatusToString(st.GetStatus());
            }

            const TStringBuf* name = clientIds.FindPtr(st.GetSrc());
            if (!name) {
                error = TStringBuilder() << st.GetSrc() << " is not allowed to use this handle (" << st.DebugInfo() << ')';
            }
        }

        if (!error.Defined()) {
            std::visit(handler, NPushNotification::GetRequests(*globalCtx, body));
        } else {
            // TODO (petrk) turn it on when everyone starts using tvm
            // handler(TError{TError::EType::SYSTEM, *error});
            LOG(ERR) <<  "push_tvm_error: " << *error << Endl;
            std::visit(handler, NPushNotification::GetRequests(*globalCtx, body));
        }
    } catch (...) {
        handler(TError{TError::EType::SYSTEM, TStringBuilder{} << "Exception: " << CurrentExceptionMessage()});
    }

    params.Output.Flush();

    return true;
}

// static
void TPushRequestHandler::RegisterHttpHandlers(THttpHandlersMap* handlers) {
    static const TString cacheDir{"tvmcache.2034422"};
    static constexpr NTvmAuth::TTvmId selfTvmId = 2034422;

    std::filesystem::create_directory(cacheDir.data());

    const NTvmAuth::NTvmApi::TClientSettings settings{
        .DiskCacheDir = cacheDir,
        .SelfTvmId = selfTvmId,
        .CheckServiceTickets = true,
    };
    static NTvmAuth::TLoggerPtr log = MakeIntrusive<NTvmAuth::TCerrLogger>(7);
    static auto tvm = std::make_unique<NTvmAuth::TTvmClient>(settings, log);

    auto factory = []() {
        static IHttpRequestHandler::TPtr handler = MakeIntrusive<TPushRequestHandler>(*tvm);
        return handler;
    };
    handlers->Add(TStringBuf("/push"), factory);
}

} // namespace NBASS

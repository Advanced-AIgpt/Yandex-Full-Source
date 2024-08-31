#include "tvm.h"
#include "utils.h"

#include <util/system/env.h>

namespace NAlice::NCuttlefish::NAppHostServices {

namespace {

static TString TVM_TOOL_TOKEN_ENV = "TVM_TOKEN";

} // namespace

NAppHostHttp::THttpRequest TTvmTool::CheckServiceTicket(const TStringBuf serviceTicket) {
    // Magic statics
    static const TString tvmToolToken = GetEnv(TVM_TOOL_TOKEN_ENV);

    NAppHostHttp::THttpRequest req;
    req.SetScheme(NAppHostHttp::THttpRequest::Http);
    req.SetPath("/checksrv");
    req.SetMethod(NAppHostHttp::THttpRequest::Get);
    AddHeader(req, "Authorization", tvmToolToken);
    AddHeader(req, "X-Ya-Service-Ticket", TString(serviceTicket));
    return req;
}

}  // namespace NAlice::NCuttlefish::NAppHostServices

//
// HOLLYWOOD FRAMEWORK
// TSource interfaces
//

#include "setup.h"

#include <alice/hollywood/library/framework/proto/framework_state.pb.h>

#include <alice/hollywood/library/http_proxy/http_proxy.h>

namespace NAlice::NHollywoodFw {

// Attach http request to outgoing
void TSetup::Attach(const THttpProxyRequest& httpRequest, TStringBuf requestKey /*= "" PROXY_REQUEST_KEY_DEFAULT*/) {
    if (requestKey == "") {
        requestKey = NHollywood::PROXY_REQUEST_KEY_DEFAULT;
    }
    // Attach two outgoing requests
    AttachRequest(requestKey, httpRequest.Request);
}

void TSetup::MergeToCtx(NAppHost::IServiceContext& ctx, TProtoHwScene& sceneResults) const {
    for (const auto& it : SetupList_) {
        const google::protobuf::Message* msg = it.Msg.get();
        ctx.AddProtobufItem(*msg, it.OutgoingName);

        TProtoSetupSource req;
        req.SetOutgoingName(it.OutgoingName);
        req.SetIncomingName(it.IncomingName);
        req.SetResponseType(TProtoSetupSource_EResponseType_Undefined);
        sceneResults.MutableNetworkRequests()->Add(std::move(req));
    }
}

} // namespace NAlice::NHollywoodFw

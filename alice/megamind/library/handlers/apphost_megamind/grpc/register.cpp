#include "register.h"

#include "grpc_finalize_handler.h"
#include "grpc_setup_handler.h"

namespace NAlice::NMegamind::NRpc {

void RegisterRpcHandlers(IGlobalCtx& globalCtx, TRegistry& registry) {
    static const TRpcSetupNodeHandler rpcSetupHandler{globalCtx, /* useAppHostStreaming= */ false};
    static const TRpcFinalizeNodeHandler rpcFinalizeHandler{globalCtx, /* useAppHostStreaming= */ false};
    registry.Add("/mm_rpc_setup_handler", [](NAppHost::IServiceContext& ctx) { rpcSetupHandler.RunSync(ctx); });
    registry.Add("/mm_rpc_finalize_handler", [](NAppHost::IServiceContext& ctx) { rpcFinalizeHandler.RunSync(ctx); });
}

} // namespace NAlice::NMegamind

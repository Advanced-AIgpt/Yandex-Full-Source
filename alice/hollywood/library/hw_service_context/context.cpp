#include "context.h"

namespace NAlice::NHollywood {

THwServiceContext::THwServiceContext(IGlobalContext& globalContext,
                                     NAppHost::IServiceContext& apphostContext,
                                     TRTLogger& logger)
    : GlobalContext_{globalContext}
    , Logger_{logger}
    , ApphostContext_{apphostContext}
{
}

void THwServiceContext::AddProtobufItemToApphostContext(const google::protobuf::Message& item,
                                                        const TStringBuf& type) {
    ApphostContext().AddProtobufItem(item, type);
}

void THwServiceContext::AddBalancingHint(TStringBuf source, ui64 hint) {
    ApphostContext().AddBalancingHint(source, hint);
}

NMetrics::ISensors& THwServiceContext::Sensors() {
    return GlobalContext().Sensors();
}

} // namespace NAlice::NHollywood

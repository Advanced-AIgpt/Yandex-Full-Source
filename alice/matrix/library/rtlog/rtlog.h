#pragma once

#include <alice/cuttlefish/library/rtlog/rtlog.h>

namespace NMatrix {

using TRtLogClient = NAlice::NCuttlefish::TRtLogClient;
using TRtLogActivation = NAlice::NCuttlefish::TRTLogActivation;

inline TMaybe<TString> TryGetRtLogTokenFromAppHostContext(NAppHost::IServiceContext& ahContext) {
    return NAlice::NCuttlefish::TryGetRtLogTokenFromAppHostContext(ahContext);
}

inline TString GetRtLogTokenFromTypedAppHostServiceContext(NAppHost::ITypedServiceContext& typedServiceContext) {
    return NAlice::NCuttlefish::GetRtLogTokenFromTypedAppHostServiceContext(typedServiceContext);
}

inline TMaybe<TString> TryGetRtLogTokenFromHttpRequestHeaders(const THttpHeaders& httpHeaders) {
    return NAlice::NCuttlefish::TryGetRtLogTokenFromHttpRequestHeaders(httpHeaders);
}

} // namespace NMatrix

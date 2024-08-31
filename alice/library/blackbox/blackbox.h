#pragma once

#include "status.h"

#include <alice/library/blackbox/proto/blackbox.pb.h>
#include <alice/library/network/request_builder.h>

#include <util/generic/flags.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice {

/// Customize blackbox request data.
enum class EBlackBoxPrepareParam {
    /// Request only user ticket.
    NeedTicket = 1ULL << 0,
    /// Request full user info.
    NeedUserInfo = 1ULL << 1,
    /// OAuth token required
    NeedAuthToken = 1ULL << 2,
};

Y_DECLARE_FLAGS(TBlackBoxPrepareParams, EBlackBoxPrepareParam);
Y_DECLARE_OPERATORS_FOR_FLAGS(TBlackBoxPrepareParams);

inline const TBlackBoxPrepareParams RequestAllBlackBoxParams
    = EBlackBoxPrepareParam::NeedTicket | EBlackBoxPrepareParam::NeedUserInfo | EBlackBoxPrepareParam::NeedAuthToken;

TBlackBoxStatus PrepareBlackBoxRequest(NNetwork::IRequestBuilder& request,
                                       TStringBuf userIp, const TMaybe<TString>& authToken,
                                       TBlackBoxPrepareParams params = RequestAllBlackBoxParams);

struct TBlackBoxApi {
    using TFullUserInfo = TBlackBoxFullUserInfoProto;
    using TUserInfo = TFullUserInfo::TUserInfo;

    TBlackBoxErrorOr<TFullUserInfo> ParseFullInfo(TStringBuf content) const;
    TBlackBoxErrorOr<TString> ParseUid(TStringBuf content) const;
    TBlackBoxErrorOr<TString> ParseTvm2UserTicket(TStringBuf content) const;
};

} // namespace NAlice

#pragma once

#include <alice/library/util/status.h>

namespace NAlice {

enum class EBlackBoxErrorCode {
    BadData /* "bad_data" */,
    Logic /* "logic" */,
    NoAuthToken /* "no_auth_token" */,
    NoRequest /* "no_request" */,
    NoResponse /* "no_response" */,
    NoServiceTicket /* "no_service_ticket" */,
    NoUid /* "no_uid" */,
    NoUserIP /* "no_user_ip" */,
    NoUserInfo /* "no_user_info" */,
    NoUserTicket /* "no_user_ticket" */
};

using TBlackBoxError = TGenericError<EBlackBoxErrorCode>;
using TBlackBoxStatus = TGenericStatus<EBlackBoxErrorCode>;

template <typename T>
using TBlackBoxErrorOr = TGenericErrorOr<EBlackBoxErrorCode, T>;

inline TBlackBoxStatus BlackBoxSuccess() {
    return Nothing();
}

} // namespace NAlice

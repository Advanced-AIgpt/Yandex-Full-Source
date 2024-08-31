#pragma once

#include "request.h"

#include <library/cpp/scheme/fwd.h> //TODO: separate serialization and drop this dependency

namespace NHttpFetcher {

NSc::TValue ResponseToJson(TResponse::TRef response);
TResponse::TRef ResponseFromJson(const NSc::TValue& json);

} // namespace NHttpFetcher

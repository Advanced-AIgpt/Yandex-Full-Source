#pragma once

#include <alice/library/blackbox/blackbox.h>
#include <alice/library/logger/logger.h>

#include <apphost/lib/proto_answers/http.pb.h>

namespace NAlice::NHollywood::NCombinators {

TMaybe<TBlackBoxFullUserInfoProto>
ParseBlackBoxHttpResponse(const NAppHostHttp::THttpResponse& blackBoxHttpResponse,
                          TRTLogger& logger);

} // NAlice::NHollywood::NCombinators

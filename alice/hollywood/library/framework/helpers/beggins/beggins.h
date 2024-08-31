#pragma once

#include <alice/hollywood/library/framework/core/request.h>

#include <search/begemot/rules/alice/response/proto/alice_response.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywoodFw {

//
// Forward declarations
//
class TRequest;

extern TMaybe<NBg::NProto::TAliceResponseResult> GetBegginsResponse(const TRequest& request);

} // namespace NAlice::NHollywoodFw

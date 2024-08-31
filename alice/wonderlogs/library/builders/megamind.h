#pragma once

#include <alice/wonderlogs/protos/megamind_prepared.pb.h>
#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NWonderlogs {

bool PreferableRequestResponse(
    const TMaybe<TUniproxyPrepared>& successfulUniproxyPrepared,
    const TMaybe<TMegamindPrepared::TMegamindRequestResponse>& successfulMegamindRequestResponse,
    const TMegamindPrepared::TMegamindRequestResponse& megamindRequestResponse);

} // namespace NAlice::NWonderlogs

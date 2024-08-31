#pragma once

#include <alice/wonderlogs/protos/uniproxy_prepared.pb.h>
#include <alice/wonderlogs/protos/wonderlogs.pb.h>

namespace NAlice::NWonderlogs {

void ParseUniproxyPrepared(TWonderlog &wonderlog, const TUniproxyPrepared& uniproxyPrepared);

bool ParseSpotterCommonStats(TWonderlog::TSpotter::TCommonStats& commonStats, const TLogSpotter::TSpotterActivationInfo& spotterActivationInfo);

} // namespace NAlice::NWonderlogs

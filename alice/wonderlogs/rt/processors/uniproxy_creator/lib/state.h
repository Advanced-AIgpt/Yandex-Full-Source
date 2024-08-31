#pragma once

#include <alice/wonderlogs/rt/processors/uniproxy_creator/protos/uniproxy_prepared_wrapper.pb.h>

#include <quality/user_sessions/rt/lib/state_managers/proto/state.h>

namespace NAlice::NWonderlogs {

using TUniproxyPreparedState = NUserSessions::NRT::TProtoState<TUniproxyPreparedWrapper>;

} // namespace NAlice::NWonderlogs

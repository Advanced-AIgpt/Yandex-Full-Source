#pragma once

#include <alice/wonderlogs/rt/processors/megamind_creator/protos/megamind_prepared_wrapper.pb.h>

#include <quality/user_sessions/rt/lib/state_managers/proto/state.h>

namespace NAlice::NWonderlogs {

using TMegamindPreparedState = NUserSessions::NRT::TProtoState<TMegamindPreparedWrapper>;

} // namespace NAlice::NWonderlogs

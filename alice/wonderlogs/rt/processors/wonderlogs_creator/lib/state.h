#pragma once

#include <alice/wonderlogs/rt/protos/uuid_message_id.pb.h>

#include <quality/user_sessions/rt/lib/state_managers/proto/state.h>

namespace NAlice::NWonderlogs {

using TUuidMessageIdState = NUserSessions::NRT::TProtoState<TUuidMessageId>;

} // namespace NAlice::NWonderlogs

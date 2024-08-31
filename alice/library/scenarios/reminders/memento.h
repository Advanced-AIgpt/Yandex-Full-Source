#pragma once

#include "common.h"

#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>

// Fwds.
namespace NAlice::NScenarios {
class TMementoData;
} // namespace NAlice::NScenarios

namespace NAlice::NRemindersApi {

TMaybe<TRemindersState> RemindersFromMemento(TStringBuf mementoBase64);
TMaybe<TRemindersState> RemindersFromMemento(const NScenarios::TMementoData& mementoProto);
TMaybe<TRemindersState> RemindersFromMemento(NScenarios::TMementoData&& mementoProto);

class TMementoReminderDirectiveBuilder {
public:
    TMementoReminderDirectiveBuilder() = default;

    NScenarios::TServerDirective BuildSaveServerDirective(const TRemindersState& state);
};

} // namespace NAlice::NRemindersApi

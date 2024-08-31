#pragma once

#include "request.h"

#include <alice/bass/forms/context/context.h>

namespace NBASS::NReminders {

bool MementoRemindersCheckIfEnabled(TContext& ctx);
void MementoRemindersCreateHandler(TRequest& request, TContext& ctx);
void MementoRemindersListHandler(TRequest& request, TContext& ctx);
void MementoRemindersCancelHandler(TRequest& request, TContext& ctx);

} // namespace NBASS::NReminders

#pragma once

#include "request.h"

#include <alice/bass/forms/context/context.h>

namespace NBASS::NReminders {

bool DeviceLocalRemindersCheckIfEnabled(TContext& ctx);
void DeviceLocalRemindersCreateHandler(TRequest& request, TContext& ctx);
void DeviceLocalRemindersListHandler(TRequest& request, TContext& ctx);
void DeviceLocalRemindersCancelHandler(TRequest& request, TContext& ctx);

} // namespace NBASS::NReminders

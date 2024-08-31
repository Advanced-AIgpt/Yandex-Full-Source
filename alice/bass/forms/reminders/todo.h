#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <util/generic/strbuf.h>

namespace NBASS {
namespace NReminders {

/* all the documentation is here:
 * Main Doc: https://wiki.yandex-team.ru/assistant/dialogs/todo/
 * VinsBass: https://wiki.yandex-team.ru/assistant/dialogs/alarm/Vins-Bass-protokol/#postavitnapominanie
 */
inline constexpr TStringBuf TODO_FORM_NAME_CANCEL = "personal_assistant.scenarios.create_todo__cancel";
inline constexpr TStringBuf TODO_FORM_NAME_CREATE = "personal_assistant.scenarios.create_todo";

void TodoCreate(TContext& ctx);
void TodoCancel(TContext& ctx);
void TodoCancelLast(TContext& ctx);
void TodosList(TContext& ctx);
void TodosListStop(TContext& ctx);

bool TodoCleanupForTestUser(TContext& ctx);

} // namespace NReminders
} // namespace NBASS

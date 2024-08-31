#include "dialog_history.h"

#include <util/generic/utility.h>

namespace NAlice {

namespace {

TDeque<TDialogHistory::TDialogTurn> StripDialogHistory(const TDeque<TDialogHistory::TDialogTurn>& dialogHistory, size_t maxDialogHistory) {
    if (dialogHistory.size() <= maxDialogHistory) {
        return dialogHistory;
    }
    const size_t beginPosition = dialogHistory.size() - maxDialogHistory;
    return TDeque<TDialogHistory::TDialogTurn>(dialogHistory.begin() + beginPosition, dialogHistory.end());
}

} // namespace

TDialogHistory::TDialogHistory(const TDeque<TDialogTurn>& dialogTurns)
    : DialogTurns(StripDialogHistory(dialogTurns, MAX_TURNS_COUNT))
{
}

void TDialogHistory::PushDialogTurn(TDialogTurn&& dialogTurn) {
    DialogTurns.emplace_back(std::move(dialogTurn));
    while (DialogTurns.size() > MAX_TURNS_COUNT) {
        DialogTurns.pop_front();
    }
}

} // namespace NAlice

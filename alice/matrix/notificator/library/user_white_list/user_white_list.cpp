#include "user_white_list.h"

namespace NMatrix::NNotificator {

TUserWhiteList::TUserWhiteList(
    const TUserWhiteListSettings& config
)
    : IsEnabled_(config.GetEnabled())
    , Puids_(config.GetPuids().begin(), config.GetPuids().end())
{}

bool TUserWhiteList::IsPuidAllowedToProcess(const TString& puid) const {
    if (Y_LIKELY(!IsEnabled_)) {
        return true;
    }

    return Puids_.contains(puid);
}

} // namespace NMatrix::NNotificator

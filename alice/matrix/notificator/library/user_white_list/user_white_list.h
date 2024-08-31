#pragma once

#include <alice/matrix/notificator/library/config/config.pb.h>

#include <util/generic/hash_set.h>

namespace NMatrix::NNotificator {

class TUserWhiteList : public TNonCopyable {
public:
    explicit TUserWhiteList(
        const TUserWhiteListSettings& config
    );

    bool IsPuidAllowedToProcess(const TString& puid) const;

private:
    const bool IsEnabled_;
    THashSet<TString> Puids_;
};

} // namespace NMatrix::NNotificator

#pragma once

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NCuttlefish {

class TAdminHandleListener : public NAppHost::IAdminHandleListener {
public:
    explicit TAdminHandleListener(ui16 port);

    void OnReopenLog(IOutputStream& response) override;

private:
    const ui16 Port_;
    const TString SvnVersion_;
    const TString HostName_;
};

} // namespace NAlice::NCuttlefish

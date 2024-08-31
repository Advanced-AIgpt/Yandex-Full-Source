#include "admin_handle_listener.h"

#include <alice/cuttlefish/library/logging/event_log.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>

#include <library/cpp/svnversion/svnversion.h>

#include <util/string/builder.h>
#include <util/system/getpid.h>
#include <util/system/hostname.h>

namespace NAlice::NCuttlefish {

TAdminHandleListener::TAdminHandleListener(ui16 port)
    : Port_(port)
    , SvnVersion_(TStringBuilder() << GetArcadiaSourceUrl() << '@' << GetArcadiaLastChange())
    , HostName_(HostName())
{}

void TAdminHandleListener::OnReopenLog(IOutputStream& response) {
    auto& logger = NCuttlefish::GetLogger();

    logger.SpawnFrame()->LogEvent(NEvClass::BeginReopenLog(HostName_, Port_, SvnVersion_, GetPID()));
    logger.ReopenLog();
    TRtLogClient::Instance().Reopen();
    logger.SpawnFrame()->LogEvent(NEvClass::EndReopenLog(HostName_, Port_, SvnVersion_, GetPID()));

    response << "OK, reopened " << logger.Config().Filename << "\n";
}

} // namespace NAlice::NCuttlefish

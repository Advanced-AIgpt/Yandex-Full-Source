#pragma once

#include <alice/matrix/notificator/library/config/config.pb.h>

#include <alice/matrix/library/clients/tvm_client/tvm_client.h>
#include <alice/matrix/library/rtlog/rtlog.h>
#include <alice/matrix/library/ydb/storage.h>

#include <util/generic/noncopyable.h>


namespace NMatrix::NNotificator {

struct TServicesCommonContext : public TMoveOnly {
    using TConfigType = TApplicationSettings;

    const TConfigType& Config;
    TRtLogClient& RtLogClient;
    TTvmClient& TvmClient;
    NYdb::TDriver& YDBDriver;
};

class TServicesCommonContextBuilder : public TNonCopyable {
public:
    using TCommonContextType = TServicesCommonContext;

public:
    TServicesCommonContextBuilder();

    TServicesCommonContextBuilder& WithConfig(const TCommonContextType::TConfigType& config);
    TServicesCommonContextBuilder& WithRtLogClient(TRtLogClient& rtLogClient);
    TServicesCommonContextBuilder& WithTvmClient(TTvmClient& tvmClient);
    TServicesCommonContextBuilder& WithYDBDriver(NYdb::TDriver& ydbDriver);

    TCommonContextType Build() const;

private:
    const TServicesCommonContext::TConfigType* Config_;
    TRtLogClient* RtLogClient_;
    TTvmClient* TvmClient_;
    NYdb::TDriver* YDBDriver_;
};

} // namespace NMatrix::NNotificator

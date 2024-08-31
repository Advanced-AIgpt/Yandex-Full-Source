#include "common_context.h"


namespace NMatrix::NScheduler {

TServicesCommonContextBuilder::TServicesCommonContextBuilder()
    : Config_(nullptr)
    , RtLogClient_(nullptr)
    , YDBDriver_(nullptr)
{}

TServicesCommonContextBuilder& TServicesCommonContextBuilder::WithConfig(const TCommonContextType::TConfigType& config) {
    Config_ = &config;
    return *this;
}

TServicesCommonContextBuilder& TServicesCommonContextBuilder::WithRtLogClient(TRtLogClient& rtLogClient) {
    RtLogClient_ = &rtLogClient;
    return *this;
}

TServicesCommonContextBuilder& TServicesCommonContextBuilder::WithYDBDriver(NYdb::TDriver& ydbDriver) {
    YDBDriver_ = &ydbDriver;
    return *this;
}

TServicesCommonContextBuilder::TCommonContextType TServicesCommonContextBuilder::Build() const {
    if (Config_ == nullptr) {
        ythrow yexception() << "Failed to build common context: config is nullptr";
    }
    if (RtLogClient_ == nullptr) {
        ythrow yexception() << "Failed to build common context: rtlog client is nullptr";
    }
    if (YDBDriver_ == nullptr) {
        ythrow yexception() << "Failed to build common context: YDB driver is nullptr";
    }

    return TServicesCommonContext({
        .Config = *Config_,
        .RtLogClient = *RtLogClient_,
        .YDBDriver = *YDBDriver_,
    });
}

} // namespace NMatrix::NScheduler

#include "requestctx.h"
#include "rtlogtoken.h"
#include "stage_timers.h"

#include <alice/bass/libs/fetcher/util.h>

#include <alice/library/metrics/names.h>
#include <alice/library/metrics/sensors.h>
#include <alice/library/network/common.h>
#include <alice/library/network/headers.h>

#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/util/str.h>

#include <alice/rtlog/client/client.h>

#include <util/string/split.h>
#include <util/string/ascii.h>
#include <util/string/type.h>
#include <util/string/cast.h>

namespace NAlice {

// TRequestCtx::IInitializer --------------------------------------------------
std::unique_ptr<NMegamind::TStageTimers> TRequestCtx::IInitializer::StealStageTimers() {
    auto timers = std::make_unique<NMegamind::TStageTimers>();
    timers->Register(NMegamind::TS_STAGE_START_REQUEST);
    return timers;
}

// TRequestCtx ----------------------------------------------------------------
TRequestCtx::TRequestCtx(IGlobalCtx& globalCtx, IInitializer&& initializer)
    : GlobalCtx_{globalCtx}
    , Logger_{initializer.Logger()}
    , StageTimers_{initializer.StealStageTimers()}
    , Cgi_{std::move(initializer.StealCgi())}
    , Headers_{std::move(initializer.StealHeaders())}
    , Uri_{std::move(initializer.StealUri())}
    , ContentType_{TRequestMeta::Json}
{
    if (const auto* h = Headers_.FindHeader(NNetwork::HEADER_CONTENT_TYPE);
        h && h->Value() == NContentTypes::APPLICATION_PROTOBUF)
    {
        ContentType_ = TRequestMeta::Protobuf;
    }
}

const TConfig& TRequestCtx::Config() const {
    return GlobalCtx_.Config();
}

const NMegamind::TClassificationConfig& TRequestCtx::ClassificationConfig() const {
    return GlobalCtx_.ClassificationConfig();
}

IGlobalCtx& TRequestCtx::GlobalCtx() {
    return GlobalCtx_;
}

const IGlobalCtx& TRequestCtx::GlobalCtx() const {
    return GlobalCtx_;
}

TRTLogger& TRequestCtx::RTLogger() {
    return Logger_;
}

void AddSensorForHttpResponseCode(NMetrics::ISensors& sensors, int code) {
    const NMonitoring::TLabels returnCodeLabel = {
        {"name", NSignal::RESPONSE_HTTP_CODE_LABEL_NAME},
        {"code", ToString(code)},
        {"protocol", "apphost"}
    };
    sensors.IncRate(returnCodeLabel);
}

} // namespace NAlice

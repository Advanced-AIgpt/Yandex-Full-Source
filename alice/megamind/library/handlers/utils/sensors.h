#pragma once

#include <alice/megamind/library/requestctx/common.h>

#include <alice/library/logger/logger.h>
#include <alice/library/metrics/sensors.h>

#include <library/cpp/monlib/metrics/labels.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

class TRequestTimeMetrics {
public:
    TRequestTimeMetrics(NMetrics::ISensors& sensors, TRTLogger& log,
                        TStringBuf nodeName, EUniproxyStage stage, ERequestResult result);
    ~TRequestTimeMetrics();

    void SetStartTime(TInstant startTime);
    void SetSkrInfo(const TString& clientName, const TString& requestPath);

    void UpdateResult(ERequestResult result);

private:
    void UpdateLabels(NMonitoring::TLabels& labels) const;

private:
    NMetrics::ISensors& Sensors_;
    TRTLogger& Log_;
    TString NodeName_;
    TMaybe<TInstant> StartTime_;
    EUniproxyStage Stage_;
    ERequestResult Result_;
    TString ClientName_;
    TString RequestPath_;
};

} // NAlice::NMegamind

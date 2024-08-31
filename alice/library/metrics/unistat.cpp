#include "unistat.h"
#include "util.h"

#include <util/generic/yexception.h>

namespace NAlice::NMetrics {

NUnistat::IHole* TUniStatSensors::GetHistogram(TStringBuf name) {
    auto* histogram = Histograms.FindPtr(name);
    return histogram ? histogram->Get() : nullptr;
}

void TUniStatSensors::WriteMegaCounters(NJsonWriter::TBuf& json) const {
    for (const auto& name2hgram : Histograms) {
        name2hgram.second->PrintValue(json);
    }
}

void TUniStatSensors::RegisterHistogram(const TString& name) {
    if (Histograms.contains(name)) {
        ythrow yexception() << "Unistat histogram with name '" << name << "' already exists";
    }

    Histograms.emplace(name, TUnistat::Instance().DrillHistogramHole(name, "hgram", NUnistat::TPriority(0), TIME_INTERVALS));
}

} // namespace NAlice::NMetrics

#pragma once

#include <library/cpp/unistat/unistat.h>

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NAlice::NMetrics {

class TUniStatSensors {
public:
    virtual ~TUniStatSensors() = default;

    NUnistat::IHole* GetHistogram(TStringBuf name);

    void WriteMegaCounters(NJsonWriter::TBuf& json) const;

protected:
    void RegisterHistogram(const TString& name);

private:
    THashMap<TString, NUnistat::IHolePtr> Histograms;
};

} // namespace NAlice::NMetrics

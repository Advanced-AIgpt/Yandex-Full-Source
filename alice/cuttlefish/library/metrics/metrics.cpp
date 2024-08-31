#include "metrics.h"

#include <library/cpp/http/misc/httpcodes.h>

namespace {

static constexpr int MIN_HTTP_CODE = 100;
static constexpr int MAX_HTTP_CODE = 600;
static const TVector<TString> HTTP_REASON_NAMES = []() {
    TVector<TString> httpReasonNames;

    static auto convertToLabel = [](const TStringBuf str) {
        TString ret;
        ret.reserve(str.size());
        for (const char c : str) {
            if (c == ' ') {
                ret.push_back('_');
            } else {
                ret.push_back(AsciiToLower(c));
            }
        }

        return ret;
    };

    for (int i = MIN_HTTP_CODE; i < MAX_HTTP_CODE; ++i) {
        if (IsHttpCode(i)) {
            httpReasonNames.push_back(convertToLabel(HttpCodeStrEx(i)));
        } else {
            httpReasonNames.push_back(ToString(i));
        }
    }

    return httpReasonNames;
}();

} // namespace

void NVoice::NMetrics::TSourceMetrics::RateHttpCode(int httpCode, TStringBuf backend, int64_t value) {
    static constexpr TStringBuf HTTP_REASON_UNKNOWN = "unknown";

    if (MIN_HTTP_CODE <= httpCode && httpCode < MAX_HTTP_CODE) {
        PushRate(value, "response", HTTP_REASON_NAMES[httpCode - MIN_HTTP_CODE], backend);
    } else {
        PushRate(value, "response", HTTP_REASON_UNKNOWN, backend);
    }
}

NVoice::NMetrics::TClientInfo NVoice::NMetrics::TSourceMetrics::MakeEmptyClientInfo() {
    // Create empty client info
    NVoice::NMetrics::TClientInfo info;
    info.ClientType = NVoice::NMetrics::EClientType::User;
    info.DeviceName = "";
    info.AppId = "";
    info.GroupName = "";
    info.SubgroupName = "";

    return info;
}

NMonitoring::TBucketBounds NVoice::NMetrics::MakeMillisBuckets() {
    return NMonitoring::TBucketBounds {
        0e3, 5e3, 10e3, 25e3, 50e3, 75e3, 100e3, 125e3, 150e3, 175e3, 200e3, 250e3, 500e3, 750e3,
        1000e3, 1250e3, 1500e3, 1750e4, 2000e3, 2500e3, 3000e3, 3500e3, 4000e3, 4500e3, 5000e3,
        6000e3, 6500e3, 7000e3, 7500e3, 8000e3, 9000e3, 10000e3
    };
}


NMonitoring::TBucketBounds NVoice::NMetrics::MakeMicrosBuckets() {
    return NMonitoring::TBucketBounds {
        0, 5, 10, 25, 50, 75, 100, 125, 150, 175, 200, 250, 500, 750,
        1000, 1250, 1500, 1750e4, 2000, 2500, 3000, 3500, 4000, 4500, 5000,
        6000, 6500, 7000, 7500, 8000, 9000, 10000
    };
}

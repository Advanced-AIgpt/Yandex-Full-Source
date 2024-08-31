#include "request_filters.h"

#include <library/cpp/logger/global/global.h>

#include <util/stream/file.h>
#include <util/system/fs.h>

namespace NRequestsLoader {

namespace {

template<typename T>
std::pair<TStringBuf, std::function<THolder<TRequestFilter>()>> BuildPair(TStringBuf buf) {
    return {
        buf,
        []() -> THolder<T> { return THolder(new T()); }
    };
}

}

/**
 * TSimpleRequestFilter
 */
bool TSimpleRequestFilter::Filter(const NJson::TJsonValue& request) {
    const auto& app = request["application"];
    return app["device_manufacturer"] != "Yandex" || app["device_model"] != "Station";
}

/**
 * TYandexStationRequestFilter
 */
bool TYandexStationRequestFilter::Filter(const NJson::TJsonValue& request) {
    const auto& app = request["application"];
    return app["device_manufacturer"] == "Yandex" && app["device_model"] == "Station";
}

/**
 * TBetaTestersRequestFilter
 */
TBetaTestersRequestFilter::TBetaTestersRequestFilter() {
    const TString idsFilename = "beta_testers.txt";
    if (!NFs::Exists(idsFilename)) {
        ythrow yexception() << "Can't open " << idsFilename << " for filter";
    }

    TFileInput file(idsFilename);
    TString line;
    while (file.ReadLine(line)) {
        DeviceIds_.insert(line);
    }
}

bool TBetaTestersRequestFilter::Filter(const NJson::TJsonValue& request) {
    const auto& deviceId = request["request"]["device_state"]["device_id"].GetString();
    return DeviceIds_.contains(deviceId);
}

/**
 * Helpers
 */
THolder<TRequestFilter> BuildRequestFilter(TStringBuf buf) {
    static const THashMap<TStringBuf, std::function<THolder<TRequestFilter>()>> filters = {
        BuildPair<TSimpleRequestFilter>("simple"),
        BuildPair<TYandexStationRequestFilter>("yandex-station"),
        BuildPair<TBetaTestersRequestFilter>("beta-testers")
    };

    const auto* filterFactory = filters.FindPtr(buf);
    if (!filterFactory) {
        ythrow yexception() << "Failed to found request filter: " << buf << Endl;
    }

    INFO_LOG << "Built request filter: " << buf << Endl;
    return (*filterFactory)();
}

}

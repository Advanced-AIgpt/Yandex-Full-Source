#pragma once

#include <library/cpp/json/json_writer.h>

#include <util/generic/hash_set.h>
#include <util/generic/noncopyable.h>
#include <util/generic/string.h>

namespace NRequestsLoader {

/**
 * Filter classes
 */
class TRequestFilter : private NNonCopyable::TNonCopyable {
public:
    virtual ~TRequestFilter() = default;

    virtual bool Filter(const NJson::TJsonValue& request) = 0;
};

class TSimpleRequestFilter final : public TRequestFilter {
public:
    bool Filter(const NJson::TJsonValue& request) override;
};

class TYandexStationRequestFilter final : public TRequestFilter {
public:
    bool Filter(const NJson::TJsonValue& request) override;
};

class TBetaTestersRequestFilter final : public TRequestFilter {
public:
    TBetaTestersRequestFilter();
    bool Filter(const NJson::TJsonValue& request) override;

private:
    THashSet<TString> DeviceIds_;
};

/**
 * Helpers
 */
THolder<TRequestFilter> BuildRequestFilter(TStringBuf buf);

}

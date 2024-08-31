#pragma once

#include <util/string/builder.h>
#include <util/system/src_location.h>

#define LOG_ADAPTER_CRIT(adapter) (adapter).Log(__LOCATION__, ELogAdapterType::CRIT)
#define LOG_ADAPTER_DEBUG(adapter) (adapter).Log(__LOCATION__, ELogAdapterType::DEBUG)
#define LOG_ADAPTER_EMERGE(adapter) (adapter).Log(__LOCATION__, ELogAdapterType::EMERGE)
#define LOG_ADAPTER_ERROR(adapter) (adapter).Log(__LOCATION__, ELogAdapterType::ERROR)
#define LOG_ADAPTER_INFO(adapter) (adapter).Log(__LOCATION__, ELogAdapterType::INFO)
#define LOG_ADAPTER_WARNING(adapter) (adapter).Log(__LOCATION__, ELogAdapterType::WARNING)

namespace NAlice {

enum class ELogAdapterType {
    CRIT,
    DEBUG,
    EMERGE,
    ERROR,
    INFO,
    WARNING
};

class TLogAdapterElement;

class TLogAdapter {
public:
    virtual ~TLogAdapter() = default;

    TLogAdapterElement Log(const TSourceLocation& location, ELogAdapterType type) const;

protected:
    virtual void LogImpl(TStringBuf msg, const TSourceLocation& location, ELogAdapterType type) const = 0;

    friend class TLogAdapterElement;
};

class TLogAdapterElement {
public:
    TLogAdapterElement(const TLogAdapter& logAdapter, const TSourceLocation& location, ELogAdapterType type)
        : Type{type},
          Adapter{logAdapter},
          Location{location}
    {
    }

    ~TLogAdapterElement() {
        Adapter.LogImpl(Builder, Location, Type);
    }

    template <typename T>
    TLogAdapterElement& operator<<(const T& t) {
        Builder << t;
        return *this;
    }

private:
    ELogAdapterType Type;
    TStringBuilder Builder;
    const TLogAdapter& Adapter;
    const TSourceLocation& Location;
};

} // namespace NAlice

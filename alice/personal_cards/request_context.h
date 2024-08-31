#pragma once

#include <library/cpp/json/json_value.h>

#include <util/generic/maybe.h>

namespace NPersonalCards {

// this class to store the request and header parameters
class TRequestContext {
public:
    explicit TRequestContext(
        NJson::TJsonMap&& request,
        const TString& forwardedFor = "",
        const TString& startTime = "",
        const TMaybe<ui64> uidFromTvm = Nothing()
    );
    ~TRequestContext() = default;

    const NJson::TJsonMap& Request() const {
        return Request_;
    }

    const TString& ForwardedFor() const {
        return ForwardedFor_;
    }

    const TString& StartTime() const {
        return StartTime_;
    }

    TMaybe<ui64> UidFromTvm() const {
        return UidFromTvm_;
    }

private:
    const NJson::TJsonMap& Request_;
    const TString ForwardedFor_;
    const TString StartTime_;
    const TMaybe<ui64> UidFromTvm_;
};

IOutputStream& operator << (IOutputStream& out, const TRequestContext& requestContext);

} // namespace NPersonalCards

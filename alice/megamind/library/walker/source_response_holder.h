#pragma once

#include <alice/megamind/library/util/status.h>

namespace NAlice::NMegamind {

/** The class holds value for a source and its status.
 * Useful in implementation TResponses (service source responses in TContext) for apphost and http.
 * It can be in the follwing states:
 *   Defaultly constructed source value and predefined error (not fetched);
 *   Successfully fetched and constructed source value and success status;
 *   Unsuccessfully fetched and defaultly constructed source value with error status;
 */
template <typename T>
struct TSourceResponseHolder {
    TSourceResponseHolder()
        : Status{CreateDefaultError()}
    {
    }

    explicit TSourceResponseHolder(T&& response)
        : Object{std::move(response)}
        , Status{Success()}
    {
    }

    explicit TSourceResponseHolder(TError&& error)
        : Status{std::move(error)}
    {
    }

    explicit TSourceResponseHolder(TErrorOr<T>&& result)
        : Status{result.MoveTo(Object)}
    {
    }

    TSourceResponseHolder& operator=(T&& response) {
        Status = Success();
        Object = std::move(response);
        return *this;
    }

    TSourceResponseHolder& operator=(TError&& error) {
        Status = std::move(error);
        Object = T{};
        return *this;
    }

    TSourceResponseHolder& operator=(TErrorOr<T>&& result) {
        Status = result.MoveTo(Object);
        if (Status.Defined()) {
            Object = T{};
        }
        return *this;
    }

    const T& Get(TStatus* status) const {
        if (status) {
            *status = Status;
        }

        return Object;
    }

    static TError CreateDefaultError() {
        return TError{TError::EType::Logic} << "not requested";
    }

    T Object;
    TStatus Status;
    TMaybe<TDuration> Duration;
};

} // namespace NAlice::NMegamind

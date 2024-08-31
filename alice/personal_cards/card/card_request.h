#pragma once

#include <library/cpp/json/json_value.h>

#include <util/datetime/base.h>

namespace NPersonalCards {

// Класс-обёртка для доступа к полям карточки.
class TCardRequest {
public:
    TCardRequest(const NJson::TJsonMap& request);
    ~TCardRequest() = default;

    const TInstant& GetTimeNow() const {
        return Timestamp_;
    }

    TString GetBestUserId() const {
        return BestUserId_;
    }

    TString GetClientId() const {
        return ClientId_;
    }

private:
    const TInstant Timestamp_;
    const TString BestUserId_;
    const TString ClientId_;
};

} // namespace NPersonalCards

#pragma once

#include <library/cpp/json/json_value.h>

namespace NAlice {

class TEntitySearchResponse {
public:
    TEntitySearchResponse() = default;

    explicit TEntitySearchResponse(NJson::TJsonValue json);

    const NJson::TJsonValue& RawResponse() const {
        return RawEntitySearchResponse;
    }

private:
    NJson::TJsonValue RawEntitySearchResponse;
};

} // namespace NAlice

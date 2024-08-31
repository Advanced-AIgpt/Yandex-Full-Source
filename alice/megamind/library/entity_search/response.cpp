#include "response.h"

namespace NAlice {

TEntitySearchResponse::TEntitySearchResponse(NJson::TJsonValue json)
    : RawEntitySearchResponse(std::move(json))
{
}

} // namespace NAlice

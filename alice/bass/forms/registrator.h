#pragma once

#include <alice/bass/forms/vins.h>

#include <util/generic/vector.h>

namespace NBASS {

struct TFormHandlerPair {
    using THandler = void (*)(TContext&);

    const TStringBuf Name;
    const THandler Handler = nullptr;
};

void Register(THandlersMap* handlers, const TVector<TFormHandlerPair>& pairs);

} // namespace NBASS

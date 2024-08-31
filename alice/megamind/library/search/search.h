#pragma once

#include <alice/library/websearch/response/response.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>

namespace NAlice {

class ISearchObtainer {
public:
    virtual ~ISearchObtainer() = default;

    /** The implementation of this function must be thread-safe.
     */
    virtual const TSearchResponse* Result() = 0;

    virtual const TMaybe<TDuration>& GetDuration() const = 0;
};
using TSearchObtainerPtr = THolder<ISearchObtainer>;

} // namespace NAlice

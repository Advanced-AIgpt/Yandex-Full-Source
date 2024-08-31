#pragma once

#include <library/cpp/scheme/domscheme_traits.h>

namespace NBASS {

namespace NMarket {

struct TBoolSchemeTraits: public TSchemeTraits {
    static inline void Set(TValueRef v, bool b)
    {
        v->SetBool(b);
    }
    template <class T>
    static inline void Set(TValueRef v, T val)
    {
        TSchemeTraits::Set(v, val);
    }
};

} // namespace NMarket

} // namespace NBASS

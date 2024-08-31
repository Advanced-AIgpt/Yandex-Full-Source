#pragma once

#include <util/generic/function.h>

namespace NAlice {

template <typename... TArgs>
struct TVisitProxy {
    void operator()();
};

template <typename TFn, typename... TFnList>
struct TVisitProxy<TFn, TFnList...> : public TVisitProxy<TFnList...> {
    explicit TVisitProxy(TFn fn, TFnList... args)
        : TVisitProxy<TFnList...>(args...)
        , Fn(fn) {
    }

    using TVisitProxy<TFnList...>::operator();

    decltype(auto) operator()(const TFunctionArg<TFn, 0>& arg) {
        return Fn(arg);
    }

    TFn Fn;
};

template <typename... TFnList>
auto MakeLambdaVisitor(TFnList... fns) {
    return TVisitProxy<TFnList...>{fns...};
}

} // namespace NAlice

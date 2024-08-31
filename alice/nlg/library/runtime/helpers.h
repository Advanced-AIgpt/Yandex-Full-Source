#pragma once

#include <alice/nlg/library/runtime_api/env.h>
#include <alice/nlg/library/runtime_api/text_stream.h>

#include <atomic>
#include <util/stream/null.h>


#ifdef NLG_COVERAGE
#define NLG_LINE(lineno) (currentStackFrame.Line = (lineno)), NAlice::NNlg::GetNlgCoverage().IncCounter(NLG_FILENAME, ((lineno)-1))
#define NLG_REGISTER(segments) NAlice::NNlg::GetNlgCoverage().RegisterModule(NLG_FILENAME, (segments))
#else
#define NLG_LINE(lineno) (currentStackFrame.Line = (lineno))
#define NLG_REGISTER(segments) (void)0
#endif

namespace NAlice::NNlg {

template <typename TCallback>
inline void RunMacroDirectOutput(IOutputStream& out, TCallback&& callback) {
    out << PushMask;
    callback(out);
    out << PopMask;
}

template <typename TCallback>
inline TValue GetMacroResult(TCallback&& callback) {
    TText result;
    TTextOutput out(result);
    callback(out);
    return TValue::String(std::move(result));
}

template <typename TCallback>
inline void RunMacroNoResult(TCallback&& callback) {
    callback(Cnull);
}

template <typename... TArgs>
inline void InvokeCaller(const TCaller* caller, IOutputStream& out, TArgs&&... args) {
    if (caller) {
        // NOTE(a-square): if compilation ever fails, just pile more variant alternatives into TCaller
        caller->Get<sizeof...(args)>()(out, std::forward<TArgs>(args)...);
    }
}

template <auto Callback, typename... TArgs>
inline std::invoke_result_t<decltype(Callback), TArgs...>
InvokeWithStackFrame(const TCallCtx& ctx, const TStringBuf name, TArgs&&... args) {
    ctx.CallStack.push_back({name, {}, {}});
    const std::invoke_result_t<decltype(Callback), TArgs...> value =
        std::invoke(Callback, std::forward<TArgs>(args)...);
    ctx.CallStack.pop_back();
    return value;
}

inline const TGlobalsChain* WrapGlobals(const TGlobals*) {
    return nullptr; // allows us to ignore with_context during initialization
}

inline const TGlobalsChain* WrapGlobals(const TGlobalsChain* chain) {
    return chain;
}

using TRenderLocalizedNlgPlaceholder = std::function<void()>;

namespace NPrivate {

void FormatLocalizedTemplate(
    const TStringBuf localizedTemplate,
    const THashMap<TStringBuf, TRenderLocalizedNlgPlaceholder>& placeholders,
    IOutputStream& out);

} // namespace NPrivate

void LocalizeTemplateWithPlaceholders(
    const TCallCtx& callCtx,
    const TStringBuf localizedTemplateKey,
    const THashMap<TStringBuf, TRenderLocalizedNlgPlaceholder>& placeholders,
    IOutputStream& out);

} // namespace NAlice::NNlg

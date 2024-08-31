#pragma once

#include "compiler_error.h"
#include "messages.h"
#include "src_line.h"
#include <alice/nlu/granet/lib/utils/variadic_format.h>

namespace NGranet::NCompiler {

template <typename... Args>
void ThrowGranetCompilerError(ELanguage uiLang, EMessageId messageId, const Args&... args) {
    const TString& message = GetMessageTable(uiLang).GetMessage(messageId);
    ythrow TCompilerError(VariadicFormat(message, args...), messageId);
}

template <typename... Args>
void ThrowGranetCompilerError(const TTextView& source, EMessageId messageId, const Args&... args) {
    const ELanguage uiLang = source.IsDefined() ? source.GetSourceText()->UILang : LANG_ENG;
    const TString& message = GetMessageTable(uiLang).GetMessage(messageId);
    ythrow TCompilerError(source, VariadicFormat(message, args...), messageId);
}

template <typename... Args>
void ThrowGranetCompilerError(const TSrcLine& line, EMessageId messageId, const Args&... args) {
    ThrowGranetCompilerError(line.Source, messageId, args...);
}

#define GRANET_COMPILER_CHECK(condition, ...)           \
    do {                                                \
        if (Y_UNLIKELY(!(condition))) {                 \
            ThrowGranetCompilerError(__VA_ARGS__);      \
        }                                               \
    } while (false)

} // namespace NGranet::NCompiler

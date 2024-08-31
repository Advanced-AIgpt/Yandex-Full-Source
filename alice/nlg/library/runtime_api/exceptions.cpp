#include "exceptions.h"

#include <util/stream/output.h>

namespace NAlice::NNlg {

namespace {

struct TStackPrinter {
    const TCallStack& CallStack;
};

}  // namespace

[[noreturn]] void TRuntimeError::ThrowWrapped(TCallStack&& callStack) {
    TRuntimeError exc(std::move(callStack));
    exc << TStringBuf("Runtime NLG error: ") << CurrentExceptionMessage()
        << TStringBuf("\n\nOriginal NLG call stack:\n") << TStackPrinter{exc.GetCallStack()};
    std::throw_with_nested(std::move(exc));
}

}  // namespace NAlice::NNlg

template <>
void Out<NAlice::NNlg::TStackPrinter>(IOutputStream& out,
                                      TTypeTraits<NAlice::NNlg::TStackPrinter>::TFuncParam stackPrinter) {
    for (const auto& frame : stackPrinter.CallStack) {
        if (const auto tmpl = frame.Template) {
            out << tmpl;

            if (const auto line = frame.Line) {
                out << ':' << *line;
            }

            out << TStringBuf(", ");
        }

        out << TStringBuf("in ") << (frame.Name ? frame.Name : TStringBuf("<unknown>")) << Endl;
    }
}

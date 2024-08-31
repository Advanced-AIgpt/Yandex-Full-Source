#pragma once

#include "call_stack.h"

#include <util/generic/yexception.h>

namespace NAlice::NNlg {

// Wrapper for an std::exception thrown somewhere in the generated NLG code.
class TRuntimeError : public yexception {
public:
    explicit TRuntimeError(TCallStack&& callStack)
        : CallStack(std::move(callStack)) {
    }

    const TCallStack& GetCallStack() const {
        return CallStack;
    }

    [[noreturn]] static void ThrowWrapped(TCallStack&& callStack);

private:
    TCallStack CallStack;
};

class TImportError : public yexception {
public:
    using yexception::yexception;
};

class TTypeError : public yexception {
public:
    using yexception::yexception;
};

class TValueError : public yexception {
public:
    using yexception::yexception;
};

class TZeroDivisionError : public TValueError {
public:
    using TValueError::TValueError;
};

class TValueFrozenError : public TValueError {
public:
    using TValueError::TValueError;
};

class TCardValidationError : public yexception {
public:
    using yexception::yexception;
};

class TTranslationError : public yexception {
public:
    using yexception::yexception;
};

}  // namespace NAlice::NNlg

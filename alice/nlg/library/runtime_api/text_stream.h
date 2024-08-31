#pragma once

#include "text.h"

#include <util/generic/stack.h>
#include <util/stream/output.h>

namespace NAlice::NNlg {

class TTextOutput : public IOutputStream {
public:
    explicit TTextOutput(TText& text)
        : Text(text) {
    }

    void DoWrite(const void* buf, size_t len) override {
        Text.Append(TStringBuf{reinterpret_cast<const char*>(buf), len}, Mask);
    }

    void PushMask() {
        MasksStack.push(Mask);
    }

    void PopMask() {
        Y_ENSURE(!MasksStack.empty());
        Mask = MasksStack.top();
        MasksStack.pop();
    }

    void SetFlag(TText::EFlag flag) {
        Mask |= flag;
    }

    void ClearFlag(TText::EFlag flag) {
        Mask &= ~TFlags{flag};
    }

private:
    TStack<TText::TFlags> MasksStack;
    TText& Text;
    TText::TFlags Mask = TText::DefaultFlags();

    friend void ::Out<NAlice::NNlg::TText>(IOutputStream& out,
                                                  TTypeTraits<NAlice::NNlg::TText>::TFuncParam value);
    friend void ::Out<NAlice::NNlg::TText::TView>(IOutputStream& out,
                                                  TTypeTraits<NAlice::NNlg::TText::TView>::TFuncParam value);
};

inline void PushMask(IOutputStream& out) {
    if (auto* textOut = dynamic_cast<NAlice::NNlg::TTextOutput*>(&out)) {
        textOut->PushMask();
    }
}

inline void PopMask(IOutputStream& out) {
    if (auto* textOut = dynamic_cast<NAlice::NNlg::TTextOutput*>(&out)) {
        textOut->PopMask();
    }
}

struct TSetFlag {
    TText::EFlag Flag;
};

struct TClearFlag {
    TText::EFlag Flag;
};

inline TSetFlag SetFlag(TText::EFlag flag) {
    return {flag};
}

inline TClearFlag ClearFlag(TText::EFlag flag) {
    return {flag};
}

} // namespace NAlice::NNlg

template <>
inline void Out<NAlice::NNlg::TSetFlag>(IOutputStream& out, TTypeTraits<NAlice::NNlg::TSetFlag>::TFuncParam value) {
    if (auto* textOut = dynamic_cast<NAlice::NNlg::TTextOutput*>(&out)) {
        textOut->SetFlag(value.Flag);
    }
}

template <>
inline void Out<NAlice::NNlg::TClearFlag>(IOutputStream& out,
                                          TTypeTraits<NAlice::NNlg::TClearFlag>::TFuncParam value) {
    if (auto* textOut = dynamic_cast<NAlice::NNlg::TTextOutput*>(&out)) {
        textOut->ClearFlag(value.Flag);
    }
}

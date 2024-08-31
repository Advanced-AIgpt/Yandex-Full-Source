#pragma once
#include <alice/nlu/libs/interval/interval.h>
#include <kernel/inflectorlib/phrase/simple/simple.h>
#include <util/generic/array_ref.h>

namespace NAlice {
    class TTokenIntervalInflector {
    public:
        TTokenIntervalInflector() = default;

        // TTokenIntervalInflector will try to use grammemes, but is not obliged to do this.
        TString Inflect(const TVector<TString>& tokens,
                        const TVector<TString>& tokenGrammemes,
                        const NNlu::TInterval& tokenInterval,
                        const TString& targetCase = "nom") const;

    private:
        bool TryInflectWithGrammemes(const TVector<TString>& tokens,
                                     const TVector<TString>& tokenGrammemes,
                                     const NNlu::TInterval& tokenInterval,
                                     const TString& targetCase,
                                     TString* result) const;
        bool TryInflectDefault(const TVector<TString>& tokens,
                               const NNlu::TInterval& tokenInterval,
                               const TString& targetCase,
                               TString* result) const;
        bool TryInflect(const TArrayRef<const TString>& tokens, const TString& targetCase, TString* result) const;

    private:
        const NInfl::TSimpleInflector Inflector;
    };
} // namespace NAlice

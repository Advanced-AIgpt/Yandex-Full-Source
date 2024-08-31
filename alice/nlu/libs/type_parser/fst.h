#pragma once
#include "type_parser.h"
#include <alice/nlu/libs/fst/fst_base.h>

namespace NAlice {
    class TFstTypeParser : public TTypeParser {
    public:
        TFstTypeParser(THolder<TFstBase>&& fst);

    public:
        void Parse(const TArrayRef<const TString>& textTokens, TEntityParsingResult* entities) const override;

    private:
        const THolder<TFstBase> Fst;
    };
} // namespace NAlice

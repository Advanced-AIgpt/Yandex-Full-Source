#pragma once
#include "type_parser.h"

namespace NAlice {
    class TDigitalTypeParser : public TTypeParser {
    public:
        void Parse(const TArrayRef<const TString>& textTokens, TEntityParsingResult* entities) const override;
    };
} // namespace NAlice

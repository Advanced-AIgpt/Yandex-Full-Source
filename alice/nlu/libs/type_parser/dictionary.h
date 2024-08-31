#pragma once
#include "type_parser.h"
#include <alice/nlu/libs/annotator/annotator_with_mapping.h>

namespace NAlice {
    class TDictionaryTypeParser : public TTypeParser {
    public:
        TDictionaryTypeParser(const TBlob& annotator);
        void Parse(const TArrayRef<const TString>& textTokens, TEntityParsingResult* entities) const override;

    protected:
        virtual void NormalizeValue(const TString& rawValue, TString* value, EEntityType* type) const;

    private:
        NAnnotator::TAnnotatorWithMapping<TString> Annotator;
    };
} // namespace NAlice

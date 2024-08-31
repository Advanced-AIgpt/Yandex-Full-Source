#pragma once
#include <alice/nlu/libs/type_parser/dictionary.h>
#include <library/cpp/langs/langs.h>
#include <search/begemot/core/filesystem.h>
#include <util/generic/string.h>

namespace NBg {
    class TTimeDictionaryTypeParser : public NAlice::TDictionaryTypeParser {
    public:
        using TDictionaryTypeParser::TDictionaryTypeParser;

    private:
        void NormalizeValue(const TString& rawValue, TString* value, NAlice::EEntityType* type) const override;
    };

    THolder<NAlice::TTypeParser> MakeTimeTypeParser(const TFileSystem& fs, const ELanguage lang);
} // namespace NBg

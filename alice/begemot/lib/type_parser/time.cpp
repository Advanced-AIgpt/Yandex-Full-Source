#include "time.h"
#include <alice/nlu/libs/fst/fst_time.h>
#include <alice/nlu/libs/type_parser/fst.h>
#include <alice/nlu/libs/type_parser/union.h>
#include <alice/begemot/lib/fst/file_system_loader.h>
#include <util/generic/hash.h>

namespace NBg {
    namespace {
        const TMap<ELanguage, TString> TIME_DICTIONARY_PATH_BY_LANG = {
            {LANG_RUS, "type_parser_time_rus.dict"}
        };
    } // namespace anonymous

    void TTimeDictionaryTypeParser::NormalizeValue(const TString& rawValue, TString* value, NAlice::EEntityType* type) const {
        *type = NAlice::EEntityType::TIME;
        *value = rawValue;
    }

    THolder<NAlice::TTypeParser> MakeTimeTypeParser(const TFileSystem& fs, const ELanguage lang) {
        TVector<NAlice::TTypeParser*> parsers;

        const auto fstDir = fs.Subdirectory(TString("fst/"));
        for (const auto& langDir : fstDir->List()) {
            if (lang != LanguageByName(langDir)) { // not sure if straight conversion will make directory name
                continue;
            }
            const auto timeFstPath = JoinFsPaths(langDir, "time");
            if (!fstDir->Exists(timeFstPath)) {
                continue;
            }
            const auto fstDataLoader = NAlice::TFileSystemDataLoader(fstDir->Subdirectory(timeFstPath));
            auto fst = MakeHolder<NAlice::TFstTime>(fstDataLoader);
            parsers.push_back(MakeHolder<NAlice::TFstTypeParser>(std::move(fst)).Release());
        }

        if (TIME_DICTIONARY_PATH_BY_LANG.contains(lang)) {
            auto dict = MakeHolder<TTimeDictionaryTypeParser>(fs.LoadBlob(TIME_DICTIONARY_PATH_BY_LANG.at(lang)));
            parsers.push_back(dict.Release());
        }

        return MakeHolder<NAlice::TUnionTypeParser>(std::move(parsers));
    }
} // namespace NBg

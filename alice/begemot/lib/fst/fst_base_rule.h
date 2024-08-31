#pragma once

#include "file_system_loader.h"
#include <alice/nlu/libs/fst/fst_custom.h>

#include <alice/nlu/proto/entities/fst.pb.h>
#include <search/begemot/core/rulebase.h>
#include <search/begemot/rules/text/proto/text.pb.h>

#include <google/protobuf/struct.pb.h>
#include <library/cpp/langs/langs.h>

#include <util/generic/variant.h>
#include <util/string/cast.h>

namespace NBg {

    template <typename TFst>
    class TFstBaseRule {
    public:
        static void FillValue(NAlice::TParsedToken::TValueType&& value, TString* jsonSerialized) {
            Y_ASSERT(jsonSerialized);
            *jsonSerialized = value.ToJsonSafe();
        }

        TFstBaseRule(const TRuleContext& /*ctx*/, const TFileSystem& fileSystem, TStringBuf fstName) {
            auto fstDir = fileSystem.Subdirectory(TString("fst/"));
            for (auto& dir: fstDir->List()) {
                auto fstPath = JoinFsPaths(dir, SnakeCase(ToString(fstName)));
                if (fstDir->Exists(fstPath)) {
                    const ELanguage lang = LanguageByName(dir);
                    if (lang == LANG_UNK) {
                        continue;
                    }
                    Fsts.emplace(lang, NAlice::TFileSystemDataLoader(std::move(fstDir->Subdirectory(fstPath))));
                }
            }
        }

        template <typename TInputType, typename TResultType>
        void FillResult(const TInputType& input, TResultType& fstResult) const {
            auto&& request = input.GetText();
            const ELanguage lang = static_cast<ELanguage>(input.GetLanguage());

            auto* fst = Fsts.FindPtr(lang);
            if (!fst) {
                return;
            }

            auto&& entities = fst->Parse(request);
            for (auto&& entity : entities) {
                auto protoEntity = fstResult.AddEntities();
                protoEntity->SetStart(entity.Start);
                protoEntity->SetEnd(entity.End);
                protoEntity->SetType(std::move(entity.ParsedToken.Type));
                protoEntity->SetStringValue(std::move(entity.ParsedToken.StringValue));
                if (entity.ParsedToken.Weight) {
                    protoEntity->SetWeight(*entity.ParsedToken.Weight);
                }
                FillValue(std::move(entity.ParsedToken.Value), protoEntity->MutableValue());
            }
        }

    private:
        TString SnakeCase(const TString& value, char delimiter = '_') {
            if (AllOf(value, [](unsigned char c) { return IsAsciiUpper(c); }))
                return to_lower(value);
            TString result;
            result.reserve(value.size() + 1);
            for (auto c : value) {
                if (IsAsciiUpper(c) || isdigit(c)) {
                    if (!result.empty() && result.back() != delimiter && (!isdigit(c) || !isdigit(result.back())))
                        result.push_back(delimiter);
                    result.push_back(AsciiToLower(c));
                } else {
                    result.push_back(c);
                }
            }
            return result;
        }

        TMap<ELanguage, TFst> Fsts;
    };

} // namespace NBg

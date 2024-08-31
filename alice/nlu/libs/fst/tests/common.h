#pragma once

#include "data.h"

#include <alice/nlu/libs/fst/fst_base.h>
#include <alice/nlu/libs/fst/fst_normalizer.h>
#include <alice/nlu/libs/fst/fst_post.h>
#include <alice/nlu/libs/fst/prefix_data_loader.h>
#include <alice/nlu/libs/fst/tokenize.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/utf8.h>
#include <util/generic/lazy_value.h>

#include <algorithm>

namespace NAlice {

    namespace NTestHelpers {

        struct TTestCase {
            TStringBuf Name;
            TVector<TEntity> Entities;
        };

        struct TTestCaseValue {
            TStringBuf Request;
            TVector<NSc::TValue> Values;
        };

        inline void PrintEntities(const TVector<TEntity>& entities) {
            for (const auto& entity : entities) {
                Cout << entity.Start << '-' << entity.End << "; "
                     << "string_value: " << entity.ParsedToken.StringValue << ", "
                     << "type: " << entity.ParsedToken.Type << ", "
                     << "weight: " << entity.ParsedToken.Weight << ", "
                     << "value: " << entity.ParsedToken.Value << Endl;
            }
        }

        inline TEntity CreateEntity(size_t start, size_t end, TString type, TParsedToken::TValueType value) {
            TEntity entity;
            entity.Start = start;
            entity.End = end;
            entity.ParsedToken.Type = type;
            entity.ParsedToken.Value = std::move(value);

            return entity;
        }

        inline void DropExcept(const TStringBuf& type, TVector<TEntity>* entities) {
            Y_VERIFY(entities);
            entities->erase(std::remove_if(
                    begin(*entities), end(*entities),
                    [&type] (const auto& entity) {
                        return (entity.ParsedToken.Type != type);
                    }),
                end(*entities));
        }

        inline bool Eq(const TEntity& lhs, const TEntity& rhs) {
            return (lhs.Start == rhs.Start
                && lhs.End == rhs.End
                && lhs.ParsedToken.Type == rhs.ParsedToken.Type
                && lhs.ParsedToken.Value == rhs.ParsedToken.Value);
        }

        inline bool Equal(const TVector<TEntity>& lhs, const TVector<TEntity>& rhs) {
            return std::equal(
                begin(lhs), end(lhs),
                begin(rhs), end(rhs),
                Eq
            );
        }

        inline bool Equal(const TVector<TEntity>& lhs, const TVector<NSc::TValue>& rhs) {
            return std::equal(
                begin(lhs), end(lhs),
                begin(rhs), end(rhs),
                [] (const auto& lhs, const auto& rhs) {
                    return lhs.ParsedToken.Value == rhs;
                }
            );
        }


        TFstNormalizer CreateNormalizer(ELanguage lang);

        template <typename TFst>
        class TTestCaseRunner {
        public:
            TTestCaseRunner(TString fstName, ELanguage lang)
                : FstName(std::move(fstName))
                , Normalizer(CreateNormalizer(lang))
                , Fst(TPrefixDataLoader{NFst::GetFstDataPath(ToLowerUTF8(FstName), lang)})
            {
            }

            template <auto N>
            void Run(const TTestCase (&testCases)[N], TStringBuf dropFilter) const {
                for (const auto& testCase : testCases) {
                    const auto& normalized = Normalizer.Normalize(NFst::Tokenize(testCase.Name));
                    Cout << "Normalized text: " << normalized << Endl;
                    auto&& entities = Fst.Parse(normalized);
                    DropExcept(dropFilter, &entities);
                    PrintEntities(entities);
                    UNIT_ASSERT(Equal(entities, testCase.Entities));
                }
            }

            template <auto N>
            void Run(const TTestCase (&testCases)[N]) const {
                Run(testCases, FstName);
            }

            template <auto N>
            void Run(const TTestCaseValue (&testCases)[N]) const {
                for (const auto& testCase : testCases) {
                    const auto& normalized = Normalizer.Normalize(NFst::Tokenize(testCase.Request));
                    Cout << "Normalized text: " << normalized << Endl;
                    auto&& entities = Fst.Parse(normalized);
                    TFstPost::CombineEntities(&entities);
                    DropExcept(FstName, &entities);
                    PrintEntities(entities);
                    const auto equal = Equal(entities, testCase.Values);
                    UNIT_ASSERT(equal);
                }
            }

        private:
            TString FstName;
            TFstNormalizer Normalizer;
            TFst Fst;
        };

    } // namespace NTestHeleprs

} // namespace NAlice

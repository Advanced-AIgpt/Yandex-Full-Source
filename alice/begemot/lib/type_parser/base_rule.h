#pragma once

#include <alice/begemot/lib/locale/locale.h>
#include <alice/nlu/libs/type_parser/type_parser.h>
#include <library/cpp/langs/langs.h>
#include <search/begemot/apphost/context.h>
#include <search/begemot/core/filesystem.h>
#include <search/begemot/rules/alice/session/proto/alice_session.pb.h>

namespace NBg {
    template <class TResult>
    class TAliceTypeParserBaseRule : public IRule<TResult> {
    public:
        BEGEMOT_RULE_DEPENDS_ON(
            NProto::TInternalContext,
            NProto::TLocaleResult,
            NProto::TAliceSessionResult
        );

        TAliceTypeParserBaseRule(const TString& ruleName, TMap<ELanguage, THolder<NAlice::TTypeParser>>&& typeParsersByLanguage)
            : RuleName(ruleName)
            , TypeParsersByLanguage(std::move(typeParsersByLanguage))
        {
        }

        void Do(const TRuleContext& ctx, TResult& result) const {
            const auto lang = NAlice::NAliceLocale::GetLanguageRobust(ctx.Require<NProto::TLocaleResult>(this));

            if (!TypeParsersByLanguage.contains(lang)) {
                return;
            }

            const auto& parser = TypeParsersByLanguage.at(lang);
            const auto& tokensProto = ctx.Require<NProto::TAliceSessionResult>(this).GetNormalizedRequest().GetTokens();
            auto& ruleResult = *result.MutableResult();
            *ruleResult.MutableTokens() = tokensProto;

            const TVector<TString> tokens(tokensProto.begin(), tokensProto.end());
            const auto parsingResult = parser->Parse(tokens);

            auto& parsedEntitiesByTypeProto = *ruleResult.MutableParsedEntitiesByType();
            for (const auto& [entityType, entities] : parsingResult) {
                const auto entityTypeString = EntityTypeToString(entityType);
                for (const auto& entity : entities) {
                    auto& entityProto = *parsedEntitiesByTypeProto[entityTypeString].AddParsedEntities();
                    entityProto.SetStartToken(entity.Interval.Begin);
                    entityProto.SetEndToken(entity.Interval.End);
                    entityProto.SetType(entityTypeString);
                    entityProto.SetValue(entity.Value);
                    entityProto.SetText(entity.Text);
                }
            }
        }

    private:
        bool IsEnabled(const TRuleContext& ctx) const {
            for (const auto& ruleMode : ctx.Require<NProto::TInternalContext>(this).GetRuleModes()) {
                if (ruleMode.GetName() == RuleName) {
                    return true;
                }
            }
            return false;
        }

    private:
        const TString RuleName;
        const TMap<ELanguage, THolder<NAlice::TTypeParser>> TypeParsersByLanguage;
    };
} // namespace NBg

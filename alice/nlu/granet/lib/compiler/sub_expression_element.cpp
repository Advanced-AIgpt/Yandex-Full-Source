#include "sub_expression_element.h"
#include <library/cpp/iterator/enumerate.h>

namespace NGranet::NCompiler {

TString GenerateSubExpressionElementName(const TSubExpressionElementKey& data, const TTokenPool& tokenPool,
    const TDeque<TCompilerElement>& elements)
{
    const bool isBag = data.Quantity.MinCount == 0
        && data.Quantity.MaxCount == Max<ui8>()
        && data.Rules.size() > 1;

    TStringBuilder out;
    out << (isBag ? '[' : '(');
    for (const auto [r, rule] : Enumerate(data.Rules)) {
        if (r > 0) {
            out << (isBag ? ' ' : '|');
        }

        TStringBuilder body;
        for (const auto [i, token] : Enumerate(rule.Chain)) {
            if (i > 0) {
                body << ' ';
            }
            if (NTokenId::IsSlotEdge(token)) {
                continue;
            }
            if (NTokenId::IsElement(token)) {
                body << elements[NTokenId::ToElementId(token)].Name;
                continue;
            }
            body << tokenPool.PrintWordToken(token);
        }

        TStringBuilder suffix;
        suffix << (isBag ? rule.BagItemParams.PrintSuffix() : "");
        suffix << (rule.ListItemParams.IsForced ? "%force" : "");
        suffix << (rule.ListItemParams.IsNegative ? "%neg" : "");

        const bool canOmitParentheses = data.Rules.size() == 1 && suffix.empty() || rule.Chain.size() == 1;
        if (canOmitParentheses) {
            out << body << suffix;
        } else {
            out << '(' << body << ')' << suffix;
        }
    }
    out << (isBag ? ']' : ')');
    if (!isBag) {
        out << data.Quantity.GetNormalizedText();
    }
    return out;
}

} // namespace NGranet::NCompiler

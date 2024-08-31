#include "digital.h"
#include <util/generic/algorithm.h>
#include <util/string/join.h>

namespace NAlice {
    namespace {
        // may contain leading zeroes
        bool IsDigital(const TString& text) {
            return AllOf(text, [](const char& symbol) {
                return '0' <= symbol && symbol <= '9';
            });
        }
    } // namespace anonymous

    void TDigitalTypeParser::Parse(const TArrayRef<const TString>& textTokens, TEntityParsingResult* entities) const {
        Y_ASSERT(entities);

        TVector<bool> isDigital(textTokens.size());
        for (size_t pos = 0; pos < textTokens.size(); ++pos) {
            isDigital[pos] = IsDigital(textTokens[pos]);
        }

        for (size_t start = 0; start < textTokens.size(); ++start) {
            for (size_t end = start + 1; end <= textTokens.size() && isDigital[end - 1]; ++end) {
                AddEntity(TParsedEntity{
                    {start, end},
                    JoinRange("", textTokens.begin() + start, textTokens.begin() + end), // Value
                    JoinRange(" ", textTokens.begin() + start, textTokens.begin() + end),  // Text
                    EEntityType::DIGITAL
                }, entities);
            }
        }
    }
} // namespace NAlice

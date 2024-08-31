#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/string/split.h>

namespace NNlu {

// ~~~~ NJoinedTokens ~~~~

// Utils for string of tokens joined by space - more effective and handy representation of token array.
namespace NJoinedTokens {

    // Right way to iterate tokens in such representation is:
    //   StringSplitter(ToLowerUTF8(tokens)).Split(' ').SkipEmpty()
    // Modifier SkipEmpty used for empty string (which represents empty list of tokens).

    // Number of tokens joined by space.
    inline size_t CountTokens(TStringBuf tokens) {
        if (tokens.empty()) {
            return 0;
        }
        return ::Count(tokens, ' ') + 1;
    }

    inline size_t CountTokens(TWtringBuf tokens) {
        if (tokens.empty()) {
            return 0;
        }
        return ::Count(tokens, u' ') + 1;
    }

    inline TUtf16String Collapse(TWtringBuf tokens) {
        TUtf16String result(Reserve(tokens.length()));
        for (const auto token : StringSplitter(tokens).Split(u' ').SkipEmpty()) {
            if (!result.empty()) {
                result.append(u' ');
            }
            result.append(token);
        }
        return result;
    }

    inline bool HasEmptyToken(TWtringBuf tokens) {
        for (size_t i = 0; i < tokens.length(); ++i) {
            if (tokens[i] == u' ' && (i == 0 || i == tokens.length() - 1 || tokens[i + 1] == u' ')) {
                return true;
            }
        }
        return false;
    }

    // Append one token to tokens joined by space.
    // Removes spaces from token, empty token represented as "_".
    inline void AppendToken(TStringBuf token, TString* tokens) {
        Y_ASSERT(tokens);
        if (!tokens->empty()) {
            tokens->append(' ');
        }
        // Append without spaces
        while (!token.empty()) {
            tokens->append(token.NextTok(' '));
        }
        // Special value for empty token
        if (tokens->empty() || tokens->EndsWith(' ')) {
            tokens->append('_');
        }
    }

    // Convert array of tokens (array of TString or TStringBuf) to string of tokens joined by space.
    // Removes spaces from tokens, empty token represented as "_".
    template<class TContainer>
    TString JoinTokens(const TContainer& tokens) {
        TString joined;
        for (const auto& token : tokens) {
            AppendToken(token, &joined);
        }
        Y_ASSERT(CountTokens(joined) == tokens.size());
        return joined;
    }

} // namespace NJoinedTokens

} // namespace NNlu

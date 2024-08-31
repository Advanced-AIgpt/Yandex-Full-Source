#include "tokenize.h"

#include <util/generic/yexception.h>
#include <util/stream/output.h>

#include <re2/re2.h>

namespace NAlice {

    namespace NFst {

        static re2::RE2::Options MakeOptions() {
            re2::RE2::Options opts{};
            opts.set_longest_match(true);
            return opts;
        }

        static RE2 TokenPattern{
            // only alphabetic
            "("
                R"([^\PL\d_]+)"
                "|"
                // alphabetic + numbers
                R"([^\PL_]+)"
                "|"
                // multi-words with separator (multi-word, multi/word, etc.)
                // partial words contain only alphabetic symbols (any 1-word or word-1 will be ignored)
                R"([^\PL\d_]+(?:[\-/\\][^\PL\d_]+)+)"
                "|"
                // any number (int, float)
                R"((?:\d+[,\.]\d+|\d+))"
                "|"
                // datetime of type dd.mm.yy{yy}
                R"(\d{1,2}[\.,\\/\-]\d{1,2}[\.,\\/\-]\d{2,4})"
                "|"
                // time of type hh:mm
                R"(\d{1,2}:\d\d)"
                "|"
                // arithmetic operations, if inside tokens, should be surrounded by digits
                R"([\+\-/\*=\^])"
                "|"
                // currency symbols
                "([$\u20ac\u20bd])"
                "|"
                // special cases with glued punctuation
                R"(\pL\+\+)"
                ")",
                MakeOptions()
                };

        static RE2 TokenPatternWithSignedNumber{
            // only alphabetic
            "("
            R"([^\PL\d_]+)"
            "|"
            // alphabetic + numbers
            R"([^\PL_]+)"
            "|"
            // multi-words with separator (multi-word, multi/word, etc.)
            // partial words contain only alphabetic symbols (any 1-word or word-1 will be ignored)
            R"([^\PL\d_]+(?:[\-/\\][^\PL\d_]+)+)"
            "|"
            // any number (int, float)
            R"((?:\d+[,\.]\d+|\d+))"
            "|"
            // negative number -1 only at the start of the token
            R"(^\-(?:\d+[,\.]\d+|\d+))"
            "|"
            // datetime of type dd.mm.yy{yy}
            R"(\d{1,2}[\.,\\/\-]\d{1,2}[\.,\\/\-]\d{2,4})"
            "|"
            // time of type hh:mm
            R"(\d{1,2}:\d\d)"
            "|"
            // arithmetic operations, if inside tokens, should be surrounded by digits
            R"([\+\-/\*=\^])"
            "|"
            // currency symbols
            "([$\u20ac\u20bd])"
            "|"
            // special cases with glued punctuation
            R"(\pL\+\+)"
            ")",
            MakeOptions()
        };

        static RE2 RawTokenPattern{R"(([^\s]+))", MakeOptions()};

        TString Tokenize(TStringBuf text) {
            Y_ENSURE(TokenPattern.ok());
            using namespace re2;
            StringPiece input(text.data(), text.size());
            TString output;
            StringPiece rawToken;
            while (RE2::FindAndConsume(&input, RawTokenPattern, &rawToken)) {
                StringPiece token;
                bool beginning = true;
                while ((beginning && RE2::Consume(&rawToken, TokenPatternWithSignedNumber, &token))
                    || RE2::FindAndConsume(&rawToken, TokenPattern, & token))
                {
                    beginning = false;
                    if (!output.empty()) {
                        output.push_back(' ');
                    }
                    output += TStringBuf{token.data(), token.size()};
                }
            }
            return output;
        }

    } // namespace NFst

} // namespace NAlice

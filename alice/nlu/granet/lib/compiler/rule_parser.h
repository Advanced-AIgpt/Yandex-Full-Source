#pragma once

#include "directives.h"
#include <alice/nlu/granet/lib/grammar/grammar_data.h>
#include <alice/nlu/granet/lib/utils/text_view.h>
#include <util/folder/path.h>
#include <util/generic/deque.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/stack.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGranet::NCompiler {

class TRuleLexer;

// ~~~~ ERuleTokenType ~~~~

enum ERuleTokenType {
    RTT_UNDEFINED = 0,

    // Range 1-255 is for one-symbol tokens:
    //   * ? + { } ( ) [ ] |

    RTT_RULE_BEGIN = 256,   // Begin of rule (width is 0)
    RTT_RULE_END,           // End of rule (width is 0)
    RTT_TAG_BODY_BEGIN,     // '   in  'Operand'(tag)
    RTT_TAG_BODY_END,       // '(  in  'Operand'(tag)
    RTT_TAG,                // tag in expression 'operand'(tag)
    RTT_CONCATENATE,        // Space between terms

    // Tokens used in TRuleToken::Operands
    RTT_RULE,               // Whole rule
    RTT_ROUND_BRACKETS,     // (Operand) or 'Operand'
    RTT_SQUARE_BRACKETS,    // [Operand]
    RTT_ELEMENT,            // $ElementName
    RTT_WORD,               // слово
    RTT_STRING,             // "слово"
    RTT_WILDCARD,           // .
    RTT_TAG_NAME,           // tag     from  'Operand'(tag)
    RTT_ANGLE_SUFFIX,       // suffix  from  Operand<suffix>
    RTT_EXPR_SUFFIX,        // Operand?, Operand*, Operand+, Operand{suffix}, 'Operand'(tag)
    RTT_EXPR_LIST,          // Operand1 | Operand2 | Operand3
    RTT_EXPR_CHAIN,         // Operand1 Operand2 Operand3
};

// ~~~~ TRuleToken ~~~~

class TRuleToken : public TSimpleRefCount<TRuleToken> {
public:
    using TRef = TIntrusivePtr<TRuleToken>;
    using TConstRef = TIntrusiveConstPtr<TRuleToken>;

public:
    int Type = RTT_UNDEFINED;

    TTextView Source;

    // All sub-tokens
    TVector<TRuleToken::TRef> Children;

    // Operands of:
    //      ?, *, + (single operand - operator symbol)
    //      RTT_RULE, RTT_ROUND_BRACKETS, RTT_SQUARE_BRACKETS (none or single operand - expression inside braces)
    //      RTT_EXPR_SUFFIX (two children - operand and suffix of type ?, *, +, RTT_ANGLE_SUFFIX or RTT_TAG_NAME)
    //      RTT_EXPR_LIST, RTT_EXPR_CHAIN (more than one operand)
    // Possible type of operand (and the root of the tree):
    //      RTT_RULE
    //      RTT_ROUND_BRACKETS
    //      RTT_SQUARE_BRACKETS
    //      RTT_ELEMENT
    //      RTT_WORD
    //      RTT_STRING
    //      RTT_WILDCARD
    //      RTT_EXPR_SUFFIX
    //      RTT_EXPR_LIST
    //      RTT_EXPR_CHAIN
    TVector<TRuleToken::TRef> Operands;

public:
    TStringBuf Str() const {
        return Source.Str();
    }
};

// ~~~~ TRuleParser ~~~~

class TRuleParser {
public:
    explicit TRuleParser(const TTextView& source);

    TRuleToken::TRef Parse();

private:
    void ProcessSuffixOperator();
    void ProcessConcatenate();
    void ProcessOperators();
    int GetBracesType(int openBrace, int closeBrace) const;
    void ReduceOperators(int toPriority);
    TRuleToken::TRef Reduce(size_t count, int tokenType);
    const TRuleToken::TRef& GetToken(int pos);
    void Check(bool condition) const;

private:
    const TTextView Source;
    TDeque<TRuleToken::TRef> Stack;
};

// ~~~~ TRuleLexer ~~~~

class TRuleLexer {
public:
    explicit TRuleLexer(const TTextView& source);

    TRuleToken::TRef GetCurrentToken();

    bool Next();

private:
    bool TryReadString();
    bool TryReadWord();
    static bool CanBeWord(char c);
    bool TryReadElement();
    static bool CanBeElementName(char symbol);
    bool TryRead(char symbol);
    bool TryRead(char symbol, int tokenType);
    bool TryRead(TStringBuf str, int tokenType);
    bool TryReadTagEnd();
    bool TryReadTagName();
    bool TryReadSuffixWithBrackets(char leftBracket, char rightBracket, ERuleTokenType type);
    bool TryReadComment();
    void SkipSpaces();
    bool TrySkip(char symbol);
    bool TrySkip(TStringBuf str);
    void CreateToken(int tokenType, size_t tokenBegin);
    void Check(bool condition) const;

private:
    const TTextView Source;
    const TStringBuf Line;
    size_t Pos = 0;
    bool IsEnd = false;
    TRuleToken::TRef CurrentToken;
    bool IsInsideTagName = false;
};

} // namespace NGranet::NCompiler

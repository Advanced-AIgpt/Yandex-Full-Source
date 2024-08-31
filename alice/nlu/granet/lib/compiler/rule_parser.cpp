#include "rule_parser.h"
#include "compiler_check.h"
#include <alice/nlu/granet/lib/sample/tag.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <dict/nerutil/tstimer.h>
#include <util/generic/cast.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/yexception.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>
#include <util/string/strip.h>
#include <util/string/cast.h>

namespace NGranet::NCompiler {

// ~~~~ NConst ~~~~

namespace NConst {

    static TSet<int> TermTypes = {
        RTT_RULE,
        RTT_ROUND_BRACKETS,
        RTT_SQUARE_BRACKETS,
        RTT_ELEMENT,
        RTT_WORD,
        RTT_STRING,
        RTT_WILDCARD,
        RTT_EXPR_SUFFIX,
        RTT_EXPR_LIST,
        RTT_EXPR_CHAIN
    };

    static TMap<int, int> OperatorsProirities = {
        {'(', 1}, {')', 1},
        {'[', 1}, {']', 1},
        {RTT_TAG_BODY_BEGIN, 1}, {RTT_TAG_BODY_END, 1},
        {RTT_RULE_BEGIN, 1}, {RTT_RULE_END, 1},
        {'|', 2},
        {RTT_CONCATENATE, 3}
    };
};

static inline bool IsSuffixOperator(int type) {
    return type == '+'
        || type == '?'
        || type == '*'
        || type == RTT_ANGLE_SUFFIX
        || type == RTT_TAG_NAME;
}

static inline bool IsOpenBrace(int type) {
    return type == '('
        || type == '['
        || type == RTT_TAG_BODY_BEGIN
        || type == RTT_RULE_BEGIN;
}

static inline bool IsCloseBrace(int type) {
    return type == ')'
        || type == ']'
        || type == RTT_TAG_BODY_END
        || type == RTT_RULE_END;
}

static inline bool IsTerm(int type) {
    return NConst::TermTypes.contains(type);
}

// ~~~~ TRuleParser ~~~~

TRuleParser::TRuleParser(const TTextView& source)
    : Source(source)
{
}

TRuleToken::TRef TRuleParser::Parse() {
    DEBUG_TIMER("TRuleParser::Parse");

    TRuleLexer lexer(Source);
    while (lexer.Next()) {
        Stack.push_back(lexer.GetCurrentToken());
        ProcessSuffixOperator();
        ProcessConcatenate();
        ProcessOperators();
    }
    Y_ENSURE(!Stack.empty());
    Check(Stack.size() == 1 && Stack.back()->Type == RTT_RULE);
    return Stack.back();
}

// Term ?                   ->  RTT_EXPR_SUFFIX
// Term *                   ->  RTT_EXPR_SUFFIX
// Term +                   ->  RTT_EXPR_SUFFIX
// Term RTT_ANGLE_SUFFIX    ->  RTT_EXPR_SUFFIX
// Term RTT_TAG_NAME        ->  RTT_EXPR_SUFFIX
void TRuleParser::ProcessSuffixOperator() {
    TRuleToken::TRef suffix = GetToken(-1);
    if (!IsSuffixOperator(suffix->Type)) {
        return;
    }
    TRuleToken::TRef operand = GetToken(-2);
    Check(IsTerm(operand->Type));
    TRuleToken::TRef reduced = Reduce(2, RTT_EXPR_SUFFIX);
    reduced->Operands.push_back(operand);
    reduced->Operands.push_back(suffix);
}

// Term1 Term2  ->  Term1 RTT_CONCATENATE Term2
// Term  (      ->  Term  RTT_CONCATENATE (
// Term  [      ->  Term  RTT_CONCATENATE [
void TRuleParser::ProcessConcatenate() {
    TRuleToken::TRef curr = GetToken(-1);
    if (!IsTerm(curr->Type) && !IsOpenBrace(curr->Type)) {
        return;
    }
    if (Stack.size() < 2) {
        return;
    }
    TRuleToken::TRef prev = GetToken(-2);
    if (!IsTerm(prev->Type)) {
        return;
    }
    TRuleToken::TRef concatenate = MakeIntrusive<TRuleToken>();
    concatenate->Type = RTT_CONCATENATE;
    concatenate->Source = curr->Source;
    concatenate->Source.SetInterval({prev->Source.GetEndPosition(), curr->Source.GetPosition()});
    Stack.insert(Stack.end() - 1, concatenate);
}

void TRuleParser::ProcessOperators() {
    TRuleToken::TRef curr = GetToken(-1);
    const int currType = curr->Type;

    if (!IsCloseBrace(currType) && currType != '|') {
        return;
    }
    const int priority = NConst::OperatorsProirities.at(currType);
    Stack.pop_back();
    ReduceOperators(priority);
    Stack.push_back(curr);

    if (!IsCloseBrace(currType)) {
        return;
    }

    if (IsOpenBrace(GetToken(-2)->Type)) {
        // Process braces without operand:
        //    (  )
        //   -2 -1
        TRuleToken::TRef openBrace = GetToken(-2);
        const int reducedType = GetBracesType(openBrace->Type, currType);
        Reduce(2, reducedType);
        return;
    }

    // Braces with operand:
    //    ( Term )
    //   -3 -2  -1
    TRuleToken::TRef term = GetToken(-2);
    TRuleToken::TRef openBrace = GetToken(-3);
    Check(IsTerm(term->Type));
    Check(IsOpenBrace(openBrace->Type));
    const int reducedType = GetBracesType(openBrace->Type, currType);
    TRuleToken::TRef reduced = Reduce(3, reducedType);
    reduced->Operands.push_back(term);
}

int TRuleParser::GetBracesType(int openBrace, int closeBrace) const {
    if (closeBrace == ')') {
        Check(openBrace == '(');
        return RTT_ROUND_BRACKETS;
    } else if (closeBrace == ']') {
        Check(openBrace == '[');
        return RTT_SQUARE_BRACKETS;
    } else if (closeBrace == RTT_TAG_BODY_END) {
        Check(openBrace == RTT_TAG_BODY_BEGIN);
        return RTT_ROUND_BRACKETS;
    } else if (closeBrace == RTT_RULE_END) {
        Check(openBrace == RTT_RULE_BEGIN);
        return RTT_RULE;
    } else {
        Check(false);
        return 0;
    }
}

void TRuleParser::ReduceOperators(int toPriority) {
    while (Stack.size() >= 2) {
        const int operatorType = GetToken(-2)->Type;
        Check(NConst::OperatorsProirities.contains(operatorType));
        if (NConst::OperatorsProirities.at(operatorType) <= toPriority) {
            break;
        }

        Check(Stack.size() >= 3);
        TRuleToken::TRef operand1 = GetToken(-3);
        TRuleToken::TRef operand2 = GetToken(-1);
        Check(IsTerm(operand1->Type));
        Check(IsTerm(operand2->Type));

        Y_ENSURE(operatorType == '|' || operatorType == RTT_CONCATENATE);
        const int reducedType = operatorType == '|' ? RTT_EXPR_LIST : RTT_EXPR_CHAIN;

        TRuleToken::TRef reduced = Reduce(3, reducedType);
        reduced->Operands.push_back(operand1);
        if (operand2->Type == reducedType) {
            Y_ENSURE(reduced->Children.back() == operand2);
            reduced->Children.pop_back();
            Extend(operand2->Children, &reduced->Children);
            Extend(operand2->Operands, &reduced->Operands);
        } else {
            reduced->Operands.push_back(operand2);
        }
    }
}

TRuleToken::TRef TRuleParser::Reduce(size_t count, int tokenType) {
    Y_ENSURE(0 < count && count <= Stack.size());
    TRuleToken::TRef token = MakeIntrusive<TRuleToken>();
    token->Type = tokenType;
    token->Children = TVector<TRuleToken::TRef>(Stack.end() - count, Stack.end());
    token->Source = Merge(token->Children.front()->Source, token->Children.back()->Source);
    Stack.erase(Stack.end() - count, Stack.end());
    Stack.push_back(token);
    return Stack.back();
}

const TRuleToken::TRef& TRuleParser::GetToken(int pos) {
    const int actual = Stack.ysize() + pos;
    Check(actual >= 0);
    return Stack.at(actual);
}

void TRuleParser::Check(bool condition) const {
    Y_ENSURE(!Stack.empty());
    GRANET_COMPILER_CHECK(condition, Stack.back()->Source, MSG_UNEXPECTED_TOKEN);
}

// ~~~~ TRuleLexer ~~~~

TRuleLexer::TRuleLexer(const TTextView& source)
    : Source(source)
    , Line(source.Str())
{
}

TRuleToken::TRef TRuleLexer::GetCurrentToken() {
    return CurrentToken;
}

bool TRuleLexer::Next() {
    if (IsEnd) {
        return false;
    }
    if (CurrentToken == nullptr) {
        Y_ENSURE(Pos == 0);
        CreateToken(RTT_RULE_BEGIN, 0);
        return true;
    }
    SkipSpaces();
    if (Pos == Line.length()) {
        CreateToken(RTT_RULE_END, Pos);
        IsEnd = true;
        return true;
    }
    const bool isOk = TryReadTagName()
        || TryReadString()
        || TryReadWord()
        || TryReadElement()
        || TryReadTagEnd()
        || TryRead('\'', RTT_TAG_BODY_BEGIN)
        || TryRead('.', RTT_WILDCARD)
        || TryRead('|')
        || TryRead('(')
        || TryRead(')')
        || TryRead('[')
        || TryRead(']')
        || TryRead('?')
        || TryRead('*')
        || TryRead('+')
        || TryReadSuffixWithBrackets('<', '>', RTT_ANGLE_SUFFIX)
        || TryReadComment();
    Check(isOk);
    return true;
}

bool TRuleLexer::TryReadString() {
    const size_t start = Pos;
    if (!TrySkip('"')) {
        return false;
    }
    bool isEscaped = false;
    while (Pos < Line.length()) {
        if (!isEscaped) {
            const char c = Line[Pos];
            if (c == '\"') {
                break;
            } else if (c == '\\') {
                isEscaped = true;
            }
        }
        Pos++;
    }
    Check(TrySkip('"'));
    CreateToken(RTT_STRING, start);
    return true;
}

bool TRuleLexer::TryReadWord() {
    const size_t start = Pos;
    while (Pos < Line.length() && CanBeWord(Line[Pos])) {
        Pos++;
    }
    if (start == Pos) {
        return false;
    }
    CreateToken(RTT_WORD, start);
    return true;
}

// TODO(samoylovboris) Strict syntax: Allow not first dot
// static
bool TRuleLexer::CanBeWord(char c) {
    return static_cast<unsigned char>(c) >= 0x80u || IsAsciiAlnum(c) || c == '_' || c == '-';
}

bool TRuleLexer::TryReadElement() {
    const size_t start = Pos;
    if (!TrySkip('$')) {
        return false;
    }
    while (Pos < Line.length() && CanBeElementName(Line[Pos])) {
        Pos++;
    }
    CreateToken(RTT_ELEMENT, start);
    return true;
}

// TODO(samoylovboris) Strict syntax: Check element, entity and form names
// TODO(samoylovboris) Strict syntax: Check not quoted value
// static
bool TRuleLexer::CanBeElementName(char c) {
    return IsAsciiAlnum(c) || c == '_' || c == '-' || c == '.';
}

bool TRuleLexer::TryRead(char symbol) {
    return TryRead(symbol, symbol);
}

bool TRuleLexer::TryRead(char symbol, int tokenType) {
    const size_t start = Pos;
    if (!TrySkip(symbol)) {
        return false;
    }
    CreateToken(tokenType, start);
    return true;
}

bool TRuleLexer::TryRead(TStringBuf str, int tokenType) {
    const size_t start = Pos;
    if (!TrySkip(str)) {
        return false;
    }
    CreateToken(tokenType, start);
    return true;
}

bool TRuleLexer::TryReadTagEnd() {
    if (!TryRead(TStringBuf("\'("), RTT_TAG_BODY_END)) {
        return false;
    }
    IsInsideTagName = true;
    return true;
}

bool TRuleLexer::TryReadTagName() {
    if (!IsInsideTagName) {
        return false;
    }
    const size_t start = Pos;
    // TODO(samoylovboris) Strict syntax: Check by regexp: (identifier_or_quoted[/identifier_or_quoted][:identifier_or_quoted])
    Pos += ScanTagName(Line.SubStr(Pos));
    CreateToken(RTT_TAG_NAME, start);
    Check(TrySkip(')'));
    IsInsideTagName = false;
    return true;
}

bool TRuleLexer::TryReadSuffixWithBrackets(char leftBracket, char rightBracket, ERuleTokenType type) {
    if (!TrySkip(leftBracket)) {
        return false;
    }
    const size_t start = Pos;
    while (Pos < Line.length() && Line[Pos] != rightBracket) {
        Pos++;
    }
    CreateToken(type, start);
    Check(TrySkip(rightBracket));
    return true;
}

bool TRuleLexer::TryReadComment() {
    const size_t start = Pos;
    if (!TrySkip('#')) {
        return false;
    }
    Pos = Line.length();
    CreateToken(RTT_RULE_END, start);
    IsEnd = true;
    return true;
}

void TRuleLexer::SkipSpaces() {
    while (Pos < Line.length() && IsAsciiSpace(Line[Pos])) {
        Pos++;
    }
}

bool TRuleLexer::TrySkip(char symbol) {
    if (Pos < Line.length() && Line[Pos] == symbol) {
        Pos++;
        return true;
    }
    return false;
}

bool TRuleLexer::TrySkip(TStringBuf str) {
    if (Line.Tail(Pos).StartsWith(str)) {
        Pos += str.length();
        return true;
    }
    return false;
}

void TRuleLexer::CreateToken(int tokenType, size_t tokenBegin) {
    Y_ENSURE(tokenBegin <= Pos);
    CurrentToken = MakeIntrusive<TRuleToken>();
    CurrentToken->Type = tokenType;
    CurrentToken->Source = Source.SubStr(tokenBegin, Pos - tokenBegin);
}

void TRuleLexer::Check(bool condition) const {
    GRANET_COMPILER_CHECK(condition, Source.Tail(Pos), MSG_UNEXPECTED_SYMBOL);
}

} // namespace NGranet::NCompiler

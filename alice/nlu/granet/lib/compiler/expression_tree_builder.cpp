#include "element_modification.h"
#include "expression_tree_builder.h"
#include "compiler_check.h"
#include "syntax.h"
#include <alice/nlu/granet/lib/lang/inflector.h>
#include <alice/nlu/granet/lib/lang/string_with_weight.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/libs/lemmatization/lemmatize.h>
#include <alice/nlu/libs/tokenization/tokenizer.h>
#include <dict/nerutil/tstimer.h>
#include <library/cpp/iterator/zip.h>
#include <util/generic/cast.h>
#include <util/string/escape.h>
#include <util/string/join.h>
#include <util/string/split.h>

namespace NGranet::NCompiler {

static const size_t BRACKET_OPENING_LIMIT = 100;

static bool IsSimpleListParams(const TListItemParams& params) {
    return !params.IsForced && !params.IsNegative && params.DataType == 0 && params.DataValue == 0;
}

static bool IsSimpleList(const TExpressionNode::TRef& node) {
    return node->Type == ENT_LIST && AllOf(node->AsList()->Params, &IsSimpleListParams);
}

TExpressionTreeBuilder::TExpressionTreeBuilder(const TTextView& rootSource, ELanguage lang,
        const TSourcesLoader& sourcesLoader, TStringPool* stringPool)
    : Lang(lang)
    , SourcesLoader(sourcesLoader)
    , StringPool(stringPool)
    , Tree(MakeIntrusive<TListExpressionNode>(rootSource))
{
    Y_ENSURE(StringPool);
}

void TExpressionTreeBuilder::DisableOptimization() {
    IsOptimizationEnabled = false;
}

void TExpressionTreeBuilder::ResetOptionsAccumulator(const TCompilerOptionsAccumulator& options) {
    Options = options;
}

void TExpressionTreeBuilder::ReadLines(const TVector<TSrcLine::TRef>& lines) {
    for (const TSrcLine::TRef& line : lines) {
        if (!line->IsParent) {
            ReadLine(line->Source);
        }
    }
}

void TExpressionTreeBuilder::ReadLine(const TTextView& line) {
    DEBUG_TIMER("TExpressionTreeBuilder::ReadLine");
    Y_ENSURE(!IsFinalized);

    // Read directive %include_raw
    if (TryIncludeRawLines(line)) {
        Options.ContextFlags |= DCF_BETWEEN_RULES;
        return;
    }

    // Read other directives
    if (TryReadDirective(line, StringPool, &Options)) {
        return;
    }

    // Read rule
    TRuleToken::TConstRef tokenTree = TRuleParser(line).Parse();
    Y_ENSURE(tokenTree);
    Y_ENSURE(tokenTree->Type == RTT_RULE);
    if (tokenTree->Operands.empty()) {
        return;
    }

    AddTopLevelLine(CreateTree(tokenTree, false));

    Options.ContextFlags |= DCF_BETWEEN_RULES;
}

// Process directive include_text:
//   %include_raw path/to/file
bool TExpressionTreeBuilder::TryIncludeRawLines(const TTextView& source) {
    TStringBuf rest = source.Str();
    if (rest.NextTok(' ') != NSyntax::NDirective::IncludeRaw) {
        return false;
    }
    const TFsPath path = SafeUnquote(source, rest, &NSyntax::NParamRegEx::NotQuotedPath);
    const TString text = SourcesLoader.ReadFileAsString(source, path);
    for (TStringBuf line : StringSplitter(text).Split('\n')) {
        TExpressionNode::TRef node = TryCreateTextNode(source, line);
        if (node == nullptr) {
            continue;
        }
        AddTopLevelLine(node);
    }
    return true;
}

void TExpressionTreeBuilder::AddTopLevelLine(TExpressionNode::TRef node) {
    Optimize(node);

    const TMaybe<TVector<TGramBitSet>> gramsList = MakeModificationGramsList(Options.CompilerFlags, Options.CustomInflection);
    if (gramsList.Defined()) {
        Inflect(node, *gramsList);
    }

    const TListItemParams params = {
        .Weight = Options.Weight,
        .IsForced = Options.CompilerFlags.HasFlags(CF_FORCED),
        .IsNegative = Options.CompilerFlags.HasFlags(CF_NEGATIVE),
        .DataType = Options.DataType,
        .DataValue = Options.DataValue,
    };
    Tree->AsList()->AddOperand(node, params);
}

TExpressionNode::TRef TExpressionTreeBuilder::CreateTree(TRuleToken::TConstRef token, bool treatChainAsBag) {
    Y_ENSURE(token);

    const ERuleTokenType type = static_cast<ERuleTokenType>(token->Type);
    const TVector<TRuleToken::TRef>& operands = token->Operands;
    const TTextView& source = token->Source;

    if (type == RTT_RULE) {
        Y_ENSURE(operands.size() == 1);
        return CreateTree(operands[0], false);
    }

    if (type == RTT_ROUND_BRACKETS) {
        GRANET_COMPILER_CHECK(!operands.empty(), source, MSG_EMPTY_EXPRESSION);
        Y_ENSURE(operands.size() == 1);
        return CreateTree(operands[0], false);
    }

    if (type == RTT_SQUARE_BRACKETS) {
        GRANET_COMPILER_CHECK(!operands.empty(), source, MSG_EMPTY_EXPRESSION);
        Y_ENSURE(operands.size() == 1);
        return CreateTree(operands[0], true);
    }

    if (type == RTT_EXPR_SUFFIX) {
        Y_ENSURE(operands.size() == 2);
        return CreateSuffixNode(source, operands[0], operands[1]);
    }

    if (type == RTT_WILDCARD) {
        return MakeIntrusive<TWordExpressionNode>(source, ".", false);
    }

    if (type == RTT_WORD || type == RTT_STRING) {
        Y_ENSURE(operands.empty());
        return CreateTextNode(token);
    }

    if (type == RTT_ELEMENT) {
        Y_ENSURE(operands.empty());
        return MakeIntrusive<TElementExpressionNode>(source, TString(token->Str()));
    }

    if (type == RTT_EXPR_LIST) {
        TIntrusivePtr<TListExpressionNode> node = MakeIntrusive<TListExpressionNode>(source);
        for (const TRuleToken::TRef& operand : operands) {
            node->AddOperand(CreateTree(operand, false), {});
        }
        CheckHasOperands(node.Get());
        return node;
    }

    if (type == RTT_EXPR_CHAIN) {
        if (treatChainAsBag) {
            return CreateBagNode(token);
        } else {
            return CreateChainNode(token);
        }
    }

    Y_ENSURE(false);
    return nullptr;
}

TExpressionNode::TRef TExpressionTreeBuilder::CreateSuffixNode(const TTextView& source,
    const TRuleToken::TConstRef& operand, const TRuleToken::TConstRef& suffix)
{
    TExpressionNode::TRef subTree = CreateTree(operand, false);

    if (suffix->Type == '?') {
        return MakeIntrusive<TQuantifierExpressionNode>(source, subTree, TQuantityParams{0, 1});
    }
    if (suffix->Type == '*') {
        return MakeIntrusive<TQuantifierExpressionNode>(source, subTree, TQuantityParams{0, Max<ui8>()});
    }
    if (suffix->Type == '+') {
        return MakeIntrusive<TQuantifierExpressionNode>(source, subTree, TQuantityParams{1, Max<ui8>()});
    }

    if (suffix->Type == RTT_ANGLE_SUFFIX) {
        TQuantityParams params;
        if (TryReadQuantityRange(suffix->Str(), &params)) {
            GRANET_COMPILER_CHECK(params.MaxCount > 0 && params.MaxCount >= params.MinCount,
                suffix->Source, MSG_INVALID_QUANTIFIER);
            return MakeIntrusive<TQuantifierExpressionNode>(source, subTree, params);
        }

        const bool isOperandElement = operand->Type == RTT_ELEMENT && subTree->Type == ENT_ELEMENT;
        const bool isOperandModifier = operand->Type == RTT_EXPR_SUFFIX && subTree->Type == ENT_MODIFIER;
        GRANET_COMPILER_CHECK(isOperandElement || isOperandModifier, source, MSG_MODIFIER_ONLY_FOR_ELEMENTS);
        return MakeIntrusive<TModifierExpressionNode>(source, subTree, suffix->Str());
    }

    if (suffix->Type == RTT_TAG_NAME) {
        TIntrusivePtr<TChainExpressionNode> chain = MakeIntrusive<TChainExpressionNode>(source);
        chain->Operands.push_back(MakeIntrusive<TTagBeginExpressionNode>(source));
        chain->Operands.push_back(subTree);
        chain->Operands.push_back(MakeIntrusive<TTagEndExpressionNode>(suffix->Source, suffix->Str()));
        return chain;
    }

    Y_ENSURE(false);
    return nullptr;
}

// static
bool TExpressionTreeBuilder::TryReadQuantityRange(TStringBuf str, TQuantityParams* params) {
    Y_ENSURE(params);

    const TVector<TString> parts = SplitAndStrip(str, ',');
    if (parts.size() == 1) {
        if (TryFromString(parts[0], params->MinCount)) {
            params->MaxCount = params->MinCount;
            return true;
        }
        return false;
    }
    if (parts.size() == 2) {
        const bool hasMin = TryFromString(parts[0], params->MinCount);
        const bool hasMax = TryFromString(parts[1], params->MaxCount);
        if (hasMin && hasMax) {
            return true;
        }
        if (hasMin && parts[1].empty()) {
            params->MaxCount = Max<ui8>();
            return true;
        }
        if (hasMax && parts[0].empty()) {
            params->MinCount = 0;
            return true;
        }
        return false;
    }
    return false;
}

TExpressionNode::TRef TExpressionTreeBuilder::CreateTextNode(TRuleToken::TConstRef token) {
    Y_ENSURE(token);
    Y_ENSURE(token->Type == RTT_WORD || token->Type == RTT_STRING);

    const TString text = token->Type == RTT_STRING ? UnescapeC(token->Str()) : TString(token->Str());
    TExpressionNode::TRef node = TryCreateTextNode(token->Source, text);
    GRANET_COMPILER_CHECK(node != nullptr, token->Source, MSG_NO_WORDS_AFTER_NORMALIZATION);
    return node;
}

TExpressionNode::TRef TExpressionTreeBuilder::TryCreateTextNode(const TTextView& source, TStringBuf text) {
    const TVector<TString> tokens = NNlu::TSmartTokenizer(text, Lang).GetNormalizedTokens();
    if (tokens.empty()) {
        return nullptr;
    }
    if (tokens.size() == 1) {
        return CreateWordNode(source, tokens[0]);
    }
    TIntrusivePtr<TChainExpressionNode> chain = MakeIntrusive<TChainExpressionNode>(source);
    for (const TString& token : tokens) {
        chain->Operands.push_back(CreateWordNode(source, token));
    }
    return chain;
}

TExpressionNode::TRef TExpressionTreeBuilder::CreateWordNode(const TTextView& source, TStringBuf word) {
    if (Options.CompilerFlags.HasFlags(CF_EXACT)
        || !HasIntersection(Options.CompilerFlags, CF_LEMMA_FLAGS))
    {
        return MakeIntrusive<TWordExpressionNode>(source, word, false);
    }
    if (Options.CompilerFlags.HasFlags(CF_LEMMA_AS_IS)) {
        return MakeIntrusive<TWordExpressionNode>(source, word, true);
    }
    const double threshold = Options.CompilerFlags.HasFlags(CF_LEMMA_BEST) ? NNlu::SURE_LEMMA_THRESHOLD : NNlu::GOOD_LEMMA_THRESHOLD;
    const TVector<TString> lemmas = NNlu::LemmatizeWord(word, Lang, threshold);
    if (lemmas.size() == 1) {
        return MakeIntrusive<TWordExpressionNode>(source, lemmas[0], true);
    }
    Y_ENSURE(lemmas.size() > 1);
    TIntrusivePtr<TListExpressionNode> list = MakeIntrusive<TListExpressionNode>(source);
    for (const TString& lemma : lemmas) {
        list->AddOperand(MakeIntrusive<TWordExpressionNode>(source, lemma, true), {});
    }
    return list;
}

TExpressionNode::TRef TExpressionTreeBuilder::CreateBagNode(const TRuleToken::TConstRef& token) {
    Y_ENSURE(token);
    Y_ENSURE(token->Type == RTT_EXPR_CHAIN);

    TIntrusivePtr<TBagExpressionNode> bag = MakeIntrusive<TBagExpressionNode>(token->Source);

    for (const TRuleToken::TRef& operand : token->Operands) {
        TRuleToken::TConstRef actualOperand = operand;
        TBagItemParams operandParams = {true, true};

        if (operand->Type == RTT_EXPR_SUFFIX) {
            Y_ENSURE(operand->Operands.size() == 2);
            TRuleToken::TConstRef suffixOperand = operand->Operands[0];
            const int suffixType = operand->Operands[1]->Type;
            if (suffixType == '?') {
                operandParams = {false, true};
                actualOperand = suffixOperand;
            } else if (suffixType == '*') {
                operandParams = {false, false};
                actualOperand = suffixOperand;
            } else if (suffixType == '+') {
                operandParams = {true, false};
                actualOperand = suffixOperand;
            }
        }
        bag->AddOperand(CreateTree(actualOperand, false), operandParams);
    }
    // 32 is number of bits in SetOfRequiredRules and SetOfLimitedRules of TGrammarElement.
    GRANET_COMPILER_CHECK(bag->Operands.size() <= 32, bag->Source, MSG_TOO_MANY_OPERANDS_IN_BAG);
    CheckHasOperands(bag.Get());
    return bag;
}

TExpressionNode::TRef TExpressionTreeBuilder::CreateChainNode(const TRuleToken::TConstRef& token) {
    Y_ENSURE(token);
    Y_ENSURE(token->Type == RTT_EXPR_CHAIN);

    TIntrusivePtr<TChainExpressionNode> chain = MakeIntrusive<TChainExpressionNode>(token->Source);
    for (const TRuleToken::TRef& operand : token->Operands) {
        chain->Operands.push_back(CreateTree(operand, false));
    }
    CheckHasOperands(chain.Get());
    return chain;
}

void TExpressionTreeBuilder::CheckHasOperands(const TExpressionNode* node) const {
    Y_ENSURE(node);
    GRANET_COMPILER_CHECK(!node->Operands.empty(), node->Source, MSG_EMPTY_EXPRESSION);
}

// ~~~~ Inflect ~~~~

void TExpressionTreeBuilder::Inflect(TExpressionNode::TRef& node, const TVector<TGramBitSet>& gramsList) const {
    const TString text = TryConvertNodeToString(node);
    if (!text.empty()) {
        node = InflectStringToNode(node->Source, text, gramsList);
        return;
    }
    for (TExpressionNode::TRef& operand : node->Operands) {
        Inflect(operand, gramsList);
    }
}

// static
TString TExpressionTreeBuilder::TryConvertNodeToString(const TExpressionNode::TRef& node) {
    if (node->Type == ENT_WORD) {
        return TryConvertWordNodesToString({node});
    }
    if (node->Type == ENT_CHAIN) {
        return TryConvertWordNodesToString(node->Operands);
    }
    return "";
}

// static
TString TExpressionTreeBuilder::TryConvertWordNodesToString(const TVector<TExpressionNode::TRef>& nodes) {
    TVector<TString> words;
    for (const TExpressionNode::TRef& node : nodes) {
        if (node->Type != ENT_WORD || node->AsWord()->IsLemma || node->AsWord()->Word == TStringBuf(".")) {
            return "";
        }
        words.push_back(node->AsWord()->Word);
    }
    return JoinSeq(" ", words);
}

TExpressionNode::TRef TExpressionTreeBuilder::InflectStringToNode(const TTextView& source, TStringBuf text,
    const TVector<TGramBitSet>& gramsList) const
{
    if (gramsList.size() == 1) {
        return InflectStringToNode(source, text, gramsList.front());
    }
    TIntrusivePtr<TListExpressionNode> list = MakeIntrusive<TListExpressionNode>(source);
    for (const TGramBitSet& grams : gramsList) {
        list->AddOperand(InflectStringToNode(source, text, grams), {});
    }
    return list;
}

TExpressionNode::TRef TExpressionTreeBuilder::InflectStringToNode(const TTextView& source, TStringBuf text,
    const TGramBitSet& grams) const
{
    TString inflectedText = InflectPhrase(text, Lang, grams);
    if (inflectedText.empty()) {
        inflectedText = TString(text);
    }
    TIntrusivePtr<TChainExpressionNode> chain = MakeIntrusive<TChainExpressionNode>(source);
    for (const TString& word : NNlu::TSmartTokenizer(inflectedText, Lang).GetNormalizedTokens()) {
        chain->Operands.push_back(MakeIntrusive<TWordExpressionNode>(source, word, false));
    }
    return chain;
}

// ~~~~ Optimize ~~~~

void TExpressionTreeBuilder::Optimize(TExpressionNode::TRef& node) const {
    Y_ENSURE(node);
    if (!IsOptimizationEnabled) {
        return;
    }
    for (TExpressionNode::TRef& operand : node->Operands) {
        Optimize(operand);
    }

    // Legend:
    //  [a b]    - ENT_BAG
    //  (a b)    - ENT_CHAIN
    //  {a|b}    - ENT_LIST
    //  a?       - ENT_QUANTIFIER
    //  a        - any type

    // ENT_BAG              ->  ENT_LIST
    // [a b]                ->  {(a b)|(b a)}
    // [a (b c)]            ->  {(a (b c))|((b c) a)}  ->  {(a b c)|(b c a)}
    // [a {b|c}]            ->  {(a {b|c})|({b|c} a)}  ->  {{(a b)|(a c)}|{(b a)|(c a)}}
    OptimizeSimpleBag(node);

    // ENT_QUANTIFIER       ->  ENT_LIST
    // a?                   ->  {()|a}
    // (a b)?               ->  {()|(a b)}
    // {a|b}?               ->  {()|{a|b}}
    OptimizeQuestion(node);

    // ENT_CHAIN            ->  ENT_CHAIN
    // (a (b c) d)          ->  (a b c d)
    // (a (b) c () d)       ->  (a b c d)
    OptimizeChainInsideChain(node);

    // ENT_CHAIN            ->  ENT_LIST
    // (a {(b)|(c d)} e)    ->  {(a b e)|(a c d e)}
    // (a {b|(c d)} e)      ->  {(a b e)|(a c d e)}
    // (a {()|(c d)} e)     ->  {(a e)|(a c d e)}
    // (a {b|c} d {e|f})    ->  {(a b d e)|(a b d f)|(a c d e)|(a c d f)}
    // ({a|b})              ->  {a|b}
    OptimizeListInsideChain(node);

    // ENT_LIST             ->  ENT_LIST
    // {a|{b|c}|d}          ->  {a|b|c|d}
    OptimizeListInsideList(node);
}

// ENT_BAG              ->  ENT_LIST
// [a b]                ->  {(a b)|(b a)}
// [a (b c)]            ->  {(a (b c))|((b c) a)}  ->  {(a b c)|(b c a)}
// [a {b|c}]            ->  {(a {b|c})|({b|c} a)}  ->  {{(a b)|(a c)}|{(b a)|(c a)}}
// static
void TExpressionTreeBuilder::OptimizeSimpleBag(TExpressionNode::TRef& node) {
    Y_ENSURE(node);

    if (node->Type != ENT_BAG) {
        return;
    }
    const TBagExpressionNode* bag = node->AsBag();
    if (bag->Operands.size() != 2
        || bag->Params[0] != BAG_PARAM_EMPTY
        || bag->Params[1] != BAG_PARAM_EMPTY)
    {
        return;
    }

    TIntrusivePtr<TListExpressionNode> list = MakeIntrusive<TListExpressionNode>(node->Source);

    TIntrusivePtr<TChainExpressionNode> chain1 = MakeIntrusive<TChainExpressionNode>(node->Source);
    chain1->Operands.push_back(node->Operands[0]->DeepCopy());
    chain1->Operands.push_back(node->Operands[1]->DeepCopy());
    list->AddOperand(chain1, {});

    TIntrusivePtr<TChainExpressionNode> chain2 = MakeIntrusive<TChainExpressionNode>(node->Source);
    chain2->Operands.push_back(node->Operands[1]->DeepCopy());
    chain2->Operands.push_back(node->Operands[0]->DeepCopy());
    list->AddOperand(chain2, {});

    node = list;

    for (TExpressionNode::TRef& operand : node->Operands) {
        OptimizeChainInsideChain(operand);
        OptimizeListInsideChain(operand);
    }
}

// ENT_QUANTIFIER       ->  ENT_LIST
// a?                   ->  {()|a}
// (a b)?               ->  {()|(a b)}
// {a|b}?               ->  {()|{a|b}}
// static
void TExpressionTreeBuilder::OptimizeQuestion(TExpressionNode::TRef& node) {
    Y_ENSURE(node);

    if (node->Type != ENT_QUANTIFIER) {
        return;
    }
    const TQuantityParams& params = node->AsQuantifier()->Params;
    if (params.MinCount != 0 || params.MaxCount != 1) {
        return;
    }
    TIntrusivePtr<TListExpressionNode> list = MakeIntrusive<TListExpressionNode>(node->Source);
    list->AddOperand(MakeIntrusive<TChainExpressionNode>(node->Source), {});
    list->AddOperand(node->Operands[0], {});
    node = list;
}

// ENT_CHAIN            ->  ENT_CHAIN
// (a (b c) d)          ->  (a b c d)
// (a (b) c () d)       ->  (a b c d)
// static
void TExpressionTreeBuilder::OptimizeChainInsideChain(TExpressionNode::TRef& node) {
    Y_ENSURE(node);
    if (node->Type != ENT_CHAIN || !node->HasOperandOfType(ENT_CHAIN)) {
        return;
    }
    TIntrusivePtr<TChainExpressionNode> newChain = MakeIntrusive<TChainExpressionNode>(node->Source);
    for (const TExpressionNode::TRef& operand : node->Operands) {
        ExtendChainOptimized(operand, false, newChain.Get());
    }
    node = newChain;
}

// ENT_CHAIN            ->  ENT_LIST
// (a {(b)|(c d)} e)    ->  {(a b e)|(a c d e)}
// (a {b|(c d)} e)      ->  {(a b e)|(a c d e)}
// (a {()|(c d)} e)     ->  {(a e)|(a c d e)}
// (a {b|c} d {e|f})    ->  {(a b d e)|(a b d f)|(a c d e)|(a c d f)}
// ({a|b})              ->  {a|b}
// static
void TExpressionTreeBuilder::OptimizeListInsideChain(TExpressionNode::TRef& node) {
    Y_ENSURE(node);

    if (node->Type != ENT_CHAIN || !node->HasOperandOfType(ENT_LIST)) {
        return;
    }
    TChainExpressionNode* oldChain = node->AsChain();
    TIntrusivePtr<TListExpressionNode> newList = MakeIntrusive<TListExpressionNode>(node->Source);
    newList->AddOperand(MakeIntrusive<TChainExpressionNode>(node->Source), {});

    for (const TExpressionNode::TRef& operand : oldChain->Operands) {
        if (IsSimpleList(operand)) {
            const TListExpressionNode* subList = operand->AsList();
            if (newList->Operands.size() * subList->Operands.size() <= BRACKET_OPENING_LIMIT) {
                newList = CartesianProductLists(newList->Source, newList.Get(), subList);
                continue;
            }
        }
        for (const TExpressionNode::TRef& chain : newList->Operands) {
            const bool shouldCopyOperand = newList->Operands.size() > 1;
            ExtendChainOptimized(operand, shouldCopyOperand, chain->AsChain());
        }
    }
    node = newList;
}

// static
TIntrusivePtr<TListExpressionNode> TExpressionTreeBuilder::CartesianProductLists(const TTextView& source,
    const TListExpressionNode* list1, const TListExpressionNode* list2)
{
    Y_ENSURE(list1);
    Y_ENSURE(list2);

    TIntrusivePtr<TListExpressionNode> product = MakeIntrusive<TListExpressionNode>(source);

    for (const auto& [rule1, params1] : Zip(list1->Operands, list1->Params)) {
        for (const auto& [rule2, params2] : Zip(list2->Operands, list2->Params)) {
            Y_ENSURE(IsSimpleListParams(params2));
            product->Params.emplace_back(params1).Weight *= params2.Weight;
            const TExpressionNode::TRef joinedRule = product->Operands.emplace_back(rule1->DeepCopy());
            ExtendChainOptimized(rule2, true, joinedRule->AsChain());
        }
    }
    return product;
}

// static
void TExpressionTreeBuilder::ExtendChainOptimized(TExpressionNode::TRef operand, bool shouldCopy,
    TChainExpressionNode* chain)
{
    Y_ENSURE(chain);
    if (shouldCopy) {
        operand = operand->DeepCopy();
    }
    if (operand->Type != ENT_CHAIN) {
        chain->Operands.push_back(operand);
    } else {
        Extend(operand->Operands, &chain->Operands);
    }
}

// ENT_LIST             ->  ENT_LIST
// {a|{b|c}|d}          ->  {a|b|c|d}
// static
void TExpressionTreeBuilder::OptimizeListInsideList(TExpressionNode::TRef& node) {
    Y_ENSURE(node);

    if (node->Type != ENT_LIST || !node->HasOperandOfType(ENT_LIST)) {
        return;
    }
    TListExpressionNode* oldList = node->AsList();
    TIntrusivePtr<TListExpressionNode> newList = MakeIntrusive<TListExpressionNode>(node->Source);

    for (const auto& [operand, params] : Zip(oldList->Operands, oldList->Params)) {
        if (!IsSimpleList(operand)) {
            newList->AddOperand(operand, params);
            continue;
        }
        if (operand->Operands.empty()) {
            continue;
        }
        TListExpressionNode* nestedList = operand->AsList();
        if (oldList->Operands.size() > 1) {
            nestedList->NormalizeWeightSum(params.Weight);
        }
        for (const auto& [nestedOperand, nestedParams] : Zip(nestedList->Operands, nestedList->Params)) {
            newList->AddOperand(nestedOperand, params);
            newList->Params.back().Weight = nestedParams.Weight;
        }
    }
    node = newList;
}

// ~~~~ Finalize ~~~~

void TExpressionTreeBuilder::Finalize() {
    DEBUG_TIMER("TExpressionTreeBuilder::Finalize");
    Y_ENSURE(!IsFinalized);

    Optimize(Tree);
    Normalize(Tree);
    IsFinalized = true;
}

// Ensure constrains:
//   Operand of ENT_LIST must be ENT_CHAIN.
//   Operand of ENT_BAG must be ENT_CHAIN.
//   Operand of ENT_CHAIN can't be ENT_CHAIN.
//   Operand of ENT_QUANTIFIER must be ENT_LIST.
// static
void TExpressionTreeBuilder::Normalize(TExpressionNode::TRef& node) {
    Y_ENSURE(node);

    if (node->Type == ENT_LIST || node->Type == ENT_BAG) {
        // Operand of ENT_LIST and ENT_BAG must be ENT_CHAIN.
        for (TExpressionNode::TRef& operand : node->Operands) {
            WrapByChainIfNeed(operand);
        }
    } else if (node->Type == ENT_CHAIN && node->HasOperandOfType(ENT_CHAIN)) {
        // Operand of ENT_CHAIN can't be ENT_CHAIN.
        for (TExpressionNode::TRef& operand : node->Operands) {
            Normalize(operand);
        }
        OptimizeChainInsideChain(node);
    } else if (node->Type == ENT_QUANTIFIER) {
        // Operand of ENT_QUANTIFIER must be ENT_LIST.
        Y_ENSURE(node->Operands.size() == 1);
        TExpressionNode::TRef& operand = node->Operands[0];
        if (operand->Type != ENT_LIST) {
            WrapByChainIfNeed(operand);
            WrapByListIfNeed(operand);
        }
    }

    for (TExpressionNode::TRef& operand : node->Operands) {
        Normalize(operand);
    }
}

// static
void TExpressionTreeBuilder::WrapByChainIfNeed(TExpressionNode::TRef& node) {
    Y_ENSURE(node);
    if (node->Type == ENT_CHAIN) {
        return;
    }
    TIntrusivePtr<TChainExpressionNode> chain = MakeIntrusive<TChainExpressionNode>(node->Source);
    chain->Operands.push_back(node);
    node = chain;
}

// static
void TExpressionTreeBuilder::WrapByListIfNeed(TExpressionNode::TRef& node) {
    Y_ENSURE(node);
    if (node->Type == ENT_LIST) {
        return;
    }
    TIntrusivePtr<TListExpressionNode> list = MakeIntrusive<TListExpressionNode>(node->Source);
    list->AddOperand(node, {});
    node = list;
}

TExpressionNode::TRef TExpressionTreeBuilder::GetTree() const {
    Y_ENSURE(IsFinalized);
    return Tree;
}

const TCompilerOptionsAccumulator& TExpressionTreeBuilder::GetOptions() const {
    return Options;
}

} // namespace NGranet::NCompiler

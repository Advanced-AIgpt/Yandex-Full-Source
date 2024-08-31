#pragma once

#include "expression_tree.h"
#include "preprocessor.h"
#include "directives.h"
#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NGranet::NCompiler {

class TExpressionTreeBuilder : TMoveOnly {
public:
    TExpressionTreeBuilder(const TTextView& rootSource, ELanguage lang, const TSourcesLoader& sourcesLoader,
        TStringPool* stringPool);

    // For unit test
    void DisableOptimization();

    void ResetOptionsAccumulator(const TCompilerOptionsAccumulator& options);

    void ReadLines(const TVector<TSrcLine::TRef>& lines);
    void ReadLine(const TTextView& line);
    void Finalize();

    // Results
    TExpressionNode::TRef GetTree() const;
    const TCompilerOptionsAccumulator& GetOptions() const;

private:
    bool TryIncludeRawLines(const TTextView& source);
    void AddTopLevelLine(TExpressionNode::TRef node);
    TExpressionNode::TRef CreateTree(TRuleToken::TConstRef token, bool treatChainAsBag);
    TExpressionNode::TRef CreateSuffixNode(const TTextView& source, const TRuleToken::TConstRef& operand,
        const TRuleToken::TConstRef& suffix);
    void CheckHasOperands(const TExpressionNode* node) const;
    static bool TryReadQuantityRange(TStringBuf str, TQuantityParams* params);
    TExpressionNode::TRef CreateTextNode(TRuleToken::TConstRef token);
    TExpressionNode::TRef TryCreateTextNode(const TTextView& source, TStringBuf text);
    TExpressionNode::TRef CreateWordNode(const TTextView& source, TStringBuf word);
    TExpressionNode::TRef CreateBagNode(const TRuleToken::TConstRef& token);
    TExpressionNode::TRef CreateChainNode(const TRuleToken::TConstRef& token);
    void Inflect(TExpressionNode::TRef& node, const TVector<TGramBitSet>& gramsList) const;
    static TString TryConvertNodeToString(const TExpressionNode::TRef& node);
    static TString TryConvertWordNodesToString(const TVector<TExpressionNode::TRef>& nodes);
    TExpressionNode::TRef InflectStringToNode(const TTextView& source, TStringBuf text, const TVector<TGramBitSet>& gramsList) const;
    TExpressionNode::TRef InflectStringToNode(const TTextView& source, TStringBuf text, const TGramBitSet& grams) const;
    void Optimize(TExpressionNode::TRef& node) const;
    static void OptimizeSimpleBag(TExpressionNode::TRef& node);
    static void OptimizeQuestion(TExpressionNode::TRef& node);
    static void OptimizeChainInsideChain(TExpressionNode::TRef& node);
    static void OptimizeListInsideChain(TExpressionNode::TRef& node);
    static TIntrusivePtr<TListExpressionNode> CartesianProductLists(const TTextView& source,
        const TListExpressionNode* list1, const TListExpressionNode* list2);
    static void ExtendChainOptimized(TExpressionNode::TRef operand, bool shouldCopy, TChainExpressionNode* chain);
    static void OptimizeListInsideList(TExpressionNode::TRef& node);
    static void Normalize(TExpressionNode::TRef& node);
    static void WrapByChainIfNeed(TExpressionNode::TRef& node);
    static void WrapByListIfNeed(TExpressionNode::TRef& node);

private:
    ELanguage Lang = LANG_UNK;
    TCompilerOptionsAccumulator Options;
    TSourcesLoader SourcesLoader;
    TStringPool* StringPool = nullptr;
    bool IsOptimizationEnabled = true;
    bool IsFinalized = false;
    TExpressionNode::TRef Tree;
};

} // namespace NGranet::NCompiler

#pragma once

#include "compiler_data.h"
#include "directives.h"
#include "rule_parser.h"
#include <alice/nlu/granet/lib/grammar/token_id.h>
#include <util/generic/vector.h>

namespace NGranet::NCompiler {

class TWordExpressionNode;
class TElementExpressionNode;
class TQuantifierExpressionNode;
class TModifierExpressionNode;
class TTagBeginExpressionNode;
class TTagEndExpressionNode;
class TListExpressionNode;
class TBagExpressionNode;
class TChainExpressionNode;

// ~~~~ EExpressionNodeType ~~~~

enum EExpressionNodeType {
    ENT_UNDEFINED,
    // Word or wildcard
    ENT_WORD,
    // Element
    ENT_ELEMENT,
    // Operand?  Operand*  Operand+  Operand<n,m>
    ENT_QUANTIFIER,
    // Operand<suffix>  (except cases represented by ENT_QUANTIFIER)
    ENT_MODIFIER,
    // Before and after 'Operand'(tag)
    ENT_TAG_BEGIN,
    ENT_TAG_END,
    // Operand1 | Operand2 | Operand3
    ENT_LIST,
    // [Operand1 Operand2 Operand3]
    ENT_BAG,
    // Operand1 Operand2 Operand3
    ENT_CHAIN
};

// ~~~~ TExpressionNode ~~~~

class TExpressionNode : public TSimpleRefCount<TExpressionNode> {
public:
    using TRef = TIntrusivePtr<TExpressionNode>;
    using TConstRef = TIntrusiveConstPtr<TExpressionNode>;

public:
    const EExpressionNodeType Type = ENT_UNDEFINED;

    // Source token.
    TTextView Source;

    TVector<TExpressionNode::TRef> Operands;

public:
    TExpressionNode(EExpressionNodeType type, const TTextView& source, TVector<TExpressionNode::TRef> operands = {});
    virtual ~TExpressionNode() = default;

    virtual const TWordExpressionNode* AsWord() const { Y_ENSURE(false); return nullptr; }
    virtual const TElementExpressionNode* AsElement() const { Y_ENSURE(false); return nullptr; }
    virtual const TQuantifierExpressionNode* AsQuantifier() const { Y_ENSURE(false); return nullptr; }
    virtual const TModifierExpressionNode* AsModifier() const { Y_ENSURE(false); return nullptr; }
    virtual const TTagBeginExpressionNode* AsTagBegin() const { Y_ENSURE(false); return nullptr; }
    virtual const TTagEndExpressionNode* AsTagEnd() const { Y_ENSURE(false); return nullptr; }
    virtual const TListExpressionNode* AsList() const { Y_ENSURE(false); return nullptr; }
    virtual const TBagExpressionNode* AsBag() const { Y_ENSURE(false); return nullptr; }
    virtual const TChainExpressionNode* AsChain() const { Y_ENSURE(false); return nullptr; }

    virtual TWordExpressionNode* AsWord() { Y_ENSURE(false); return nullptr; }
    virtual TElementExpressionNode* AsElement() { Y_ENSURE(false); return nullptr; }
    virtual TQuantifierExpressionNode* AsQuantifier() { Y_ENSURE(false); return nullptr; }
    virtual TModifierExpressionNode* AsModifier() { Y_ENSURE(false); return nullptr; }
    virtual TTagBeginExpressionNode* AsTagBegin() { Y_ENSURE(false); return nullptr; }
    virtual TTagEndExpressionNode* AsTagEnd() { Y_ENSURE(false); return nullptr; }
    virtual TListExpressionNode* AsList() { Y_ENSURE(false); return nullptr; }
    virtual TBagExpressionNode* AsBag() { Y_ENSURE(false); return nullptr; }
    virtual TChainExpressionNode* AsChain() { Y_ENSURE(false); return nullptr; }

    bool HasOperandOfType(EExpressionNodeType type) const;

    // TODO(samoylovboris) Use deep copy only with TCowPtr.
    virtual TExpressionNode::TRef DeepCopy() const = 0;

    virtual void DumpInline(IOutputStream* log) const = 0;
};

// ~~~~ TWordExpressionNode ~~~~

// Word or wildcard.
// No operands.
class TWordExpressionNode : public TExpressionNode {
public:
    TString Word;
    bool IsLemma = false;

public:
    TWordExpressionNode(const TTextView& source, TStringBuf word, bool isLemma);

    virtual const TWordExpressionNode* AsWord() const override { return this; }
    virtual TWordExpressionNode* AsWord() override { return this; }

    virtual TExpressionNode::TRef DeepCopy() const override;
    virtual void DumpInline(IOutputStream* log) const override;
};

// ~~~~ TElementExpressionNode ~~~~

// Element.
// No operands.
class TElementExpressionNode : public TExpressionNode {
public:
    TString ElementName;

public:
    TElementExpressionNode(const TTextView& source, TString elementName);

    virtual const TElementExpressionNode* AsElement() const override { return this; }
    virtual TElementExpressionNode* AsElement() override { return this; }

    virtual TExpressionNode::TRef DeepCopy() const override;
    virtual void DumpInline(IOutputStream* log) const override;
};

// ~~~~ TQuantifierExpressionNode ~~~~

// Operand?  Operand*  Operand+  Operand{n,m}
// Single operand. Possible type of operand:
//   before normalization: any
//   after normalization: ENT_LIST
class TQuantifierExpressionNode : public TExpressionNode {
public:
    TQuantityParams Params;

public:
    TQuantifierExpressionNode(const TTextView& source, TExpressionNode::TRef operand, TQuantityParams params);

    virtual const TQuantifierExpressionNode* AsQuantifier() const override { return this; }
    virtual TQuantifierExpressionNode* AsQuantifier() override { return this; }

    virtual TExpressionNode::TRef DeepCopy() const override;
    virtual void DumpInline(IOutputStream* log) const override;
};

// ~~~~ TModifierExpressionNode ~~~~

// Operand{suffix}  (except cases represented by ENT_QUANTIFIER)
// Single operand. Possible type of operand:
//   before normalization: ENT_ELEMENT
//   after normalization: ENT_ELEMENT
class TModifierExpressionNode : public TExpressionNode {
public:
    TString Suffix;

public:
    TModifierExpressionNode(const TTextView& source, TExpressionNode::TRef operand, TStringBuf suffix);

    virtual const TModifierExpressionNode* AsModifier() const override { return this; }
    virtual TModifierExpressionNode* AsModifier() override { return this; }

    virtual TExpressionNode::TRef DeepCopy() const override;
    virtual void DumpInline(IOutputStream* log) const override;
};

// ~~~~ TTagBeginExpressionNode ~~~~

// No operands.
class TTagBeginExpressionNode : public TExpressionNode {
public:
    explicit TTagBeginExpressionNode(const TTextView& source);

    virtual const TTagBeginExpressionNode* AsTagBegin() const override { return this; }
    virtual TTagBeginExpressionNode* AsTagBegin() override { return this; }

    virtual TExpressionNode::TRef DeepCopy() const override;
    virtual void DumpInline(IOutputStream* log) const override;
};

// ~~~~ TTagEndExpressionNode ~~~~

// No operands.
class TTagEndExpressionNode : public TExpressionNode {
public:
    TString Tag;

public:
    TTagEndExpressionNode(const TTextView& source, TStringBuf tag);

    virtual const TTagEndExpressionNode* AsTagEnd() const override { return this; }
    virtual TTagEndExpressionNode* AsTagEnd() override { return this; }

    virtual TExpressionNode::TRef DeepCopy() const override;
    virtual void DumpInline(IOutputStream* log) const override;
};

// ~~~~ TListExpressionNode ~~~~

// Operand1 | Operand2 | Operand3
// Possible type of operands:
//   before normalization: any
//   after normalization: ENT_CHAIN
class TListExpressionNode : public TExpressionNode {
public:
    // Parallel to Operands.
    TVector<TListItemParams> Params;

public:
    explicit TListExpressionNode(const TTextView& source);

    void AddOperand(const TExpressionNode::TRef& operand, const TListItemParams& params);
    void AddOperands(const TVector<TExpressionNode::TRef>& operands, const TVector<TListItemParams>& params);

    float GetWeightSum() const;
    void NormalizeWeightSum(float toWeightSum);

    virtual const TListExpressionNode* AsList() const override { return this; }
    virtual TListExpressionNode* AsList() override { return this; }

    virtual TExpressionNode::TRef DeepCopy() const override;
    virtual void DumpInline(IOutputStream* log) const override;
};

// ~~~~ TBagExpressionNode ~~~~

// [Operand1 Operand2 Operand3]
// Possible type of operands:
//   before normalization: any
//   after normalization: ENT_CHAIN
class TBagExpressionNode : public TExpressionNode {
public:
    // Parallel to Operands.
    TVector<TBagItemParams> Params;

public:
    explicit TBagExpressionNode(const TTextView& source);

    void AddOperand(const TExpressionNode::TRef& operand, const TBagItemParams& params);

    virtual const TBagExpressionNode* AsBag() const override { return this; }
    virtual TBagExpressionNode* AsBag() override { return this; }

    virtual TExpressionNode::TRef DeepCopy() const override;
    virtual void DumpInline(IOutputStream* log) const override;
};

// ~~~~ TChainExpressionNode ~~~~

// Operand1 Operand2 Operand3
// Possible type of operands:
//   before normalization: any
//   after normalization: any type except ENT_CHAIN
class TChainExpressionNode : public TExpressionNode {
public:
    explicit TChainExpressionNode(const TTextView& source);

    void AddOperand(const TExpressionNode::TRef& operand);

    virtual const TChainExpressionNode* AsChain() const override { return this; }
    virtual TChainExpressionNode* AsChain() override { return this; }

    virtual TExpressionNode::TRef DeepCopy() const override;
    virtual void DumpInline(IOutputStream* log) const override;
};

} // namespace NGranet::NCompiler

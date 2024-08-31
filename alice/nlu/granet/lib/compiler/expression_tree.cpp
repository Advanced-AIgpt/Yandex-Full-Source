#include "expression_tree.h"
#include <alice/nlu/granet/lib/utils/flag_utils.h>
#include <library/cpp/iterator/zip.h>
#include <util/generic/singleton.h>
#include <util/generic/xrange.h>

namespace NGranet::NCompiler {

template <class T>
TIntrusivePtr<T> MakeExpressionNodeDeepCopy(const T& node) {
    TIntrusivePtr<T> copy = MakeIntrusive<T>(node);
    for (TExpressionNode::TRef& operand : copy->Operands) {
        operand = operand->DeepCopy();
    }
    return copy;
}

// ~~~~ TExpressionNode ~~~~

TExpressionNode::TExpressionNode(EExpressionNodeType type, const TTextView& source, TVector<TExpressionNode::TRef> operands)
    : Type(type)
    , Source(source)
    , Operands(std::move(operands))
{
}

bool TExpressionNode::HasOperandOfType(EExpressionNodeType type) const {
    for (const TExpressionNode::TRef& operand : Operands) {
        if (operand->Type == type) {
            return true;
        }
    }
    return false;
}

// ~~~~ TWordExpressionNode ~~~~

TWordExpressionNode::TWordExpressionNode(const TTextView& source, TStringBuf word, bool isLemma)
    : TExpressionNode(ENT_WORD, source)
    , Word(word)
    , IsLemma(isLemma)
{
}

TExpressionNode::TRef TWordExpressionNode::DeepCopy() const {
    return MakeExpressionNodeDeepCopy(*this);
}

void TWordExpressionNode::DumpInline(IOutputStream* log) const {
    Y_ENSURE(log);
    *log << (IsLemma ? "~" : "") << Word;
}

// ~~~~ TElementExpressionNode ~~~~

TElementExpressionNode::TElementExpressionNode(const TTextView& source, TString elementName)
    : TExpressionNode(ENT_ELEMENT, source)
    , ElementName(std::move(elementName))
{
}

TExpressionNode::TRef TElementExpressionNode::DeepCopy() const {
    return MakeExpressionNodeDeepCopy(*this);
}

void TElementExpressionNode::DumpInline(IOutputStream* log) const {
    Y_ENSURE(log);
    *log << ElementName;
}

// ~~~~ TQuantifierExpressionNode ~~~~

TQuantifierExpressionNode::TQuantifierExpressionNode(const TTextView& source, TExpressionNode::TRef operand,
        TQuantityParams params)
    : TExpressionNode(ENT_QUANTIFIER, source, {operand})
    , Params(params)
{
}

TExpressionNode::TRef TQuantifierExpressionNode::DeepCopy() const {
    return MakeExpressionNodeDeepCopy(*this);
}

void TQuantifierExpressionNode::DumpInline(IOutputStream* log) const {
    Y_ENSURE(log);
    Y_ENSURE(Operands.size() == 1);
    Operands[0]->DumpInline(log);
    *log << Params.GetNormalizedText();
}

// ~~~~ TModifierExpressionNode ~~~~

TModifierExpressionNode::TModifierExpressionNode(const TTextView& source, TExpressionNode::TRef operand,
        TStringBuf suffix)
    : TExpressionNode(ENT_MODIFIER, source, {operand})
    , Suffix(TString{suffix})
{
}

TExpressionNode::TRef TModifierExpressionNode::DeepCopy() const {
    return MakeExpressionNodeDeepCopy(*this);
}

void TModifierExpressionNode::DumpInline(IOutputStream* log) const {
    Y_ENSURE(log);
    Y_ENSURE(Operands.size() == 1);
    Operands[0]->DumpInline(log);
    *log << Suffix;
}

// ~~~~ TTagBeginExpressionNode ~~~~

TTagBeginExpressionNode::TTagBeginExpressionNode(const TTextView& source)
    : TExpressionNode(ENT_TAG_BEGIN, source)
{
}

TExpressionNode::TRef TTagBeginExpressionNode::DeepCopy() const {
    return MakeExpressionNodeDeepCopy(*this);
}

void TTagBeginExpressionNode::DumpInline(IOutputStream* log) const {
    Y_ENSURE(log);
    *log << "TAG_BEGIN";
}

// ~~~~ TTagEndExpressionNode ~~~~

TTagEndExpressionNode::TTagEndExpressionNode(const TTextView& source, TStringBuf tag)
    : TExpressionNode(ENT_TAG_END, source)
    , Tag(TString{tag})
{
}

TExpressionNode::TRef TTagEndExpressionNode::DeepCopy() const {
    return MakeExpressionNodeDeepCopy(*this);
}

void TTagEndExpressionNode::DumpInline(IOutputStream* log) const {
    Y_ENSURE(log);
    *log << "TAG_END<tag:" << Tag << ">";
}

// ~~~~ TListExpressionNode ~~~~

TListExpressionNode::TListExpressionNode(const TTextView& source)
    : TExpressionNode(ENT_LIST, source)
{
}

void TListExpressionNode::AddOperand(const TExpressionNode::TRef& operand, const TListItemParams& params) {
    Operands.push_back(operand);
    Params.push_back(params);
}

void TListExpressionNode::AddOperands(const TVector<TExpressionNode::TRef>& operands,
    const TVector<TListItemParams>& params)
{
    Extend(operands, &Operands);
    Extend(params, &Params);
}

float TListExpressionNode::GetWeightSum() const {
    float result = 0;
    for (const TListItemParams& params : Params) {
        result += params.Weight;
    }
    return result;
}

void TListExpressionNode::NormalizeWeightSum(float toWeightSum) {
    Y_ENSURE(!Operands.empty());
    const float weightSum = GetWeightSum();
    Y_ENSURE(weightSum > 0);
    const float coeff = toWeightSum / weightSum;
    for (TListItemParams& params : Params) {
        params.Weight *= coeff;
    }
}

TExpressionNode::TRef TListExpressionNode::DeepCopy() const {
    return MakeExpressionNodeDeepCopy(*this);
}

void TListExpressionNode::DumpInline(IOutputStream* log) const {
    Y_ENSURE(log);
    *log << "{";
    for (const size_t i : xrange(Operands.size())) {
        *log << (i > 0 ? "|" : "");
        Operands[i]->DumpInline(log);
        const TListItemParams& params = Params[i];
        *log << (params.IsForced ? "<forced>" : "");
        *log << (params.IsNegative ? "<negative>" : "");
        if (params.Weight != 1.) {
            *log << "<w:" << params.Weight << ">";
        }
    }
    *log << "}";
}

// ~~~~ TBagExpressionNode ~~~~

TBagExpressionNode::TBagExpressionNode(const TTextView& source)
    : TExpressionNode(ENT_BAG, source)
{
}

void TBagExpressionNode::AddOperand(const TExpressionNode::TRef& operand, const TBagItemParams& params) {
    Operands.push_back(operand);
    Params.push_back(params);
}

TExpressionNode::TRef TBagExpressionNode::DeepCopy() const {
    return MakeExpressionNodeDeepCopy(*this);
}

void TBagExpressionNode::DumpInline(IOutputStream* log) const {
    Y_ENSURE(log);
    Y_ENSURE(Operands.size() == Params.size());
    *log << "[";
    for (const size_t i : xrange(Operands.size())) {
        *log << (i > 0 ? " " : "");
        Operands[i]->DumpInline(log);
        *log << Params[i].PrintSuffix();
    }
    *log << "]";
}

// ~~~~ TChainExpressionNode ~~~~

TChainExpressionNode::TChainExpressionNode(const TTextView& source)
    : TExpressionNode(ENT_CHAIN, source)
{
}

void TChainExpressionNode::AddOperand(const TExpressionNode::TRef& operand) {
    Operands.push_back(operand);
}

TExpressionNode::TRef TChainExpressionNode::DeepCopy() const {
    return MakeExpressionNodeDeepCopy(*this);
}

void TChainExpressionNode::DumpInline(IOutputStream* log) const {
    Y_ENSURE(log);
    *log << "(";
    for (const size_t i : xrange(Operands.size())) {
        *log << (i > 0 ? " " : "");
        Operands[i]->DumpInline(log);
    }
    *log << ")";
}

} // namespace NGranet::NCompiler

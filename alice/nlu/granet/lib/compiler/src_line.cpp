#include "src_line.h"
#include "compiler_check.h"
#include <util/generic/array_ref.h>

namespace NGranet::NCompiler {

// ~~~~ TSrcLine ~~~~

TSrcLine::TSrcLine(TTextView source)
    : Source(std::move(source))
{
}

TStringBuf TSrcLine::Str() const {
    return Source.Str();
}

bool TSrcLine::IsCompatibilityMode() const {
    return Source.GetSourceText()->IsCompatibilityMode;
}

static bool MatchPattern(TStringBuf str, const TArrayRef<const TString>& patterns) {
    return ::AnyOf(patterns, [str](TStringBuf pattern) {
        return pattern.ChopSuffix("*") ? str.StartsWith(pattern) : (str == pattern);
    });
}

void TSrcLine::CheckAllowedChildren(const TArrayRef<const TString>& patterns) const {
    const TArrayRef<const TString> dummy;
    CheckAllowedChildren(patterns, dummy);
}

void TSrcLine::CheckAllowedChildren(const TArrayRef<const TString>& patterns1,
    const TArrayRef<const TString>& patterns2) const
{
    if (IsCompatibilityMode()) {
        return;
    }
    for (const TSrcLine::TRef& child : Children) {
        GRANET_COMPILER_CHECK(MatchPattern(child->Str(), patterns1) || MatchPattern(child->Str(), patterns2),
            *child, MSG_UNEXPECTED_KEYWORD);
    }
}

const TSrcLine* TSrcLine::FindKey(TStringBuf key) const {
    const TSrcLine* result = nullptr;
    for (const TSrcLine::TRef& child : Children) {
        if (child->Str() != key) {
            continue;
        }
        GRANET_COMPILER_CHECK(IsCompatibilityMode() || result == nullptr, *child, MSG_DUPLICATED_KEY);
        result = child.Get();
    }
    return result;
}

const TSrcLine* TSrcLine::FindKey(TStringBuf key, TStringBuf keyAlias) const {
    const TSrcLine* child1 = FindKey(key);
    const TSrcLine* child2 = FindKey(keyAlias);
    GRANET_COMPILER_CHECK(IsCompatibilityMode() || child1 == nullptr || child2 == nullptr, *child2, MSG_DUPLICATED_KEY);
    return child1 != nullptr ? child1 : child2;
}

const TSrcLine* TSrcLine::FindValueByKey(TStringBuf keyStr) const {
    const TSrcLine* key = FindKey(keyStr);
    if (key == nullptr) {
        return nullptr;
    }
    GRANET_COMPILER_CHECK(key->Children.size() == 1, *key, MSG_NO_VALUE);
    const TSrcLine::TConstRef& value = key->Children[0];
    GRANET_COMPILER_CHECK(!value->IsParent, *value, MSG_NOT_A_VALUE);
    return value.Get();
}

bool TSrcLine::GetBooleanValueByKey(TStringBuf key, bool def) const {
    bool result = def;
    if (const TSrcLine* value = FindValueByKey(key)) {
        GRANET_COMPILER_CHECK(TryFromString(value->Str(), result), *value, MSG_BOOLEAN_EXPECTED);
    }
    return result;
}

ui32 TSrcLine::GetUnsignedIntValueByKey(TStringBuf key, ui32 def) const {
    ui32 result = def;
    if (const TSrcLine* value = FindValueByKey(key)) {
        GRANET_COMPILER_CHECK(TryFromString(value->Str(), result), *value, MSG_UNSIGNED_INT_EXPECTED);
    }
    return result;
}

void TSrcLine::DumpTree(IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << Str() << Endl;
    for (const TSrcLine::TRef& child : Children) {
        child->DumpTree(log, indent + "  ");
    }
}

} // namespace NGranet::NCompiler

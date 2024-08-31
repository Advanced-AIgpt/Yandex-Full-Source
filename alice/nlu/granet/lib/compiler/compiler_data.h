#pragma once

#include "directives.h"
#include "element_modification.h"
#include "src_line.h"
#include <alice/nlu/granet/lib/grammar/grammar_data.h>
#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGranet::NCompiler {

struct TCompilerElement;
struct TElementDefinition;

// ~~~~ EScopeType ~~~~

enum EScopeType {
    // Root of scope tree.
    ST_Project,
    // Child of ST_Project.
    ST_File,
    // Child of ST_File.
    ST_Task,
    // Child of ST_File, ST_Task or ST_Element.
    ST_Element,
};

// ~~~~ TScope ~~~~

struct TScope {
    EScopeType Type = ST_Project;
    TScope* Parent = nullptr;
    TVector<TScope*> Children;

    // Name of this scope as namespace.
    TString Name;

    // Not null for all scopes except ST_Project.
    TSrcLine::TRef Lines;

    TVector<const TScope*> ImportedFiles;

    THashMap<TString, TElementDefinition*> VisibleElements;

    TParserTask* AsTask = nullptr;
    TFsPath AsFile;
};

// ~~~~ TListItemParams ~~~~

struct TListItemParams {
    float Weight = 1.f;
    bool IsForced = false; // %force_negative or %force_positive
    bool IsNegative = false; // %negative or %force_negative
    TStringId DataType = 0;
    TStringId DataValue = 0;

    DECLARE_TUPLE_LIKE_TYPE(TListItemParams, Weight, IsForced, IsNegative, DataType, DataValue);
};

// ~~~~ TBagItemParams ~~~~

struct TBagItemParams {
    bool IsRequired = false;
    bool IsLimited = false;

    DECLARE_TUPLE_LIKE_TYPE(TBagItemParams, IsRequired, IsLimited);

    TString PrintSuffix() const;
};

inline const TBagItemParams BAG_PARAM_EMPTY = {.IsRequired = true, .IsLimited = true};
inline const TBagItemParams BAG_PARAM_QUESTION = {.IsRequired = false, .IsLimited = true};
inline const TBagItemParams BAG_PARAM_PLUS = {.IsRequired = true, .IsLimited = false};
inline const TBagItemParams BAG_PARAM_STAR = {.IsRequired = false, .IsLimited = false};

// ~~~~ TCompiledRule ~~~~

struct TCompiledRule {
    TVector<TTokenId> Chain;
    TListItemParams ListItemParams;
    TBagItemParams BagItemParams;

    DECLARE_TUPLE_LIKE_TYPE(TCompiledRule, Chain, ListItemParams, BagItemParams);

    void AppendElement(TElementId id) {
        Chain.push_back(NTokenId::FromElementId(id));
    }

    void AppendToken(TTokenId id) {
        Chain.push_back(id);
    }
};

// ~~~~ TCompilerElementParams ~~~~

// Element params which:
// - Created from rules by element compiler.
// - Copied into modified element (inflected or lemmatized element).
struct TCompilerElementParams {
    bool AnchorToBegin = false;
    bool AnchorToEnd = false;
    bool EnableFillers = false;
    bool CoverFillers = false;
    bool EnableEdgeFillers = false;
    ESynonymFlags EnableSynonymFlagsMask = 0;
    ESynonymFlags EnableSynonymFlags = 0;
    TString EntityName;
    TQuantityParams Quantity;
};

// ~~~~ TCompilerElement ~~~~

struct TCompilerElement {
    TElementId Id = UNDEFINED_ELEMENT_ID;
    TString Name;

    const TElementDefinition* Definition = nullptr;
    TElementModification Modification;

    // For error report.
    TTextView Source;

    bool IsCompiled = false;

    bool IsDependenciesDefined = false;
    THashSet<TElementId> Dependencies;
    ui32 Level = 0;

    TCompilerElementParams Params;
    TVector<TCompiledRule> Rules;
};

inline TElementId GetOptionalElementId(const TCompilerElement* element) {
    return element ? element->Id : UNDEFINED_ELEMENT_ID;
}

// ~~~~ TElementDefinition ~~~~

struct TElementDefinition {
    TString Name;

    // Element scope (of type ST_Element).
    // Not null only for explicit elements (user defined elements).
    TScope* Scope = nullptr;

    // For error report.
    TTextView Source;

    bool IsBuiltin = false;

    TVector<TSlotDescriptionId> SourceForSlots;

    TVector<TCompilerElement*> Elements;
};

} // namespace NGranet::NCompiler

// ~~~~ global namespace ~~~~

template <>
struct THash<TVector<NGranet::TTokenId>>: public TSimpleRangeHash {
};

template <>
struct THash<NGranet::NCompiler::TCompiledRule>: public TTupleLikeTypeHash {
};

template <>
struct THash<TVector<NGranet::NCompiler::TCompiledRule>>: public TSimpleRangeHash {
};

template <>
struct THash<NGranet::NCompiler::TListItemParams>: public TTupleLikeTypeHash {
};

template <>
struct THash<NGranet::NCompiler::TBagItemParams>: public TTupleLikeTypeHash {
};

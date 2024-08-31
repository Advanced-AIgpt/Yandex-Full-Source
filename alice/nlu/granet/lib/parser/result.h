#pragma once

#include "debug.h"
#include "element_occurrence.h"
#include "state.h"
#include <alice/nlu/granet/lib/sample/tag.h>
#include <alice/nlu/granet/lib/sample/sample.h>
#include <alice/nlu/granet/lib/sample/markup.h>
#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <library/cpp/dbg_output/dump.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NGranet {

// ~~~~ TResultSlotValue ~~~~

// Value from entity stored in slot of form.
struct TResultSlotValue {
    // Position of element (entity) which produced this value.
    NNlu::TInterval Interval;
    TString Type;
    TString Value;

    DECLARE_TUPLE_LIKE_TYPE(TResultSlotValue, Interval, Type, Value);
};

// ~~~~ TResultSlot ~~~~

// Slot of parsed form
struct TResultSlot {
    // Position of slot on sample tokens.
    NNlu::TInterval Interval;
    TString Name;
    TVector<TResultSlotValue> Data;

    TTag ToTag() const;
    void Dump(IOutputStream* log, const TSample::TConstRef& sample, const TString& indent = "") const;
};

// ~~~~ TParserVariant ~~~~

// TODO(samoylovboris) comments
class TParserVariant : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TParserVariant>;
    using TConstRef = TIntrusiveConstPtr<TParserVariant>;

public:
    TSample::TConstRef Sample;
    TVector<TResultSlot> Slots;
    TElementOccurrence::TRef ElementTree;
    float LogProb = 0;

public:
    TVector<TSlotMarkup> ToMarkup() const;
    TVector<TSlotMarkup> ToNonterminalMarkup(const TGrammar::TConstRef& grammar) const;

    void Dump(const TGrammar::TConstRef& grammar, IOutputStream* log, const TString& indent = "") const;
};

// ~~~~ TParserTaskResult ~~~~

class TParserFormResult;
class TParserEntityResult;

// Result of parsing of one task (TParserTask)
// Base class for TParserFormResult and TParserEntityResult
class TParserTaskResult : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TParserTaskResult>;
    using TConstRef = TIntrusiveConstPtr<TParserTaskResult>;

public:
    // Name of form or entity.
    const TString& GetName() const {
        return Name;
    }

    virtual void SetName(TStringBuf name) {
        Name = TString{name};
    }

    virtual bool IsPositive() const = 0;
    virtual TSampleMarkup ToMarkup() const = 0;

    const TParserDebugInfo::TConstRef& GetDebugInfo() const;
    void SetDebugInfo(const TParserDebugInfo::TConstRef& debugInfo);

    virtual void Dump(IOutputStream* log, const TString& indent = "") const = 0;

    virtual const TParserFormResult* AsFormResult() const {
        Y_ENSURE(false);
        return nullptr;
    }

    virtual TParserFormResult* AsFormResult() {
        Y_ENSURE(false);
        return nullptr;
    }

    virtual const TParserEntityResult* AsEntityResult() const {
        Y_ENSURE(false);
        return nullptr;
    }

    virtual TParserEntityResult* AsEntityResult() {
        Y_ENSURE(false);
        return nullptr;
    }

    bool GetIsInternal() const {
        return IsInternal;
    }

protected:
    TParserTaskResult(const TString& name, bool isInternal, const TSample::TConstRef& sample,
            const TGrammar::TConstRef& grammar)
        : Name(name)
        , IsInternal(isInternal)
        , Sample(sample)
        , Grammar(grammar)
    { }

    void DumpDebugInfo(IOutputStream* log, const TString& indent = "") const;

protected:
    // Adjusted name of task (maybe without "ifexp" suffix)
    TString Name;
    bool IsInternal = false;

    // Additional info for debugging and testing purposes
    TSample::TConstRef Sample;
    TGrammar::TConstRef Grammar;
    TParserDebugInfo::TConstRef DebugInfo;
};

// ~~~~ TParserFormResult ~~~~

// Result of parsing of one form.
// Created in TParser.
class TParserFormResult : public TParserTaskResult {
public:
    using TRef = TIntrusivePtr<TParserFormResult>;
    using TConstRef = TIntrusiveConstPtr<TParserFormResult>;

public:
    static TParserFormResult::TRef Create(const TString& name, bool isInternal,
        const TSample::TConstRef& sample, const TGrammar::TConstRef& grammar,
        TVector<TParserVariant::TConstRef> variants)
    {
        return new TParserFormResult(name, isInternal, sample, grammar, std::move(variants));
    }

    virtual bool IsPositive() const override {
        return !Variants.empty();
    }

    const TParserVariant::TConstRef& GetBestVariant() const {
        Y_ENSURE(!Variants.empty());
        return Variants.front();
    }

    // Variants of parsing this form.
    // Sorted by quality (LogProb). Best variant is first. Can be empty.
    const TVector<TParserVariant::TConstRef>& GetVariants() const {
        return Variants;
    }

    virtual TSampleMarkup ToMarkup() const override;

    virtual void Dump(IOutputStream* log, const TString& indent = "") const override;

    virtual const TParserFormResult* AsFormResult() const override {
        return this;
    }

    virtual TParserFormResult* AsFormResult() override {
        return this;
    }

private:
    TParserFormResult(const TString& name, bool isInternal,
            const TSample::TConstRef& sample, const TGrammar::TConstRef& grammar,
            TVector<TParserVariant::TConstRef> variants)
        : TParserTaskResult(name, isInternal, sample, grammar)
        , Variants(std::move(variants))
    { }

private:
    TVector<TParserVariant::TConstRef> Variants;
};

// ~~~~ TParserEntityResult ~~~~

// Result of parsing of one entity.
// Created in TParser.
class TParserEntityResult : public TParserTaskResult {
public:
    using TRef = TIntrusivePtr<TParserEntityResult>;
    using TConstRef = TIntrusiveConstPtr<TParserEntityResult>;

public:
    static TParserEntityResult::TRef Create(const TString& name, bool isInternal,
        const TSample::TConstRef& sample, const TGrammar::TConstRef& grammar,
        TVector<TEntity> entities)
    {
        return new TParserEntityResult(name, isInternal, sample, grammar, std::move(entities));
    }

    virtual void SetName(TStringBuf name) override;

    virtual bool IsPositive() const override {
        return !Entities.empty();
    }

    const TVector<TEntity>& GetEntities() const {
        return Entities;
    }

    virtual TSampleMarkup ToMarkup() const override;

    virtual void Dump(IOutputStream* log, const TString& indent = "") const override;

    virtual const TParserEntityResult* AsEntityResult() const override {
        return this;
    }

    virtual TParserEntityResult* AsEntityResult() override {
        return this;
    }

private:
    TParserEntityResult(const TString& name, bool isInternal, const TSample::TConstRef& sample,
            const TGrammar::TConstRef& grammar, TVector<TEntity> entities)
        : TParserTaskResult(name, isInternal, sample, grammar)
        , Entities(std::move(entities))
    { }

private:
    TVector<TEntity> Entities;
};

// ~~~~ Helpers ~~~~

TParserTaskResult::TRef CreateParserEmptyResult(const TParserTask& task, const TSample::TConstRef& sample,
    const TGrammar::TConstRef& grammar);

} // namespace NGranet

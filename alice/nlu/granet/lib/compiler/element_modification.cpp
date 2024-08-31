#include "element_modification.h"
#include "compiler_check.h"
#include "syntax.h"

namespace NGranet::NCompiler {

// Syntax:
//   Params:            Result:
//   acc,pl             text_acc_pl
//   acc,pl|gen,sg      text_acc_pl, text_gen_sg
//   |acc,pl            text, text_acc_pl
//   |acc,pl|gen,sg     text, text_acc_pl, text_gen_sg
static TVector<TGramBitSet> ReadGramsList(TStringBuf params, const TTextView& source) {
    TVector<TGramBitSet> gramsList;
    for (TStringBuf gramsStr : StringSplitter(params).Split('|')) {
        gramsStr = StripString(gramsStr);
        if (gramsStr.Empty()) {
            gramsList.push_back({});
            continue;
        }
        try {
            gramsList.push_back(TGramBitSet::FromString(gramsStr));
        } catch (const TGramBitSet::TException& e) {
            GRANET_COMPILER_CHECK(false, source, MSG_GRAMMEME_PARSER_ERROR, e.what());
        }
    }
    return gramsList;
}

static const TVector<TGramBitSet> INFLECT_CASES_GRAMS = ReadGramsList("|nom|gen|dat|acc|ins|abl|loc", {});
static const TVector<TGramBitSet> INFLECT_GENDERS_GRAMS = ReadGramsList("|f|m|n|pl", {});
static const TVector<TGramBitSet> INFLECT_NUMBERS_GRAMS = ReadGramsList("|sg|pl", {});

static TVector<TGramBitSet> CombineGramsLists(const TVector<TGramBitSet>& gramsList1, const TVector<TGramBitSet>& gramsList2) {
    TVector<TGramBitSet> product;
    for (const TGramBitSet& grams1 : gramsList1) {
        for (const TGramBitSet& grams2 : gramsList2) {
            product.push_back(grams1 | grams2);
        }
    }
    return product;
}

TMaybe<TVector<TGramBitSet>> MakeModificationGramsList(ECompilerFlags compilerFlags, TStringBuf customInflection) {
    if (HasIntersection(compilerFlags, CF_LEMMA_FLAGS | CF_EXACT)) {
        return Nothing();
    }
    if (!HasIntersection(compilerFlags, CF_INFLECT_FLAGS) && customInflection.empty()) {
        return Nothing();
    }
    TVector<TGramBitSet> gramsList;
    gramsList.push_back({});
    if (compilerFlags.HasFlags(CF_INFLECT_CASES)) {
        gramsList = CombineGramsLists(gramsList, INFLECT_CASES_GRAMS);
    }
    if (compilerFlags.HasFlags(CF_INFLECT_GENDERS)) {
        gramsList = CombineGramsLists(gramsList, INFLECT_GENDERS_GRAMS);
    }
    if (compilerFlags.HasFlags(CF_INFLECT_NUMBERS)) {
        gramsList = CombineGramsLists(gramsList, INFLECT_NUMBERS_GRAMS);
    }
    if (!customInflection.empty()) {
        gramsList = CombineGramsLists(gramsList, ReadGramsList(customInflection, {}));
    }
    return gramsList;
}

TElementModification OverrideModification(const TElementModification& prev, const TElementModification& curr) {
    if (HasIntersection(curr.CompilerFlags, CF_LEMMA_FLAGS | CF_EXACT)) {
        return curr;
    }
    if (HasIntersection(curr.CompilerFlags, CF_INFLECT_FLAGS)) {
        TElementModification result = curr;
        result.CompilerFlags |= prev.CompilerFlags & CF_INFLECT_FLAGS;
        return result;
    }
    return curr;
}

TElementModification ParseElementModification(TStringBuf modificationStr, const TTextView& source) {
    TStringBuf params;
    TStringBuf type;
    modificationStr.Split(':', type, params);
    params = StripString(params);
    type = StripString(type);

    if (type == NSyntax::NElementModifier::Gram) {
        // Validate params here, because here we have 'source' to show position of error
        ReadGramsList(params, source);
        return {0, TString(params)};
    }

    GRANET_COMPILER_CHECK(params.empty(), source, MSG_UNEXPECTED_PARAM_OF_MODIFIER);
    if (type == NSyntax::NElementModifier::Lemma) {
        return {CF_LEMMA_GOOD, ""};
    }
    if (type == NSyntax::NElementModifier::LemmaAsIs) {
        return {CF_LEMMA_AS_IS, ""};
    }
    if (type == NSyntax::NElementModifier::InflectCases) {
        return {CF_INFLECT_CASES, ""};
    }
    if (type == NSyntax::NElementModifier::InflectGenders) {
        return {CF_INFLECT_GENDERS, ""};
    }
    if (type == NSyntax::NElementModifier::InflectNumbers) {
        return {CF_INFLECT_NUMBERS, ""};
    }

    GRANET_COMPILER_CHECK(false, source, MSG_UNKNOWN_TYPE_OF_MODIFICATION, type);
    return {};
}

} // namespace NGranet::NCompiler

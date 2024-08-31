#pragma once

#include <alice/nlu/granet/lib/compiler/messages.h>
#include <alice/nlu/granet/lib/grammar/grammar_data.h>
#include <alice/nlu/granet/lib/utils/text_view.h>
#include <library/cpp/regex/pcre/regexp.h>

#include <util/string/split.h>

namespace NGranet::NCompiler {

namespace NSyntax {

    namespace NDirective {
        inline const TString Include = "%include";
        inline const TString IncludeRaw = "%include_raw";
        inline const TString EnableSynonyms = "%enable_synonyms";
        inline const TString DisableSynonyms = "%disable_synonyms";
        inline const TString Lemma = "%lemma";
        inline const TString LemmaAsIs = "%lemma_as_is";
        inline const TString InflectCases = "%inflect_cases";
        inline const TString InflectGenders = "%inflect_genders";
        inline const TString InflectNumbers = "%inflect_numbers";
        inline const TString Exact = "%exact";
        inline const TString Weight = "%weight";
        inline const TString Value = "%value";
        inline const TString Type = "%type";
        inline const TString Negative = "%negative";
        inline const TString Positive = "%positive";
        inline const TString ForceNegative = "%force_negative";
        inline const TString ForcePositive = "%force_positive";
        inline const TString AnchorToBegin = "%anchor_to_begin";
        inline const TString AnchorToEnd = "%anchor_to_end";
        inline const TString CoverFillers = "%cover_fillers";
        inline const TString Fillers = "%fillers";
    }

    namespace NElement {
        inline const TString NamePrefix = "$";
        inline const TString NamePattern = NamePrefix + "*";
        inline const TString Root = "root";
        inline const TString Filler = "filler";
        inline const TString AutoFiller = "auto_filler";
        inline const TString Void = "$sys.void";
    }

    namespace NElementModifier {
        inline const TString Lemma = "lemma";
        inline const TString LemmaAsIs = "lemma_as_is";
        inline const TString InflectCases = "inflect_cases";
        inline const TString InflectGenders = "inflect_genders";
        inline const TString InflectNumbers = "inflect_numbers";
        inline const TString Gram = "g";
    }

    namespace NCommonKey {
        inline const TString Import = "import";
    }

    namespace NFileKey {
        inline const TString Form = "form";
        inline const TString Entity = "entity";
        inline const TString FormPattern = Form + " *"; // "form my_form_name"
        inline const TString EntityPattern = Entity + " *"; // "entity my_entity_name"

        inline const TVector<TString> AllowedKeys = {
            NCommonKey::Import,
            FormPattern,
            EntityPattern,
            NElement::Root,
            NElement::Filler,
            NElement::NamePattern,
        };
    }

    namespace NTaskKey {
        inline const TString Slots = "slots";
        inline const TString Values = "values";
        inline const TString EnableGranetParser = "enable_granet_parser";
        inline const TString EnableAliceTagger = "enable_alice_tagger";
        inline const TString EnableAutoFiller = "enable_auto_filler";
        inline const TString EnableSynonyms = "enable_synonyms";
        inline const TString DisableSynonyms = "disable_synonyms";
        inline const TString Exact = "exact"; // TODO(samoylovboris) for foward compatibility
        inline const TString Fuzzy = "fuzzy"; // TODO(samoylovboris) for foward compatibility
        inline const TString Lemma = "lemma";
        inline const TString LemmaAsIs = "lemma_as_is";
        inline const TString InflectCases = "inflect_cases";
        inline const TString InflectGenders = "inflect_genders";
        inline const TString InflectNumbers = "inflect_numbers";
        inline const TString KeepVariants = "keep_variants";
        inline const TString KeepOverlapped = "keep_overlapped";
        inline const TString IsAction = "is_action";
        inline const TString IsFixlist = "is_fixlist";
        inline const TString IsConditional = "is_conditional";
        inline const TString IsInternal = "is_internal";
        inline const TString Fresh = "fresh";
        inline const TString Freshness = "freshness";

        inline const TVector<TString> AllowedCommonKeys = {
            NCommonKey::Import,
            Slots,
            Values,
            EnableGranetParser,
            EnableAliceTagger,
            EnableAutoFiller,
            EnableSynonyms,
            DisableSynonyms,
            Exact,
            Fuzzy,
            Lemma,
            LemmaAsIs,
            InflectCases,
            InflectGenders,
            InflectNumbers,
            KeepVariants,
            IsConditional,
            IsInternal,
            Fresh,
            Freshness,
            NElement::Root,
            NElement::Filler,
            NElement::NamePattern,
        };
        inline const TVector<TString> AllowedSpecificKeys[PTT_COUNT] = {
            {Slots, IsAction, IsFixlist}, // Allowed in form
            {Values, KeepOverlapped} // Allowed in entity
        };
    }

    namespace NSlotKey {
        inline const TString Type = "type";
        inline const TString Types = "types";
        inline const TString Source = "source";
        inline const TString Sources = "sources";
        // Value is ESlotMatchingType
        inline const TString MatchingType = "matching_type";
        inline const TString ConcatenateStrings = "concatenate_strings";
        inline const TString KeepVariants = "keep_variants";

        inline const TVector<TString> AllowedKeys = {
            Type,
            Types,
            Source,
            Sources,
            MatchingType,
            ConcatenateStrings,
            KeepVariants
        };
    }

    namespace NSynonymsKey {
        inline const TString All = "all";
        inline const TString Translit = "translit";
        inline const TString Synset = "synset";
        inline const TString DiminName = "dimin_name";
        inline const TString Synon = "synon";
        inline const TString Fio = "fio";
        inline const TString True = "true"; // deprecated (needed for backward compatibility)
    }

    namespace NParamRegEx {
        inline const TString Identifier = R"([a-zA-Z_][0-9a-zA-Z_\.]*)";

        inline const TRegExMatch FormName("^" + Identifier + "$");
        inline const TRegExMatch TypeName("^" + Identifier + "$");
        inline const TRegExMatch SlotName("^" + Identifier + "$");
        inline const TRegExMatch ElementName("^\\$" + Identifier + "$");

        inline const TRegExMatch NotQuotedValue(R"(^[0-9a-zA-Z_\.\-]+$)");
        inline const TRegExMatch NotQuotedPath(R"(^[0-9a-zA-Z_\.\-\/]+$)");

        // PASkills Granet Server inserts guid before form name
        inline const TString Guid = R"([0-9a-f]{8}\-[0-9a-f]{4}\-[0-9a-f]{4}\-[0-9a-f]{4}\-[0-9a-f]{12})";
        inline const TString GuidPrefix = "(?:" + Guid + "\\.)?";
        inline const TRegExMatch FormNamePASkills("^" + GuidPrefix + R"([0-9a-zA-Z_][0-9a-zA-Z_\.]*$)");
        inline const TRegExMatch TypeNamePASkills("^" + GuidPrefix + Identifier + "$");
    }
}

// Synonym name -> synonym info
inline const THashMap<TStringBuf, ESynonymFlags> SynonymInfoTable = {
    {NSyntax::NSynonymsKey::All,         SF_TRANSLIT | SF_SYNSET | SF_DIMIN_NAME | SF_SYNON | SF_FIO},
    {NSyntax::NSynonymsKey::True,        SF_TRANSLIT | SF_SYNSET | SF_DIMIN_NAME | SF_SYNON | SF_FIO},
    {NSyntax::NSynonymsKey::Translit,    SF_TRANSLIT},
    {NSyntax::NSynonymsKey::Synset,      SF_SYNSET},
    {NSyntax::NSynonymsKey::DiminName,   SF_DIMIN_NAME},
    {NSyntax::NSynonymsKey::Synon,       SF_SYNON},
    {NSyntax::NSynonymsKey::Fio,         SF_FIO},
};

void ReadSynonyms(const TTextView& source, TStringBuf line, ESynonymFlags& synonymFlagsMask, ESynonymFlags& synonymFlags, bool isEnable);

TString SafeUnquote(const TTextView& source, TStringBuf str, const TRegExMatch* notQuotedStringCheck = nullptr);
TString SafeUnquote(const TTextView& source, const TRegExMatch* notQuotedStringCheck = nullptr);

} // namespace NGranet::NCompiler

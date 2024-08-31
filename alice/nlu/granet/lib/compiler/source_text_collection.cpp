#include "source_text_collection.h"
#include <alice/library/compression/compression.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/generic/deque.h>
#include <util/string/join.h>
#include <util/string/split.h>

namespace NGranet::NCompiler {

namespace NScheme {
    static const TStringBuf FormatVersionKey = "granet_format_version";
    static const TStringBuf FormatVersionValue = "1";
    static const TStringBuf Language = "lang";
    static const TStringBuf IsPASkills = "paskills";
    static const TStringBuf IsWizard = "wizard";
    static const TStringBuf IsSnezhana = "snezhana";
    static const TStringBuf MainTextPath = "main";
    static const TStringBuf Texts = "texts";
}

// ~~~~ TSourceTextCollection ~~~~

bool TSourceTextCollection::IsEmpty() const {
    return Texts.empty();
}

NSc::TValue TSourceTextCollection::ToTValue() const {
    NSc::TValue result;
    result[NScheme::FormatVersionKey] = NScheme::FormatVersionValue;
    result[NScheme::Language] = Domain.GetLangName();
    result[NScheme::IsPASkills].SetBool(Domain.IsPASkills);
    result[NScheme::IsWizard].SetBool(Domain.IsWizard);
    result[NScheme::IsSnezhana].SetBool(Domain.IsSnezhana);
    result[NScheme::MainTextPath] = MainTextPath;
    NSc::TDict& resultTexts = result[NScheme::Texts].GetDictMutable();
    for (const auto& [path, text] : Texts) {
        // To be more readable split into lines.
        resultTexts[path] = NJsonConverters::ToTValue(StringSplitter(text).Split('\n').ToList<TString>());
    }
    return result;
}

void TSourceTextCollection::FromTValue(const NSc::TValue& value, const bool validate) {
    *this = {};
    if (value[NScheme::FormatVersionKey].GetString() != NScheme::FormatVersionValue) {
        return;
    }
    Domain.Lang = LanguageByName(value[NScheme::Language].GetString());
    Domain.IsPASkills = value[NScheme::IsPASkills].GetBool(false);
    Domain.IsWizard = value[NScheme::IsWizard].GetBool(false);
    Domain.IsSnezhana = value[NScheme::IsSnezhana].GetBool(false);
    MainTextPath = value[NScheme::MainTextPath];
    for (const auto& [path, lines] : value[NScheme::Texts].GetDict()) {
        Texts[path] = JoinSeq("\n", NJsonConverters::FromTValue<TVector<TString>>(lines, validate));
    }
}

TString TSourceTextCollection::ToBase64() const {
    return Base64EncodeUrl(ToJson(true));
}

void TSourceTextCollection::FromBase64(TStringBuf base64) {
    FromJson(Base64DecodeUneven(base64), true);
}

TString TSourceTextCollection::ToCompressedBase64() const {
    return Base64EncodeUrl(NAlice::ZLibCompress(ToJson(true)));
}

void TSourceTextCollection::FromCompressedBase64(TStringBuf str) {
    FromJson(NAlice::ZLibDecompress(Base64DecodeUneven(str)), true);
}

// ~~~~ TReaderFromSourceTextCollection ~~~~

TReaderFromSourceTextCollection::TReaderFromSourceTextCollection(const TSourceTextCollection& collection)
    : Collection(collection)
{
}

bool TReaderFromSourceTextCollection::IsFile(const TFsPath& path) {
    return Collection.Texts.contains(path.GetPath());
}

TString TReaderFromSourceTextCollection::ReadTextFile(const TFsPath& path) {
    const TString* text = Collection.Texts.FindPtr(path.GetPath());
    Y_ENSURE(text, "Granet data not found: " + Cite(path));
    return *text;
}

} // namespace NGranet::NCompiler


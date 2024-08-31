#include "sample.h"
#include "entity_utils.h"
#include "tag.h"
#include <alice/nlu/granet/lib/utils/json_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/libs/lemmatization/lemmatize.h>
#include <alice/nlu/libs/normalization/normalize.h>
#include <alice/nlu/libs/token_aligner/joined_tokens.h>
#include <alice/nlu/libs/tokenization/tokenizer.h>
#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/json/json_writer.h>
#include <util/charset/utf8.h>
#include <util/generic/algorithm.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/stream/format.h>
#include <util/string/split.h>

namespace NGranet {

using namespace NJson;

namespace NScheme {
    static const TStringBuf Lang = "Lang";
    static const TStringBuf Text = "Text";
    // for granet tsv datasets
    namespace NCompact {
        static const TStringBuf Tokens = "Tokens";
        static const TStringBuf TokenBegin = "TokenBegin";
        static const TStringBuf TokenEnd = "TokenEnd";
        static const TStringBuf Entities = "Entities";
        static const TStringBuf Order[] = {Lang, Text, Tokens, TokenBegin, TokenEnd, Entities};
    }
    // for begemot response
    namespace NBegemot {
        static const TStringBuf AllEntities = "AllEntities";
    }
};

TSample::TSample()
{
}

TSample::TSample(TStringBuf text, ELanguage lang)
    : Lang(lang)
    , Text(text)
{
    InitTokens();
    InitHelpers();
}

void TSample::InitTokens() {
    Tokenize(Text, Lang, &Tokens, &TokensIntervals);
}

// static
void TSample::Tokenize(TStringBuf text, ELanguage lang, TVector<TString>* tokens,
    TVector<NNlu::TInterval>* tokensIntervals)
{
    NNlu::TSmartTokenizer tokenizer(text, lang);
    if (tokens) {
        *tokens = tokenizer.GetNormalizedTokens();
    }
    if (tokensIntervals) {
        *tokensIntervals = tokenizer.GetTokensIntervals();
    }
}

TSample::TSample(const TVector<TString>& tokens, ELanguage lang)
    : Lang(lang)
    , Tokens(Reserve(tokens.size()))
    , TokensIntervals(Reserve(tokens.size()))
{
    for (const TString& token : tokens) {
        if (!Text.empty()) {
            Text += ' ';
        }
        TokensIntervals.push_back({Text.length(), Text.length() + token.length()});
        Text += token;
        Tokens.push_back(NNlu::NormalizeWord(token, Lang));
    }

    InitHelpers();
}

// static
TSample::TRef TSample::Create(TStringBuf text, ELanguage lang, TVector<TString> tokens,
    TVector<NNlu::TInterval> intervals)
{
    return new TSample(text, lang, std::move(tokens), std::move(intervals));
}

TSample::TSample(TStringBuf text, ELanguage lang, TVector<TString> tokens, TVector<NNlu::TInterval> intervals)
    : Lang(lang)
    , Text(text)
    , Tokens(std::move(tokens))
    , TokensIntervals(std::move(intervals))
{
    Y_ENSURE(Tokens.size() == TokensIntervals.size());
    Y_ENSURE(TokensIntervals.empty() || TokensIntervals.back().End <= Text.length());

    InitHelpers();
}

TSample::TRef TSample::Copy() const {
    return new TSample(*this);
}

TSample::TSample(const TSample& other)
    : Lang(other.Lang)
    , Text(other.Text)
    , Tokens(other.Tokens)
    , JoinedTokens(other.JoinedTokens)
    , TokensIntervals(other.TokensIntervals)
    , Entities(other.Entities)
    , BestLemmas(other.BestLemmas)
    , SureLemmas(other.SureLemmas)
    , MapForBegins(other.MapForBegins)
    , MapForEnds(other.MapForEnds)
{
}

void TSample::InitHelpers() {
    JoinedTokens = NNlu::NJoinedTokens::JoinTokens(Tokens);
    InitLemmas();
    InitMaps();
}

void TSample::InitLemmas() {
    BestLemmas.clear();
    SureLemmas.clear();
    for (const auto& [i, token] : Enumerate(Tokens)) {
        const TVector<TString> variants = NNlu::LemmatizeWord(token, Lang, NNlu::SURE_LEMMA_THRESHOLD);
        Y_ENSURE(!variants.empty());
        BestLemmas.push_back(variants.front());
        SureLemmas.push_back(JoinSeq(TStringBuf(","), variants));
    }
}

void TSample::InitMaps() {
    Y_ASSERT(MapForBegins.empty());
    Y_ASSERT(MapForEnds.empty());

    MapForBegins.reserve(Text.length() + 1);
    MapForEnds.reserve(Text.length() + 1);
    for (size_t i = 0; i < Tokens.size(); ++i) {
        const NNlu::TInterval& interval = TokensIntervals[i];
        while (MapForBegins.size() <= interval.Begin) {
            MapForBegins.push_back(i);
        }
        while (MapForEnds.size() < interval.End) {
            MapForEnds.push_back(i);
        }
    }
    while (MapForBegins.size() <= Text.length()) {
        MapForBegins.push_back(TokensIntervals.size());
    }
    while (MapForEnds.size() <= Text.length()) {
        MapForEnds.push_back(TokensIntervals.size());
    }
}

// static
TSample::TRef TSample::CreateFromJsonValue(const TJsonValue& json, ELanguage lang) {
    return new TSample(json, lang);
}

// static
TSample::TRef TSample::CreateFromJsonString(const TString& string, ELanguage lang) {
    const TJsonValue json = NJsonUtils::ReadJsonStringVerbose(string, "");
    return CreateFromJsonValue(json, lang);
}

TSample::TSample(const TJsonValue& json, ELanguage lang) {
    Load(json, lang);
}

void TSample::AddEntityOnText(TEntity entity) {
    TEntity& added = Entities.emplace_back(std::move(entity));
    added.Interval = ConvertPositionToTokens(added.Interval);
}

void TSample::AddEntityOnTokens(TEntity entity) {
    Entities.push_back(std::move(entity));
}

void TSample::AddEntitiesOnTokens(const TVector<TEntity>& entities) {
    Extend(entities, &Entities);
}

NNlu::TInterval TSample::ConvertPositionToTokens(const NNlu::TInterval& src) const {
    Y_ENSURE(src.Valid());
    Y_ENSURE(src.End <= Text.length());

    NNlu::TInterval dest;
    dest.Begin = MapForBegins[src.Begin];
    dest.End = MapForEnds[src.End];
    Y_ASSERT(dest.Valid());
    return dest;
}

NNlu::TInterval TSample::ConvertPositionToText(const NNlu::TInterval& src) const {
    Y_ENSURE(src.Valid());
    Y_ENSURE(src.End <= Tokens.size());

    NNlu::TInterval dest = src;
    dest.Begin = TokensIntervals[src.Begin].Begin;
    dest.End = src.End > src.Begin ? TokensIntervals[src.End - 1].End : dest.Begin;
    Y_ASSERT(dest.Valid());
    return dest;
}

TJsonValue TSample::SaveToJsonValue() const {
    TJsonValue json;
    json[NScheme::Text] = Text;
    json[NScheme::Lang] = IsoNameByLanguage(Lang);
    json[NScheme::NCompact::Tokens] = JoinSeq(" ", Tokens);
    TJsonValue::TArray& beginnings = json[NScheme::NCompact::TokenBegin].SetType(JSON_ARRAY).GetArraySafe();
    TJsonValue::TArray& endings = json[NScheme::NCompact::TokenEnd].SetType(JSON_ARRAY).GetArraySafe();
    for (const NNlu::TInterval& interval : TokensIntervals) {
        beginnings.push_back(interval.Begin);
        endings.push_back(interval.End);
    }
    json[NScheme::NCompact::Entities] = SaveEntitiesToJsonCompact(Entities);
    return json;
}

TString TSample::SaveToJsonString() const {
    TJsonValue jsonValue = SaveToJsonValue();
    // Manual sort
    NJsonWriter::TBuf json;
    json.SetIndentSpaces(0);
    json.BeginObject();
    for (const TStringBuf& key : NScheme::NCompact::Order) {
        json.WriteKey(key).WriteJsonValue(&jsonValue[key]);
    }
    json.EndObject();
    return json.Str();
}

void TSample::Load(const TJsonValue& json, ELanguage lang) {
    Lang = lang;
    Text = json[NScheme::Text].GetStringSafe();

    InitTokens();
    InitHelpers();

    Entities.clear();
    LoadEntitiesFromJson(json[NScheme::NCompact::Entities])
        || LoadEntitiesFromJson(json[NScheme::NBegemot::AllEntities]);
}

bool TSample::LoadEntitiesFromJson(const NJson::TJsonValue& json) {
    if (!json.IsDefined()) {
        return false;
    }
    Entities = NGranet::LoadEntitiesFromJson(json);
    RemoveInvalidEntities(Tokens.size(), &Entities);
    return true;
}

NJson::TJsonValue TSample::SaveEntitiesToJson() const {
    return SaveEntitiesToJsonCompact(Entities);
}

TSampleMarkup TSample::GetEntitiesAsMarkup(TStringBuf filter, bool alwaysPositive) {
    TSampleMarkup markup;
    markup.IsPositive = alwaysPositive;
    markup.Text = Text;
    for (const TEntity& entity : Entities) {
        if (!filter.empty() && entity.Type != filter) {
            continue;
        }
        markup.IsPositive = true;
        TSlotMarkup& slot = markup.Slots.emplace_back();
        slot.Interval = ConvertPositionToText(entity.Interval);
        slot.Name = entity.Type;
        slot.Values.push_back(entity.Value);
    }
    Sort(markup.Slots);
    return markup;
}

void TSample::Dump(IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << "TSample:" << Endl;
    *log << indent << "  Lang: " << Lang << Endl;
    *log << indent << "  Text:   " << Text << Endl;
    *log << indent << "  Tokens: " << JoinSeq(" ", Tokens) << Endl;
    *log << indent << "  BestLemmas: " << JoinSeq(" ", BestLemmas) << Endl;
    *log << indent << "  SureLemmas: " << JoinSeq(" ", SureLemmas) << Endl;
    *log << indent << "  TokensIntervals: " << JoinSeq(", ", TokensIntervals) << Endl;
    *log << indent << "  Entities:" << Endl;
    DumpEntities(Entities, log, indent + "    ");
}

void TSample::DumpEntities(const TVector<TEntity>& entities, IOutputStream* log, const TString& indent) const {
    if (entities.empty()) {
        return;
    }
    *log << indent << "( " << JoinedTokens << " ) "
        << LeftPad("LogProb", 7) << "  "
        << RightPad("Source", 6) << "  "
        << RightPad("Type", 28) << "  "
        << "Value" << Endl;
    for (const TEntity& entity : entities) {
        *log << indent << "( " << PrintMaskedTokens(entity.Interval) << " ) "
            << Sprintf("%7.2f", entity.LogProbability) << "  "
            << RightPad(entity.Source, 6) << "  "
            << RightPad(entity.Type, 28) << "  "
            << entity.Value << Endl;
    }
}

TString TSample::PrintMaskedTokens(const NNlu::TInterval& interval) const {
    TStringBuilder out;
    for (const auto& [i, token] : Enumerate(Tokens)) {
        if (i > 0) {
            out << ' ';
        }
        if (interval.Contains(i)) {
            out << token;
        } else {
            out << TString(GetNumberOfUTF8Chars(token), ' ');
        }
    }
    return out;
}

TString TSample::GetTextByIntervalOnTokens(const NNlu::TInterval& tokenInterval) const {
    const NNlu::TInterval textInterval = ConvertPositionToText(tokenInterval);
    return Text.substr(textInterval.Begin, textInterval.Length());
}

} // namespace NGranet

#include "sample_graph.h"

#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/libs/lemmatization/lemmatize.h>

#include <kernel/inflectorlib/phrase/complexword.h>

#include <util/generic/algorithm.h>
#include <util/generic/xrange.h>
#include <util/string/join.h>
#include <util/string/split.h>

namespace NAlice::NNlu {

namespace {

constexpr double EXACT_LOG_PROB = 0;
constexpr double LEMMA_LOG_PROB = -1;

const TVector<TString> FIO_TYPES = {
    NGranet::NEntityTypes::SYS_FIO_NAME,
    NGranet::NEntityTypes::SYS_FIO_PATRONYM,
    NGranet::NEntityTypes::SYS_FIO_SURNAME
};

const TVector<TString> LEMMA_TYPES = {
    NGranet::NEntityTypes::SYN_THESAURUS_LEMMA,
    NGranet::NEntityTypes::SYN_THESAURUS_DIMIN_NAME_LEMMA,
    NGranet::NEntityTypes::SYN_THESAURUS_SYNSET_LEMMA,
    NGranet::NEntityTypes::SYN_THESAURUS_TRANSLIT_LEMMA,
    NGranet::NEntityTypes::SYN_THESAURUS_TRANSLIT_EN_LEMMA,
    NGranet::NEntityTypes::SYN_THESAURUS_TRANSLIT_RU_LEMMA,
    NGranet::NEntityTypes::SYN_TRANSLIT_EN_LEMMA,
    NGranet::NEntityTypes::SYN_TRANSLIT_RU_LEMMA
};

const TVector<TString> TRANSLIT_TYPES = {
    NGranet::NEntityTypes::SYN_TRANSLIT_EN,
    NGranet::NEntityTypes::SYN_TRANSLIT_RU
};

void AddSynonym(const ::NNlu::TInterval& interval, TStringBuf text, double logProb, TVector<TSynonym>* synonyms) {
    Y_ASSERT(synonyms);
    synonyms->push_back(TSynonym{
        .Interval = interval,
        .Text = TString(text),
        .LogProbability = logProb
    });
}

bool IsIn(const TVector<TString>& strings, const TString& string) {
    return Find(strings, string) != strings.end();
}

void AddEntityToSynonyms(const NGranet::TEntity& entity, TVector<TSynonym>* synonyms) {
    Y_ASSERT(synonyms);
    if (IsIn(FIO_TYPES, entity.Type)) {
        TStringBuf value = entity.Value;
        if (NGranet::TryRemoveBraces(&value, '"', '"')) {
            AddSynonym(entity.Interval, value, LEMMA_LOG_PROB, synonyms);
        }
    } else if (IsIn(LEMMA_TYPES, entity.Type) || IsIn(TRANSLIT_TYPES, entity.Type)) {
        double logProb = entity.LogProbability;
        if (IsIn(LEMMA_TYPES, entity.Type)) {
            logProb += LEMMA_LOG_PROB;
        }
        for (const auto& subValue : StringSplitter(entity.Value).Split(',')) {
            AddSynonym(entity.Interval, subValue, logProb, synonyms);
        }
    }
}

void AddNominativesAsLemma(const TVector<TString>& tokens, ELanguage lang, TVector<TString>& lemmas) {
    Y_ENSURE(tokens.size() == lemmas.size());
    static const TGramBitSet femine(gFeminine);
    static const TGramBitSet plural(gPlural);
    static const TGramBitSet nom(gNominative);

    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto tokenWide = UTF8ToWide(tokens[i]);
        const auto tokenLemmas = ::NNlu::GenerateLemmas(tokenWide, lang);
        const NInfl::TComplexWord tokenComplex(tokenWide, tokenLemmas);
        const bool needFemine = (tokenComplex.Grammems() & femine) == femine;
        const bool needPlural = (tokenComplex.Grammems() & plural) == plural;
        if (!needFemine && !needPlural) {
            continue;
        }

        TVector<TString> currentLemmas = StringSplitter(lemmas[i]).Split(',').ToList<TString>();
        const size_t currentLemmasSize = currentLemmas.size();
        for (size_t j = 0; j < currentLemmasSize; ++j) {
            NInfl::TComplexWord lemmaComplex(UTF8ToWide(currentLemmas[j]), tokenLemmas);
            TUtf16String inflectedResultWide;
            if (needFemine && lemmaComplex.Inflect(nom | femine, inflectedResultWide)) {
                ToLower(inflectedResultWide);
                auto inflectedResult = WideToUTF8(inflectedResultWide);
                if (!IsIn(currentLemmas, inflectedResult)) {
                    currentLemmas.push_back(std::move(inflectedResult));
                }
            }
            if (needPlural && lemmaComplex.Inflect(nom | plural, inflectedResultWide)) {
                ToLower(inflectedResultWide);
                auto inflectedResult = WideToUTF8(inflectedResultWide);
                if (!IsIn(currentLemmas, inflectedResult)) {
                    currentLemmas.push_back(std::move(inflectedResult));
                }
            }
        }

        if (currentLemmas.size() > currentLemmasSize) {
            lemmas[i] = JoinSeq(TStringBuf(","), currentLemmas);
        }
    }
}

} // namespace

TSampleGraph::TSampleGraph(const TVector<TString>& tokens, const TVector<TString>& lemmas,
                           const TVector<TSynonym>& synonyms) {
    Y_ENSURE(tokens.size() == lemmas.size());
    Vertices.resize(tokens.size() + 1);
    AddTokenArcs(tokens);
    AddLemmaArcs(lemmas);
    AddSynonymArcs(synonyms);
    RemoveDuplicates();
}

void TSampleGraph::AddTokenArcs(const TVector<TString>& tokens) {
    for (size_t i : xrange(tokens.size())) {
        const ::NNlu::TInterval interval = {i, i + 1};
        AddTokenArc(interval, tokens[i], EXACT_LOG_PROB);
    }
}

void TSampleGraph::AddLemmaArcs(const TVector<TString>& lemmas) {
    for (size_t i : xrange(lemmas.size())) {
        const ::NNlu::TInterval interval = {i, i + 1};
        for (const auto& lemma : StringSplitter(lemmas[i]).Split(',')) {
            AddTokenArc(interval, lemma, LEMMA_LOG_PROB);
        }
    }
}

void TSampleGraph::AddSynonymArcs(const TVector<TSynonym>& synonyms) {
    for (ui32 synonymIndex = 0; synonymIndex < synonyms.size(); ++synonymIndex) {
        AddSynonymArc(synonyms[synonymIndex]);
    }
}

void TSampleGraph::AddSynonymArc(const TSynonym& synonym) {
    if (synonym.Interval.Empty()) {
        // Not supported
        return;
    }
    Y_ASSERT(synonym.Interval.End < Vertices.size());
    AddTokenArc(synonym.Interval, synonym.Text, synonym.LogProbability);
}

void TSampleGraph::AddTokenArc(const ::NNlu::TInterval& interval, TStringBuf token, double logProb) {
    Vertices[interval.Begin].Arcs.push_back(TArc{
        .To = interval.End,
        .Token = TString(token),
        .LogProbability = logProb
    });
}

void TSampleGraph::RemoveDuplicates() {
    for (size_t i = 0; i < Vertices.size(); ++i) {
        Sort(Vertices[i].Arcs, [](const TSampleGraph::TArc& a, const TSampleGraph::TArc& b) {
            return tie(a.To, a.Token, b.LogProbability) < tie(b.To, b.Token, a.LogProbability);
        });
        Vertices[i].Arcs.erase(
            Unique(
                Vertices[i].Arcs.begin(),
                Vertices[i].Arcs.end(),
                [](const TSampleGraph::TArc& a, const TSampleGraph::TArc& b) {
                    return tie(a.To, a.Token) == tie(b.To, b.Token);
                }
            ),
            Vertices[i].Arcs.end()
        );
    }
}

const TVector<TSampleGraph::TArc>& TSampleGraph::GetArcsOnVertex(size_t vertexId) const {
    return Vertices[vertexId].Arcs;
}

size_t TSampleGraph::Size() const {
    return Vertices.size();
}

// static
TSampleGraph TSampleGraph::FromGranetSample(const NGranet::TSample::TConstRef& sample, bool addSynonyms, bool addNominativesAsLemma) {
    TVector<TSynonym> synonyms;
    if (addSynonyms) {
        synonyms.reserve(sample->GetEntities().size());
        for (const NGranet::TEntity& entity : sample->GetEntities()) {
            AddEntityToSynonyms(entity, &synonyms);
        }
    }

    TVector<TString> lemmas = sample->GetSureLemmas();
    if (addNominativesAsLemma) {
        AddNominativesAsLemma(sample->GetTokens(), sample->GetLanguage(), lemmas);
    }

    return TSampleGraph(sample->GetTokens(), lemmas, synonyms);
}

} // namespace NAlice::NNlu

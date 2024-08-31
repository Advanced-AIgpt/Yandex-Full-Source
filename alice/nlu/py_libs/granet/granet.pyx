from granet_lib cimport *

from cython.operator cimport dereference as deref
from libcpp cimport nullptr

from io import open
import os

from util.stream.str cimport TStringOutputPtr

cdef STANDARD_INDENT = TString("  ")


cdef class SourceTextCollection:
    cdef TSourceTextCollection sourceTextCollection
    cdef TString main

    def __cinit__(self):
        self.main = "Main.grnt"

    def add_source_for_import(self, key: bytes, text: bytes):
        self.sourceTextCollection.Texts[TString(key)] = TString(text)

    def update_main_text(self, text:bytes):
        self.sourceTextCollection.MainTextPath = self.main
        self.sourceTextCollection.Texts[self.main] = TString(text)

    def to_base64(self):
        return self.sourceTextCollection.ToCompressedBase64().data()

    def from_base64(self, s: bytes):
        self.sourceTextCollection.FromCompressedBase64(TStringBuf(s, len(s)))

    def add_all_from_path(self, path: bytes):
        path = path.rstrip(b'/')

        for root, dirs, files in os.walk(path):
            for file in files:
                if any(file.endswith(ext) for ext in [b".grnt", b".txt"]):
                    file_path = os.path.join(root, file)
                    with open(file_path, "r", encoding="utf8") as f:
                        content = f.read().encode('utf8')
                        file_key = os.path.join(root[len(path) + 1:], file)
                        self.add_source_for_import(file_key, content)


cdef class Element:
    cdef name
    cdef dataTypes
    cdef dataValues

    def __cinit__(self, name, dataTypes, dataValues):
        self.name = name
        self.dataTypes = dataTypes
        self.dataValues = dataValues

    def get_name(self):
        return self.name

    def get_data_types(self):
        return self.dataTypes

    def get_data_values(self):
        return self.dataValues


cdef class GrammarData:
    cdef TGrammarData grammarData

    def __cinit__(self, grammar: Grammar):
        self.grammarData = grammar.grammar.Get().GetData()

    def elements(self):
        for e in self.grammarData.Elements:
            name = e.Name
            dataTypes = [i for i in e.DataTypes]
            dataValues = [i for i in e.DataValues]
            yield Element(name, dataTypes, dataValues)

    def string_pool(self, idx):
        return self.grammarData.StringPool[idx]


cdef class Grammar:
    cdef TGrammar.TRef grammar
    cdef TReaderFromSourceTextCollection* sourceReader
    cdef TString sourceCollectionRoot
    cdef text

    def __cinit__(self, sourceCollection: SourceTextCollection, is_paskills: bool=False):
        self.text = sourceCollection.sourceTextCollection.Texts[sourceCollection.main].data()
        self.sourceCollectionRoot = sourceCollection.sourceTextCollection.MainTextPath
        self.sourceReader =  new TReaderFromSourceTextCollection(sourceCollection.sourceTextCollection)
        cdef TGranetDomain domain
        domain.IsPASkills = is_paskills
        self.grammar = CompileGrammarFromPath(TFsPath(sourceCollection.main), domain, <IDataLoader*>self.sourceReader)

    def __dealloc__(self):
        del self.sourceReader

    def data(self):
        return GrammarData(self)

    def to_base64(self):
        sourceTextCollection = SourceTextCollection()

        cdef TCompiler compiler
        sourceTextCollection.sourceTextCollection = compiler.CollectSourceTexts(
            TFsPath(self.sourceCollectionRoot),
            TGranetDomain(),
            <IDataLoader*>self.sourceReader)
        return sourceTextCollection.to_base64()

    def parse(self, sample: Sample):
        return ParserFormResults(self, sample)

    def get_text(self):
        return self.text


cdef class Sample:
    cdef TSample.TRef sample

    @classmethod
    def from_text(cls, s: bytes):
        obj = cls()
        obj._from_text(s)
        return obj

    @classmethod
    def from_tsv(cls, tsvSampleDataset: TsvSampleDataset, i: int):
        obj = cls()
        obj._from_tsv(tsvSampleDataset, i)
        return obj

    def _from_text(self, s: bytes):
        self.sample = CreateSample(TStringBuf(s, len(s)), ELanguage.LANG_RUS)
        FetchEntities(<const TSample.TRef&>self.sample, TGranetDomain(), TBegemotFetcherOptions())

    def _from_tsv(self, tsvSampleDataset: TsvSampleDataset, i: int):
        tsvSample = tsvSampleDataset.tsvSampleDataset.Get().ReadSample(i)
        self.sample = tsvSampleDataset.sampleCreator.Get().CreateSample(tsvSample, EEntitySourceType.EST_TSV)

    def get_text(self):
        return self.sample.Get().GetText().data()

    def get_mock(self):
        return self.sample.Get().SaveToJsonString().data()


cdef class SlotMarkup:
    cdef TSlotMarkup slot

    def __str__(self):
        return self.slot.PrintMarkup(False, False).data()

    def get_interval(self):
        return (self.slot.Interval.Begin, self.slot.Interval.End)

    def get_name(self):
        return self.slot.Name.data()

    def get_words_from_rule(self, grammar: Grammar):
        cdef TGrammarData grammarData = grammar.grammar.Get().GetData()
        cdef TTokenPool tokenPool = TTokenPool(grammarData.WordTrie)

        words = []
        for elem in grammarData.Elements:
            # if <bool>(elem.Flags & EF_IS_PUBLIC) and (not elem.IsEntity()):
            if elem.Name != self.slot.Name:
                continue
            for val in grammarData.RuleTriePool[elem.RuleTrieIndex]:
                rule, data = val.first, val.second
                for idf in rule:
                    if not IsElement(idf):
                        words.append(tokenPool.GetWord(idf).data())
        return words


cdef class MarkupSlots:
    cdef TVector[TSlotMarkup] slots
    cdef TVector[TSlotMarkup] nonterminalSlots

    def __cinit__(self, parserResults: ParserFormResults = None, formIdx: int = None):
        if parserResults is None or formIdx is None:  # emulating default c-tor
            return
        if not (0 <= formIdx < parserResults.parserFormResults.size()):
            raise IndexError
        if parserResults.is_positive(formIdx):
            self.slots = parserResults.parserFormResults[formIdx].Get().GetBestVariant().Get().ToMarkup()
            self.nonterminalSlots = parserResults.parserFormResults[formIdx].Get().GetBestVariant().Get().ToNonterminalMarkup(<TGrammar.TConstRef>(parserResults.grammar.Get()))

    def get_slot_by_name(self, name: bytes):
        for i in range(self.slots.size()):
            if self.slots[i].Name.data() == name:
                pySlot = SlotMarkup()
                pySlot.slot = <TSlotMarkup>self.slots[i]
                return pySlot

    def iter_nonterminals(self):
        for i in range(self.nonterminalSlots.size()):
            pySlot = SlotMarkup()
            pySlot.slot = <TSlotMarkup>self.nonterminalSlots[i]
            yield pySlot

    def nonterminals_as_str(self, text):
        cdef TSampleMarkup sample_markup = TSampleMarkup()
        sample_markup.Text = text
        sample_markup.Slots = self.nonterminalSlots

        return str(sample_markup.PrintMarkup(False, False).data())

cdef class ParserFormResults:
    cdef TVector[TParserFormResult.TConstRef] parserFormResults
    cdef TSample.TRef sample
    cdef TGrammar.TRef grammar  # to access nonterminal slots

    def __cinit__(self, grammar: Grammar, sample: Sample):
        if grammar is not None and sample is not None:
            self.parserFormResults = ParseSample(TGrammar.TConstRef(grammar.grammar.Get()), sample.sample)
            self.sample = sample.sample
            self.grammar = grammar.grammar

    def _assert_correct_form_index(self, formIdx):
        if not (0 <= formIdx < self.parserFormResults.size()):
            raise IndexError

    def is_positive(self, formIdx: int):
        self._assert_correct_form_index(formIdx)
        return self.parserFormResults[formIdx].Get().IsPositive()

    def get_form_name(self, formIdx: int):
        self._assert_correct_form_index(formIdx)
        return self.parserFormResults[formIdx].Get().GetName().data()

    def forms_count(self):
        return self.parserFormResults.size()

    def get_best_variant_slots(self, formIdx: int):
        self._assert_correct_form_index(formIdx)
        return MarkupSlots(self, formIdx)

    def get_best_variant_as_str(self, formIdx: int, needValues=False, needTypes=False):
        self._assert_correct_form_index(formIdx)
        markupSlots = MarkupSlots(self, formIdx).slots

        cdef TSampleMarkup sample_markup = TSampleMarkup()
        sample_markup.Text = self.sample.Get().GetText()
        sample_markup.Slots = markupSlots

        return str(sample_markup.PrintMarkup(needValues, needTypes).data())

    def dump_to_str(self, formIdx: int):
        self._assert_correct_form_index(formIdx)
        cdef TString s = TString()
        cdef TStringOutputPtr sout = TStringOutputPtr(new TStringOutput(s))
        self.parserFormResults[formIdx].Get().Dump(sout.Get(), STANDARD_INDENT)
        return s.data()


cdef class TsvSampleDataset:
    cdef TTsvSampleDataset.TRef tsvSampleDataset
    cdef TSampleCreatorWithCache.TRef sampleCreator
    cdef TVector[TSample.TRef] cSamples
    cdef samples

    def __cinit__(self):
        self.samples = []

    def load(self, path: bytes, from_mocks=True):
        self.tsvSampleDataset = new TTsvSampleDataset(TFsPath(TString(path, len(path))))
        cacheLimit = self.tsvSampleDataset.Get().Size()
        self.sampleCreator = TSampleCreatorWithCache.Create(TGranetDomain(), cacheLimit)
        mode = EEntitySourceType.EST_TSV if from_mocks else EEntitySourceType.EST_ONLINE
        for i in range(self.tsvSampleDataset.Get().Size()):
            tsvSample = self.tsvSampleDataset.Get().ReadSample(i);

            cSample = self.sampleCreator.Get().CreateSample(tsvSample, mode);
            self.cSamples.emplace_back(cSample)

            sample = Sample()
            sample.sample = cSample
            self.samples.append(sample)

    def load_part(self, sampleDataset: TsvSampleDataset, idxs: list):
        for i in idxs:
            self.cSamples.push_back(sampleDataset.cSamples[i])
            self.samples.append(sampleDataset.samples[i])

    def parse(self, grammar: Grammar, needValues=False, needTypes=False, num_threads=8, block_size=10):
        res = ParseInParallel(<const TGrammar.TRef&>grammar.grammar, self.cSamples, num_threads, block_size, needValues, needTypes)

        # TODO wrap to DatasetParserResults + current version will not work if empty string should be matched
        return [(res[i].data(), i) for i in range(len(self)) if len(res[i].data()) > 0]

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, i: int):
        return self.samples[i]


def parse_samples(samples: list, grammar: Grammar, needValues=False, needTypes=False, num_threads=8, block_size=10):
    cdef TVector[TSample.TRef] cSamples;
    for sample in samples:
        cSamples.push_back((<Sample>sample).sample)
    res = ParseInParallel(<const TGrammar.TRef&>grammar.grammar, cSamples, num_threads, block_size, needValues, needTypes)

    # TODO current version will not work if empty string should be matched
    return [(res[i].data(), i) for i in range(len(samples)) if len(res[i].data()) > 0]


def parse_all_samples(samples: list, grammar: Grammar, num_threads=8, block_size=10):
    cdef TVector[TSample.TRef] cSamples;
    for sample in samples:
        cSamples.push_back((<Sample>sample).sample)
    parseResults = ParseToResultsInParallel(<const TGrammar.TRef&>grammar.grammar, cSamples, num_threads, block_size)

    results = []
    cdef TString debugInfo = TString()
    cdef TStringOutputPtr debugInfoStream = TStringOutputPtr(new TStringOutput(debugInfo))
    cdef TVector[TSlotMarkup] slots
    cdef TSampleMarkup sampleMarkup = TSampleMarkup()

    for i in range(parseResults.size()):
        deref(debugInfoStream.Get()).Flush()
        debugInfo.clear()
        parseResults[i].Get().Dump(debugInfoStream.Get(), STANDARD_INDENT)

        if not parseResults[i].Get().GetVariants().empty():
            slots = parseResults[i].Get().GetBestVariant().Get().ToMarkup()
        sampleMarkup.Text = samples[i].get_text()
        sampleMarkup.Slots = slots
        markup = sampleMarkup.PrintMarkup(False, False).data()

        results.append((parseResults[i].Get().IsPositive(), debugInfo.data(), markup))
    return results


# fetch almost parsed means to pass grammar with .* in the filler and check parts matched by the rest of the grammar
# here we assume that grammar has .* only in the filler (keywords are still seen this way)
def parse_to_almost_matched_parts(samples: list, grammar: Grammar, num_threads=8, block_size=10):
    cdef TVector[TSample.TRef] cSamples;
    for sample in samples:
        cSamples.push_back((<Sample>sample).sample)

    parts = []
    cdef TVector[TVector[pair[TInterval, bool]]] results = ParseToAlmostMatchedPartsInParallel(grammar.grammar, cSamples, num_threads, block_size)
    for i in range(results.size()):
        sample_parts = []
        sample_text = cSamples[i].Get().GetText().data()
        for part in results[i]:
            text = sample_text[part.first.Begin:part.first.End]
            sample_parts.append((text, 1 if part.second else 0))
        parts.append(sample_parts)

    return parts

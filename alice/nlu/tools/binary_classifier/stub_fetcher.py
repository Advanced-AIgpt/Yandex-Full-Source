import json
import os
import requests
import urllib

from alice.nlu.tools.binary_classifier.dataset import Dataset
from alice.nlu.tools.binary_classifier.embeddings import find_embedding_in_collection


# ==== request_begemot ====

BEGEMOT_HAMSTER_URL = 'http://hamzard.yandex.net:8891/wizard'
BEGEMOT_BASIC_WIZEXTRA = 'alice_preprocessing=true;granet_print_sample_mock=true'
BEGEMOT_BASIC_RWR = ','.join([
    "AliceThesaurus", "AliceTranslit", "AliceEmbeddings", "AliceEmbeddingsExport", "AliceIot", "AliceIotConfigParser", "AliceCustomEntities",
    "AliceNonsenseTagger", "AliceNormalizer", "AliceRequest", "AliceSampleFeatures",
    "AliceSession", "AliceTokenEmbedder", "AliceTypeParserTime", "AliceUserEntities", "CustomEntities",
    "Date", "DirtyLang", "EntityFinder", "ExternalMarkup", "Fio", "FstAlbum", "FstArtist", "FstCalc",
    "FstCurrency", "FstDate", "FstDatetime", "FstDatetimeRange", "FstFilms100_750", "FstFilms50Filtered",
    "FstFio", "FstFloat", "FstGeo", "FstNum", "FstPoiCategoryRu", "FstSite", "FstSoft", "FstSwear",
    "FstTime", "FstTrack", "FstUnitsTime", "FstWeekdays", "GeoAddr", "Granet", "GranetCompiler",
    "GranetConfig", "IsNav", "Wares"])


def request_begemot(text, language='ru', begemot_url=BEGEMOT_HAMSTER_URL):
    params = {
        'text': text,
        'format': 'json',
        'wizclient': 'megamind',
        'tld': language,
        'uil': language,
        'wizextra': BEGEMOT_BASIC_WIZEXTRA,
        'rwr': BEGEMOT_BASIC_RWR,
    }
    url = begemot_url + '?' + urllib.parse.urlencode(params)
    resp = requests.get(url, timeout=3)
    resp.raise_for_status()
    return json.loads(resp.text)


# ==== StubFetcher ====

STUB_STORAGE_HEADER = ['text', 'mock', 'embedding_dssm']


class StubFetcher(object):
    def __init__(
            self,
            path=None,
            language='ru',
            header=STUB_STORAGE_HEADER,
            begemot_url=BEGEMOT_HAMSTER_URL):
        self.header = header
        self.language = language
        self.begemot_url = begemot_url
        self.path = path
        self.storage = Dataset(header=self.header)
        self.text_to_index = {}
        self.is_changed = True
        self._try_load()

    def _try_load(self):
        if not self.path:
            return False
        if not os.path.isfile(self.path):
            return False
        self.storage = Dataset(path=self.path).select_columns(self.header)
        self.text_to_index = {self.storage.get_text(i): i for i in range(len(self.storage.rows))}
        return True

    def save_changes(self):
        if not self.is_changed:
            return
        if not self.path:
            return
        self.storage.save(self.path)
        self.is_changed = False

    def fetch(self, text, stub_name):
        column_index = self.storage.columns[stub_name]
        if text in self.text_to_index:
            stub = self.storage.rows[self.text_to_index[text]][column_index]
            if stub:
                return stub
        row = self._do_fetch(text)
        self._write_row_to_cache(text, row)
        return row[column_index]

    def _write_row_to_cache(self, text, row):
        if text in self.text_to_index:
            self.storage.rows[self.text_to_index[text]] = row
        else:
            self.text_to_index[text] = len(self.storage.rows)
            self.storage.rows.append(row)

    def _do_fetch(self, text):
        # print('      Fetch for text:', text)
        resp = request_begemot(text, self.language, self.begemot_url)
        row = []
        for name in self.header:
            if name == 'text':
                row.append(text)
            elif name == 'mock':
                row.append(resp['rules']['Granet']['SampleMock'])
            elif name == 'embedding_dssm':
                embedding = find_embedding_in_collection(resp['rules']['AliceEmbeddingsExport']['Embeddings'], name)
                if embedding is None:
                    raise ValueError('Embedding %s is not found in Begemot response.' % name)
                row.append(embedding)
            else:
                raise ValueError('Unknown name of stub storage column: %s.' % name)
        return row


# ==== StubFetcherCollection ====

class StubFetcherCollection(object):
    def __init__(self):
        self.path_to_fetcher = {}

    def get_or_create(self, path=None):
        if not path:
            return None
        if path not in self.path_to_fetcher:
            self.path_to_fetcher[path] = StubFetcher(path)
        return self.path_to_fetcher[path]

    def save_changes(self):
        for fetcher in self.path_to_fetcher.values():
            fetcher.save_changes()

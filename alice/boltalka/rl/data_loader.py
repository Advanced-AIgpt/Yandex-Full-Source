import numpy as np
import yt.wrapper as yt
from tqdm import tqdm, trange
import re
import string

PUNCT_REGEX = re.compile('([' + string.punctuation + r'\\])')


def separate_punctuation(s):
    s = re.sub(PUNCT_REGEX, r' \1 ', s).strip()
    s = re.sub(r'\s+', ' ', s).strip()
    return s


def tqdm_yt_read(*args, **kwargs):
    table = args[0]
    return tqdm(yt.read_table(*args, **kwargs), total=yt.get(table + '/@row_count'))


def pad(sequences, target):
    max_len = max(len(el) for el in sequences)
    max_sub_len = max(max(len(sub_el) for sub_el in el) for el in sequences)
    data = np.zeros((len(sequences), max_len, max_sub_len))
    target_data = np.zeros((len(sequences), max_len, 1))
    mask = np.zeros_like(target_data)
    for i in range(len(sequences)):
        target_data[i, :len(sequences[i]), 0] = target[i]
        mask[i, :len(sequences[i])] = 1
        for j in range(len(sequences[i])):
            data[i, j, :len(sequences[i][j])] = sequences[i][j]
    return data, target_data, mask

class DataLoader:
    def __init__(self, table_path, batch_size):
        self.table_path = table_path
        self.batch_size = batch_size

    @property
    def size(self):
        return yt.get(self.table_path + '/@row_count') // self.batch_size

    def iter_batches(self):
        batch = []
        batch_target = []
        for x, y in self._iter_items():
            batch.append(x)
            batch_target.append(y)
            if len(batch) >= self.batch_size:
                yield self._process_batch(batch, batch_target)
                batch = []
                batch_target = []

    def tqdm_batches(self):
        return tqdm(self.iter_batches(), total=self.size)


class EmbeddingDataLoader(DataLoader):
    def __init__(self, table_path="//home/voice/nzinov/rudder_dsat_2409", batch_size=512, parse_target=None):
        self.parse_target = lambda x: x
        if parse_target is not None:
            self.parse_target = parse_target
        self.table_path = table_path
        self.batch_size = batch_size

    def _iter_items(self):
        for row in yt.read_table(self.table_path, format=yt.YsonFormat(encoding=None), unordered=False, enable_read_parallel=True):
            embedding = np.concatenate([np.fromstring(row[b'context_embedding'], dtype=np.float32), np.fromstring(row[b'reply_embedding'], dtype=np.float32)])
            target = self.parse_target(row.get(b'label', 0))
            yield (embedding, target)

    def _process_batch(self, batch, batch_target):
        return np.stack(batch), np.stack(batch_target)


class TextDataLoader(DataLoader):
    SKIP = '<SKIP>'

    def _iter_items(self):
        for row in yt.read_table(self.table_path):
            if row['feedback'] > -1:
                session = [(el[len(self.SKIP):], True) if el.startswith(self.SKIP) else (el, False) for el in row['session'].split('\t')]
                if len(session) > 5:
                    yield (session, row['feedback'])

    def _process_batch(self, batch, batch_target):
        return (batch, batch_target)


class SeqDataLoader(TextDataLoader):
    def __init__(self, table_path, batch_size, dictionary_path):
        super().__init__(table_path, batch_size)
        id2word = [line.strip().split()[0] for line in open(dictionary_path)]
        self.dictionary = {w: i + 2 for i, w in enumerate(id2word) if i + 2 < 20000}

    def _process_turn(self, turn):
        res = [self.dictionary.get(el, 0) for el in separate_punctuation(turn.lower()).split()]
        res.append(1)
        return res

    def _iter_items(self):
        for session, target in super()._iter_items():
            session = [self._process_turn(turn) for turn in session]
            if len(session) < 50 and all(len(turn) < 20 for turn in session):
                yield session, target

    def _process_batch(self, batch, batch_target):
        return pad(batch, batch_target)

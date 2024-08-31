# -*- coding: utf-8 -*-

import numpy as np
import codecs, re, string

import yt.wrapper as yt


BOS = '_BOS_'
EOS = '_EOS_'
UNK = '_UNK_'



def cut_prefix(line, max_len):
    if len(line) <= max_len:
        return line
    beg_idx = line.find(u' ', -(max_len+1)) + 1
    return line[beg_idx:]


def cut_suffix(line, max_len):
    if len(line) <= max_len:
        return line
    end_idx = line.rfind(u' ', 0, max_len+1)
    return line[:end_idx]


def cut_line(line, max_len):
    if max_len > 0:
        return cut_suffix(line, max_len)
    return cut_prefix(line, -max_len)


def convert_line_to_word_ngrams_ids(line, dct, n, bos=BOS, eos=EOS):
    ids = []
    words = [bos] + line.split(u' ') + [eos]
    for i in xrange(len(words)-n+1):
        token = u' '.join(words[i:i+n])
        if token in dct:
            ids.append(dct[token])
    return ids


def convert_line_to_word_ids(line, dct, unk=None, bos=None, eos=None):
    ids = []
    if bos:
        ids.append(dct[bos])
    words = line.split(u' ')
    for word in words:
        if word in dct:
            ids.append(dct[word])
        elif unk:
            ids.append(dct[unk])
    if eos:
        ids.append(dct[eos])
    return ids


def convert_line_to_char_ngrams_ids(line, dct, n):
    ids = []
    line = u' '+line+u' '
    for i in xrange(len(line)-n+1):
        token = line[i:i+n]
        if token in dct:
            ids.append(dct[token])
    return ids


def convert_line_to_char_ngrams_ids_skip_spaces(line, dct, n):
    ids = [convert_line_to_char_ngrams_ids(word, dct, n)
           for word in line.split(u' ')]
    return ids


def drop_tokens(seq, drop_prob, unk_code=None):
    mask = np.random.uniform(size=len(seq)) > drop_prob
    new_seq = []
    for x, flag in zip(seq, mask):
        if flag:
            new_seq.append(x)
        elif not unk_code is None:
            new_seq.append(unk_code)
    return new_seq


def line_converter_for_bow(line, word_dct, trigram_dct, wbigram_dct=None, word_drop_prob=0.0):
    output = [convert_line_to_word_ids(line, word_dct),
              convert_line_to_char_ngrams_ids(line, trigram_dct, n=3)]
    if wbigram_dct:
        output.append(convert_line_to_word_ngrams_ids(line, wbigram_dct, n=2))
    if word_drop_prob != 0.0:
        output[0] = drop_tokens(output[0], word_drop_prob)
    return output


def line_converter_for_bow_dense(line, word_dct, trigram_dct, wbigram_dct=None, word_drop_prob=0.0):
    output = [convert_line_to_word_ids(line, word_dct),
              convert_line_to_char_ngrams_ids_skip_spaces(line, trigram_dct, n=3)]
    if wbigram_dct:
        output.append(convert_line_to_word_ngrams_ids(line, wbigram_dct, n=2))
    if word_drop_prob != 0.0:
        output[0] = drop_tokens(output[0], word_drop_prob)
    return output


def line_converter_for_lm(line, word_dct, trigram_dct, word_drop_prob=0.0):
    output =  [convert_line_to_word_ids(line, word_dct, unk=UNK),
               convert_line_to_char_ngrams_ids_skip_spaces(line, trigram_dct, n=3)]
    if word_drop_prob != 0.0:
        output[0] = drop_tokens(output[0], word_drop_prob, word_dct[UNK])
    output[0].append(word_dct[EOS])
    output[1].append([])
    return output


def parse_column_settings(cols_settings, is_yt=False):
    text_cols = []
    label_col = None

    for col_settings in cols_settings.split(','):
        if col_settings[0] == '[' and col_settings[-1] == ']':
            label_col = col_settings[1:-1]
        else:
            parts = col_settings.split(':')
            if len(parts) == 1:
                parts.append(None)
            text_cols.append(tuple(parts))

    if not is_yt:
        text_cols = [(int(name), _) for name, _ in text_cols]
        if label_col:
            label_col = int(label_col)

    return text_cols, label_col


class Preprocessor(object):
    def __init__(self, columns_settings, strip_punctuation, keep_marks, line_converter, converter_params, is_yt=False):
        self.is_yt = is_yt

        self.text_cols, self.label_col = parse_column_settings(columns_settings, is_yt)

        self.strip_punctuation = strip_punctuation
        # self.strip_punctuation += '–'
        punctuation = ''.join([p for p in string.punctuation if not p in set(keep_marks)])
        self.punctuation_regex = re.compile(u'['+punctuation+']', re.UNICODE)

        self.line_converter = line_converter
        self.converter_params = converter_params

    def __call__(self, row):
        line_ids = []

        if not self.is_yt:
            row = row.rstrip(u'\n').split(u'\t')

        for col, cut_len in self.text_cols:
            part = row[col]
            if self.is_yt:
                part = part.decode('utf-8')
            if cut_len:
                part = cut_line(part, cut_len)
            if self.strip_punctuation:
                part = self.punctuation_regex.sub(' ', part)
                part = re.sub('\s+', ' ', part).strip()
            part = self.line_converter(part, **self.converter_params)
            line_ids.append(part)

        if len(line_ids) == 1:
            line_ids = line_ids[0]

        if self.label_col:
            line_ids.append(int(row[self.label_col]))

        return line_ids


def iterate_batches_one_pass(f, batch_size, preprocessor, first_batch_idx):
    batch = []
    batch_idx = first_batch_idx

    for _ in xrange(first_batch_idx * batch_size):
        next(f)

    for line in f:
        batch.append(preprocessor(line))
        if len(batch) == batch_size:
            yield np.array(batch), batch_idx
            batch = []
            batch_idx += 1
    if batch:
        yield np.array(batch), batch_idx


def iterate_batches(path, columns_settings, batch_size,
                    word_dct, trigram_dct, wbigram_dct=None,
                    line_converter=None, strip_punctuation=False, keep='',
                    is_yt=False, skip_to_count=0,
                    word_drop_prob=0.0,
                    first_epoch_idx=0, first_batch_idx=0,
                    infinite=False):

    converter_params = {'word_dct': word_dct,
                        'trigram_dct': trigram_dct,
                        'word_drop_prob': word_drop_prob}
    if wbigram_dct:
        converter_params['wbigram_dct'] = wbigram_dct

    preprocessor = Preprocessor(columns_settings, strip_punctuation, keep, line_converter, converter_params, is_yt)

    run = True
    epoch_idx = first_epoch_idx

    # batch_size -- number of positive pairs
    batch_size *= 1 + skip_to_count

    while run:
        run = infinite
        f = yt.read_table(path) if is_yt else codecs.open(path, 'r', 'utf-8')
        for batch, batch_idx in iterate_batches_one_pass(f, batch_size, preprocessor, first_batch_idx):
            yield batch, batch_idx, epoch_idx
        if not is_yt:
            f.close()
        first_batch_idx = 0
        epoch_idx += 1


def load_dictionary(filename, addition=[]):
    dct = {}
    for i, line in enumerate(codecs.open(filename, 'r', 'utf-8')):
        token = line.rstrip(u'\n')
        dct[token] = i
    for token in addition:
        if not token in dct:
            dct[token] = len(dct)
    return dct


def reverse_dictionary(dct):
    return {value: key for key, value in dct.iteritems()}

import logging
import codecs
import collections
import numpy as np

BOS = '_BOS_'
EOS = '_EOS_'
UNK = '_UNK_'

dssm_logger = logging.getLogger(__name__)


def get_apply_feed(feed_type, input):
    input_feed = {}
    if feed_type == 'text_encoder':
        word_ids, num_words = get_batch(input[:, 0])
        trigram_ids, trigram_batch_map = get_sparse_batch(
            [subline for line in input[:, 1] for subline in line]
        )
        input_feed['word_ids'] = word_ids
        input_feed['num_words'] = num_words
        input_feed['trigram_ids'] = trigram_ids
        input_feed['trigram_batch_map'] = trigram_batch_map
    else:
        raise NotImplementedError("feed rules for '{}' are not implemented.".format(feed_type))
    return input_feed


def pad_line(line, length, pad):
    return line + [pad] * (length - len(line))


def get_batch(input):
    num_tokens = []
    for line in input:
        num_tokens.append(len(line))
    max_num_tokens = max(num_tokens)

    # empty batch plug
    if max_num_tokens == 0:
        max_num_tokens = 1

    token_ids = []
    for line in input:
        line = pad_line(list(line), max_num_tokens, 0)
        token_ids.append(line)
    return np.array(token_ids), np.array(num_tokens)


def get_sparse_batch(input):
    indices = []
    values = []
    batch_map = []
    repr_idx = 0

    for line in input:
        if len(line) == 0:
            batch_map.append(-1)
            continue
        batch_map.append(repr_idx)
        values.extend(line)
        indices.extend(zip([repr_idx] * len(line), xrange(len(line))))
        repr_idx += 1

    batch_map = np.array(batch_map)
    batch_map[batch_map == -1] = repr_idx

    # empty batch plug -- ignore it if batch_map[-1] == 0
    if repr_idx == 0:
        indices = [[0]]
        values = [0]

    SparseMatrix = collections.namedtuple('SparseMatrix', ['indices', 'values'])
    return SparseMatrix(indices, values), batch_map


def load_dictionary(filename, addition=[], max_dct_size=None, keep_indices_filename=None):
    dct = {}
    total_dct_size = 0
    keep_index = set()
    if keep_indices_filename is not None:
        for line in (codecs.open(keep_indices_filename, "r", "utf-8")):
            keep_index.add(int(line.rstrip(u'\n')))

    for i, line in enumerate(codecs.open(filename, "r", "utf-8")):
        total_dct_size += 1
        if (max_dct_size is None or len(dct) < max_dct_size) and \
                (keep_indices_filename is None or i in keep_index):
            token = line.rstrip(u'\n')
            dct[token] = i
    for token in addition:
        total_dct_size += 1
        if token not in dct:
            dct[token] = len(dct)
    dssm_logger.info("Actual dict size is %s" % len(dct))
    dssm_logger.info("Total dict size is %s" % total_dct_size)
    return dct, total_dct_size


def line_converter_for_lm(line, word_dct, trigram_dct, wbigram_dct=None):
    output = [convert_line_to_word_ids(line, word_dct, unk=UNK),
              convert_line_to_char_ngrams_ids_skip_spaces(line, trigram_dct, n=3)]
    output[0].append(word_dct[EOS])
    output[1].append([])
    return output


def convert_line_to_word_ids(line, dct, unk=None, bos=None, eos=None):
    ids = []
    if bos:
        ids.append(dct[bos])
    words = line.split(u" ")
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
    line = u" " + line + u" "
    for i in xrange(len(line) - n + 1):
        token = line[i:i + n]
        if token in dct:
            ids.append(dct[token])
    return ids


def convert_line_to_char_ngrams_ids_skip_spaces(line, dct, n):
    ids = [convert_line_to_char_ngrams_ids(word, dct, n)
           for word in line.split(u" ")]
    return ids

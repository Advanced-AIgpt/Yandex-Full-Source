import numpy as np
import scipy.special
import tfnn.data

from ml_data_reader.parsers import BaseParser
from ml_data_reader.text_processing.tokens import ids_and_words_from_line, lines2ids
from ml_data_reader.text_processing.parsers import SplitVocParser


def ids_and_words_from_line_insertion(line, voc, **kwargs):
    ids, words = ids_and_words_from_line(line, voc, bos=1, eos=1, **kwargs)
    # Remove redundant BOS and EOS tokens
    ids = ids[1:-1]
    return ids, words


def lines2ids_insertion(lines, voc, line_sampling_mode, n_tokens, inslot_smoothing_mode, temperature, **kwargs):
    # Read as-is, without padding.
    ids_all = []
    words_all = []
    for line in lines:
        ids, words = ids_and_words_from_line_insertion(line, voc, **kwargs)
        ids_all.append(ids)
        words_all.append(words)
    lengths = np.asarray([len(ids) for ids in ids_all], dtype=np.int32)
    if line_sampling_mode == 'insertion':
        # Sample k ~ U(0,...,|y|)
        ks = np.ceil(np.random.uniform(-1, lengths)).astype(np.int32)
        # Get substring of indexes of length k
        sublines = [np.sort(np.random.choice(length, k, replace=False)) for length, k in zip(lengths, ks)]
    elif line_sampling_mode == 'last_n_tokens':
        prefix_lengths = np.maximum(np.zeros_like(lengths), lengths - n_tokens)
        # Sample suffix length ~ U(0,...,n_tokens)
        suffix_lengths = np.ceil(np.random.uniform(-1, lengths - prefix_lengths)).astype(np.int32)
        prefixes = [np.arange(prefix_length) for prefix_length in prefix_lengths]
        suffixes = [
            prefix_length + np.sort(np.random.choice(length - prefix_length, suffix_length, replace=False))
            for length, prefix_length, suffix_length in zip(lengths, prefix_lengths, suffix_lengths)
        ]
        ks = prefix_lengths + suffix_lengths
        sublines = [np.concatenate([prefix, suffix]) for prefix, suffix in zip(prefixes, suffixes)]
    elif line_sampling_mode == 'left_to_right':
        # Sample k ~ U(0,...,|y|)
        ks = np.ceil(np.random.uniform(-1, lengths)).astype(np.int32)
        # Get prefixes of lenght k
        sublines = [np.arange(k) for k in ks]
    else:
        raise ValueError("Inslot smoothing might be one of {'insertion', 'last_token', 'left_to_right'}")
    # Get COO representation of slots which corresponds to lines where all slots are empty (sequence finalization mode)
    finalizing_slots_all = []
    for line_n, (k, length, is_finished) in enumerate(zip(ks, lengths, ks == lengths)):
        for insert_pos in range(is_finished * (k + 1)):
            finalizing_slots_all.append((line_n, insert_pos))
    finalizing_slots_all = np.asarray(finalizing_slots_all, dtype=np.int32).reshape([-1, 2])
    # Get slots corresponding each line
    slots_all = []
    for ids, subline in zip(ids_all, sublines):
        slots = []
        prev_value = -1
        for value in subline:
            slots.append(ids[prev_value + 1:value])
            prev_value = value
        slots.append(ids[prev_value + 1:])
        slots_all.append(slots)
    # Calc weights of tokens in each slot
    weights_all = []
    for slots in slots_all:
        weights = []
        for slot in slots:
            if inslot_smoothing_mode == 'binary_tree':
                weights.append(
                    scipy.special.softmax(
                        np.abs((len(slot) - 1) / 2 - np.arange(len(slot), dtype=np.float32)) / temperature
                    ) if len(slot) > 0 else np.array([], dtype=np.float32)
                )
            elif inslot_smoothing_mode == 'uniform':
                weights.append(
                    np.ones_like(slot, dtype=np.float32) / len(slot)
                    if len(slot) > 0 else np.array([], dtype=np.float32)
                )
            elif inslot_smoothing_mode == 'left_to_right':
                weights.append(
                    np.reshape(np.eye(1, len(slot), dtype=np.float32), [-1])
                    if len(slot) > 0 else np.array([], dtype=np.float32)
                )
            else:
                raise ValueError("Inslot smoothing might be one of {'binary_tree', 'uniform', 'left_to_right'}")
        weights_all.append(weights)
    # Concat all weights into single array
    weights = np.concatenate([slot_weights for weights in weights_all for slot_weights in weights])
    # Convert slots into COO representation
    inserts_all = []
    # Save information about empty slots to evaluate termination part of loss (slot finalization model)
    empty_slots_all = []
    for line_n, slots in enumerate(slots_all):
        for insert_pos, slot in enumerate(slots):
            for insert_token in slot:
                inserts_all.append((line_n, insert_pos, insert_token))
            if len(slot) == 0:
                empty_slots_all.append((line_n, insert_pos))
    inserts_all = np.asarray(inserts_all, dtype=np.int32).reshape([-1, 3])
    empty_slots_all = np.asarray(empty_slots_all, dtype=np.int32).reshape([-1, 2])
    # Append BOS and EOS to be able to get slot representation further
    ids_all_expanded = []
    for ids, subline in zip(ids_all, sublines):
        ids_all_expanded.append([voc.BOS] + list(np.take(ids, subline)) + [voc.EOS])
    # Pad and transpose.
    ids_all_expanded, _ = tfnn.data.pad_seq_list(ids_all_expanded, voc.EOS)
    ids_all_expanded = np.asarray(ids_all_expanded, dtype=np.int32)
    words_all, _ = tfnn.data.pad_seq_list(words_all, voc.words(voc.EOS))
    return (
        lengths,                # [n_lines] -- lengths of original output
        ids_all_expanded,       # [n_lines, (n_out + 2)] -- substring of original output padded with BOS and EOS
        words_all,
        ks + 1,                 # [n_lines] -- number of slots defined by selected substring
        weights,                # [n_inserts] -- smoothed weights of tokens in all spans
        inserts_all,            # [n_inserts, 3] -- COO representation of correct insertions
        empty_slots_all,        # [n_empty_slots, 2] -- COO representation of empty slots (for EOSLOT insertions)
        finalizing_slots_all    # [n_finished_lines, 2] -- COO representation of competed lines (for EOS insertions)
    )


def make_batch_data_insertion(
        batch, vocs, force_bos, names, add_inp_words=False, **kwargs
):
    """
    :param batch:
    :param vocs:
    :param force_bos:
    :param names:
    :param add_inp_words:
    :keyword merge_unks: bool
    :keyword line_sampling_mode: {'insertion', 'last_n_tokens', 'left_to_right'}
    :keyword n_tokens: int
    :keyword inslot_smoothing_mode: {'binary_tree', 'uniform', 'left_to_right'}
    :keyword temperature: float
    :return:
    """
    assert len(names) == len(set(names)), "Names must be distinct: {}".format(names)
    assert len(vocs) == len(names), \
        "Number of vocabularies ({}) must match the number of names ({})".format(
            len(vocs), len(names)
        )
    assert tfnn.util.is_iterable(batch) and not isinstance(batch, dict), "Applying make_batch_data to non-sequence"
    batch = zip(*batch)
    batch_data = {}
    for i, (name, lines, voc) in enumerate(zip(names, batch, vocs)):
        if name == 'inp':
            sentences, lengths, words = lines2ids(lines, voc, bos=force_bos, **kwargs)
            if add_inp_words:
                batch_data['inp_words'] = np.array(words, dtype=np.str)
            batch_data.update({
                'inp': np.array(sentences, dtype=np.int32),
                'inp_len': np.array(lengths, dtype=np.int32)
            })
        elif name == 'out':
            lengths, sublines, _, n_slots, weights, inserts, empty_slots, finalizing_slots = lines2ids_insertion(
                lines, voc, **kwargs
            )
            batch_data.update({
                'out_len': lengths,
                'out_sublines': sublines, 'n_slots': n_slots,
                'weights': weights, 'inserts': inserts,
                'empty_slots': empty_slots, 'finalizing_slots': finalizing_slots
            })
        elif name == 'condition':
            sentences, lengths, words = lines2ids(lines, voc, bos=False, eos=False, **kwargs)
            lengths = np.array(lengths, dtype=np.int32)
            sentences = np.array(sentences, dtype=np.int32)
            batch_data.update({
                'condition_len': lengths,
                'condition': sentences, 'condition_n_slots': lengths + 1,
            })
        else:
            raise ValueError(
                "Insertion Transformer model support only 'inp', 'out', 'condition' names in data batch. \n"
                "Check you reader implementation to satisfy it."
            )
    return batch_data


class SplitVocParserOutInsertion(BaseParser):
    def __init__(self, name, voc, skip_dev=False, default_on_dev=None, line_sampling_mode='insertion', n_tokens=0,
                 inslot_smoothing_mode='binary_tree', temperature=1.0):
        super().__init__(name, skip_dev=skip_dev, default_on_dev=default_on_dev)
        self.voc = voc
        self.line_sampling_mode = line_sampling_mode
        self.n_tokens = n_tokens
        self.inslot_smoothing_mode = inslot_smoothing_mode
        self.temperature = temperature

    def __call__(self, lines):
        lengths, sublines, _, n_slots, weights, inserts, empty_slots, finalizing_slots = lines2ids_insertion(
            lines, self.voc, line_sampling_mode=self.line_sampling_mode, n_tokens=self.n_tokens,
            inslot_smoothing_mode=self.inslot_smoothing_mode, temperature=self.temperature
        )

        return {
            'out_len': lengths,
            'out_sublines': sublines, 'n_slots': n_slots,
            'weights': weights, 'inserts': inserts,
            'empty_slots': empty_slots, 'finalizing_slots': finalizing_slots
        }


class SplitVocParserConditionInsertion(SplitVocParser):
    def __init__(self, *args, **kwargs):
        assert kwargs.get('force_eos', False) is False, 'Condition parser removes unnecessary eos at the end'
        super().__init__(*args, force_eos=False, **kwargs)

    def __call__(self, lines):
        batch_data = super().__call__(lines)
        batch_data['{}_n_slots'.format(self.name)] = batch_data['{}_len'.format(self.name)] + 1
        return batch_data

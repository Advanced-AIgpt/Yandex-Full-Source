import yt.wrapper as yt
import numpy as np

import torch
import torch.nn as nn


yt.config["proxy"]["url"] = "hahn.yt.yandex.net"
yt.config["read_parallel"]["enable"] = True
yt.config["read_parallel"]["max_thread_count"] = 64


interests = yt.read_table("//home/voice/nickpon/clustered_interests/clustered_replies")

interests_list = sorted(list(set(row['clustered_reply'] for row in interests)))
interests_dict = {x : i for i, x in enumerate(interests_list)}

def encode_intrests_list(interests) :
    label = np.zeros(len(interests_dict))
    for interest in interests:
        label[interests_dict[interest.decode('utf-8')]] += 1
    label /= np.sum(label)
    return label

class FeedbackSessionsDataset(object):
    def __init__(self, src_table):
        """
        Parameters
        ----------
        src_table : str
            YT table with columns `session_id`, `uuid`, `phrase_idx`, `context_embedding`, `feedback_value`.
            Must be sorted by (session_id, phrase_idx).
        """
        self.src_table = src_table
        self.row_count = yt.row_count(src_table)

    @classmethod
    def _vec_from_binary(cls, binary_str):
        return np.fromstring(binary_str, dtype=np.float32)

    def _make_iter(self):
        table_it = yt.read_table(self.src_table, format=yt.YsonFormat(encoding=None))

        first_row = next(table_it)
        sample = [[self._vec_from_binary(first_row[b'context_embedding']), self._vec_from_binary(first_row[b'reply_embedding'])], encode_intrests_list(first_row[b'updated_interests']), first_row[b'session_id']]
        for row in table_it:
            if row[b'session_id'] != sample[2]:
                self._prepare_for_return(sample)

                yield sample

                sample = [[self._vec_from_binary(row[b'context_embedding']), self._vec_from_binary(row[b'reply_embedding'])], encode_intrests_list(row[b'updated_interests']), row[b'session_id']]
            else:
                sample[0].append(self._vec_from_binary(row[b'context_embedding']))
                sample[0].append(self._vec_from_binary(row[b'reply_embedding']))
        self._prepare_for_return(sample)
        yield sample

    def _prepare_for_return(self, sample):
        sample[0].pop()
        sample[0] = np.vstack(sample[0])

    def __iter__(self):
        return self._make_iter()

    @classmethod
    def str_to_label_ix(cls, label_str):
        return int(label_str)


def make_batch(dataset_iterator, batch_size, return_samples=False):
    samples = [next(dataset_iterator) for _ in range(batch_size)]

    return make_batch_from_samples(samples, return_samples=return_samples)


def make_batch_from_samples(samples, return_samples=False):
    # Sort by descending order of lengths because of PyTorch PackedSequences.
    samples = sorted(samples, key=lambda s: s[0].shape[0], reverse=True)
    lengths = [s[0].shape[0] for s in samples]
    embeddings = [torch.from_numpy(s[0]) for s in samples]
    labels = torch.from_numpy(np.vstack([s[1] for s in samples]))

    # Pad each embeddings array from (lengths[i], vec_dim) to (max_length, vec_dim).
    embeddings = nn.utils.rnn.pad_sequence(embeddings, batch_first=True)

    if return_samples:
        return nn.utils.rnn.pack_padded_sequence(embeddings, lengths, batch_first=True), labels, samples
    else:
        return nn.utils.rnn.pack_padded_sequence(embeddings, lengths, batch_first=True), labels

def make_batch_from_samples_with_prefexes(samples, return_samples=False):
    # Sort by descending order of lengths because of PyTorch PackedSequences.
    expected_lengthes = np.random.randint(5, 120, len(samples))
    for sample, l in zip(samples, expected_lengthes):
        sample[0] = sample[0][:l]
    samples = sorted(samples, key=lambda s: s[0].shape[0], reverse=True)
    lengths = [s[0].shape[0] for s in samples]
    embeddings = [torch.from_numpy(s[0]) for s in samples]
    labels = torch.from_numpy(np.vstack([s[1] for s in samples]))

    # Pad each embeddings array from (lengths[i], vec_dim) to (max_length, vec_dim).
    embeddings = nn.utils.rnn.pad_sequence(embeddings, batch_first=True)

    if return_samples:
        return nn.utils.rnn.pack_padded_sequence(embeddings, lengths, batch_first=True), labels, samples
    else:
        return nn.utils.rnn.pack_padded_sequence(embeddings, lengths, batch_first=True), labels

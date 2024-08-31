import tqdm
import torch
import torch.nn as nn
import torch.nn.functional as F
import yt.wrapper as yt
import numpy as np

yt.config["proxy"]["url"] = "hahn.yt.yandex.net"
yt.config["read_parallel"]["enable"] = True
yt.config["read_parallel"]["max_thread_count"] = 64


class LSTMScorer(nn.Module):
    def __init__(self, input_size=300, num_layers=1, hidden_size=256):
        super(LSTMScorer, self).__init__()

        self.lstm = nn.LSTM(input_size=input_size, num_layers=num_layers,
                            hidden_size=hidden_size, batch_first=True)
        self.linear = nn.Linear(hidden_size * num_layers, input_size)
        self.reply_encoder = nn.Sequential(nn.Linear(input_size, input_size), nn.ReLU(
        ), nn.Linear(input_size, input_size), nn.ReLU(), nn.Linear(input_size, input_size))
        self.final_change = nn.Linear(1, 1)

    def forward(self, input, replies):
        (output, (h_n, c_n)) = self.lstm(input)
        output = self.linear(output)
        output = output[:, ::2, :]
        rel_embeds = self.reply_encoder(replies)
        rel_embeds = F.normalize(self.reply_encoder(replies), dim=2)
        output = F.normalize(output, dim=2)

        scores = (rel_embeds * output).sum(2)
        scores = self.final_change(scores.unsqueeze(2)).squeeze(2)

        return scores

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
        table_it = yt.read_table(
            self.src_table, format=yt.YsonFormat(encoding=None))
        i = 0
        first_row = next(table_it)
        sample = [[self._vec_from_binary(first_row[b'context_embedding']), self._vec_from_binary(first_row[b'reply_embedding'])], [
            first_row[b'delta']], first_row[b'session_id'], [self._vec_from_binary(first_row[b'reply_embedding'])]]
        for row in table_it:
            if row[b'session_id'] != sample[2]:
                self._prepare_for_return(sample)

                yield sample
                i += 1
                sample = [[self._vec_from_binary(row[b'context_embedding']), self._vec_from_binary(row[b'reply_embedding'])], [
                    first_row[b'delta']], row[b'session_id'], [self._vec_from_binary(row[b'reply_embedding'])]]
            else:
                sample[0].append(self._vec_from_binary(
                    row[b'context_embedding']))
                sample[0].append(self._vec_from_binary(
                    row[b'reply_embedding']))
                sample[-1].append(self._vec_from_binary(row[b'reply_embedding']))
                sample[1].append(row[b'delta'])
        self._prepare_for_return(sample)
        yield sample

    def _prepare_for_return(self, sample):
        sample[0].pop()
        sample[0] = np.vstack(sample[0])
        sample[-1] = np.vstack(sample[-1])

    def __iter__(self):
        return self._make_iter()

    @classmethod
    def str_to_label_ix(cls, label_str):
        return int(label_str)


def make_batch(dataset_iterator, batch_size, return_samples=False):
    samples = [next(dataset_iterator) for _ in range(batch_size)]

    return make_batch_from_samples(samples, return_samples=return_samples)


def make_batch_from_samples(samples, return_samples=False):
    samples = sorted(samples, key=lambda s: s[0].shape[0], reverse=True)
    lengths = torch.from_numpy(np.array([s[-1].shape[0] for s in samples]))
    embeddings = [torch.from_numpy(s[0]) for s in samples]
    reply_embeddings = [torch.from_numpy(s[-1]) for s in samples]
    labels = nn.utils.rnn.pad_sequence(
        [torch.from_numpy(np.array(s[1])) for s in samples], batch_first=True)
    embeddings = nn.utils.rnn.pad_sequence(embeddings, batch_first=True)
    reply_embeddings = nn.utils.rnn.pad_sequence(
        reply_embeddings, batch_first=True)
    if return_samples:
        return embeddings, reply_embeddings, labels, lengths, samples
    else:
        return embeddings, reply_embeddings, labels, lengths


def make_batch_from_samples_with_prefexes(samples, return_samples=False):
    expected_lengthes = np.random.randint(5, 60, len(samples))
    for sample, l in zip(samples, expected_lengthes):
        sample[0] = sample[0][:(2 * l) - 1]
        sample[1] = sample[1][:l]
        sample[-1] = sample[-1][:l]
    samples = sorted(samples, key=lambda s: s[0].shape[0], reverse=True)
    lengths = torch.from_numpy(np.array([s[-1].shape[0] for s in samples]))
    embeddings = [torch.from_numpy(s[0]) for s in samples]
    reply_embeddings = [torch.from_numpy(s[-1]) for s in samples]
    labels = nn.utils.rnn.pad_sequence(
        [torch.from_numpy(np.array(s[1])) for s in samples], batch_first=True)

    embeddings = nn.utils.rnn.pad_sequence(embeddings, batch_first=True)
    reply_embeddings = nn.utils.rnn.pad_sequence(
        reply_embeddings, batch_first=True)

    if return_samples:
        return embeddings, reply_embeddings, labels, lengths, samples
    else:
        return embeddings, reply_embeddings, labels, lengths


def load_dataset(table):
    dataset = FeedbackSessionsDataset(table)
    iterator = iter(dataset)
    samples = [x for x in tqdm.tqdm(iterator)]
    return samples


def create_model():
    scorer = LSTMScorer()
    scorer = scorer.cuda()
    return scorer


def calculate_loss(model, batch_samples, with_prefixes=True):
    if with_prefixes:
        embeds, rembeds, labels, lengths = make_batch_from_samples_with_prefexes(
            batch_samples)
    else:
        embeds, rembeds, labels, lengths = make_batch_from_samples(
            batch_samples)
    embeds = embeds.cuda()
    rembeds = rembeds.cuda()
    labels = labels.cuda()
    lengths = lengths.cuda()

    result = model(embeds, rembeds)
    loss = torch.mean(((result - labels.float()) ** 2)
                      [:, 1:].cumsum(1)[torch.arange(lengths.shape[0]), lengths - 2])
    return loss


def optimization_step(model, optimizer, batch_samples):
    loss = calculate_loss(model, batch_samples)
    model.zero_grad()
    loss.backward()
    optimizer.step()
    return loss

import argparse
import yt.wrapper as yt
import string
import re
from collections import Counter
yt.config.set_proxy("hahn.yt.yandex.net")


def normalize_text(text):
    text = re.sub(r"([" + string.punctuation + r"\\])", r" \1 ", text)
    text = re.sub(r"\s+", " ", text)
    text = text.strip().lower()
    return text


def get_words(text):
    text = normalize_text(text)
    return [t for t in text.split(' ') if t not in string.punctuation]


def max_num_words(utterances):
    num_words = [len(get_words(u)) for u in utterances if u]
    return max(num_words) if num_words else 0


def mean_num_words(utterances):
    num_words = [len(get_words(u)) for u in utterances if u]
    return sum(num_words) / float(len(num_words)) if num_words else 0.


def have_only_oneletter_words(utterance):
    words = get_words(utterance)
    return words and max(map(len, words)) == 1


def max_len_equal_subseq(values):
    max_successive_values = 1
    current_max = 1
    for i in range(1, len(values)):
        if values[i] and values[i - 1] and values[i] == values[i - 1]:
            current_max += 1
        else:
            current_max = 1
        max_successive_values = max(max_successive_values, current_max)
    return max_successive_values


def have_empty_utterances_inside(utterances):
    earliest_nonempty_idx = None
    for i in range(len(utterances)):
        if utterances[i]:
            earliest_nonempty_idx = i
            break
    latest_empty_idx = None
    for i in range(len(utterances) - 1, -1, -1):
        if not utterances[i]:
            latest_empty_idx = i
            break
    assert earliest_nonempty_idx != latest_empty_idx
    return earliest_nonempty_idx is None or (latest_empty_idx and earliest_nonempty_idx < latest_empty_idx)


def max_len_subseq_with_prefix(values, prefix):
    max_successive_prefixes = 0
    current_max = 0
    for v in values:
        if v and v.startswith(prefix):
            current_max += 1
        else:
            current_max = 0
        max_successive_prefixes = max(max_successive_prefixes, current_max)
    return max_successive_prefixes


def max_neighbor_iou(utterances, step=1):
    iou = [calc_iou(utterances[i], utterances[i-1]) for i in range(1, len(utterances), step) if utterances[i] and utterances[i-1]]
    return max(iou) if iou else 0.


def calc_iou(utterance_a, utterance_b):
    words_a = Counter(get_words(utterance_a))
    words_b = Counter(get_words(utterance_b))
    intersection = sum((words_a & words_b).values())
    union = sum((words_a | words_b).values())
    iou = intersection / float(union) if union != 0 else 0.
    return iou


BAD_SUCCESSIVE_INTENT_PREFIXES = ['personal_assistant\tscenarios\tsearch\tserp',
                                  'personal_assistant\tscenarios\timage_what_is_this',
                                  'external_skill\trequest']

@yt.with_context
class LogsFilterMapper(object):
    def __init__(self, min_mean_num_words, max_delta, max_iou):
        self.min_mean_num_words = min_mean_num_words
        self.max_delta = max_delta
        self.max_iou = max_iou
    def __call__(self, row, context):
        context = []
        while 'context_' + str(len(context)) in row:
            context.append(row['context_' + str(len(context))])
        user_utterances = [context[i] for i in range(0, len(context), 2)][::-1]
        all_utterances = context[::-1] + [row['reply']]

        if max_neighbor_iou(user_utterances) > self.max_iou:
            return
        if max_neighbor_iou(context, step=2) > self.max_iou:
            return
        if any(max_len_equal_subseq(get_words(u)) > 2 for u in user_utterances):
            return
        if any(have_only_oneletter_words(u) for u in user_utterances):
            return
        if mean_num_words(user_utterances) < self.min_mean_num_words:
            return
        if max(d for d in row['deltas'] if d is not None) > self.max_delta:
            return
        if any(i == '' for i in row['intents'] if i is not None):
            return
        if all(s == 'click' for s in row['sources'] if s):
            return
        for prefix in BAD_SUCCESSIVE_INTENT_PREFIXES:
            if max_len_subseq_with_prefix(row['intents'], prefix) > 1:
                return
        if have_empty_utterances_inside(user_utterances):
            return
        yield row


def main(args):
    yt.run_map(LogsFilterMapper(args.min_mean_num_words, args.max_delta, args.max_iou), args.src, args.dst, spec={'job_io': {'control_attributes': {'enable_row_index': True}}})


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--min-mean-num-words', type=float, default=3.0)
    parser.add_argument('--max-delta', type=int, default=60)
    parser.add_argument('--max-iou', type=float, default=0.75)
    args = parser.parse_args()
    main(args)

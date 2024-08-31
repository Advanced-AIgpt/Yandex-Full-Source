# coding: utf-8

import re
import yatest.common

from collections import defaultdict


def _group_by_slot(tokens, tags):
    """ Split sentence into same-slot sequences BI+ (normal slots) or I+ (continued slots) and iterate over them """
    slot_toks, slot_idx, slot_tags = [], [], []
    prev_tag = None
    for idx, (token, raw_tag) in enumerate(zip(tokens, tags)):
        tag = re.sub('^(B-|I-)', '', raw_tag)
        if tag != prev_tag or raw_tag.startswith('B-'):
            if prev_tag is not None:
                yield slot_toks, slot_idx, slot_tags, prev_tag
                slot_toks, slot_idx, slot_tags = [], [], []
        slot_toks.append(token)
        slot_idx.append(idx)
        slot_tags.append(raw_tag)
        prev_tag = tag
    if prev_tag is not None:
        yield slot_toks, slot_idx, slot_tags, prev_tag


def _convert_to_slots(tokens, tags):
    slots = defaultdict(list)
    for slot_tokens, slot_idx, slot_tags, slot_key in _group_by_slot(tokens, tags):
        start, end = slot_idx[0], slot_idx[-1] + 1
        if slot_key == 'O':
            continue

        new_slot = {
            'substr': ' '.join(slot_tokens),
            'start': start,
            'end': end,
            'is_continuation': slot_tags[0].startswith('I-')
        }
        slots[slot_key].append(new_slot)
    return slots


def _slot_match(slot_name, true_values, predicted_slots):
    """ Check whether dict predicted_slots contains the slot_name at the position(s) specified at true_values """
    if slot_name not in predicted_slots:
        return False
    predicted_values = predicted_slots[slot_name]
    # convert to multislot format
    if not isinstance(predicted_values, list):
        predicted_values = [predicted_values]
    if not isinstance(true_values, list):
        true_values = [true_values]
    for one_of_targets in true_values:
        start, end = one_of_targets['start'], one_of_targets['end']
        # the match is considered incorrect if at least one of the targets is incorrect
        target_is_found = False
        for one_of_predicted in predicted_values:
            # the match is considered incorrect if at least one of start or end is incorrect
            if start == one_of_predicted['start'] and end == one_of_predicted['end']:
                target_is_found = True
                break
        if not target_is_found:
            return False
    return True


def _calc_f1_score(precision, recall):
    if precision + recall == 0.:
        return 0.
    return 2 * precision * recall / (precision + recall)


def _calc_recall_by_slots(recalls_at, true_slots_list, pred_slots_list, true_intents=None):
    slot_recalls_at = defaultdict(lambda: dict.fromkeys(recalls_at, 0.0))
    slot_precisions_at1 = defaultdict(lambda: 0.0)
    slot_num_true = defaultdict(float)
    slot_num_pred = defaultdict(float)

    if true_intents is None:
        true_intents = [None]*len(true_slots_list)

    for true_slots, pred_slots, true_intent in zip(true_slots_list, pred_slots_list, true_intents):
        # estimate recalls-at-k
        for true_slot, true_value in true_slots.items():
            if true_intent is not None:
                true_slot_name = true_intent + ':' + true_slot
            else:
                true_slot_name = true_slot
            slot_num_true[true_slot_name] += 1.0
            for recall_at in recalls_at:
                if any(_slot_match(true_slot, true_value, pred_slots_hyp)
                       for pred_slots_hyp in pred_slots[:recall_at]):
                    slot_recalls_at[true_slot_name][recall_at] += 1.0
        # estimate precision-at-1
        for pred_slot, pred_value in pred_slots[0].items():
            if true_intent is not None:
                pred_slot_name = true_intent + ':' + pred_slot
            else:
                pred_slot_name = pred_slot
            slot_num_pred[pred_slot_name] += 1.0
            if _slot_match(pred_slot, pred_value, true_slots):
                slot_precisions_at1[pred_slot_name] += 1.0

    for true_slot_name in slot_recalls_at:
        for recall_at in recalls_at:
            slot_recalls_at[true_slot_name][recall_at] /= slot_num_true[true_slot_name]

    for pred_slot_name in slot_precisions_at1:
        slot_precisions_at1[pred_slot_name] /= slot_num_pred[pred_slot_name]

    weighted_mean_recall = sum(slot_recalls_at[slot][1] * slot_num_true[slot] for slot in slot_num_true)
    if sum(slot_num_true.values()) != 0.:
        weighted_mean_recall /= sum(slot_num_true.values())

    weighted_mean_precision = sum(slot_precisions_at1[slot] * slot_num_true[slot] for slot in slot_num_true)
    if sum(slot_num_true.values()) != 0.:
        weighted_mean_precision /= sum(slot_num_true.values())

    weighted_mean_f1_score = _calc_f1_score(weighted_mean_precision, weighted_mean_recall)

    return (slot_recalls_at, slot_precisions_at1, slot_num_true,
            weighted_mean_recall, weighted_mean_precision, weighted_mean_f1_score)


def _dump_metrics(intent, true_slots_list, pred_slots_list, output_file):
    recalls_at = [1]
    metrics = _calc_recall_by_slots(recalls_at, true_slots_list, pred_slots_list)

    slot_recalls_at, slot_precisions_at1, slot_num_true = metrics[:3]
    weighted_mean_recall, weighted_mean_precision, weighted_mean_f1_score = metrics[3:]

    output_file.write('Intent: {}\n'.format(intent))
    output_file.write('{:25} Recall Precision F1-score Support\n'.format('Slot'))
    for slot in sorted(slot_recalls_at, key=lambda slot: slot_num_true[slot], reverse=True):
        output_file.write('{:25} {:.2%}\t{:.2%}\t{:.2%}\t{}\n'.format(
            slot,
            slot_recalls_at[slot][1],
            slot_precisions_at1[slot],
            _calc_f1_score(slot_precisions_at1[slot], slot_recalls_at[slot][1]),
            int(slot_num_true[slot]),
        ))

    output_file.write('Mean metrics:\n')
    output_file.write('Recall = {:.2%}, Precision = {:.2%}, F1 = {:.2%}\n\n'.format(
        weighted_mean_recall, weighted_mean_precision, weighted_mean_f1_score))


def _evaluate_taggers(input_path, output_path):
    with open(input_path) as f_in, open(output_path, 'w') as f_out:
        intent = None
        true_slots_list, pred_slots_list = [], []
        for line in f_in:
            line = line.rstrip()
            fields = line.rstrip().split('\t')
            if len(fields) == 1:
                if intent:
                    _dump_metrics(intent, true_slots_list, pred_slots_list, f_out)
                    true_slots_list, pred_slots_list = [], []
                intent = fields[0]
                continue

            text, labels, pred_labels = fields[:3]
            tokens, labels, pred_labels = text.split(), labels.split(), pred_labels.split()
            true_slots_list.append(_convert_to_slots(tokens, labels))
            pred_slots_list.append([_convert_to_slots(tokens, pred_labels)])

        _dump_metrics(intent, true_slots_list, pred_slots_list, f_out)


def _is_same_slot(tag1, tag2):
    if tag1 == 'O' or tag2 == 'O':
        return tag1 == tag2
    return tag1[2:] == tag2[2:]


def _get_slot_markup(tokens, tag):
    slot_text = ' '.join(tokens)
    if tag == 'O':
        return slot_text + ' '

    slot = '+' if tag.startswith('I') else ''
    slot += tag[2:]
    return "'{}'({})".format(slot_text, slot) + ' '


def _to_markup(tokens, tags):
    markup = ''
    prev_pos = 0
    for pos in range(len(tokens)):
        if not _is_same_slot(tags[prev_pos], tags[pos]):
            markup += _get_slot_markup(tokens[prev_pos: pos], tags[prev_pos])
            prev_pos = pos
    markup += _get_slot_markup(tokens[prev_pos:], tags[prev_pos])

    return markup.strip()


MARKUP_SLOT_PATTERN = re.compile(r'\([\w_\+]+\)')


def _to_string(markup):
    markup = markup.replace("'", '')
    for el in re.findall(MARKUP_SLOT_PATTERN, markup):
        markup = markup.replace(el, '')
    assert '(' not in markup and ')' not in markup, markup
    return markup


def _run_applier(data_path, output_path):
    wizard_data_path = 'search/wizard/data/wizard/'
    cmd = (
        yatest.common.binary_path('alice/nlu/libs/rnn_tagger/quality_test/canonize_applier/canonize_applier'),
        '--data-path', data_path,
        '--output-path', output_path,
        '--taggers-dir', yatest.common.binary_path(wizard_data_path + 'AliceTagger'),
        '--embeddings-dir', yatest.common.binary_path(wizard_data_path + 'AliceTokenEmbedder/ru'),
        '--custom-entities-path', yatest.common.binary_path(wizard_data_path + 'CustomEntities/ru/custom_entities.trie'),
    )
    yatest.common.execute(cmd)


def test_quality():
    applier_output_path = 'quality_predictions.txt'
    _run_applier(data_path='quality_data.tsv', output_path=applier_output_path)

    report_path = 'quality_report.txt'
    _evaluate_taggers(applier_output_path, report_path)

    return yatest.common.canonical_file(report_path, local=True)


def test_medium():
    applier_output_path = 'medium_raw_predictions.txt'
    _run_applier(data_path='medium_data.tsv', output_path=applier_output_path)

    output_path = 'medium_predictions.txt'

    current_intent_samples = []
    with open(applier_output_path) as f_in, open(output_path, 'w') as f_out:
        for line in f_in:
            fields = line.strip().split('\t')
            if len(fields) == 1:
                current_intent_samples.sort(key=_to_string)
                for sample in current_intent_samples:
                    f_out.write(sample + '\n')
                current_intent_samples = []
                f_out.write('Intent: {}\n'.format(fields[0]))
            else:
                current_intent_samples.append(_to_markup(fields[0].split(), fields[2].split()))
        current_intent_samples.sort(key=_to_string)
        for sample in current_intent_samples:
            f_out.write(sample + '\n')

    diff_tool = yatest.common.binary_path('alice/nlu/libs/rnn_tagger/quality_test/'
                                          'canonized_data_diff/canonized_data_diff')
    return yatest.common.canonical_file(output_path, local=True, diff_tool=diff_tool)

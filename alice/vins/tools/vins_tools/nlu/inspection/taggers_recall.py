import attr

from collections import defaultdict
from itertools import izip


@attr.s
class RecallsBySlotsResult(object):
    slot_recalls_at = attr.ib()
    slot_precisions_at1 = attr.ib()
    slot_num_true = attr.ib()
    sum_slot_recalls = attr.ib()
    mean_slot_recall = attr.ib()
    mean_slot_precision_at_1 = attr.ib()
    weighted_mean_recall = attr.ib()
    weighted_mean_precision = attr.ib()
    weighted_mean_f1_score = attr.ib()

    def to_dict(self):
        return attr.asdict(self)


def slot_match(slot_name, true_values, predicted_slots):
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


def slots_match(slots_pred, slots_true):
    """ Check whether dicts slots_pred and slots_true contain exactly the same slots at the same places """
    if len(slots_pred) != len(slots_true):
        return False
    for slot_pred, value_pred in slots_pred.iteritems():
        if not slot_match(slot_pred, value_pred, slots_true):
            return False
    return True


def calc_recalls_at(recalls_at, true_slots_list, pred_slots_list):
    recalls_at_values = dict.fromkeys(recalls_at, 0.0)
    for true_slots, pred_slots in izip(true_slots_list, pred_slots_list):
        for recall_at in recalls_at:
            if any(slots_match(pred_slots_hyp, true_slots)
                   for pred_slots_hyp in pred_slots[:recall_at]):
                recalls_at_values[recall_at] += 1.0
    for k in recalls_at:
        recalls_at_values[k] /= len(true_slots_list)

    return recalls_at_values


def calc_recall_by_slots(recalls_at, true_slots_list, pred_slots_list, true_intents=None):
    slot_recalls_at = defaultdict(lambda: dict.fromkeys(recalls_at, 0.0))
    slot_precisions_at1 = defaultdict(lambda: 0.0)
    slot_num_true = defaultdict(float)
    slot_num_pred = defaultdict(float)

    if true_intents is None:
        true_intents = [None]*len(true_slots_list)

    for true_slots, pred_slots, true_intent in izip(true_slots_list, pred_slots_list, true_intents):
        # estimate recalls-at-k
        for true_slot, true_value in true_slots.iteritems():
            if true_intent is not None:
                true_slot_name = true_intent + ':' + true_slot
            else:
                true_slot_name = true_slot
            slot_num_true[true_slot_name] += 1.0
            for recall_at in recalls_at:
                if any(slot_match(true_slot, true_value, pred_slots_hyp)
                       for pred_slots_hyp in pred_slots[:recall_at]):
                    slot_recalls_at[true_slot_name][recall_at] += 1.0
        # estimate precision-at-1
        for pred_slot, pred_value in pred_slots[0].iteritems():
            if true_intent is not None:
                pred_slot_name = true_intent + ':' + pred_slot
            else:
                pred_slot_name = pred_slot
            slot_num_pred[pred_slot_name] += 1.0
            if slot_match(pred_slot, pred_value, true_slots):
                slot_precisions_at1[pred_slot_name] += 1.0

    for true_slot_name in slot_recalls_at:
        for recall_at in recalls_at:
            slot_recalls_at[true_slot_name][recall_at] /= slot_num_true[true_slot_name]

    for pred_slot_name in slot_precisions_at1:
        slot_precisions_at1[pred_slot_name] /= slot_num_pred[pred_slot_name]

    sum_slot_recalls = defaultdict(float)
    for true_slot_name in slot_num_true:
        for recall_at in recalls_at:
            sum_slot_recalls[recall_at] += slot_recalls_at[true_slot_name][recall_at]

    mean_slot_recall = {}
    for recall_at in recalls_at:
        mean_slot_recall[recall_at] = sum_slot_recalls[recall_at] / len(slot_num_true)

    mean_slot_precision_at_1 = None
    if slot_precisions_at1:
        mean_slot_precision_at_1 = sum(slot_precisions_at1.values()) / len(slot_precisions_at1)

    weighted_mean_recall = sum(slot_recalls_at[slot][1] * slot_num_true[slot] for slot in slot_num_true)
    if sum(slot_num_true.values()) != 0.:
        weighted_mean_recall /= sum(slot_num_true.values())

    weighted_mean_precision = sum(slot_precisions_at1[slot] * slot_num_true[slot] for slot in slot_num_true)
    if sum(slot_num_true.values()) != 0.:
        weighted_mean_precision /= sum(slot_num_true.values())

    weighted_mean_f1_score = 2 * weighted_mean_recall * weighted_mean_precision
    if weighted_mean_recall + weighted_mean_precision != 0.:
        weighted_mean_f1_score /= (weighted_mean_recall + weighted_mean_precision)

    return RecallsBySlotsResult(slot_recalls_at, slot_precisions_at1, slot_num_true, sum_slot_recalls,
                                mean_slot_recall, mean_slot_precision_at_1,
                                weighted_mean_recall, weighted_mean_precision, weighted_mean_f1_score)

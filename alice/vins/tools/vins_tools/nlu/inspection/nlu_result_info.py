# coding: utf-8
from __future__ import unicode_literals

import json
import codecs
import attr
import pandas as pd
import numpy as np
import time
import logging.config
import os
import re
import cPickle as pickle

from itertools import izip, imap
from operator import itemgetter
from collections import Counter, OrderedDict
from sklearn.metrics import precision_recall_fscore_support, accuracy_score, confusion_matrix
from sklearn.utils.multiclass import unique_labels
from collections import defaultdict

from vins_core.utils.misc import dict_zip, set_of_segments_intersection
from vins_core.utils.intent_renamer import IntentRenamer
from vins_tools.nlu.inspection.taggers_recall import calc_recalls_at, calc_recall_by_slots, slots_match, slot_match


logger = logging.getLogger(__name__)


@attr.s(slots=True, frozen=True, cmp=True)
class UtteranceInfo(object):
    true_intent = attr.ib(default=None)
    prev_intent = attr.ib(default=None)
    slots = attr.ib(default=None)
    weight = attr.ib(default=1)
    device_state = attr.ib(default=None)
    additional_data = attr.ib(default=None)


@attr.s
class NluResult(object):
    semantic_frames = attr.ib()
    input_sample = attr.ib()
    selected_item = attr.ib()


@attr.s
class AppResult(object):
    intent_name = attr.ib()


@attr.s
class ProcessedItem(object):
    utterance = attr.ib()
    utterance_info = attr.ib(validator=attr.validators.instance_of(UtteranceInfo))
    nlu_result = attr.ib(validator=attr.validators.instance_of(NluResult))
    app_result = attr.ib(default=None)


def save_items(processed_items, file_path):
    with open(file_path, mode='wb') as fout:
        pickle.dump(processed_items, fout, protocol=pickle.HIGHEST_PROTOCOL)


def load_items(file_path):
    with open(file_path, mode='rb') as f:
        processed_items = pickle.load(f)
    return processed_items


class NluResultInfo(object):
    def __init__(
        self, results_file,
        intent_classification_result=True,
        nlu_markup=True,
        errors_output=True,
        additional_info=None,
        rename_intents=None,
        exclude_reference_intents=None,
        append_time_to_filename=True,
        recalls_at=(1, 2, 5, 10),
        recall_by_slots=False,
        common_tagger_errors=False,
        num_most_common_errors=10,
        other=None,
        detailed_recall_for_intents=None,
        baseline=None,
        intent_prediction_filename=None,
        intent_metrics_filename=None,
        slot_metrics_filename=None,
        intent_errors_filename=None,
        slot_errors_filename=None,
        tag_errors_filename=None,
        word_errors_filename=None,
        nlu_filename=None
    ):
        self._intent_classification_result = intent_classification_result
        self._nlu_markup = nlu_markup
        self._errors_output = errors_output
        self._additional_info = additional_info or {}
        self._exclude_reference_intents = exclude_reference_intents
        self._results_file = results_file
        self._recalls_at = recalls_at
        self._renamer = IntentRenamer(rename_intents)
        self._recall_by_slots = recall_by_slots
        self._common_tagger_errors = common_tagger_errors
        self._num_most_common_errors = num_most_common_errors
        self._other = other
        self._detailed_recall_for_intents = detailed_recall_for_intents
        self._baseline_file = baseline

        output_dir = os.path.dirname(results_file)
        self._file_prefix = os.path.join(output_dir, os.path.splitext(os.path.basename(results_file))[0])
        if append_time_to_filename:
            self._file_prefix += '.%s' % time.strftime("%Y%m%d-%H%M%S")
        self.intent_prediction_filename = intent_prediction_filename or self._file_prefix + '.intents'
        self.intent_metrics_filename = intent_metrics_filename or self._file_prefix + '.intent.metrics.json'
        self.slot_metrics_filename = slot_metrics_filename or self._file_prefix + '.slot.metrics.json'
        self.intent_errors_filename = intent_errors_filename or self._file_prefix + '.intent.errors'
        self.slot_errors_filename = slot_errors_filename or self._file_prefix + '.slot.errors'
        self.tag_errors_filename = tag_errors_filename or self._file_prefix + '.tag_errors.csv'
        self.word_errors_filename = word_errors_filename or self._file_prefix + '.word_errors.csv'
        self.nlu_filename = nlu_filename or self._file_prefix + '.nlu'

    def make_reports(self):
        results = self._process(self._results_file)
        strout = ''
        if self._intent_classification_result:
            strout += self._print_intent_classification_result(results)
        if self._has_intent_references(results):
            if self._errors_output:
                strout += self._print_intent_errors(results)
            strout += self._print_intent_metrics(results, self._additional_info, self._baseline_file)
        if self._has_slots_references(results):
            if self._errors_output:
                strout += self._print_slot_errors(results)
            strout += self._print_slot_metrics(results, self._additional_info)
        if self._nlu_markup:
            strout += self._print_nlu_markup(results)
        if self._common_tagger_errors:
            strout += self._print_common_tagger_errors(results)
        logger.info(strout)

    @staticmethod
    def _has_intent_references(results):
        return any(r['true_intent'] for r in results)

    @staticmethod
    def _has_slots_references(results):
        return any(r['true_slots'] for r in results)

    def _process(self, results_file):
        output = []
        processed_items = load_items(results_file)
        for item in processed_items:
            if item.utterance_info.true_intent and self._exclude_reference_intents and re.match(
                self._exclude_reference_intents, item.utterance_info.true_intent
            ):
                continue
            if item.app_result:
                pred_intent = item.app_result.intent_name
            else:
                pred_intent = item.nlu_result.semantic_frames[0]['intent_name']
            data = {
                'utterance': item.utterance,
                'tokens': item.nlu_result.input_sample.tokens,
                'true_intent_raw': item.utterance_info.true_intent,
                'true_intent': self._renamer(item.utterance_info.true_intent, IntentRenamer.By.TRUE_INTENT),
                'prev_intent': item.utterance_info.prev_intent,
                'pred_intent_raw': pred_intent,
                'pred_intent': self._renamer(pred_intent, IntentRenamer.By.PRED_INTENT),
                'score': item.nlu_result.semantic_frames[0]['confidence'],
                'true_markup': self.get_nlu_string(item.nlu_result.input_sample.tokens, item.utterance_info.slots),
                'pred_markup': self.get_nlu_string(
                    item.nlu_result.input_sample.tokens, item.nlu_result.semantic_frames[0]['slots']),
                'pred_slots': map(itemgetter('slots'), item.nlu_result.semantic_frames),
                'true_slots': item.utterance_info.slots,  # map slot_name -> value or list of values
                'weight': item.utterance_info.weight,
                'selected_item': item.nlu_result.selected_item,
            }
            if item.utterance_info.additional_data is not None:
                data['additional_data'] = item.utterance_info.additional_data
            output.append(data)
        return output

    @staticmethod
    def get_nlu_string(tokens, slots):
        tokens = tokens or []
        slots = slots or {}
        left_edges = {}
        right_edges = {}
        for slot_name, slot_values in slots.iteritems():
            for slot in slot_values:
                left_edges[slot['start']] = left_edges.get(slot['start'], []) + [slot_name]
                right_edges[slot['end'] - 1] = right_edges.get(slot['end'] - 1, []) + [slot_name]

        res = ''
        n = 0
        for t in tokens:
            if n in left_edges:
                res += '\'' * len(left_edges[n])
            res += t
            if n in right_edges:
                res += '\'' * len(right_edges[n])
                for slot_name in right_edges[n]:
                    res += '(%s)' % slot_name
            n += 1
            if n < len(tokens):
                res += ' '

        return res

    def _print_intent_classification_result(self, results):
        logger.info('Create output file: %s', self.intent_prediction_filename)

        true_intents = map(itemgetter('true_intent'), results)
        pred_intents = map(itemgetter('pred_intent'), results)
        utterances = map(itemgetter('utterance'), results)
        scores = map(itemgetter('score'), results)
        if true_intents[0]:
            output = pd.DataFrame(OrderedDict([
                ('utterance', utterances),
                ('true_intent', true_intents),
                ('pred_intent', pred_intents),
                ('scores', scores),
            ]))
        else:
            output = pd.DataFrame(OrderedDict([
                ('utterance', utterances),
                ('pred_intent', pred_intents),
                ('scores', scores),
            ]))
        if 'additional_data' in results[0]:
            additional_data = pd.DataFrame.from_records([r['additional_data'] for r in results])
            output = pd.concat([output, additional_data], axis=1)
        selected_items = map(itemgetter('selected_item'), results)
        if any(s is not None for s in selected_items):
            output['selected_item'] = selected_items

        output.to_csv(self.intent_prediction_filename, sep=b'\t', index=False, encoding='utf-8')
        return ''

    def _get_pandas_report(self, true_intents, pred_intents, total=None, baseline=None):
        labels = unique_labels(true_intents, pred_intents)
        if self._other:
            labels = filter(lambda label: not re.match(self._other, label), labels)
        if self._other:
            labels = filter(lambda label: not re.match(self._other, label), labels)
        precisions, recalls, f1_measures, supports = precision_recall_fscore_support(
            true_intents, pred_intents, labels=labels, average=None
        )
        predictions = pd.Series(pred_intents).value_counts().reindex(labels).fillna(0).astype(int)
        report = pd.DataFrame({
            'precision': precisions,
            'recall': recalls,
            'f1_measure': f1_measures,
            'support': supports,
            'predictions': predictions
        }, index=labels)
        # reorder the columns
        report = report[['precision', 'recall', 'f1_measure', 'support', 'predictions']]
        if total:
            last_row = precision_recall_fscore_support(
                true_intents, pred_intents, labels=labels, average=total)
            last_row = list(last_row) + [last_row[-1]]
            report.loc['total'] = last_row
            report.loc['total', 'support'] = len(true_intents)
            report.loc['total', 'predictions'] = len(true_intents)
        if baseline is not None:
            baseline = baseline[['precision', 'recall', 'f1_measure']]
            diff = report[baseline.columns] - baseline
            baseline.columns = [c + '_base' for c in baseline.columns]
            diff.columns = [c + '_diff' for c in baseline.columns]
            # if baseline and report contain different classes, fill the missings with 0
            lost_indices = baseline.index.difference(report.index)
            new_indices = report.index.difference(baseline.index)
            logger.info('Missing intents from the baseline: {}'.format(lost_indices))
            logger.info('Intents not from the baseline: {}'.format(new_indices))
            booted = self._bootstrap_classification_report(true_intents, pred_intents, labels, total=total)
            p_value = np.mean(
                np.moveaxis(booted[:, :3, :], 2, 0) >= baseline.loc[report.index].fillna(0).values,
                axis=0
            )
            p_value = pd.DataFrame(p_value, columns=['precision_pv', 'recall_pv', 'f1_measure_pv'], index=report.index)
            report = pd.concat([report, baseline, diff, p_value], axis=1).fillna(0)
        return report

    def _print_intent_metrics(self, results, additional_info, baseline_file=None):
        logger.info('Create output file: %s', self.intent_metrics_filename)

        true_intents = map(itemgetter('true_intent'), results)
        pred_intents = map(itemgetter('pred_intent'), results)

        if baseline_file:
            try:
                with open(baseline_file, 'r') as f:
                    metrics = json.load(f)
                baseline = pd.DataFrame(metrics['report']).T
            except IOError:
                logger.warning('Baseline file {} was not found, proceeding without baseline'.format(baseline_file))
                baseline = None
        else:
            baseline = None

        pandas_report = self._get_pandas_report(true_intents, pred_intents, baseline=baseline, total='weighted')
        json_report = pandas_report.to_dict(orient='index')
        json_report_support = {key: value for key, value in json_report.iteritems() if value['support'] > 0}

        if len(json_report_support) > 0:
            macro_f_score = np.mean([values['f1_measure'] for values in json_report_support.itervalues()])
            weighted_f_score = (
                np.sum([values['f1_measure'] * values['support'] for values in json_report_support.itervalues()]) /
                np.sum([values['support'] for values in json_report_support.itervalues()])
            )
        else:
            macro_f_score = None
            weighted_f_score = None

        accuracy = accuracy_score(true_intents, pred_intents)

        pd.set_option('display.width', 2000)
        pd.set_option('display.max_rows', 1000)
        pd.set_option('display.max_columns', 100)
        pd.set_option('display.max_colwidth', 100)
        pd.set_option('precision', 4)

        strout = 'Report:\n{}\n'.format(str(pandas_report))
        strout += 'Accuracy: {}\n'.format(accuracy)
        strout += 'Macro F-score: {}\n'.format(macro_f_score)
        strout += 'Weighted F-score: {}\n'.format(weighted_f_score)

        if self._detailed_recall_for_intents is not None:
            raw_true_intents = map(itemgetter('true_intent_raw'), results)
            detailed_intents = {intent for intent in set(raw_true_intents)
                                if re.match(self._detailed_recall_for_intents, intent)}
            supports = defaultdict(int)
            hits = defaultdict(int)
            for raw_intent, true_intent, pred_intent in zip(raw_true_intents, true_intents, pred_intents):
                if raw_intent in detailed_intents:
                    supports[raw_intent] += 1
                    if true_intent == pred_intent:
                        hits[raw_intent] += 1
            strout += '\nDetailed recalls for some intents:\n'
            detailed_recalls = pd.DataFrame({'hit': hits, 'support': supports}).fillna(0)
            detailed_recalls['recall'] = detailed_recalls['hit'] / detailed_recalls['support']
            detailed_recalls.index.name = 'true intent'
            strout += str(detailed_recalls.sort_index(axis=1)) + '\n'

        output = {
            'report': json_report,
            'accuracy': accuracy,
            'macro_f_score': macro_f_score,
            'weighted_f_score': weighted_f_score
        }

        intents = np.unique(true_intents)
        cm = confusion_matrix(true_intents, pred_intents, labels=intents)
        cm = cm.astype('float') / cm.sum(axis=1)[:, np.newaxis]
        output['confusion_matrix'] = {intent: dict_zip(intents, row) for intent, row in izip(intents, cm)}

        output.update(additional_info)
        with open(self.intent_metrics_filename, mode='w') as fout:
            fout.write(json.dumps(output, sort_keys=True, indent=4))
        return strout

    def _print_slot_metrics(self, results, additional_info):
        logger.info('Create output file: %s', self.slot_metrics_filename)

        true_slots_list = map(itemgetter('true_slots'), results)
        pred_slots_list = map(itemgetter('pred_slots'), results)

        output = {}

        recalls_at = calc_recalls_at(self._recalls_at, true_slots_list, pred_slots_list)
        strout = 'Slot recalls (accuracy of the best attempt):\n%s\n' % '\n'.join('recall@%d: %.4f' % (k, v)
                                                                                  for k, v in recalls_at.iteritems())
        output['recall'] = recalls_at

        if self._recall_by_slots:
            true_intents = map(itemgetter('pred_intent'), results)
            recall_by_slots = calc_recall_by_slots(self._recalls_at, true_slots_list, pred_slots_list, true_intents)

            for true_slot_name in recall_by_slots.slot_recalls_at:
                strout += 'Slot "%s" recalls:\n%s\n' % (
                    true_slot_name,
                    '\n'.join('recall@%d: %.4f' % (k, v)
                              for k, v in recall_by_slots.slot_recalls_at[true_slot_name].iteritems())
                )
            for pred_slot_name in recall_by_slots.slot_precisions_at1:
                strout += 'Slot "%60s" precision@1: %.4f\n' % (pred_slot_name,
                                                               recall_by_slots.slot_precisions_at1[pred_slot_name])

            output['slot_recall'] = recall_by_slots.slot_recalls_at
            output['slot_precision_at_1'] = recall_by_slots.slot_precisions_at1
            output['mean_slot_recall'] = recall_by_slots.mean_slot_recall

            strout += 'Mean slot recalls:\n%s\n' % (
                '\n'.join('mean slot recall@%d: %.4f' % (k, v) for k, v in output['mean_slot_recall'].iteritems())
            )
            if recall_by_slots.mean_slot_precision_at_1:
                output['mean_slot_precision_at_1'] = recall_by_slots.mean_slot_precision_at_1
                strout += 'Mean slot precision@1: %.4f\n' % (output['mean_slot_precision_at_1'])

            strout += '\n'
            strout += 'Weighted slot recall = {:.1%}\n'.format(recall_by_slots.weighted_mean_recall)
            strout += 'Weighted slot precision = {:.1%}\n'.format(recall_by_slots.weighted_mean_precision)
            strout += 'Weighted slot F1-score = {:.1%}\n'.format(recall_by_slots.weighted_mean_f1_score)

        output.update(additional_info)
        with open(self.slot_metrics_filename, mode='w') as fout:
            fout.write(json.dumps(output, sort_keys=True, indent=4))
        return strout

    @staticmethod
    def _joint_slot_text(slot_values):
        if not isinstance(slot_values, list):
            slot_values = [slot_values]
        return ' ... '.join(slot_value['substr'] for slot_value in slot_values)

    @staticmethod
    def _classify_slot_errors(results):
        true_slots_list = map(itemgetter('true_slots'), results)
        pred_slots_list = map(itemgetter('pred_slots'), results)
        utterance_list = map(itemgetter('utterance'), results)
        tokens_list = map(itemgetter('tokens'), results)
        first_slots_list = map(itemgetter(0), pred_slots_list)

        errors = []
        error_words = []
        for true_slots, pred_slots, utterance, tokens in zip(
                true_slots_list, first_slots_list, utterance_list, tokens_list):
            # find difficult tags
            all_keys = set(true_slots.keys() + pred_slots.keys())
            for key in all_keys:
                if key not in true_slots:
                    errors.append({'type': 'false_alarm', 'tag': key, 'utterance': utterance,
                                   'pred': NluResultInfo._joint_slot_text(pred_slots[key])
                                   })
                elif key not in pred_slots:
                    errors.append({'type': 'false_reject', 'tag': key, 'utterance': utterance,
                                   'fact': NluResultInfo._joint_slot_text(true_slots[key])
                                   })
                elif slot_match(key, true_slots[key], pred_slots):
                    # it is not an error
                    pass
                elif set_of_segments_intersection(pred_slots[key], true_slots[key]):
                    errors.append({'type': 'partial_mismatch', 'tag': key, 'utterance': utterance,
                                   'fact': NluResultInfo._joint_slot_text(true_slots[key]),
                                   'pred': NluResultInfo._joint_slot_text(pred_slots[key])
                                   })
                else:
                    errors.append({'type': 'total_mismatch', 'tag': key, 'utterance': utterance,
                                   'fact': NluResultInfo._joint_slot_text(true_slots[key]),
                                   'pred': NluResultInfo._joint_slot_text(pred_slots[key])
                                   })
            # find difficult keys
            true_tags = ['none'] * len(tokens)
            pred_tags = ['none'] * len(tokens)
            for k, v in true_slots.iteritems():
                if not isinstance(v, list):
                    v = [v]
                for one_of_slots in v:
                    for i in range(one_of_slots['start'], one_of_slots['end']):
                        true_tags[i] = k
            for k, v in pred_slots.iteritems():
                if not isinstance(v, list):
                    v = [v]
                for one_of_slots in v:
                    for i in range(one_of_slots['start'], one_of_slots['end']):
                        pred_tags[i] = k
            for token, true_tag, pred_tag in zip(tokens, true_tags, pred_tags):
                if true_tag != pred_tag:
                    error_words.append([token, true_tag, pred_tag])

        return pd.DataFrame(errors), pd.DataFrame(error_words, columns=['token', 'true', 'pred'])

    def _print_slot_errors(self, results):
        logger.info('Create slot errors output file: %s', self.slot_errors_filename)
        true_slots_list = map(itemgetter('true_slots'), results)
        pred_slots_list = map(itemgetter('pred_slots'), results)
        true_intents = map(itemgetter('true_intent'), results)
        pred_intents = map(itemgetter('pred_intent'), results)
        true_markups = map(itemgetter('true_markup'), results)
        pred_markups = map(itemgetter('pred_markup'), results)
        errors = []
        for true_slots, pred_slots, true_markup, pred_markup, true_intent, pred_intent in izip(
                true_slots_list, pred_slots_list, true_markups, pred_markups, true_intents, pred_intents):
            for recall_at in self._recalls_at:
                if not any(slots_match(pred_slots_hyp, true_slots) for pred_slots_hyp in pred_slots[:recall_at]):
                    errors.append((true_markup, pred_markup, true_intent, pred_intent, recall_at))
        pd.DataFrame(
            errors,
            columns=['true_markup', 'pred_markup', 'true_intent', 'pred_intent', 'recall_at']
        ).to_csv(self.slot_errors_filename, sep=b'\t', index=False, encoding='utf-8', na_rep='null')
        return ''

    def _print_intent_errors(self, results):
        logger.info('Create intent errors output file: %s', self.intent_errors_filename)
        utterances = map(itemgetter('utterance'), results)
        true_intents = map(itemgetter('true_intent'), results)
        pred_intents = map(itemgetter('pred_intent'), results)
        pred_intents_raw = map(itemgetter('pred_intent_raw'), results)
        scores = map(itemgetter('score'), results)
        prev_intents = map(itemgetter('prev_intent'), results)
        errors = [
            (utterance, true_intent, pred_intent, score, pred_intent_raw, prev_intent)
            for utterance, true_intent, pred_intent, score, pred_intent_raw, prev_intent in izip(
                utterances, true_intents, pred_intents, scores, pred_intents_raw, prev_intents)
            if true_intent != pred_intent
        ]
        errors_count = Counter((true_intent, pred_intent) for _, true_intent, pred_intent, _, _, _ in errors)
        most_common_errors_log_string = '\n'.join([
            '%d: %s --> %s' % (num_errors, true_intent, pred_intent)
            for (true_intent, pred_intent), num_errors in errors_count.most_common(self._num_most_common_errors)
        ])

        strout = 'Most common errors:\n%s\n' % most_common_errors_log_string
        pd.DataFrame(
            errors,
            columns=['utterance', 'true_intent', 'pred_intent', 'score', 'pred_intent_raw', 'prev_intent']
        ).to_csv(self.intent_errors_filename, sep=b'\t', index=False, encoding='utf-8')
        return strout

    def _print_common_tagger_errors(self, results):
        difficult_tags, difficult_words = self._classify_slot_errors(results)
        difficult_tags.to_csv(self.tag_errors_filename, sep=b'\t', index=False, encoding='utf-8')
        difficult_words.to_csv(self.word_errors_filename, sep=b'\t', index=False, encoding='utf-8')
        top_words = difficult_words.apply(
            lambda row: u'{:15} : {:15} -> {:15}'.format(*row[['token', 'true', 'pred']]), axis=1)
        top_words = top_words.value_counts().head(self._num_most_common_errors)
        top_tags = difficult_tags.groupby(['type', 'tag'])['utterance'].count()
        top_tags = top_tags.sort_values(ascending=False).reset_index().head(self._num_most_common_errors)
        return u"\nDifficult words for tagger:\n{}\n\nTypical errors by tag and error type:\n{}\n".format(top_words,
                                                                                                          top_tags)

    def _print_nlu_markup(self, results):
        logger.info('Create output file: %s', self.nlu_filename)
        with codecs.open(self.nlu_filename, mode='w', encoding='utf-8') as fout:
            for res in imap(itemgetter('pred_markup'), results):
                fout.write(res + '\n')
        return ''

    @staticmethod
    def _get_long_confusion_matrix(data, normalize=False):
        counts = data.groupby(['pred', 'fact']).pred.count()
        if normalize:
            counts = counts / counts.sum()
        counts.name = 'weight'
        return counts.reset_index()

    @staticmethod
    def _bootstrap_classification_report(true_intents, pred_intents, labels, n_bags=1000,
                                         total='weighted', by_class=True):
        """
        Calculate bootstrapped distribution of precision, recall, f-score and support.
        """
        assert total or by_class, 'At least one way of aggregation must be chosen'
        predfact = pd.DataFrame({'pred': pred_intents, 'fact': true_intents})

        dedup_master = NluResultInfo._get_long_confusion_matrix(predfact, normalize=True)

        n_rows = int(total is not None) + int(by_class) * len(labels)

        np.random.seed(1)
        booted = np.empty(dtype=np.float64, shape=[n_rows, 4, n_bags])

        logger.info('Calculating significance with bootstrap...')
        for i in range(n_bags):
            new_weights = np.random.multinomial(predfact.shape[0], pvals=dedup_master.weight)
            if by_class:
                p, r, f, s, = precision_recall_fscore_support(
                    dedup_master.fact, dedup_master.pred, labels=labels, average=None,
                    sample_weight=new_weights
                )
                booted[:len(labels), 0, i] = p
                booted[:len(labels), 1, i] = r
                booted[:len(labels), 2, i] = f
                booted[:len(labels), 3, i] = s
            if total:
                p, r, f, s, = precision_recall_fscore_support(
                    dedup_master.fact, dedup_master.pred, labels=labels, average=total,
                    sample_weight=new_weights
                )
                booted[-1, 0, i] = p
                booted[-1, 1, i] = r
                booted[-1, 2, i] = f
                booted[-1, 3, i] = len(true_intents)
        return booted

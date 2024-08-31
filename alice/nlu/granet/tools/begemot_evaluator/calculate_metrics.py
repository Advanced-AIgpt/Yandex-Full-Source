# -*- coding: utf-8 -*-

import os
import argparse
import json

from sklearn.metrics import classification_report


def _process_file(input_path, intent_renames):
    predictable_intents = set(intent_renames.values())
    correct_intents, predicted_intents, weights = [], [], []

    input_path_without_ext, ext = os.path.splitext(input_path)
    false_positives_path = input_path_without_ext + '_false_positives' + ext
    false_negatives_path = input_path_without_ext + '_false_negatives' + ext
    with open(input_path) as f, open(false_positives_path, 'w') as f_fp, open(false_negatives_path, 'w') as f_fn:
        f_fp.write('Text\tPredicted intents\tCorrect intent\tCount\n')
        f_fn.write('Text\tPredicted intents\tCorrect intent\tCount\n')
        f.readline()

        for line in f:
            text, predictions, correct_intent, count = line.rstrip().split('\t')
            count = int(count)
            renamed_correct_intent = intent_renames.get(correct_intent, 'other')
            predictions = [intent for intent in predictions.split(',') if intent in predictable_intents]

            if not predictions:
                predictions = ['other']

            for intent in predictions:
                correct_intents.append(renamed_correct_intent)
                predicted_intents.append(intent)
                weights.append(count)

            if renamed_correct_intent == 'other' and len(set(predictions) - {'other'}) > 0:
                f_fp.write('{text}\t{predicted_intents}\t{correct_intent}\t{count}\n'.format(
                    text=text, predicted_intents=','.join(predictions), correct_intent=correct_intent, count=count
                ))
            if renamed_correct_intent != 'other' and renamed_correct_intent not in predictions:
                f_fn.write('{text}\t{predicted_intents}\t{correct_intent}\t{count}\n'.format(
                    text=text, predicted_intents=','.join(predictions), correct_intent=correct_intent, count=count
                ))

    print 'By unique:'
    print classification_report(
        y_true=correct_intents, y_pred=predicted_intents,
        labels=sorted(predictable_intents), digits=4
    )

    print 'With freqs:'
    print classification_report(
        y_true=correct_intents, y_pred=predicted_intents, sample_weight=weights,
        labels=sorted(predictable_intents), digits=4
    )


def main():
    argument_parser = argparse.ArgumentParser(add_help=True)
    argument_parser.add_argument('--input-path', required=True)
    argument_parser.add_argument('--intent-renames', required=True)
    args = argument_parser.parse_args()

    with open(args.intent_renames) as f:
        intent_renames = json.load(f)

    _process_file(args.input_path, intent_renames)

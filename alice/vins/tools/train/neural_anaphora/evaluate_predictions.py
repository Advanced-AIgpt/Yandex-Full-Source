# coding: utf-8

import argparse
import os
import logging

logger = logging.getLogger(__name__)


def _is_partial_match(entity, correct_entities):
    return any(
        len(set(entity.split()) & set(other_entity.split())) > 0
        for other_entity in correct_entities
    )


def evaluate(predictions_path, report_path, errors_path, allow_partial_matches=False):
    if report_path is not None:
        report_path = report_path
    else:
        report_path = os.path.splitext(predictions_path)[0] + '_report.txt'

    if errors_path is not None:
        errors_path = errors_path
    else:
        errors_path = os.path.splitext(predictions_path)[0] + '_errors.tsv'

    tp = 0.  # correct entity was extracted
    fp = 0.  # incorrect entity was extracted
    fn = 0.  # correct entity wasn't extracted
    tn = 0.  # correctly predicted that there is nothing to extract

    result_lines = []

    with open(predictions_path) as f:
        for line in f:
            _, correct_entities, predicted_entity = line[:-1].split('\t')[:3]
            correct_entities = [entity for entity in correct_entities.split(', ') if entity]

            if predicted_entity:
                is_partial_match = _is_partial_match(predicted_entity, correct_entities)
                if predicted_entity in correct_entities or (allow_partial_matches and is_partial_match):
                    tp += 1.
                    verdict = 'TP'
                else:
                    fp += 1.
                    verdict = 'FP'
            else:
                if correct_entities:
                    fn += 1.
                    verdict = 'FN'
                else:
                    tn += 1.
                    verdict = 'TN'

            result_lines.append('[{}] {}'.format(verdict, line))

    precision = tp / (tp + fp) if tp + fp != 0. else 0.
    recall = tp / (tp + fn) if tp + fn != 0. else 0.
    f1 = 2 * precision * recall / (precision + recall) if precision + recall != 0. else 0.

    with open(report_path, 'w') as f:
        f.write('TP = {}, FP = {}, FN = {}, TN = {}\n'.format(tp, fp, fn, tn))
        f.write('Precision = {:.2%}, Recall = {:.2%}, F1 = {:.2%}'.format(precision, recall, f1))

        logger.info('TP = {}, FP = {}, FN = {}, TN = {}'.format(tp, fp, fn, tn))
        logger.info('Precision = {:.2%}, Recall = {:.2%}, F1 = {:.2%}'.format(precision, recall, f1))

    result_lines = sorted(result_lines)
    with open(errors_path, 'w') as f:
        for line in result_lines:
            f.write(line)


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--predictions-path', required=True)
    parser.add_argument('--report-path', default=None)
    parser.add_argument('--errors-path', default=None)
    parser.add_argument('--allow-partial-matches', default=False, action='store_true')

    args = parser.parse_args()

    evaluate(args.predictions_path, args.report_path, args.errors_path, args.allow_partial_matches)


if __name__ == '__main__':
    main()

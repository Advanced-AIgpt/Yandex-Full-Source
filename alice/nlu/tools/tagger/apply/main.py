# coding: utf-8

import argparse
import json
import logging
import os
import yt.wrapper as yt

from alice.nlu.py_libs import tagger as m_tagger
from alice.nlu.tools.tagger.lib.data import load_data_from_disk, load_data_from_yt

yt.config['proxy']['url'] = os.environ.get('YT_PROXY', 'hahn')
logger = logging.getLogger(__name__)


def _test(model_dir, intent_to_data, out_predictions, out_quality, need_sample_features, need_tags, all_preds):
    quality = {}
    for path in os.listdir(model_dir):
        intent = path
        intent_data = intent_to_data.get(intent, [])
        logger.info('Intent {0} ({1})'.format(intent, len(intent_data)))
        if len(intent_data) == 0:
            logging.warning('No data for intent {0}'.format(intent))
            continue
        tagger = m_tagger.RnnTaggerApplier(os.path.join(model_dir, intent))
        matches = 0
        for sample_features in intent_data:
            tag_list_list, _ = tagger.predict([sample_features], nbest=1)  # outputs predictions and scores
            predicted_tags = tag_list_list[0][0]  # first batch, best prediction

            gold_tags = sample_features.sample.tags
            if predicted_tags == gold_tags:
                matches += 1

                if not all_preds:
                    continue

            gold_markup = sample_features.sample.to_markup()
            predicted_markup = sample_features.sample.with_tags(predicted_tags).to_markup()

            output_dict = {
                'intent': intent,
                'predicted': predicted_markup,
                'gold': gold_markup,
            }

            if need_tags:
                output_dict['gold_tags'] = gold_tags
                output_dict['predicted_tags'] = predicted_tags

            if need_sample_features:
                output_dict['sample_features'] = sample_features.as_dict()

            info_string = (json.dumps(output_dict, ensure_ascii=False, sort_keys=True) + '\n').encode('utf-8')
            out_predictions.write(info_string)

        accuracy = 1.0 * matches / len(intent_data)
        quality[intent] = accuracy
        logging.info('{0}: {1:.4f}'.format(intent, accuracy))
    out_quality.write(json.dumps(quality, indent=2))


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-T', '--data-table', help='Path to YT table with data to test trained model on')
    group.add_argument('-D', '--data-path', help='Path to file with data to test trained model on')
    parser.add_argument('-p', '--predicted-data-path', required=True, help='Path for data with tagger incorrect predictions')
    parser.add_argument('-q', '--quality-path', required=True, help='Path to file with quality estimations')
    parser.add_argument('-m', '--model-dir', required=True)
    parser.add_argument('--sample-features', action='store_true', help='Store sample features in predictions file')
    parser.add_argument('--token-tags', action='store_true', help='Store predicted and gold tags for tokens in predictions file')
    parser.add_argument('--all-preds', action='store_true', help='Store all predictions (not only negative)')
    args = parser.parse_args()

    with open(args.predicted_data_path, 'w') as out_predictions, open(args.quality_path, 'w') as out_quality:
        data = load_data_from_yt(args.data_table) if args.data_table else load_data_from_disk(args.data_path)
        logger.info('Testing...')
        _test(args.model_dir, data, out_predictions, out_quality, args.sample_features, args.token_tags, args.all_preds)

    logger.info('Done.')

    return 0

if __name__ == '__main__':
    main()

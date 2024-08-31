import copy
import logging
import random
from collections import defaultdict

import numpy as np
import sklearn

import verstehen.metrics.util as util
from verstehen.metrics.plot_writer import PlotWriter
from verstehen.config import set_defaults_to_indexes_configs
from verstehen.config.metrics_config import MetricsConfig
from verstehen.index.index_registry import IndexRegistry
from verstehen.metrics.metrics import precision_at_k, mean_pairwise_distance
from verstehen.util import DssmApplier
from verstehen.preprocess.preprocessing import get_text_preprocessing_fn

logger = logging.getLogger(__name__)


def create_index(index_config, texts):
    """
    Creating index from config and texts
    """
    return IndexRegistry.create_index(index_config, texts=texts)


def read_data_and_preprocess(config):
    """
    Reading data for measuring metrics. Returning the training data, validation data and queries data
    """

    intent_to_text = util.get_json_data(config['data_path'])

    # filter the same texts
    intent_to_text = {key: list(set(values)) for key, values in intent_to_text.items()}

    min_samples = config['min_samples']
    total_cutoff = config['query_len'] + min_samples
    train_data_size = config['train_data_size']

    filtered_data = dict()
    for key in intent_to_text:
        if len(intent_to_text[key]) >= total_cutoff:
            filtered_data[key] = intent_to_text[key][:total_cutoff]

    intent_to_text = filtered_data

    train_index_data, val_index_data, queries = dict(), dict(), dict()

    for key in intent_to_text:
        texts = intent_to_text[key]
        random.shuffle(texts)

        test_portion_first_id = int(train_data_size * min_samples)
        train_index_data[key] = texts[:test_portion_first_id]
        val_index_data[key] = texts[test_portion_first_id:min_samples]
        queries[key] = texts[min_samples:]

    train_index_data = util.flatten_dict_of_lists(train_index_data)
    val_index_data = util.flatten_dict_of_lists(val_index_data)

    return train_index_data, val_index_data, queries


def expand_query(query, query_intent, filtered_ids, found_ids, texts, intents):
    """
    Given a query, expanding it with new results depending on their intents
    """
    for id in found_ids:
        found_intent = intents[id]
        found_text = texts[id]

        if query_intent == found_intent:
            query['positive'].append(found_text)
        else:
            query['negative'].append(found_text)

    for id in found_ids:
        filtered_ids.add(id)


class MetricsWrapper:
    def __init__(self, config):
        self.metrics = {
            'mean_pairwise_distance_at_k': MetricsWrapper.mean_pairwise_distance_at_k,
            'precision_at_k': MetricsWrapper.precision_at_k,
            'roc_auc': MetricsWrapper.roc_auc
        }
        self.metrics_to_compute = []

        for metric_config in config:
            if metric_config['metric_type'] not in self.metrics:
                raise ValueError('Unsupported metric type "{}"'.format(metric_config['metric_type']))

            self.metrics_to_compute.append(self.metrics[metric_config['metric_type']](metric_config))

    @staticmethod
    def mean_pairwise_distance_at_k(metric_config):
        dssm_applier = DssmApplier(
            dssm_model_path=metric_config['model_path'],
            input_name=metric_config['model_input_name'],
            output_name=metric_config['model_output_name'],
            text_preprocessing_fn=get_text_preprocessing_fn(metric_config['text_preprocessing_fn'])
        )

        def calc(metrics_data):
            points = []
            max_k = max(metric_config['k_values'])
            result = {}
            for line in metrics_data['y_arg_sort']:
                points.append([dssm_applier.predict(text) for text in metrics_data['texts'][line[:max_k]]])

            for k in metric_config['k_values']:
                distances = np.empty(len(points))
                for i in range(len(points)):
                    distances[i] = mean_pairwise_distance(points[i][:k])
                result[metric_config['name'] + str(k)] = np.mean(distances)
            return result
        return calc

    @staticmethod
    def precision_at_k(metric_config):
        def calc(metrics_data):
            result = {}
            y_true_sorted = np.take_along_axis(metrics_data['y_true'], metrics_data['y_arg_sort'], axis=1)
            for k in metric_config['k_values']:
                precision_at_k_value = precision_at_k(np.full((metrics_data['y_true'].shape[0],), fill_value=True),
                                                      y_true_sorted, k=k)[0]

                result[metric_config['name'] + str(k)] = precision_at_k_value
            return result
        return calc

    @staticmethod
    def roc_auc(metric_config):
        def calc(metrics_data):
            # Transposing because sklearn expects matrix of shape (n_samples, n_classes)
            roc_auc_score = sklearn.metrics.roc_auc_score(metrics_data['y_true'].T, metrics_data['y_scores'].T)
            return {metric_config['name']: roc_auc_score}
        return calc

    def compute(self, y_true, y_scores, y_arg_sort, texts):
        metrics_data = {
            'y_true': y_true,
            'y_scores': y_scores,
            'y_arg_sort': y_arg_sort,
            'texts': np.array(texts)
        }

        result = {}

        for metric in self.metrics_to_compute:
            result.update(metric(metrics_data))

        return result


def append_missing_indexes(array, n):
    mask = np.ones(n, np.bool)
    mask[array] = 0
    sorted_index = np.arange(n)
    return np.append(array, sorted_index[mask])


def generate_report_message(train_index_name, val_index_name, metrics_results):
    message = "train: {}\nval: {}\n".format(train_index_name, str(val_index_name))
    message += ''.join(['{}: {:.4f}\n'.format(metric_name, metrics_results[metric_name])
                       for metric_name in metrics_results])
    return message


def main():
    logging.basicConfig(format='%(asctime)s %(levelname)s:%(name)s %(message)s', level=logging.DEBUG)

    experiment_config = MetricsConfig.EXPERIMENT_CONFIG

    set_defaults_to_indexes_configs(
        experiment_config['indexes_configs'],
        experiment_config['indexes_defaults']
    )

    indexes_configs = experiment_config['indexes_configs']

    (train_intents, train_texts), (val_intents, val_texts), queries = read_data_and_preprocess(
        experiment_config['data_config']
    )
    val_n_samples = len(val_texts)
    val_intents = np.array(val_intents)
    queries = [({'positive': value, 'negative': []}, key, set()) for key, value in queries.items()]

    rounds_config = experiment_config['rounds_config']
    n_samples_each_round = rounds_config['n_samples_each_round']
    metrics_wrapper = MetricsWrapper(experiment_config['metrics_config'])
    results = []

    plot_writer = PlotWriter()

    for index_config in indexes_configs:
        if isinstance(index_config, tuple):
            train_index_config, val_index_config = index_config
        else:
            train_index_config, val_index_config = index_config, index_config

        train_index = create_index(train_index_config, train_texts)
        val_index = create_index(val_index_config, val_texts)

        queries_copy = copy.deepcopy(queries)
        y_true = {}
        # metric_name => rounds
        metrics_rounds_results = defaultdict(lambda: np.empty((rounds_config['n_rounds'],)))
        for i_round in range(rounds_config['n_rounds']):
            # Calculating the equality of the current intent and validation intents
            all_y_true, all_y_arg_sort, all_y_scores = [], [], []

            for i_intent, (query, query_intent, filtered_ids) in enumerate(queries_copy):
                if query_intent not in y_true:
                    y_true[query_intent] = val_intents == query_intent

                # Performing search
                res = train_index.search(query, n_samples=len(filtered_ids) + n_samples_each_round)

                # Filtering
                ids = [elem[0] for elem in res if elem[0] not in filtered_ids][:n_samples_each_round]

                # Marking up query and adding marked up things to the query
                expand_query(query, query_intent, filtered_ids, ids, train_texts, train_intents)

                # Performing index search on validation data
                found_ids, found_scores = zip(*val_index.search(query, n_samples=val_n_samples))

                found_ids, found_scores = np.array(found_ids), np.array(found_scores)

                # Making full matrices with dummy values in case index didn't produce enough results
                scores = np.full((val_n_samples,), fill_value=-10e8)
                scores[found_ids] = found_scores

                all_y_true.append(y_true[query_intent])
                all_y_scores.append(scores)
                if len(found_ids) != len(y_true[query_intent]):
                    all_y_arg_sort.append(append_missing_indexes(found_ids, len(y_true[query_intent])))
                else:
                    all_y_arg_sort.append(found_ids)

            # Stacking results to numpy matrices
            all_y_true, all_y_scores = np.stack(all_y_true), np.stack(all_y_scores)
            all_y_arg_sort = np.stack(all_y_arg_sort)
            metrics_result = metrics_wrapper.compute(all_y_true, all_y_scores, all_y_arg_sort, val_texts)

            for metric_name in metrics_result:
                metrics_rounds_results[metric_name][i_round] = metrics_result[metric_name]

        # add data to plot_writer
        case_name = '{}_{}'.format(train_index_config['name'], val_index_config['name'])
        plot_writer.add_metrics_values(case_name, metrics_rounds_results)

        # Calculating necessary metrics
        results.append(generate_report_message(train_index_config['name'], val_index_config['name'], metrics_result))

    # Logging the report
    logger.debug('Report:')
    logger.debug('\n'.join(results))
    plot_writer.save(experiment_config['report_config']['path'])

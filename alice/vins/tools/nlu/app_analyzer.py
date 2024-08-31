# coding: utf-8
from __future__ import unicode_literals

import click
import numpy as np
import pandas as pd
import codecs
import os
import logging
import json
import re
import attr
import sys

from tqdm import tqdm
from collections import OrderedDict, defaultdict
from shutil import rmtree

from vins_core.app_utils import load_app
from vins_core.utils.data import get_resource_full_path
from vins_core.utils.archives import TarArchive
from vins_core.nlu.token_classifier import create_token_classifier, KNNModel
from vector_storage import VectorStorage


logger = logging.getLogger(__name__)

APPS = [
    'personal_assistant',
    'navi_app',
]

_MIN_KNN_SCORE = -1e8


@attr.s
class KNNModelView(object):
    vectors = attr.ib()
    texts = attr.ib()
    labels = attr.ib()
    intents = attr.ib()

    def slice(self, indices):
        return KNNModelView(self.vectors[indices], self.texts[indices], self.labels[indices], self.intents)

    def split(self):
        out = OrderedDict()
        for label in np.unique(self.labels):
            i = np.where(self.labels == label)[0]
            out[self.intents[label]] = KNNModelView(self.vectors[i], self.texts[i], self.labels[i], self.intents[label])
        return out


class KNNAnalyzer(object):

    _MAX_BATCH_MEMORY = int(1e9)

    @staticmethod
    def _only_neigbors_to_toloka_format(data, output_file):
        output_file += '.json'
        logger.info('%d unique false examples were found. Dump to toloka format: %s', data.shape[0], output_file)
        with codecs.open(output_file, encoding='utf-8', mode='w') as fout:
            json.dump([
                {'dialog': [neighbor], 'key': str(i)} for i, neighbor in enumerate(data.neighbor)
            ], fout, ensure_ascii=False, indent=4)

    @staticmethod
    def _only_neighbors_to_tsv_format(data, output_file, drop_scores=False, header=True):
        output_file += '.tsv'
        logger.info('%d unique false examples were found. Dump to TSV format: %s', data.shape[0], output_file)
        if drop_scores:
            data = data.drop('score', axis=1)
        data.to_csv(output_file, sep=b'\t', encoding='utf-8', index=False, header=header)

    @staticmethod
    def _format_sources(items):
        return '; '.join('original: {}, source: {}'.format(item['original_text'], item['source']) for item in items)

    @staticmethod
    def _add_source(df, text_column, text_to_source=None, destination_col='source'):
        if text_to_source is not None:
            assert destination_col not in df, 'Dataframe already contains destination column'

            df = df.copy()
            df[destination_col] = [KNNAnalyzer._format_sources(text_to_source.get(text, []))
                                   for text in df[text_column]]

        return df

    @staticmethod
    def get_knn_model_view(app_name, classifier, model_archive=None):
        if model_archive is not None:
            archive_file = model_archive
        else:
            archive_file = get_resource_full_path(os.path.join(app_name + '_model', app_name + '_model.tar.gz'))

        logger.info('Loading model from %s', archive_file)
        c = create_token_classifier('knn')
        with TarArchive(archive_file) as arch:
            with arch.nested('classifiers') as archive:
                c.load(archive, classifier)
        if not isinstance(c.final_estimator, KNNModel):
            raise ValueError('%s:%s classifier doesn''t contain proper knn model: should be instance of %s' % (
                app_name, classifier, KNNModel.__name__
            ))
        knn = c.final_estimator
        return KNNModelView(knn._vectors, np.array(knn._texts), knn._labels, c.classes)

    @staticmethod
    def _group_indices_by_intent(target_intents, knn):
        for target_intent in (intent for intent in knn.intents if re.match(target_intents, intent)):
            target_label = knn.intents.tolist().index(target_intent)
            target_indices = np.where(knn.labels == target_label)[0]
            non_target_indices = np.where(knn.labels != target_label)[0]
            yield target_intent, target_label, target_indices, non_target_indices

    @classmethod
    def false_knn_embeddings(
        cls, app_name, classifier, target_intents, output_dir, chunk_size=1000, threshold=0.99, view_all=False,
        only_neighbors=False, neighbor_intent=None, toloka_format=False, drop_scores=False, header=False,
        text_to_source=None, force_remove=False
    ):
        if os.path.exists(output_dir):
            if force_remove:
                rmtree(output_dir)
            else:
                raise ValueError('Output directory "{output_dir}" already exists, either pick a new one or run with '
                                 '--force option to reuse "{output_dir}"'.format(output_dir=output_dir))

        os.makedirs(output_dir)

        if text_to_source is not None:
            with open(text_to_source, 'r') as infile:
                text_to_source = json.load(infile)

            text_to_source = {text: [item for item in info if classifier in item['trainable_classifiers']]
                              for text, info in text_to_source.iteritems()}

        knn = cls.get_knn_model_view(app_name, classifier)

        if only_neighbors:
            scores_dir = os.path.join(output_dir, 'scores')
            os.makedirs(scores_dir)

            counts_dir = os.path.join(output_dir, 'counts')
            os.makedirs(counts_dir)

        total_stats = []
        for target_intent, target_label, target_indices, non_target_indices in cls._group_indices_by_intent(
            target_intents, knn
        ):
            if view_all:
                non_target_knn = knn.slice(non_target_indices)
            total_examples = len(target_indices)
            logger.info('%d target samples found for intent %s', len(target_indices), target_intent)
            error_texts, error_neighbors, error_intents, error_scores = [], [], [], []
            num_splits = int(np.ceil(total_examples / float(chunk_size)))
            for target_indices_chunk in tqdm(np.array_split(target_indices, num_splits)):
                target_texts = knn.texts[target_indices_chunk]
                if view_all:
                    scores = np.dot(knn.vectors[target_indices_chunk], non_target_knn.vectors.T)
                    error_indices, neighbor_indices = np.where(scores > threshold)
                    error_scores.extend(scores[error_indices, neighbor_indices])
                    error_texts.extend(target_texts[error_indices])
                    error_intents.extend(knn.intents[non_target_knn.labels[neighbor_indices]])
                    error_neighbors.extend(non_target_knn.texts[neighbor_indices])
                else:
                    scores = np.dot(knn.vectors[target_indices_chunk], knn.vectors.T)
                    scores[np.arange(len(target_indices_chunk)), target_indices_chunk] = _MIN_KNN_SCORE
                    neighbor_indices = scores.argmax(axis=1)
                    neighbor_labels = knn.labels[neighbor_indices]
                    error_indices = np.logical_and(
                        scores[np.arange(len(target_indices_chunk)), neighbor_indices] > threshold,
                        neighbor_labels != target_label
                    )
                    error_neighbor = neighbor_indices[error_indices]
                    error_texts.extend(target_texts[error_indices])
                    error_intents.extend(knn.intents[neighbor_labels[error_indices]])
                    error_neighbors.extend(knn.texts[error_neighbor])
                    error_scores.extend(scores[np.where(error_indices)[0], error_neighbor])

            num_errors = len(error_texts)
            total_stats.append({
                'intent': target_intent,
                'total_examples': total_examples,
                'total_errors': num_errors
            })

            output = pd.DataFrame(
                data=zip(error_texts, error_neighbors, error_intents, error_scores),
                columns=('text', 'neighbor', 'intent', 'score')
            )
            if neighbor_intent:
                logger.info('Filtering intents by "{}"'.format(neighbor_intent))
                output = output[output.intent.str.match(neighbor_intent, as_indexer=True)]
            if only_neighbors:
                if output.empty:
                    logger.info('Nothing to aggregate. Skipping')
                    continue

                target_texts = output.groupby('text').mean().sort_values(by='score', ascending=False).reset_index()
                output_file = os.path.join(scores_dir, target_intent)

                if toloka_format:
                    cls._only_neigbors_to_toloka_format(target_texts, output_file)
                else:
                    cls._only_neighbors_to_tsv_format(target_texts, output_file, drop_scores, header)

                intent_output_dir = os.path.join(counts_dir, target_intent)
                os.makedirs(intent_output_dir)

                for neigh_intent, neigh_data in output.groupby('intent'):
                    neigh_score = neigh_data.groupby('neighbor').count().sort_values(
                        by='score', ascending=False).score.divide(float(total_examples)).reset_index()

                    output_file = os.path.join(intent_output_dir, neigh_intent)

                    if toloka_format:
                        cls._only_neigbors_to_toloka_format(neigh_score, output_file)
                    else:
                        neigh_score = cls._add_source(neigh_score, 'neighbor', text_to_source)

                        cls._only_neighbors_to_tsv_format(neigh_score, output_file, drop_scores, header)
            else:
                output_file = os.path.join(output_dir, '%s.tsv' % target_intent)
                logger.info(
                    '%d false examples were found for intent %s. Saving to %s',
                    output.shape[0], target_intent, output_file
                )
                if output.shape[0] > 0:
                    output.to_csv(output_file, sep=b'\t', encoding='utf-8', index=False)
        output_file = os.path.join(output_dir, 'stats.json')
        logger.info('Saving total stats to %s', output_file)
        with open(output_file, mode='w') as fout:
            json.dump(total_stats, fout, indent=4, sort_keys=True)

    @classmethod
    def cross_classify(cls, app_name, classifier, probe_intents, target_intents, output_dir, top_k,
                       only_neighbors, exclude_target, model_archive):
        if not os.path.exists(output_dir):
            logger.info('Creating output directory %s', output_dir)
            os.makedirs(output_dir)

        knn = cls.get_knn_model_view(app_name, classifier, model_archive).split()
        result = defaultdict(lambda: defaultdict(list))

        for probe_intent, probe_knn in tqdm(knn.items()):
            if not re.match(probe_intents, probe_intent):
                continue

            queries, queries_meta = [], []

            for test_intent, test_knn in knn.iteritems():
                if not re.match(target_intents, test_intent) or (exclude_target and probe_intent == test_intent):
                    continue

                queries.append(test_knn.vectors)
                queries_meta.append((test_knn.vectors.shape[0], test_intent))

            storage = VectorStorage(top_k).fit(probe_knn.vectors)

            scores = storage.predict(np.concatenate(queries))
            pos = 0
            for size, intent in queries_meta:
                result[intent]['scores'].append(scores[pos:pos + size])
                result[intent]['intents_seen'].append(probe_intent)

                pos += size

        for test_intent, data in result.iteritems():
            scores = pd.DataFrame(
                data=np.vstack(data['scores']).T,
                index=knn[test_intent].texts,
                columns=data['intents_seen'],
            )
            output_file_prefix = os.path.join(output_dir, test_intent)
            if only_neighbors:
                scores = pd.concat((scores.idxmax(axis=1), scores.max(axis=1)), axis=1)
                scores.columns = ['intent', 'score']

            scores.reset_index().to_csv(
                output_file_prefix + '.tsv', sep=b'\t', encoding='utf-8', index=False
            )


def _report_false_knn_embeddings_and_exit(directory, notification_threshold, failure_threshold,
                                          significant_num_samples):
    with open(os.path.join(directory, 'stats.json'), 'r') as infile:
        report = pd.DataFrame([
            [item['intent'], item['total_examples'], float(item['total_errors']) / item['total_examples']]
            for item in json.load(infile)],
            columns=['intent', 'num_samples', 'error_rate']).set_index('intent').sort_values(by='error_rate',
                                                                                             ascending=False)

    report.to_csv(os.path.join(directory, 'report.tsv'), sep=b'\t', encoding='utf-8', index=True)

    significant_mask = report.num_samples > significant_num_samples

    failure_mask = (report.error_rate > failure_threshold) & significant_mask
    notification_mask = (report.error_rate > notification_threshold) & ~failure_mask & significant_mask
    ok_mask = ~notification_mask & ~failure_mask

    print 'Definitely ok intents:'
    print report[ok_mask]
    print

    print 'Intents that look suspicious:'
    print report[notification_mask]
    print

    if failure_mask.any():
        print 'Intents that cause task failure:'
        print report[failure_mask]

        sys.exit(1)


@click.group()
def analyzer():
    pass


@analyzer.command('false_knn_embeddings')
@click.argument('app', type=click.Choice(APPS))
@click.argument('classifier')
@click.option('--intents', help='Intents to be processed (regexp pattern)')
@click.option('--output-dir', help='Output directory with results (created if doesn''t exist)')
@click.option('--threshold', type=float, default=0.99, help='Threshold applied to cut off knn score')
@click.option('--chunk-size', type=int, default=1000,
              help='Number of examples to be processed at each iteration '
                   '(greater chunks run faster at the cost of memory)')
@click.option('--view-all', help='View all neighbors from other intents with score > threshold', is_flag=True)
@click.option('--only-neighbors', help='Output only false neighbors rather than full info', is_flag=True)
@click.option('--toloka-format', help='When --only-neighbors is active, dump output in format'
                                      ' ready for upload to toloka', is_flag=True)
@click.option('--neighbor-intent', help='If set, inspect only neighbors from this intent')
@click.option('--drop-scores', is_flag=True, help='Don''t output score column')
@click.option('--header', is_flag=True, help='Prepend header')
@click.option('--force', is_flag=True, help='Clear --output_dir if exists before execution')
def false_knn_embeddings(
    app, classifier, intents, output_dir, threshold, chunk_size, view_all, only_neighbors, toloka_format,
    neighbor_intent, drop_scores, header, force
):
    """
    This command finds false KNN points in embedding space provided by "classifier".
    False KNN points are those points which are close to target points,
    but lie in diverse intents.
    There are two options for declaring proximity:
    - the closest points lie in diverse intent (by default)
    - all points in specific radius (by using `--view-all` and `--threshold` options)
    Outputs are dumped in tsv/json files
    see https://wiki.yandex-team.ru/users/lubimovnik/Active-Learning-i-NLU-Debugging-v-Alise/ for details
    """
    KNNAnalyzer.false_knn_embeddings(
        app_name=app,
        classifier=classifier,
        target_intents=intents,
        output_dir=output_dir,
        threshold=threshold,
        chunk_size=chunk_size,
        view_all=view_all,
        only_neighbors=only_neighbors,
        neighbor_intent=neighbor_intent,
        toloka_format=toloka_format,
        drop_scores=drop_scores,
        header=header,
        force_remove=force
    )


@analyzer.command('cross_classify')
@click.argument('app', type=click.Choice(APPS))
@click.argument('classifier')
@click.option('--probe-intents', help='Intents to be processed as probe (regexp pattern)', default=".*")
@click.option('--target-intents', help='Intents to be processed as target (regexp pattern)', default=".*")
@click.option('--output-dir', help='Output directory with results (created if doesn''t exist)')
@click.option('--top-k', help='Number of neighbors used for classification', type=int, default=1)
@click.option('--only-neighbors', help='Output only winner intents', is_flag=True)
@click.option('--exclude-target', help='When running classification, exclude target intent', is_flag=True)
@click.option('--model-archive', help='Path to custom model archive', type=click.Path())
def cross_classify(app, classifier, probe_intents, target_intents, output_dir, top_k, only_neighbors,
                   exclude_target, model_archive):
    """
    This command provides view-like fast classification, assuming target vectors are already loaded
    in KNN model as "custom intents" sources
    see https://wiki.yandex-team.ru/users/lubimovnik/Active-Learning-i-NLU-Debugging-v-Alise/ for details
    """
    KNNAnalyzer.cross_classify(app, classifier, probe_intents, target_intents, output_dir, top_k, only_neighbors,
                               exclude_target, model_archive)


@analyzer.command('dump_dataset')
@click.argument('app', type=click.Choice(APPS))
@click.option('-o', '--output-dir', help='Output dir where to dump txt files per intent')
def dump_dataset(app, output_dir):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    app_obj = load_app(app, force_rebuild=False)
    for intent_name, nlu_source_items in app_obj.nlu.nlu_sources_data.iteritems():
        output_file = os.path.join(output_dir, intent_name + '.txt')
        with codecs.open(output_file, mode='w', encoding='utf-8') as f:
            for nlu_source_item in sorted(nlu_source_items):
                f.write(nlu_source_item.text + '\n')


_FAILURE_HELP = 'The command will exit with code 1 if for any intent \n(1) its error_rate exceeds ' \
                'failure_threshold;\n(2) number of train samples for this intent exceed significant_num_samples'


@analyzer.command('check_false_knn_embeddings')
@click.argument('app', type=click.Choice(APPS))
@click.argument('classifier')
@click.option('--text-to-source', type=click.Path(),
              help='Provide text_to_source json file (see where_is_this_phrase tool) if you need sources '
                   'of false neighbours')
@click.option('--intents', type=str, default='.*', help='Intents to be processed (regexp pattern)')
@click.option('--directory', type=click.Path(), default='false_knn_embeddings', help='Directory to store results in')
@click.option('--notification-threshold', type=float, default=0.1,
              help='If error_rate exceeds this threshold and number of train samples exceeds significant_num_samples '
                   'for some intent, it will be mentioned in report')
@click.option('--failure-threshold', type=float, default=0.5, help=_FAILURE_HELP)
@click.option('--significant-num-samples', type=int, default=1, help=_FAILURE_HELP)
def check_false_knn_embeddings(app, classifier, text_to_source, intents, directory, notification_threshold,
                               failure_threshold, significant_num_samples):
    KNNAnalyzer.false_knn_embeddings(
        app_name=app,
        classifier=classifier,
        target_intents=intents,
        output_dir=directory,
        threshold=-1.,
        chunk_size=1000,
        view_all=False,
        only_neighbors=True,
        neighbor_intent=None,
        toloka_format=False,
        drop_scores=False,
        header=True,
        text_to_source=text_to_source,
        force_remove=False
    )

    _report_false_knn_embeddings_and_exit(
        directory=directory,
        notification_threshold=notification_threshold,
        failure_threshold=failure_threshold,
        significant_num_samples=significant_num_samples)


def app_analyzer_logging_config():
    logging.config.dictConfig({
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'standard': {
                'format': '[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s',
            },
        },
        'handlers': {
            'console': {
                'class': 'logging.StreamHandler',
                'level': 'INFO',
                'formatter': 'standard',
            }
        },
        'loggers': {
            '': {
                'handlers': ['console'],
                'level': 'INFO',
                'propagate': True,
            },
        },
    })


if __name__ == "__main__":
    app_analyzer_logging_config()

    analyzer(obj={})

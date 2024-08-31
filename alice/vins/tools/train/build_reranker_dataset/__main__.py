# coding: utf-8

import codecs
import click
import json
import os
import yt.wrapper as yt
import logging

from vins_core.dm.form_filler.form_candidate import FormCandidate
from vins_core.nlu.features.base import SampleFeatures
from vins_core.nlu.reranker.factor_calcer import FactorCalcer
from vins_core.nlu.intent_candidate import IntentCandidate
from vins_core.utils.data import get_vinsfile_data
from vins_core.utils.intent_renamer import IntentRenamer

logger = logging.getLogger(__name__)


_DEFAULT_RENAMING_DIR = 'personal_assistant://tests/validation_sets/'
_DEFAULT_RENAMING_FILES = 'toloka_intent_renames_quasar.json,toloka_intent_renames.json'

_TSV_META_INFO_FORMAT = u'{label}\t{group_id}\t{text}\t{true_intents}\t{pred_intent}\t'

_OTHER_INTENTS = {
    'personal_assistant.scenarios.search',
    'personal_assistant.general_conversation.general_conversation'
}


@click.command()
@click.option('--input-table', required=True)
@click.option('--output-table', required=True)
@click.option('--output-cd', required=True)
@click.option('--yt-token', default=None)
@click.option('--reranker-name', default='catboost_post_classifier_with_setup',
              help='Name of the experiment in Vinsfile.json used to obtain the list of factors')
@click.option('--renaming-files', default=_DEFAULT_RENAMING_FILES,
              help='Comma separated list to renaming configuration paths')
@click.option('--toloka-true-intents', default=True, type=bool)
def main(input_table, output_table, output_cd, yt_token, reranker_name, renaming_files, toloka_true_intents):
    with open(input_table) as f:
        input_table_config = json.load(f)

    yt_proxy = input_table_config['cluster']
    input_table = input_table_config['table']

    yt_client = yt.YtClient(
        proxy=yt_proxy,
        token=yt_token or os.environ['YT_TOKEN']
    )

    vinsfile = get_vinsfile_data(package='personal_assistant')
    post_classifier_config = vinsfile.get('post_classifier', {})
    factor_calcer = FactorCalcer.from_config(post_classifier_config.get('factor_calcer', {}))

    catboost_params = post_classifier_config['reranker']['params']['passed_candidates_reranker']['params']
    reranker_configs = catboost_params.get('inner_models', [])

    required_factors, known_intents = _get_reranker_params(reranker_configs, reranker_name)

    assert known_intents, 'known_intents was not provided'

    if not required_factors:
        logger.warning('No factors to calculate was provided. Only the predict_intents scores are going to be used')

    renaming_paths = renaming_files.split(',')
    intent_renamer = IntentRenamer(renaming_paths)

    _build_dataset(input_table, output_table, output_cd, yt_client, factor_calcer,
                   required_factors, known_intents, intent_renamer, toloka_true_intents)


def _get_reranker_params(reranker_configs, reranker_name):
    for reranker_config in reranker_configs:
        if reranker_config.get('experiment') == reranker_name:
            required_factors = reranker_config['params'].get('used_factors', [])
            known_intents = set(reranker_config['params'].get('known_intents', []))
            return required_factors, known_intents


def _build_dataset(input_table, output_table_path, output_cd_path, yt_client, factor_calcer,
                   required_factors, known_intents, intent_renamer, toloka_true_intents):

    total_count = yt_client.row_count(input_table)
    with codecs.open(output_table_path, 'w', encoding='utf8') as f_out:
        for index, row in enumerate(yt_client.read_table(input_table)):
            if index != 0 and index % 1000 == 0:
                logger.info('Processing {} / {} item'.format(index, total_count))

            sample_features = SampleFeatures.from_bytes(row['sample_features'])

            text = row['text'].decode('utf8')
            true_intents = row['true_intents'].split(',')

            true_intents = _get_true_intents(true_intents, intent_renamer, known_intents)
            form_candidates = _build_candidates(sample_features, known_intents)

            has_true_candidate = any(
                _get_pred_intent(candidate, toloka_true_intents) in true_intents
                for candidate in form_candidates
            )
            if not (form_candidates and true_intents and has_true_candidate):
                continue

            factor_values = factor_calcer(sample_features, form_candidates, required_factors)
            _write_candidates(index, text, true_intents, form_candidates, factor_values, toloka_true_intents, f_out)

    _write_column_description_file(output_cd_path, factor_calcer, required_factors)


def _get_pred_intent(candidate, toloka_true_intents):
    if toloka_true_intents:
        return candidate.intent.name_for_reranker
    return candidate.intent.name


def _get_true_intents(true_intents, intent_renamer, known_intents):
    true_intents = [intent_renamer(intent, IntentRenamer.By.TRUE_INTENT) for intent in true_intents]

    for intent in true_intents:
        if not intent.startswith('personal_assistant'):
            logger.warning('Got true intent: %s. Looks like you should update the intent mappings', intent)

    allowed_intents = known_intents | _OTHER_INTENTS
    return set(filter(lambda intent: intent in allowed_intents, true_intents))


def _build_candidates(sample_features, known_intents):
    intent_scores = sample_features.classification_scores['max_intents']

    form_candidates = []
    for intent_score in intent_scores:
        name_for_reranker = intent_score.name
        if name_for_reranker in _OTHER_INTENTS:
            name_for_reranker = 'personal_assistant.scenarios.other'
        if name_for_reranker not in known_intents or intent_score.score == 0.:
            # We rerank only known intents with a positive score
            # TODO(dan-anastasev): more correctly would be collect candidates after
            #  FormFillingDialogManager._remove_fake_candidates
            continue

        intent_candidate = IntentCandidate(
            name=intent_score.name, score=intent_score.score,
            name_for_reranker=name_for_reranker
        )

        form_candidates.append(FormCandidate(form=None, intent=intent_candidate))

    return form_candidates


def _write_candidates(group_id, text, true_intents, form_candidates, factor_values, toloka_true_intents, f_out):
    true_intents_string = ','.join(true_intents)

    for candidate, factor_value in zip(form_candidates, factor_values):
        # In case of toloka markup the predicted intent has to be converted to a compatible with mappings format
        # (because gc and search are mapped into other in this mapping)
        pred_intent = _get_pred_intent(candidate, toloka_true_intents)
        label = int(pred_intent in true_intents)

        f_out.write(_TSV_META_INFO_FORMAT.format(
            label=label,
            group_id=group_id,
            text=text,
            true_intents=true_intents_string,
            pred_intent=pred_intent
        ))

        f_out.write('\t'.join(str(val) for val in factor_value) + '\n')


def _write_column_description_file(output_cd_path, factor_calcer, required_factors):
    with open(output_cd_path, 'w') as f:
        def add_column_description(col_index, col_type, col_desc=None):
            if col_desc:
                f.write('{}\t{}\t{}\n'.format(col_index, col_type, col_desc))
            else:
                f.write('{}\t{}\n'.format(col_index, col_type))

        add_column_description(col_index=0, col_type='Label')
        add_column_description(col_index=1, col_type='GroupId')
        add_column_description(col_index=2, col_type='Auxiliary', col_desc='Query text')
        add_column_description(col_index=3, col_type='Auxiliary', col_desc='True label')
        add_column_description(col_index=4, col_type='Auxiliary', col_desc='Predicted label')
        add_column_description(col_index=5, col_type='Num', col_desc='scenarios')

        index = 6
        for factor_name in required_factors:
            for column_name in factor_calcer.get_factor(factor_name).get_factor_value_names():
                # TODO(dan-anastasev): update me when we have non-numeral factors
                add_column_description(col_index=index, col_type='Num', col_desc=column_name)
                index += 1


if __name__ == '__main__':
    logging.basicConfig(
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        level=logging.INFO
    )

    main()

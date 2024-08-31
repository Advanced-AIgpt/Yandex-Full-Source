# coding: utf-8
from __future__ import unicode_literals


import logging.config
import click
import json
import os

from vins_tools.nlu.inspection.nlu_processing_on_dataset import do_classify, do_report, do_crossval

click.disable_unicode_literals_warning = True

APPS = [
    'personal_assistant',
    'navi_app',
    'crm_bot',
]

logger = logging.getLogger(__name__)


def set_logging(level='INFO'):
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
                'formatter': 'standard',
            },
        },
        'loggers': {
            'vins_tools.nlu.inspection.nlu_result_info': {
                'handlers': ['console'],
                'level': 'DEBUG',
                'propagate': False,
            },
            '': {
                'handlers': ['console'],
                'level': level,
                'propagate': True,
            },
        },
    })


@click.group()
@click.argument('app', type=click.Choice(APPS))
@click.option('--log-level', type=click.Choice(('DEBUG', 'INFO', 'WARNING', 'ERROR')), default='WARNING')
@click.option('--ignore-errors', is_flag=True)
@click.pass_context
def main(ctx, app, log_level, ignore_errors):
    ctx.obj['app'] = app
    ctx.obj['ignore_errors'] = ignore_errors
    set_logging(log_level)


@main.command('classify', short_help='run intent classifier on given utterances')
@click.argument('input_file', type=click.Path())
@click.option('-f', '--format', help='input file format', type=click.Choice(
    ['text', 'tsv', 'nlu', 'yt']), default='tsv')
@click.option('-o', '--output-file', help='output file to dump the results', default=None)
@click.option('--nbest', help='get N best intent predictions', default=1)
@click.option('--force-intent', help='force collecting result on specific intent', default=None)
@click.option('--text-col', help='text column name', default='text')
@click.option('--intent-col', help='intent column name', default='intent')
@click.option('--prev-intent-col', help='previous intent column name')
@click.option('--weight-col', help='weights column name')
@click.option('--device-state-col', help='json encoded device state column name')
@click.option('--additional-columns', help='additional columns to preserve, separated by commas')
@click.option('--vinsfile', help='specify VINS config file if differs from app native')
@click.option('--app-info', help='JSON with AppInfo kwargs', default='{}')
@click.option('--device-state', help='JSON with device state', default='{}')
@click.option('--experiments', help='Experiments flags separated by commas', default='')
@click.option('--use-bass', is_flag=True, help='send request to BASS', default=False)
@click.option('--apply-item-selection', is_flag=True, help='apply item selector after NLU', default=False)
@click.pass_context
def classify(
    ctx, input_file, format, output_file, nbest, force_intent, text_col, intent_col, prev_intent_col, weight_col,
    device_state_col, additional_columns, vinsfile, app_info, device_state, experiments, use_bass, apply_item_selection
):
    if additional_columns is not None:
        additional_columns = [c.strip() for c in additional_columns.split(',')]
    do_classify(
        app_name=ctx.obj['app'],
        input_file=input_file,
        format=format,
        output_file=output_file,
        nbest=nbest,
        force_intent=force_intent,
        text_col=text_col,
        intent_col=intent_col,
        prev_intent_col=prev_intent_col,
        device_state_col=device_state_col,
        additional_columns=additional_columns,
        vinsfile=vinsfile,
        app_info_kwargs=json.loads(app_info),
        weight_col=weight_col,
        device_state=json.loads(device_state),
        ignore_errors=ctx.obj['ignore_errors'],
        experiments=experiments.split(','),
        use_bass=use_bass,
        apply_item_selection=apply_item_selection,
    )


@main.command('report', short_help='dump NLU results on given output file')
@click.argument('results_file', type=click.Path(exists=True))
@click.option('--intent-clf', help='print intent classification result in TSV format', is_flag=True)
@click.option('--nlu-markup', help='dump tagging results using nlu format', is_flag=True)
@click.option('--errors', help='dump erroneously identified utterances', is_flag=True)
@click.option('--rename', default=['{}'], help='JSON string or filepath with regexp-to-new-intent-name mappings',
              multiple=True)
@click.option('--exclude', default=None, help='Regexp to identify which intent names must be excluded from reference')
@click.option('--append-time', help='append time info to created files', is_flag=True)
@click.option('--recalls-at', help='recall@K levels, e.g. "1,2,5,10"', default='1,2,5,10')
@click.option('--recall-by-slots', help='calculate recall by slot', is_flag=True)
@click.option('--common-tagger-errors', help='typical words and tags difficult for the tagger', is_flag=True)
@click.option('--num-most-common-errors', help='number of top errors to be displayed in console', default=10)
@click.option('--other', help='Specify intents regexp that is considered as "other"')
@click.option('--detailed-recall-for-intents', help='Specify intents regexp that will show detailed recall')
@click.option('--baseline', help='file with the baseline classification metrics', default=None)
@click.pass_context
def report(
    ctx, results_file, intent_clf, nlu_markup, errors, rename, exclude, append_time, recalls_at, recall_by_slots,
        common_tagger_errors, num_most_common_errors, other, detailed_recall_for_intents, baseline):
    do_report(
        results_file=results_file,
        intent_classification_result=intent_clf,
        nlu_markup=nlu_markup,
        errors_output=errors,
        rename_intents=rename,
        exclude_reference_intents=exclude,
        append_time_to_filename=append_time,
        recalls_at=list(int(r) for r in recalls_at.split(',')),
        recall_by_slots=recall_by_slots,
        common_tagger_errors=common_tagger_errors,
        num_most_common_errors=num_most_common_errors,
        other=other,
        detailed_recall_for_intents=detailed_recall_for_intents,
        baseline=baseline,
    )


@main.command('crossval', short_help='run crossvalidation using random subsampling on actual training dataset')
@click.option('-o', '--output-file',
              help='output file prefix to dump the results. '
                   'Prefix will be appended with ".classifiers.pkl" and "taggers.pkl"', default='crossvalidation')
@click.option('-n', '--num-runs', help='number of train runs', type=int, default=3)
@click.option('--taggers', help='run crossvalidation on taggers', is_flag=True)
@click.option('--feature-cache', help='file path to store / retrieve precomputed train features', type=click.Path())
@click.option('-v', '--validation', help='fraction of dataset retained for validation', type=float, default=0.2)
@click.option('--validate-intent', help='regexp to validating only specific intent', default=None)
@click.option('--classifiers', help='specify classifiers to crossvalidate, separated by commas', default='')
@click.pass_context
def crossval(ctx, output_file, num_runs, taggers, feature_cache, validation, validate_intent, classifiers):
    do_crossval(
        app_name=ctx.obj['app'],
        output_file=output_file,
        num_runs=num_runs,
        classifiers=classifiers.split(','),
        taggers=taggers,
        feature_cache=feature_cache,
        validation=validation,
        validate_intent=validate_intent
    )


def do_main():
    try:
        import tensorflow as tf
        tf.logging.set_verbosity(tf.logging.ERROR)
    except Exception:
        pass

    # feature extractor uses tensorflow loaders, therefore lazy initialization is needed  to avoid hunging problem
    os.environ['VINS_LOAD_TF_ON_CALL'] = '1'

    main(obj={})


if __name__ == '__main__':
    do_main()

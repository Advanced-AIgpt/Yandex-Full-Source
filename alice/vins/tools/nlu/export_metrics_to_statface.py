# coding: utf-8
import json
import click
import codecs
import logging.config

from statface_exporter import SimpleStatfaceExporter


def export_quality(statface, metrics_json, name, title, branch, validation_set):
    statface.export_config(
        name,
        ['branch', 'intent', 'validation_set'],
        ['precision', 'recall', 'f1_measure', 'support'],
        title
    )

    with codecs.open(metrics_json, 'r', encoding='utf-8') as f:
        metrics = json.load(f)

        statface.export_data(
            name,
            [
                {
                    'branch': branch,
                    'validation_set': validation_set,
                    'intent': intent,
                    'precision': numbers['precision'],
                    'recall': numbers['recall'],
                    'f1_measure': numbers['f1_measure'],
                    'support': numbers['support']
                }
                for intent, numbers in metrics['report'].iteritems()
            ]
        )


def export_confusion(statface, metrics_json, name, title, branch, validation_set):
    statface.export_config(
        name,
        ['branch', 'validation_set', 'true_intent', 'pred_intent'],
        ['norm_error_count'],
        title
    )

    with codecs.open(metrics_json, 'r', encoding='utf-8') as f:
        metrics = json.load(f)

        data = []
        for true_intent, errors in metrics['confusion_matrix'].iteritems():
            for pred_intent, norm_error_count in errors.iteritems():
                data.append({
                    'branch': branch,
                    'validation_set': validation_set,
                    'true_intent': true_intent,
                    'pred_intent': pred_intent,
                    'norm_error_count': norm_error_count
                })
        statface.export_data(name, data)


@click.command(short_help='Export nlu metrics to statface')
@click.argument('metrics_json', type=click.Path(exists=True))
@click.option(
    '--name', required=True,
    help='Name of the statface report (usually something like "Voicetech/user/report_name")'
)
@click.option(
    '--title', required=True,
    help='String that will be displayed as title of the report'
)
@click.option(
    '--branch', required=True,
    help='Name of the git branch on which the quality was measured. Will be stored in a separate dimension.'
)
@click.option(
    '--validation_set', required=True,
    help='Name of the validation set that was used to measure the quality. Will be stored in a separate dimension.'
)
@click.option(
    '--mode', required=True,
    help='Export quality metrics ("quality") or confusion matrix ("confusion")',
    type=click.Choice(['quality', 'confusion'])
)
def main(metrics_json, name, title, branch, mode, validation_set):
    statface = SimpleStatfaceExporter()

    if mode == 'quality':
        export_quality(statface, metrics_json, name, title, branch, validation_set)
    elif mode == 'confusion':
        export_confusion(statface, metrics_json, name, title, branch, validation_set)


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    main()

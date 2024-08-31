import json
import click

from alice.analytics.ww.arabic.asr_annotation.aggregator.lib.aggregate import process_item


@click.command()
@click.option('--toloka_results', required=True)
@click.option('--output', required=True)
@click.option('--config_override')
def main(toloka_results, output, config_override):
    with open(toloka_results) as f:
        toloka_results = json.load(f)

    config_override_dict = {}
    if config_override:
        config_override_dict = json.loads(config_override)

    for item in toloka_results:
        aggregation_results = process_item(item['answers_raw'], config_override=config_override_dict)
        item.update(aggregation_results)

    with open(output, 'w') as f:
        json.dump(toloka_results, f, indent=2, ensure_ascii=False, sort_keys=True, separators=(',', ': '))

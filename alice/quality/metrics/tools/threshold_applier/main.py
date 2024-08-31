import json

import click
import yt.wrapper

from alice.quality.metrics.lib.thresholds.threshold_applier import THRESHOLD_APPLIERS
from alice.quality.metrics.lib.thresholds.threshold_finder import OptimumType


@click.command()
@click.option('--cluster', default='hahn')
@click.option('--input-table', required=True)
@click.option('--predict-column', required=True)
@click.option('--output-table', required=True)
@click.option('--classification-type', required=True, type=click.Choice(THRESHOLD_APPLIERS))
@click.option('--thresholds-json-path', required=True, type=click.Path(exists=True, dir_okay=False))
@click.option('--optimum-type', required=True, type=click.Choice(OptimumType))
def main(
    cluster, input_table,
    predict_column, output_table,
    classification_type, thresholds_json_path, optimum_type,
):
    client = yt.wrapper.YtClient(proxy=cluster)

    with open(thresholds_json_path, 'r', encoding='utf-8') as f:
        thresholds_json = json.load(f)

    thresholds = thresholds_json[optimum_type]
    applier = THRESHOLD_APPLIERS[classification_type](predict_column, thresholds)

    schema = yt.wrapper.get_attribute(input_table, 'schema', client=client)
    output_table = yt.wrapper.TablePath(output_table, attributes={'schema': schema}, client=client)

    yt.wrapper.run_map(applier, input_table, output_table, ordered=True, client=client)

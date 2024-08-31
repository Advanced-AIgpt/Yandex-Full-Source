import click
import yt.wrapper

from alice.quality.metrics.lib.core.metric_type import MetricType
from alice.quality.metrics.lib.core.utils import IntentLabelEncoder
from alice.quality.metrics.lib.mispredictions.misprediction_finder import BinaryMispredictionFinder, MultilabelMispredictionFinder

from yt.wrapper.schema import TableSchema


@click.command()
@click.option('--cluster', default='hahn')
@click.option('--input-table', required=True)
@click.option('--predict-column', required=True)
@click.option('--target-column', required=True)
@click.option('--output-table', required=True)
@click.option('--classification-type', required=True, type=click.Choice([MetricType.BINARY, MetricType.MULTILABEL]))
@click.option('--threshold', default=0.5, type=float)
@click.option('--label-encode-path', required=False, type=click.Path(exists=True, dir_okay=False))
def main(
    cluster, input_table,
    predict_column, target_column, output_table,
    classification_type, threshold, label_encode_path,
):
    client = yt.wrapper.YtClient(proxy=cluster)

    if classification_type == MetricType.BINARY:
        finder = BinaryMispredictionFinder(predict_column, target_column, threshold)
    elif classification_type == MetricType.MULTILABEL:
        if label_encode_path is None:
            raise ValueError(f'label encoder is necessary to process {classification_type}')

        label_encoder = IntentLabelEncoder(label_encode_path, target_column)
        finder = MultilabelMispredictionFinder(predict_column, target_column, label_encoder, threshold)
    else:
        raise ValueError(f'{classification_type} is not supported')

    schema = TableSchema.from_yson_type(yt.wrapper.get_attribute(input_table, "schema", client=client))
    finder.update_schema(schema)

    output_table = yt.wrapper.TablePath(output_table, attributes={"schema": schema}, client=client)

    yt.wrapper.run_map(finder, input_table, output_table, client=client)
    yt.wrapper.run_sort(output_table, sort_by=finder.sort_by_columns(), client=client)
